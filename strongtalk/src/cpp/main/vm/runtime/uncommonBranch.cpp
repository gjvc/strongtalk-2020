//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/runtime/StackChunkBuilder.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/recompiler/RecompilationPolicy.hpp"
#include "vm/runtime/uncommonBranch.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


// -----------------------------------------------------------------------------

// Tells whether the call has been executed before
// %note:
//    This function is highly INTEL specific.

bool_t patch_uncommon_call( Frame * f ) {
    // patch the call site:
    //  from: call _unused_uncommon_trap
    //  to:   call _used_uncommon_trap

    int * next_inst = ( int * ) f->pc();
    int * dest_addr = next_inst - 1;
    int dest = *dest_addr + ( int ) next_inst;

    // return true if the call has been executed before
    if ( dest == ( int ) StubRoutines::used_uncommon_trap_entry() )
        return true;

    st_assert( dest == ( int ) StubRoutines::unused_uncommon_trap_entry(), "Make sure we are patching the right call" );

    // patch with used_uncommon_trap
    *dest_addr = ( ( int ) StubRoutines::used_uncommon_trap_entry() ) - ( ( int ) next_inst );

    st_assert( *dest_addr + ( int ) next_inst == ( int ) StubRoutines::used_uncommon_trap_entry(), "Check the patch" );

    // return false since the call is patched
    return false;
}


// Tells whether the frame is a candidate for deoptimization by
// checking if the frame uses contextOops with forward pointers.
static bool_t has_invalid_context( Frame * f ) {
    // Return false if we're not in compiled code
    if ( not f->is_compiled_frame() )
        return false;

    // Iterate over the vframes and check the compiled_context
    CompiledVirtualFrame * vf = ( CompiledVirtualFrame * ) VirtualFrame::new_vframe( f );
    st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
    while ( true ) {
        ContextOop con = vf->compiled_context();

        // _console->print_cr("checking context fp = 0x%lx, pc = 0x%lx", f->fp(), f->pc());
        if ( con )
            con->print();

        if ( con and con->unoptimized_context() )
            return true;
        if ( vf->is_top() )
            break;
        vf = ( CompiledVirtualFrame * ) vf->sender();
        st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
    }
    return false;
}


// -----------------------------------------------------------------------------

class FrameAndContextElement : public ResourceObject {
    public:
        Frame      fr;
        ContextOop con;


        FrameAndContextElement( Frame * f, ContextOop c ) {
            fr  = *f;
            con = c;
        }
};


void collect_compiled_contexts_for( Frame * f, GrowableArray <FrameAndContextElement *> * elements ) {
    // Return false if we're not in compiled code
    if ( not f->is_compiled_frame() )
        return;

    // Iterate over the vframes and check the compiled_context
    CompiledVirtualFrame * vf = ( CompiledVirtualFrame * ) VirtualFrame::new_vframe( f );
    st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
    while ( true ) {
        ContextOop con = vf->compiled_context();
        if ( con ) {
            elements->append( new FrameAndContextElement( f, con ) );
        }
        if ( vf->is_top() )
            break;
        vf = ( CompiledVirtualFrame * ) vf->sender();
        st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
    }
}


// -----------------------------------------------------------------------------

class EnableDeoptimization : StackAllocatedObject {
    public:
        EnableDeoptimization() {
            StackChunkBuilder::begin_deoptimization();
        }


        ~EnableDeoptimization() {
            StackChunkBuilder::end_deoptimization();
        }
};


void uncommon_trap() {

//    if ( UseNewBackend ) {
//        warning( "uncommon traps not supported yet for new backend" );
//        Unimplemented();
//    }

    ResourceMark resourceMark;
    // Find the frame that caused the uncommon trap
    DeltaProcess * process = DeltaProcess::active();

    process->verify();

    FlagSetting fl( processSemaphore, true );

    process->enter_uncommon();
    Frame f = process->last_frame();

    // Patch the call destination if necessary
    bool_t used = patch_uncommon_call( &f );

    // Find the NativeMethod containing the uncommon trap
    CompiledVirtualFrame * vf = ( CompiledVirtualFrame * ) VirtualFrame::new_vframe( &f );
    st_assert( vf->is_compiled_frame(), "must be compiled frame" );
    NativeMethod * nm = vf->code();

    nm->inc_uncommon_trap_counter();

    LOG_EVENT3( "Uncommon trap in 0x%lx@%d #%d", nm, vf->scope()->offset(), nm->uncommon_trap_counter() );

//    /* For Debugging inserted by Lars Bak 5-13-96
    if ( nm->is_block() ) {
        PrintUncommonBranches = true;
        TraceDeoptimization   = true;
    } else {
        PrintUncommonBranches = false;
        TraceDeoptimization   = false;
    }
//    */

    if ( PrintUncommonBranches ) {

        _console->print( "%s trap in ", used ? "Uncommon" : "New uncommon" );
        nm->print_value_on( _console );
        _console->print( " #%d", nm->uncommon_trap_counter() );

        if ( WizardMode )
            _console->print( " @%d called from %#x", vf->scope()->offset(), f.pc() - static_cast<int>( Assembler::Constants::sizeOfCall ) );
        _console->cr();

        if ( TraceDeoptimization )
            vf->print_activation( 0 );
        process->trace_top( 0, 3 );

    }

    // if counter is high enough, recompile the NativeMethod
    if ( RecompilationPolicy::shouldRecompileAfterUncommonTrap( nm ) ) {
        if ( nm->isZombie() )
            nm->resurrect();
        Recompilation recomp( vf->receiver(), nm, true );
        VMProcess::execute( &recomp );
        nm->makeZombie( false ); // only allow the old method to be release AFTER recompilation
    }

    {
        FinalResourceMark    rm;
        EnableDeoptimization ed; // Wrapper that enables canonicalization when deoptimizing.

        DeltaProcess::deoptimize_redo_last_send();
        process->deoptimize_stretch( &f, &f );
        if ( not nm->is_method() ) {
            // This is a top level block NativeMethod so we have to make sure all frames on the stack
            // referring the context chain are deoptimized.
            // Frames on other stacks might be candidates for deoptimization but are ignored for now.  fix this later
            //
            // Recipe:
            //   walk the stack and collect a work list of {frame, compiled_context} pairs.
            //   iterate over the work list until no deoptimized contextOops are present.
            GrowableArray <FrameAndContextElement *> * elements = new GrowableArray <FrameAndContextElement *>( 10 );

            for ( Frame current_frame = f.sender(); not current_frame.is_first_frame(); current_frame = current_frame.sender() ) {
                collect_compiled_contexts_for( &current_frame, elements );
            }

            bool_t done = false;

            while ( not done ) {
                done = true;
                for ( int i = 0; i < elements->length() and done; i++ ) {
                    FrameAndContextElement * e = elements->at( i );
                    if ( e and e->con->unoptimized_context() ) {
                        process->deoptimize_stretch( &e->fr, &e->fr );

                        for ( int j = 0; j < elements->length(); j++ ) {
                            if ( elements->at( j ) and elements->at( j )->fr.fp() == e->fr.fp() )
                                elements->at_put( j, nullptr );
                        }
                        done = false;
                    }
                }
            }
        }
    }

    process->exit_uncommon();
}
