//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Frame.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/utilities/lprintf.hpp"


uint8_t * Frame::hp() const {
    // Lars, please check -- assertion fails
    // st_assert(is_nullptrinterpreted_frame(), "must be interpreted");
    return *hp_addr();
}


void Frame::set_hp( uint8_t * hp ) {
    // st_assert(is_interpreted_frame(), "must be interpreted");
    *hp_addr() = hp;
}


void Frame::patch_pc( const char * pc ) {
    const char ** pc_addr = ( const char ** ) sp() - 1;
    *pc_addr = pc;
}


ObjectArrayOop * Frame::frame_array_addr() const {
    st_assert( frame_size() >= minimum_size_for_deoptimized_frame, "Compiler frame is too small for deoptimization" );
//    st_assert( is_deoptimized_frame(), "must be deoptimized frame" );
    return ( ObjectArrayOop * ) addr_at( frame_frame_array_offset );
}


ObjectArrayOop Frame::frame_array() const {
    ObjectArrayOop result = *frame_array_addr();
    st_assert( result->is_objArray(), "must be objArray" );
    return result;
}


Oop ** Frame::real_sender_sp_addr() const {
//    st_assert( is_deoptimized_frame(), "must be deoptimized frame" );
    return ( Oop ** ) addr_at( frame_real_sender_sp_offset );
}


void Frame::patch_fp( int * fp ) {
    Frame previous( nullptr, ( ( int * ) sp() ) - frame_sender_sp_offset, nullptr );
    previous.set_link( fp );
}


MethodOop Frame::method() const {
    st_assert( is_interpreted_frame(), "must be interpreter frame" );
    // First we will check the interpreter frame is valid by checking the frame size.
    // The interpreter guarantees hp is valid if the frame is at least 4 in size.
    // (return address, link, receiver, hcode pointer)
    if ( frame_size() < minimum_size_for_deoptimized_frame )
        return nullptr;

    uint8_t * h = hp();
    if ( not Universe::old_gen.contains( h ) )
        return nullptr;
    MemOop obj = as_memOop( Universe::object_start( ( Oop * ) h ) );
    return obj->is_method() ? MethodOop( obj ) : nullptr;
}


NativeMethod * Frame::code() const {
    st_assert( is_compiled_frame(), "no code" );
    return findNativeMethod( pc() );
}


bool_t Frame::is_interpreted_frame() const {
    return Interpreter::contains( pc() );
}


bool_t Frame::is_compiled_frame() const {
    return Universe::code->contains( pc() );
}


bool_t Frame::is_deoptimized_frame() const {
    return pc() == StubRoutines::unpack_unoptimized_frames();
}


InlineCacheIterator * Frame::sender_ic_iterator() const {
    return is_entry_frame() ? nullptr : sender().current_ic_iterator();
}


InlineCacheIterator * Frame::current_ic_iterator() const {

    if ( is_interpreted_frame() ) {
        InterpretedInlineCache * ic = current_interpretedIC();
        if ( ic and not ByteCodes::is_send_code( ic->send_code() ) )
            return nullptr;
        return ic ? new InterpretedInlineCacheIterator( ic ) : nullptr;
    }

    if ( is_compiled_frame() ) {
        CompiledInlineCache * ic = current_compiledIC();
        return ic->inlineCache() ? new CompiledInlineCacheIterator( ic ) : nullptr; // a perform, not a send
    }

    // entry or deoptimized frame
    return nullptr;
}


InterpretedInlineCache * Frame::current_interpretedIC() const {

    if ( is_interpreted_frame() ) {
        MethodOop m             = method();
        int       byteCodeIndex = m->byteCodeIndex_from( hp() );
        uint8_t * codeptr = m->codes( byteCodeIndex );
        if ( ByteCodes::is_send_code( ByteCodes::Code( *codeptr ) ) ) {
            InterpretedInlineCache * ic = as_InterpretedIC( ( const char * ) hp() );
            st_assert( ic->send_code_addr() == codeptr, "found wrong ic" );
            return ic;
        } else {
            return nullptr;      // perform, dll call, etc.
        }
    }

    return nullptr;      // doesn't have InterpretedInlineCache
}


CompiledInlineCache * Frame::current_compiledIC() const {
    return is_compiled_frame() ? CompiledIC_from_return_addr( pc() )  // may fail if current frame isn't at a send -- caller must know
                               : nullptr;
}


bool_t Frame::is_entry_frame() const {
    return pc() == StubRoutines::return_from_Delta();
}


bool_t Frame::has_next_Delta_fp() const {
    return at( frame_next_Delta_fp_offset ) not_eq 0;
}


int * Frame::next_Delta_fp() const {
    return ( int * ) at( frame_next_Delta_fp_offset );
}


Oop * Frame::next_Delta_sp() const {
    return ( Oop * ) at( frame_next_Delta_sp_offset );
}


bool_t Frame::is_first_frame() const {
    return is_entry_frame() and not has_next_Delta_fp();
}


bool_t Frame::is_first_delta_frame() const {
    // last Delta frame isn't necessarily is_first_frame(), so check is a bit more complicated
    // [I don't understand why, but the first Delta frame of a process isn't an entry frame  -Urs 2/96]
    Frame s;
    for ( s = sender(); not( s.is_delta_frame() or s.is_first_frame() ); s = s.sender() );
    return s.is_first_frame();
}


const char * Frame::print_name() const {
    if ( is_interpreted_frame() )
        return "interpreted";
    if ( is_compiled_frame() )
        return "compiled";
    if ( is_deoptimized_frame() )
        return "deoptimized";
    return "C";
}


void Frame::print() const {
    _console->print( "[%s frame: fp = %#lx, sp = %#lx, pc = %#lx", print_name(), fp(), sp(), pc() );
    if ( is_compiled_frame() ) {
        _console->print( ", nm = %#x", findNativeMethod( pc() ) );
    } else if ( is_interpreted_frame() ) {
        _console->print( ", hp = %#x, method = %#x", hp(), method() );
    }
    _console->print_cr( "]" );

    if ( PrintLongFrames ) {
        for ( Oop * p = sp(); p < ( Oop * ) fp(); p++ )
            _console->print_cr( "  - 0x%lx: 0x%lx", p, *p );
    }
}


static void print_context_chain( ContextOop con, ConsoleOutputStream * stream ) {
    if ( con ) {
        // Print out the contexts chain
        stream->print( "    context " );
        con->print_value_on( stream );
        while ( con->has_outer_context() ) {
            con = con->outer_context();
            stream->print( " -> " );
            con->print_value_on( stream );
        }
        stream->cr();
    }
}


void Frame::print_for_deoptimization( ConsoleOutputStream * stream ) {
    ResourceMark resourceMark;
    stream->print( " - " );
    if ( is_interpreted_frame() ) {
        stream->print( "I " );
        InterpretedVirtualFrame * vf = ( InterpretedVirtualFrame * ) VirtualFrame::new_vframe( this );
        vf->method()->print_value_on( stream );
        if ( ActivationShowByteCodeIndex ) {
            stream->print( " byteCodeIndex=0x%08x ", vf->byteCodeIndex() );
        }
        _console->print_cr( " @ 0x%lx", fp() );
        print_context_chain( vf->interpreter_context(), stream );
        if ( ActivationShowExpressionStack ) {
            GrowableArray <Oop> * stack = vf->expression_stack();
            for ( int index = 0; index < stack->length(); index++ ) {
                stream->print( "    %3d: ", index );
                stack->at( index )->print_value_on( stream );
                stream->cr();
            }
        }
        return;
    }

    if ( is_compiled_frame() ) {
        stream->print( "C " );
        CompiledVirtualFrame * vf = ( CompiledVirtualFrame * ) VirtualFrame::new_vframe( this );
        st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
        vf->code()->print_value_on( stream );
        _console->print_cr( " @ 0x%lx", fp() );

        while ( true ) {
            stream->print( "    " );
            vf->method()->print_value_on( stream );
            _console->print_cr( " @ 0x%08x", vf->scope()->offset() );
            print_context_chain( vf->compiled_context(), stream );
            if ( vf->is_top() )
                break;
            vf = ( CompiledVirtualFrame * ) vf->sender();
            st_assert( vf->is_compiled_frame(), "should be compiled VirtualFrame" );
        }
        return;
    }

    if ( is_deoptimized_frame() ) {
        stream->print( "D " );
        frame_array()->print_value();
        _console->print_cr( " @ 0x%lx", fp() );

        DeoptimizedVirtualFrame * vf = ( DeoptimizedVirtualFrame * ) VirtualFrame::new_vframe( this );
        st_assert( vf->is_deoptimized_frame(), "should be deoptimized VirtualFrame" );
        while ( true ) {
            stream->print( "    " );
            vf->method()->print_value_on( stream );
            _console->cr();
            print_context_chain( vf->deoptimized_context(), stream );
            if ( vf->is_top() )
                break;
            vf = ( DeoptimizedVirtualFrame * ) vf->sender();
            st_assert( vf->is_deoptimized_frame(), "should be deoptimized VirtualFrame" );
        }
        return;
    }

    stream->print( "E foreign frame @ 0x%lx", fp() );
}


void Frame::layout_iterate( FrameLayoutClosure * blk ) {
    if ( is_interpreted_frame() ) {
        Oop       * eos = temp_addr( 0 );
        for ( Oop * p   = sp(); p <= eos; p++ )
            blk->do_stack( eos - p, p );
        blk->do_hp( hp_addr() );
        blk->do_receiver( receiver_addr() );
        blk->do_link( link_addr() );
        blk->do_return_addr( return_addr_addr() );
    }
}


bool_t Frame::has_interpreted_float_marker() const {
    return Oop( at( interpreted_frame_float_magic_offset ) ) == Floats::magic_value();
}


bool_t Frame::has_compiled_float_marker() const {
    return Oop( at( compiled_frame_magic_oop_offset ) ) == Floats::magic_value();
}


bool_t Frame::oop_iterate_interpreted_float_frame( OopClosure * blk ) {
    MethodOop m = MethodOopDescriptor::methodOop_from_hcode( hp() );
    // Return if this activation has no floats (the marker is conservative)
    if ( not m->has_float_temporaries() )
        return false;

    // Iterator from stack pointer to end of float section
    Oop       * end = ( Oop * ) addr_at( m->float_section_start_offset() - m->float_section_size() );
    for ( Oop * p   = sp(); p <= end; p++ ) {
        blk->do_oop( p );
    }

    // Skip the float section and magic_value

    // Iterate from just before the float section to the first temp
    for ( Oop * q = ( Oop * ) addr_at( m->float_section_start_offset() + 2 ); q <= temp_addr( 0 ); q++ ) {
        blk->do_oop( q );
    }

    // The receiver
    blk->do_oop( receiver_addr() );

    return true;
}


bool_t Frame::oop_iterate_compiled_float_frame( OopClosure * blk ) {
    warning( "oop_iterate_compiled_float_frame not implemented" );
    return false;
}


void Frame::oop_iterate( OopClosure * blk ) {
    if ( is_interpreted_frame() ) {
        if ( has_interpreted_float_marker() and oop_iterate_interpreted_float_frame( blk ) )
            return;

        // lprintf("Frame: fp = %#lx, sp = %#lx]\n", fp(), sp());
        for ( Oop * p = sp(); p <= temp_addr( 0 ); p++ ) {
            // lprintf("\t[%#lx]: ", p);
            // (*p)->short_print();
            // lprintf("\n");
            blk->do_oop( p );
        }
        // lprintf("\t{%#lx}: ", receiver_addr());
        // (*receiver_addr())->short_print();
        // lprintf("\n");
        blk->do_oop( receiver_addr() );
        return;
    }

    if ( is_compiled_frame() ) {
        if ( has_compiled_float_marker() and oop_iterate_compiled_float_frame( blk ) )
            return;

        // All oops are [sp..fp[
        for ( Oop * p = sp(); p < ( Oop * ) fp(); p++ ) {
            blk->do_oop( p );
        }
        return;
    }

    if ( is_entry_frame() ) {
        // Need to iterate over the arguments passed to the frame called by the entry frame,
        // but not the rest of the cruft (esi, edi, last sp and last fp) - slr 09/08.
        for ( Oop * p = sp(); p < ( Oop * ) fp() - 4; p++ ) {
            blk->do_oop( p );
        }
        return;
    }

    if ( is_deoptimized_frame() ) {
        // Expression stack
        Oop       * end = ( Oop * ) fp() + frame_real_sender_sp_offset;
        // All oops are [sp..end[
        for ( Oop * p   = sp(); p < end; p++ ) {
            blk->do_oop( p );
        }
        blk->do_oop( ( Oop * ) frame_array_addr() );
        return;
    }
}


bool_t Frame::follow_roots_interpreted_float_frame() {
    MethodOop m = MethodOop( hp() );
    st_assert( m->is_method(), "must be method" );
    // Return if this activation has no floats (the marker is conservative)
    if ( not m->has_float_temporaries() )
        return false;

    // Iterator from stack pointer to end of float section
    Oop       * end = ( Oop * ) addr_at( m->float_section_start_offset() - m->float_section_size() );
    for ( Oop * p   = sp(); p <= end; p++ ) {
        MarkSweep::follow_root( p );
    }

    // Skip the float section and magic_value

    // Iterate from just before the float section to the first temp
    for ( Oop * q = ( Oop * ) addr_at( m->float_section_start_offset() + 2 ); q <= temp_addr( 0 ); q++ ) {
        MarkSweep::follow_root( q );
    }

    // The receiver
    MarkSweep::follow_root( receiver_addr() );

    return true;
}


bool_t Frame::follow_roots_compiled_float_frame() {
    warning( "follow_roots_compiled_float_frame not implemented" );
    return true;
}


void Frame::follow_roots() {
    if ( is_interpreted_frame() ) {
        if ( has_interpreted_float_marker() and follow_roots_interpreted_float_frame() )
            return;

        // Follow the roots of the frame
        for ( Oop * p = sp(); p <= temp_addr( 0 ); p++ ) {
            MarkSweep::follow_root( p );
        }
        MarkSweep::follow_root( ( Oop * ) hp_addr() );
        MarkSweep::follow_root( receiver_addr() );
        return;
    }

    if ( is_compiled_frame() ) {
        if ( has_compiled_float_marker() and follow_roots_compiled_float_frame() )
            return;

        for ( Oop * p = sp(); p < ( Oop * ) fp(); p++ )
            MarkSweep::follow_root( p );
        return;
    }

    if ( is_entry_frame() ) {
        //for (Oop* p = sp(); p < (Oop*)fp(); p++) {
        // %hack
        //   MarkSweep::follow_root(p);
        //}
        // Should be the following to match oop_iterate() (used by scavenge) slr - 11/09
        // %TODO how to test?
        for ( Oop * p = sp(); p < ( Oop * ) fp() - 4; p++ ) {
            MarkSweep::follow_root( p );
        }
        return;
    }

    if ( is_deoptimized_frame() ) {
        // Expression stack
        Oop       * end = ( Oop * ) fp() + frame_real_sender_sp_offset;
        for ( Oop * p   = sp(); p < end; p++ )
            MarkSweep::follow_root( p );
        MarkSweep::follow_root( ( Oop * ) frame_array_addr() );
        return;
    }
}


void Frame::convert_heap_code_pointer() {
    if ( not is_interpreted_frame() )
        return;
    // Adjust hcode pointer to object start
    uint8_t * h   = hp();
    uint8_t * obj = ( uint8_t * ) as_memOop( Universe::object_start( ( Oop * ) h ) );
    set_hp( obj );
    // Save the offset
    MarkSweep::add_heap_code_offset( h - obj );
    if ( WizardMode )
        lprintf( "[0x%lx+%d]\n", obj, h - obj );
}


void Frame::restore_heap_code_pointer() {
    if ( not is_interpreted_frame() )
        return;
    // Readjust hcode pointer
    uint8_t * obj = hp();
    int offset = MarkSweep::next_heap_code_offset();
    if ( WizardMode )
        lprintf( "[0x%lx+%d]\n", obj, offset );
    set_hp( obj + offset );
}


class VerifyOopClosure : public OopClosure {
    public:
        Frame * fr;


        void do_oop( Oop * o ) {
            Oop obj = *o;
            if ( not obj->verify() ) {
                lprintf( "Verify failed in frame:\n" );
                fr->print();
            }
        }
};


void Frame::verify() const {
    if ( fp() == nullptr ) fatal( "fp cannot be nullptr" );
    if ( sp() == nullptr ) fatal( "sp cannot be nullptr" );
    VerifyOopClosure blk;
    blk.fr = ( Frame * ) this;
    ( ( Frame * ) this )->oop_iterate( &blk );
}


Frame Frame::sender() const {
    Frame result;
    if ( is_entry_frame() ) {
        // Delta frame called from C; skip all C frames and return top C
        // frame of that chunk as the sender
        st_assert( has_next_Delta_fp(), "next Delta fp must be non zero" );
        st_assert( next_Delta_fp() > _fp, "must be above this frame on stack" );
        result = Frame( next_Delta_sp(), next_Delta_fp() );
    } else if ( is_deoptimized_frame() ) {
        result = Frame( real_sender_sp(), link(), return_addr() );
    } else {
        result = Frame( sender_sp(), link(), return_addr() );
    }
    return result;
}


Frame Frame::delta_sender() const {
    Frame s = sender();
    for ( ; not s.is_delta_frame(); s = s.sender() );
    return s;
}


bool_t Frame::should_be_deoptimized() const {
    if ( not is_compiled_frame() )
        return false;
    NativeMethod * nm = code();
    if ( TraceApplyChange ) {
        _console->print( "checking (%s) ", nm->is_marked_for_deoptimization() ? "true" : "false" );
        nm->print_value_on( _console );
        _console->cr();
    }
    return nm->is_marked_for_deoptimization();
}
