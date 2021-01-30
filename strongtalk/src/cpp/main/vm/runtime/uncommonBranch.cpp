
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/recompiler/RecompilationPolicy.hpp"
#include "vm/runtime/uncommonBranch.hpp"
#include "vm/runtime/ResourceMark.hpp"


// -----------------------------------------------------------------------------

// Tells whether the call has been executed before
// %note:
//    This function is highly INTEL specific.

bool patch_uncommon_call( Frame *f ) {
    // patch the call site:
    //  from: call _unused_uncommon_trap
    //  to:   call _used_uncommon_trap

    std::int32_t *next_inst = (std::int32_t *) f->pc();
    std::int32_t *dest_addr = next_inst - 1;
    std::int32_t dest       = *dest_addr + (std::int32_t) next_inst;

    // return true if the call has been executed before
    if ( dest == (std::int32_t) StubRoutines::used_uncommon_trap_entry() ) {
        return true;
    }

    st_assert( dest == (std::int32_t) StubRoutines::unused_uncommon_trap_entry(), "Make sure we are patching the right call" );

    // patch with used_uncommon_trap
    *dest_addr = ( (std::int32_t) StubRoutines::used_uncommon_trap_entry() ) - ( (std::int32_t) next_inst );

    st_assert( *dest_addr + (std::int32_t) next_inst == (std::int32_t) StubRoutines::used_uncommon_trap_entry(), "Check the patch" );

    // return false since the call is patched
    return false;
}


// Tells whether the frame is a candidate for deoptimization by
// checking if the frame uses contextOops with forward pointers.
static bool has_invalid_context( Frame *f ) {
    // Return false if we're not in compiled code
    if ( not f->is_compiled_frame() ) {
        return false;
    }

    // Iterate over the vframes and check the compiled_context
    CompiledVirtualFrame *vf = (CompiledVirtualFrame *) VirtualFrame::new_vframe( f );
    st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
    while ( true ) {
        ContextOop con = vf->compiled_context();

        // spdlog::info("checking context fp = 0x%lx, pc = 0x%lx", f->fp(), f->pc());
        if ( con )
            con->print();

        if ( con and con->unoptimized_context() )
            return true;
        if ( vf->is_top() )
            break;
        vf = (CompiledVirtualFrame *) vf->sender();
        st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
    }

    return false;
}


void collect_compiled_contexts_for( Frame *f, GrowableArray<FrameAndContextElement *> *elements ) {
    // Return false if we're not in compiled code
    if ( not f->is_compiled_frame() ) {
        return;
    }

    // Iterate over the vframes and check the compiled_context
    CompiledVirtualFrame *vf = (CompiledVirtualFrame *) VirtualFrame::new_vframe( f );
    st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
    while ( true ) {
        ContextOop con = vf->compiled_context();
        if ( con ) {
            elements->append( new FrameAndContextElement( f, con ) );
        }
        if ( vf->is_top() )
            break;
        vf = (CompiledVirtualFrame *) vf->sender();
        st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
    }
}


void uncommon_trap() {

    if ( UseNewBackend ) {
        spdlog::warn( "uncommon traps not supported yet for new backend" );
        Unimplemented();
    }

    ResourceMark resourceMark;
    // Find the frame that caused the uncommon trap
    DeltaProcess *process = DeltaProcess::active();

    process->verify();

    FlagSetting fl( processSemaphore, true );

    process->enter_uncommon();
    Frame f = process->last_frame();

    // Patch the call destination if necessary
    bool used = patch_uncommon_call( &f );

    // Find the NativeMethod containing the uncommon trap
    CompiledVirtualFrame *vf = (CompiledVirtualFrame *) VirtualFrame::new_vframe( &f );
    st_assert( vf->is_compiled_frame(), "must be compiled frame" );
    NativeMethod *nm = vf->code();

    nm->inc_uncommon_trap_counter();

    spdlog::info( "Uncommon trap in 0x{x} @ {d} #{d}", static_cast<const void *>( nm ), vf->scope()->offset(), nm->uncommon_trap_counter() );

    if ( nm->is_block() ) {
        PrintUncommonBranches = true;
        TraceDeoptimization   = true;
    } else {
        PrintUncommonBranches = false;
        TraceDeoptimization   = false;
    }

    if ( PrintUncommonBranches ) {

        _console->print( "%s trap in ", used ? "Uncommon" : "New uncommon" );
        nm->print_value_on( _console );
        _console->print( " #%d", nm->uncommon_trap_counter() );

        if ( WizardMode )
            spdlog::info( "{:d} called from {:x}", vf->scope()->offset(), f.pc() - static_cast<std::int32_t>( Assembler::Constants::sizeOfCall ) );
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
            GrowableArray<FrameAndContextElement *> *elements = new GrowableArray<FrameAndContextElement *>( 10 );

            for ( Frame current_frame = f.sender(); not current_frame.is_first_frame(); current_frame = current_frame.sender() ) {
                collect_compiled_contexts_for( &current_frame, elements );
            }

            bool done = false;

            while ( not done ) {
                done = true;
                for ( std::int32_t i = 0; i < elements->length() and done; i++ ) {
                    FrameAndContextElement *e = elements->at( i );
                    if ( e and e->_context->unoptimized_context() ) {
                        process->deoptimize_stretch( &e->_frame, &e->_frame );

                        for ( std::int32_t j = 0; j < elements->length(); j++ ) {
                            if ( elements->at( j ) and elements->at( j )->_frame.fp() == e->_frame.fp() ) {
                                elements->at_put( j, nullptr );
                            }
                        }

                        done = false;
                    }
                }
            }
        }
    }

    process->exit_uncommon();
}
