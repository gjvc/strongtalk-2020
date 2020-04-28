//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/ObjectArrayKlass.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"


InterpretedInlineCache * as_InterpretedIC( const char * address_of_next_instr ) {
    return ( InterpretedInlineCache * ) ( address_of_next_instr - InterpretedInlineCache::size );
}




// Implementation of Interpreter_PICs
//
// A simple free list manager for interpreted PICs.

class Interpreter_PICs : AllStatic {

    public:
        static ObjectArrayOop free_list() {
            return Universe::pic_free_list();
        }


        static ObjectArrayOop allocate( int size ) {
            Oop first = free_list()->obj_at( size - 1 );
            if ( first == nilObj ) {
                return ObjectArrayKlass::allocate_tenured_pic( size * 2 );
            }
            free_list()->obj_at_put( size - 1, ObjectArrayOop( first )->obj_at( 1 ) );

            ObjectArrayOop result = ObjectArrayOop( first );
            st_assert( result->is_objArray(), "must be object array" );
            st_assert( result->is_old(), "must be tenured" );
            st_assert( result->length() == size * 2, "checking size" );
            return result;
        }


        static ObjectArrayOop extend( ObjectArrayOop old_pic ) {
            int old_size = old_pic->length() / 2;
            if ( old_size >= size_of_largest_interpreterPIC )
                return nullptr;
            ObjectArrayOop result = allocate( old_size + 1 );
            for ( int      index  = 1; index <= old_size * 2; index++ ) {
                result->obj_at_put( index, old_pic->obj_at( index ) );
            }
            return result;
        }


        static void deallocate( ObjectArrayOop pic ) {
            int entry = ( pic->length() / 2 ) - 1;
            Oop first = free_list()->obj_at( entry );
            pic->obj_at_put( 1, first );
            free_list()->obj_at_put( entry, pic );
        }


        static void set_first( ObjectArrayOop pic, Oop first, Oop second ) {
            pic->obj_at_put( 1, first );
            pic->obj_at_put( 2, second );
        }


        static void set_second( ObjectArrayOop pic, Oop first, Oop second ) {
            pic->obj_at_put( 3, first );
            pic->obj_at_put( 4, second );
        }


        static void set_last( ObjectArrayOop pic, Oop first, Oop second ) {
            int size = pic->length();
            pic->obj_at_put( size--, second );
            pic->obj_at_put( size, first );
        }
};


// Implementation of InterpretedInlineCache

void InterpretedInlineCache::set( ByteCodes::Code send_code, Oop first_word, Oop second_word ) {

    // Remember pics are allocated in new Space and require a store check when saved in the InlineCache
    // Note: If the second_word is a PolymorphicInlineCache, it has to be deallocated before using set!

    *send_code_addr() = static_cast<uint8_t>(send_code); /// XXXXX gjvc XXXX
    Universe::store( first_word_addr(), first_word, first_word->is_new() );
    Universe::store( second_word_addr(), second_word, second_word->is_new() );
}


uint8_t * InterpretedInlineCache::findStartOfSend( uint8_t * sel_addr ) {
    uint8_t * p = sel_addr;            // start of inline cache
    while ( *--p == static_cast<uint8_t>(ByteCodes::Code::halt) );    // skip alignment bytes if there - this works only if the nofArgs byte not_eq halt ! FIX THIS!
    if ( *p < 128 )
        --p;            // skip nofArgs byte if there - this works only for sends with less than 128 args
    // and assumes that no send bytecodes is smaller than 128. FIX THIS!!!
    if ( not ByteCodes::is_send_code( ByteCodes::Code( *p ) ) ) {
        return nullptr;   // not a send
    }
    return p;
    // Clean solution:
    //
    // make all send bytecodes holding the no. of arguments,
    // use special filler value (e.g. restrict no of. args to 254).
    // not urgent (gri 1/29/96)
}


int InterpretedInlineCache::findStartOfSend( MethodOop m, int byteCodeIndex ) {
    uint8_t * p = findStartOfSend( m->codes( byteCodeIndex ) );
    return ( p == nullptr ) ? IllegalByteCodeIndex : p - m->codes() + 1;
}


SymbolOop InterpretedInlineCache::selector() const {
    Oop fw = first_word();
    if ( fw->is_symbol() ) {
        return SymbolOop( fw );
    } else if ( fw->is_method() ) {
        return MethodOop( fw )->selector();
    } else {
        JumpTableEntry * e  = jump_table_entry();
        NativeMethod   * nm = e->method();
        st_assert( nm not_eq nullptr, "must have an NativeMethod" );
        return nm->_lookupKey.selector();
    }
}


JumpTableEntry * InterpretedInlineCache::jump_table_entry() const {
    st_assert( send_type() == ByteCodes::SendType::compiled_send or send_type() == ByteCodes::SendType::megamorphic_send, "must be a compiled call" );
    st_assert( first_word()->is_smi(), "must be smi_t" );
    return ( JumpTableEntry * ) first_word();
}


int InterpretedInlineCache::nof_arguments() const {
    uint8_t * p = send_code_addr();
    switch ( ByteCodes::argument_spec( ByteCodes::Code( *p ) ) ) {
        case ByteCodes::ArgumentSpec::recv_0_args:
            return 0;
        case ByteCodes::ArgumentSpec::recv_1_args:
            return 1;
        case ByteCodes::ArgumentSpec::recv_2_args:
            return 2;
        case ByteCodes::ArgumentSpec::recv_n_args: {
            int n = selector()->number_of_arguments();
            st_assert( n = int( *( p + 1 ) ), "just checkin'..." ); // send_code_addr()+1 must hold the number of arguments
            return n;
        }
        case ByteCodes::ArgumentSpec::args_only:
            return selector()->number_of_arguments();
    }
    ShouldNotReachHere();
    return 0;
}


ByteCodes::SendType InterpretedInlineCache::send_type() const {
    return ByteCodes::send_type( send_code() );
}


ByteCodes::ArgumentSpec InterpretedInlineCache::argument_spec() const {
    return ByteCodes::argument_spec( send_code() );
}


void InterpretedInlineCache::clear() {

    if ( is_empty() )
        return;

    if ( send_type() == ByteCodes::SendType::polymorphic_send ) {
        // recycle PolymorphicInlineCache
        st_assert( second_word()->is_objArray(), "must be a pic" );
        Interpreter_PICs::deallocate( ObjectArrayOop( second_word() ) );
    }

    set( ByteCodes::original_send_code_for( send_code() ), Oop( selector() ), smiOop_zero );
}


void InterpretedInlineCache::replace( LookupResult result, KlassOop receiver_klass ) {
    // InlineCache entries before modification - used for loging only
    ByteCodes::Code code_before  = send_code();
    Oop             word1_before = first_word();
    Oop             word2_before = second_word();
    int             transition   = 0;

    // modify InlineCache
    guarantee( word2_before == receiver_klass, "klass should be the same" );
    if ( result.is_empty() ) {
        clear();
        transition = 1;
    } else if ( result.is_method() ) {
        if ( send_type() == ByteCodes::SendType::megamorphic_send ) {
            set( send_code(), result.method(), receiver_klass );
            transition = 2;
        } else {
            // Please Fix this Robert
            // implement set_monomorphic(klass, method)
            clear();
            transition = 3;
        }
    } else {
        if ( send_type() == ByteCodes::SendType::megamorphic_send ) {
            set( send_code(), Oop( result.entry() ), receiver_klass );
            transition = 4;
        } else {
            st_assert( result.is_entry(), "must be jump table entry" );
            // a jump table entry of a NativeMethod is found so let's update the current send
            set( ByteCodes::compiled_send_code_for( send_code() ), Oop( result.entry() ), receiver_klass );
            transition = 5;
        }
    }

    // InlineCache entries after modification - used for loging only
    ByteCodes::Code code_after  = send_code();
    Oop             word1_after = first_word();
    Oop             word2_after = second_word();

    // log modification
    LOG_EVENT3( "InterpretedInlineCache::replace: InlineCache at 0x%x: entry for klass 0x%x replaced (transition %d)", this, receiver_klass, transition );
    LOG_EVENT3( "  from (%s, 0x%x, 0x%x)", ByteCodes::name( code_before ), word1_before, word2_before );
    LOG_EVENT3( "  to   (%s, 0x%x, 0x%x)", ByteCodes::name( code_after ), word1_after, word2_after );
}


void InterpretedInlineCache::cleanup() {
    if ( is_empty() )
        return; // Nothing to cleanup

    switch ( send_type() ) {
        case ByteCodes::SendType::accessor_send:    // fall through
        case ByteCodes::SendType::primitive_send:   // fall through
        case ByteCodes::SendType::predicted_send:   // fall through
        case ByteCodes::SendType::interpreted_send: { // check if the interpreted send should be replaced by a compiled send
            KlassOop receiver_klass = KlassOop( second_word() );
            st_assert( receiver_klass->is_klass(), "receiver klass must be a klass" );
            MethodOop method = MethodOop( first_word() );
            st_assert( method->is_method(), "first word in interpreter InlineCache must be method" );
            if ( not ByteCodes::is_super_send( send_code() ) ) {
                // super sends cannot be handled since the sending method holder is unknown at this point.
                LookupKey    key( receiver_klass, selector() );
                LookupResult result = LookupCache::lookup( &key );
                if ( not result.matches( method ) ) {
                    replace( result, receiver_klass );
                }
            }
        }
            break;
        case ByteCodes::SendType::compiled_send: { // check if the current compiled send is valid
            KlassOop receiver_klass = KlassOop( second_word() );
            st_assert( receiver_klass->is_klass(), "receiver klass must be a klass" );
            JumpTableEntry * entry = ( JumpTableEntry * ) first_word();
            NativeMethod   * nm    = entry->method();
            LookupResult   result  = LookupCache::lookup( &nm->_lookupKey );
            if ( not result.matches( nm ) ) {
                replace( result, receiver_klass );
            }
        }
            break;
        case ByteCodes::SendType::megamorphic_send:
            // Note that with the current definition of is_empty()
            // this will not be called for normal megamorphic sends
            // since they store only the selector.
        {
            KlassOop receiver_klass = KlassOop( second_word() );
            if ( first_word()->is_smi() ) {
                JumpTableEntry * entry = ( JumpTableEntry * ) first_word();
                NativeMethod   * nm    = entry->method();
                LookupResult   result  = LookupCache::lookup( &nm->_lookupKey );
                if ( not result.matches( nm ) ) {
                    replace( result, receiver_klass );
                }
            } else {
                MethodOop method = MethodOop( first_word() );
                st_assert( method->is_method(), "first word in interpreter InlineCache must be method" );
                if ( not ByteCodes::is_super_send( send_code() ) ) {
                    // super sends cannot be handled since the sending method holder is unknown at this point.
                    LookupKey    key( receiver_klass, selector() );
                    LookupResult result = LookupCache::lookup( &key );
                    if ( not result.matches( method ) ) {
                        replace( result, receiver_klass );
                    }
                }
            }
        }
            break;
        case ByteCodes::SendType::polymorphic_send: {
            // %implementation note:
            //   when cleaning up we can always preserve the old pic since the
            //   the only transitions are:
            //     (compiled    -> compiled)
            //     (compiled    -> interpreted)
            //     (interpreted -> compiled)
            //   in case of a super send we do not have to cleanup because
            //   no nativeMethods are compiled for super sends.
            if ( not ByteCodes::is_super_send( send_code() ) ) {
                ObjectArrayOop pic = pic_array();
                for ( int   index = pic->length(); index > 0; index -= 2 ) {
                    KlassOop klass = KlassOop( pic->obj_at( index ) );
                    st_assert( klass->is_klass(), "receiver klass must be klass" );
                    Oop first = pic->obj_at( index - 1 );
                    if ( first->is_smi() ) {
                        JumpTableEntry * entry = ( JumpTableEntry * ) first;
                        NativeMethod   * nm    = entry->method();
                        LookupResult   result  = LookupCache::lookup( &nm->_lookupKey );
                        if ( not result.matches( nm ) ) {
                            pic->obj_at_put( index - 1, result.value() );
                        }
                    } else {
                        MethodOop method = MethodOop( first );
                        st_assert( method->is_method(), "first word in interpreter InlineCache must be method" );
                        LookupKey    key( klass, selector() );
                        LookupResult result = LookupCache::lookup( &key );
                        if ( not result.matches( method ) ) {
                            pic->obj_at_put( index - 1, result.value() );
                        }
                    }
                }
            }
        }
    }
}


void InterpretedInlineCache::clear_without_deallocation_pic() {
    if ( is_empty() )
        return;
    set( ByteCodes::original_send_code_for( send_code() ), Oop( selector() ), smiOop_zero );
}


void InterpretedInlineCache::replace( NativeMethod * nm ) {
    // replace entry with nm's klass by nm (if entry exists)
    SMIOop entry_point = SMIOop( nm->jump_table_entry()->entry_point() );
    st_assert( selector() == nm->_lookupKey.selector(), "mismatched selector" );
    if ( is_empty() )
        return;

    switch ( send_type() ) {
        case ByteCodes::SendType::accessor_send:    // fall through
        case ByteCodes::SendType::primitive_send:   // fall through
        case ByteCodes::SendType::predicted_send:   // fall through
        case ByteCodes::SendType::interpreted_send: { // replace the monomorphic interpreted send with compiled send
            KlassOop receiver_klass = KlassOop( second_word() );
            st_assert( receiver_klass->is_klass(), "receiver klass must be a klass" );
            if ( receiver_klass == nm->_lookupKey.klass() ) {
                set( ByteCodes::compiled_send_code_for( send_code() ), entry_point, nm->_lookupKey.klass() );
            }
        }
            break;
        case ByteCodes::SendType::compiled_send:   // fall through
        case ByteCodes::SendType::megamorphic_send:
            // replace the monomorphic compiled send with compiled send
            set( send_code(), entry_point, nm->_lookupKey.klass() );
            break;
        case ByteCodes::SendType::polymorphic_send: {
            ObjectArrayOop pic = pic_array();
            for ( int   index = pic->length(); index > 0; index -= 2 ) {
                KlassOop receiver_klass = KlassOop( pic->obj_at( index ) );
                st_assert( receiver_klass->is_klass(), "receiver klass must be klass" );
                if ( receiver_klass == nm->_lookupKey.klass() ) {
                    pic->obj_at_put( index - 1, entry_point );
                    return;
                }
            }
        }
            // did not find klass
            break;
        default: fatal( "unknown send type" );
    }
    LOG_EVENT3( "interpreted InlineCache at 0x%x: new NativeMethod 0x%x for klass 0x%x replaces old entry", this, nm, nm->_lookupKey.klass() );
}


void InterpretedInlineCache::print() {
    _console->print( "Inline cache (" );
    if ( is_empty() ) {
        _console->print( "empty" );
    } else {
        _console->print( ByteCodes::send_type_as_string( send_type() ) );
    }
    _console->print( ") " );
    selector()->print_value();
    _console->cr();
    InterpretedInlineCacheIterator it( this );
    while ( not it.at_end() ) {
        _console->print( "\t- klass: " );
        it.klass()->print_value();
        if ( it.is_interpreted() ) {
            _console->print_cr( ";\tmethod  %#x", it.interpreted_method() );
        } else {
            _console->print_cr( ";\tNativeMethod %#x", it.compiled_method() );
        }
        it.advance();
    }
}


ObjectArrayOop InterpretedInlineCache::pic_array() {
    st_assert( send_type() == ByteCodes::SendType::polymorphic_send, "Must be a polymorphic send site" );
    ObjectArrayOop result = ObjectArrayOop( second_word() );
    st_assert( result->is_objArray(), "interpreter pic must be object array" );
    st_assert( result->length() >= 4, "pic should contain at least two entries" );
    return result;
}


void InterpretedInlineCache::update_inline_cache( InterpretedInlineCache * ic, Frame * f, ByteCodes::Code send_code, KlassOop klass, LookupResult result ) {
    // update inline cache
    if ( ic->is_empty() and ic->send_type() not_eq ByteCodes::SendType::megamorphic_send ) {
        // fill ic for the first time
        ByteCodes::Code new_send_code = ByteCodes::Code::halt;
        if ( result.is_entry() ) {

            MethodOop method = result.method();
            if ( UseAccessMethods and ByteCodes::has_access_send_code( send_code ) and method->is_accessMethod() ) {
                // access method found ==> switch to access send
                new_send_code = ByteCodes::access_send_code_for( send_code );
                ic->set( new_send_code, method, klass );

            } else if ( UsePredictedMethods and ByteCodes::has_predicted_send_code( send_code ) and method->is_special_primitiveMethod() ) {
                // predictable method found ==> switch to predicted send
                // NB: ic of predicted send should be empty so that the compiler knows whether another type occurred or not
                // i.e., {predicted + empty} --> 1 class, {predicted + nonempty} --> 2 klasses (polymorphic)
                // but: this actually doesn't work (yet?) since the interpreter fills the ic on any failure (e.g. overflow)
                new_send_code = method->special_primitive_code();
                method        = MethodOop( ic->selector() ); // ic must stay empty
                klass         = nullptr;                     // ic must stay empty
                ic->set( new_send_code, method, klass );

            } else {
                // jump table entry found ==> switch to compiled send
                new_send_code = ByteCodes::compiled_send_code_for( send_code );
                ic->set( new_send_code, Oop( result.entry()->entry_point() ), klass );
            }
        } else {
            // methodOop found
            MethodOop method = result.method();

            if ( UseAccessMethods and ByteCodes::has_access_send_code( send_code ) and method->is_accessMethod() ) {
                // access method found ==> switch to access send
                new_send_code = ByteCodes::access_send_code_for( send_code );

            } else if ( UsePredictedMethods and ByteCodes::has_predicted_send_code( send_code ) and method->is_special_primitiveMethod() ) {
                // predictable method found ==> switch to predicted send
                // NB: ic of predicted send should be empty so that the compiler knows whether another type occurred or not
                // i.e., {predicted + empty} --> 1 class, {predicted + nonempty} --> 2 klasses (polymorphic)
                // but: this actually doesn't work (yet?) since the interpreter fills the ic on any failure (e.g. overflow)
                new_send_code = method->special_primitive_code();
                method        = MethodOop( ic->selector() ); // ic must stay empty
                klass         = nullptr;                     // ic must stay empty

            } else if ( UsePrimitiveMethods and method->is_primitiveMethod() ) {
                // primitive method found ==> switch to primitive send
                new_send_code = ByteCodes::primitive_send_code_for( send_code );
                Unimplemented(); // take this out when all primitive send bytecodes implemented

            } else {
                // normal interpreted send ==> do not change
                new_send_code = send_code;
                st_assert( new_send_code == ByteCodes::original_send_code_for( send_code ), "bytecode should not change" );
            }
            st_assert( new_send_code not_eq ByteCodes::Code::halt, "new_send_code not set" );
            ic->set( new_send_code, method, klass );
        }
    } else {
        // ic not empty
        switch ( ic->send_type() ) {
            // monomorphic send
            case ByteCodes::SendType::accessor_send   : // fall through
            case ByteCodes::SendType::predicted_send  : // fall through
            case ByteCodes::SendType::compiled_send   : // fall through
            case ByteCodes::SendType::interpreted_send: {
                // switch to polymorphic send with 2 entries
                ObjectArrayOop pic = Interpreter_PICs::allocate( 2 );
                Interpreter_PICs::set_first( pic, ic->first_word(), ic->second_word() );
                Interpreter_PICs::set_second( pic, result.value(), klass );
                ic->set( ByteCodes::polymorphic_send_code_for( send_code ), ic->selector(), pic );
                break;
            }

                // polymorphic send
            case ByteCodes::SendType::polymorphic_send: {

                ObjectArrayOop old_pic = ic->pic_array();
                ObjectArrayOop new_pic = Interpreter_PICs::extend( old_pic ); // add an entry to the PolymorphicInlineCache if appropriate
                if ( new_pic == nullptr ) {
                    // switch to megamorphic send
                    if ( ByteCodes::is_super_send( send_code ) ) {
                        ic->set( ByteCodes::megamorphic_send_code_for( send_code ), result.value(), klass );
                    } else {
                        ic->set( ByteCodes::megamorphic_send_code_for( send_code ), ic->selector(), nullptr );
                    }
                } else {
                    // still a polymorphic send, add entry and set ic to new_pic
                    Interpreter_PICs::set_last( new_pic, result.value(), klass );
                    ic->set( send_code, ic->selector(), new_pic );
                }
                // recycle old pic
                Interpreter_PICs::deallocate( old_pic );
                break;
            }

                // megamorphic send
            case ByteCodes::SendType::megamorphic_send: {
                if ( ByteCodes::is_super_send( send_code ) ) {
                    ic->set( send_code, result.value(), klass );
                }
                break;
            }

            default: ShouldNotReachHere();
        }
    }

    // redo send (reset instruction pointer)
    f->set_hp( ic->send_code_addr() );
}


extern "C" bool_t have_nlr_through_C;


Oop InterpretedInlineCache::does_not_understand( Oop receiver, InterpretedInlineCache * ic, Frame * f ) {
    MemOop    msg;
    SymbolOop sel;
    {
        // message not understood...
        BlockScavenge bs; // make sure that no scavenge happens
        KlassOop msgKlass = KlassOop( Universe::find_global( "Message" ) );
        Oop      obj      = msgKlass->klass_part()->allocateObject();
        st_assert( obj->is_mem(), "just checkin'..." );
        msg = MemOop( obj );
        int            nofArgs = ic->selector()->number_of_arguments();
        ObjectArrayOop args    = oopFactory::new_objArray( nofArgs );
        for ( int   i       = 1; i <= nofArgs; i++ ) {
            args->obj_at_put( i, f->expr( nofArgs - i ) );
        }
        // for now: assume instance variables are there...
        // later: should check this or use a VM interface:
        // msg->set_receiver(recv);
        // msg->set_selector(ic->selector());
        // msg->set_arguments(args);
        msg->raw_at_put( 2, receiver );
        msg->raw_at_put( 3, ic->selector() );
        msg->raw_at_put( 4, args );
        sel = oopFactory::new_symbol( "doesNotUnderstand:" );
        if ( interpreter_normal_lookup( receiver->klass(), sel ).is_empty() ) {
            // doesNotUnderstand: not found ==> process error
            {
                ResourceMark resourceMark;
                _console->print_cr( "LOOKUP ERROR" );
                sel->print_value();
                _console->print_cr( " not found" );
            }
            if ( DeltaProcess::active()->is_scheduler() ) {
                DeltaProcess::active()->trace_stack();
                fatal( "lookup error in scheduler" );
            } else {
                DeltaProcess::active()->suspend( ProcessState::lookup_error );
            }
            ShouldNotReachHere();
        }
    }
    // return marked result of doesNotUnderstand: message
    return Delta::call( receiver, sel, msg );

    // Solution: use an NativeMethod-like stub-routine called from
    // within a (possibly one-element) PolymorphicInlineCache (in order to keep
    // the method selector in the InlineCache. That stub does the
    // Delta:call and pops the right number of arguments
    // taken from the selector (the no. of arguments from
    // the selector can be optimized).
}


void InterpretedInlineCache::trace_inline_cache_miss( InterpretedInlineCache * ic, KlassOop klass, LookupResult result ) {
    _console->print( "InterpretedInlineCache lookup (" );
    klass->print_value();
    _console->print( ", " );
    ic->selector()->print_value();
    _console->print( ") --> " );
    result.print_short_on( _console );
    _console->cr();
}


ObjectArrayOop cacheMissResult( Oop result, int argCount ) {
    BlockScavenge  bs;
    ObjectArrayOop resultHolder = oopFactory::new_objArray( 2 );
    resultHolder->obj_at_put( 1, smiOopFromValue( argCount ) );
    resultHolder->obj_at_put( 2, result );
    return resultHolder;
}

// The following routine handles all inline cache misses in the interpreter
// by looking at the ic state and the send byte code issuing the send. The
// inline cache is updated and the send bytecode might be backpatched such
// that it corresponds to the inline cache contents.

Oop * InterpretedInlineCache::inline_cache_miss() {
    NoGCVerifier noGC;

    // get ic info
    Frame                  f         = DeltaProcess::active()->last_frame();
    InterpretedInlineCache * ic      = f.current_interpretedIC();
    ByteCodes::Code        send_code = ic->send_code();

    Oop receiver = ic->argument_spec() == ByteCodes::ArgumentSpec::args_only // Are we at a self or super send?
                   ? f.receiver()                                //  yes: take receiver of frame
                   : f.expr( ic->nof_arguments() );                //  no:  take receiver pushed before the arguments

    // do the lookup
    KlassOop     klass  = receiver->klass();
    LookupResult result = ByteCodes::is_super_send( send_code ) ? interpreter_super_lookup( ic->selector() ) : interpreter_normal_lookup( klass, ic->selector() );

    // tracing
    if ( TraceInlineCacheMiss ) {
        _console->print( "InlineCache miss, " );
        trace_inline_cache_miss( ic, klass, result );
    }
    // handle the lookup result
    if ( not result.is_empty() ) {
        update_inline_cache( ic, &f, ic->send_code(), klass, result );
        return nullptr;
    } else {
        return cacheMissResult( does_not_understand( receiver, ic, &f ), ic->nof_arguments() + ( ic->argument_spec() == ByteCodes::ArgumentSpec::args_only ? 0 : 1 ) )->objs( 1 );
    }
}

// Implementation of InterpretedInlineCacheIterator

InterpretedInlineCacheIterator::InterpretedInlineCacheIterator( InterpretedInlineCache * ic ) {
    _ic = ic;
    init_iteration();
}


void InterpretedInlineCacheIterator::set_klass( Oop k ) {
    st_assert( k->is_klass(), "not a klass" );
    _klass = KlassOop( k );
}


void InterpretedInlineCacheIterator::set_method( Oop m ) {
    if ( m->is_mem() ) {
        st_assert( m->is_method(), "must be a method" );
        _method       = ( MethodOop ) m;
        _nativeMethod = nullptr;
    } else {
        JumpTableEntry * e = ( JumpTableEntry * ) m;
        _nativeMethod = e->method();
        _method       = _nativeMethod->method();
    }
}


void InterpretedInlineCacheIterator::init_iteration() {
    _pic   = nullptr;
    _index = 0;
    // determine initial state
    switch ( _ic->send_type() ) {
        case ByteCodes::SendType::interpreted_send:
            if ( _ic->is_empty() ) {
                // anamorphic call site (has never been executed => no type information)
                _number_of_targets = 0;
                _info              = anamorphic;
            } else {
                // monomorphic call site
                _number_of_targets = 1;
                _info              = monomorphic;
                set_klass( _ic->second_word() );
                set_method( _ic->first_word() );
            }
            break;
        case ByteCodes::SendType::compiled_send:
            _number_of_targets = 1;
            _info              = monomorphic;
            set_klass( _ic->second_word() );
            st_assert( _ic->first_word()->is_smi(), "must have JumpTableEntry" );
            set_method( _ic->first_word() );
            st_assert( is_compiled(), "bad type" );
            break;
        case ByteCodes::SendType::accessor_send   : // fall through
        case ByteCodes::SendType::primitive_send:
            _number_of_targets = 1;
            _info              = monomorphic;
            set_klass( _ic->second_word() );
            set_method( _ic->first_word() );
            st_assert( is_interpreted(), "bad type" );
            break;
        case ByteCodes::SendType::megamorphic_send:
            // no type information stored
            _number_of_targets = 0;
            _info              = megamorphic;
            break;
        case ByteCodes::SendType::polymorphic_send:
            // information on many types
            _pic               = ObjectArrayOop( _ic->second_word() );
            _number_of_targets = _pic->length() / 2;
            _info              = polymorphic;
            set_klass( _pic->obj_at( 2 ) );
            set_method( _pic->obj_at( 1 ) );
            break;
        case ByteCodes::SendType::predicted_send:
            if ( _ic->is_empty() or _ic->second_word() == smiKlassObj ) {
                _number_of_targets = 1;
                _info              = monomorphic;
            } else {
                _number_of_targets = 2;
                _info              = polymorphic;
            }
            set_klass( smiKlassObj );
            set_method( interpreter_normal_lookup( smiKlassObj, selector() ).value() );
            st_assert( _method not_eq nullptr and _method->is_mem(), "this method must be there" );
            break;
        default: ShouldNotReachHere();
    }
    st_assert( ( number_of_targets() > 1 ) == ( _info == polymorphic ), "inconsistency" );
}


void InterpretedInlineCacheIterator::advance() {
    st_assert( not at_end(), "iterated over the end" );
    _index++;
    if ( not at_end() ) {
        if ( _pic not_eq nullptr ) {
            // polymorphic inline cache
            int index = _index + 1;    // array is 1-origin
            set_klass( _pic->obj_at( 2 * index ) );
            set_method( _pic->obj_at( 2 * index - 1 ) );
        } else {
            // predicted send with non_empty inline cache
            st_assert( _index < 2, "illegal index" );
            set_klass( _ic->second_word() );
            set_method( _ic->first_word() );
        }
    }
}


MethodOop InterpretedInlineCacheIterator::interpreted_method() const {
    if ( is_interpreted() ) {
        MethodOop m = ( MethodOop ) _method;
        st_assert( m->is_old(), "methods must be old" );
        m->verify();
        return m;
    } else {
        return compiled_method()->method();
    }
}


NativeMethod * InterpretedInlineCacheIterator::compiled_method() const {
    if ( not is_compiled() )
        return nullptr;
    if ( not _nativeMethod->isNativeMethod() ) fatal( "not an NativeMethod" ); // cheap test
    _nativeMethod->verify();    // very slow
    return _nativeMethod;
}


bool_t InterpretedInlineCacheIterator::is_super_send() const {
    return ByteCodes::is_super_send( _ic->send_code() );
}


void InterpretedInlineCacheIterator::print() {
    _console->print_cr( "InterpretedInlineCacheIterator %#x for ic %#x (%s)", this, _ic, selector()->as_string() );
}

