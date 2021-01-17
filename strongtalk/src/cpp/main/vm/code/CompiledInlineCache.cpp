//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/CompiledInlineCache.hpp"
#include "vm/runtime/evaluator.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/utilities/lprintf.hpp"


extern "C" void UncommonTrap() {
    ResourceMark resourceMark;
    DeltaProcess::active()->trace_stack();
    error( "Uncommon trap has been executed" );
    evaluator::read_eval_loop();
}


const char * CompiledInlineCache::normalLookupRoutine() {
    return StubRoutines::ic_normal_lookup_entry();
}


const char * CompiledInlineCache::superLookupRoutine() {
    return StubRoutines::ic_super_lookup_entry();
}


extern "C" const char * icNormalLookup( Oop recv, CompiledInlineCache * ic ) {
    // As soon as the lookup routine handles 'message not understood' correctly, allocation may take place.
    // Then we have to fix the lookup stub as well. (receiver cannot be saved/restored within the C frame).
    VerifyNoScavenge vna;
    return ic->normalLookup( recv );
}


bool_t CompiledInlineCache::is_empty() const {
    char * d = destination();
    return d == normalLookupRoutine() or d == superLookupRoutine();
}


int CompiledInlineCache::ntargets() const {
    if ( is_empty() )
        return 0;
    PolymorphicInlineCache * p = pic();
    return p not_eq nullptr ? p->number_of_targets() : 1;
}


void CompiledInlineCache::set_call_destination( const char * entry_point ) {
    // if the InlineCache has a PolymorphicInlineCache we deallocate the PolymorphicInlineCache before setting the entry_point
    PolymorphicInlineCache * p = pic();
    st_assert( p == nullptr or p->entry() not_eq entry_point, "replacing with same address -- shouldn't dealloc" );
    if ( p not_eq nullptr )
        delete p;
    NativeCall::set_destination( entry_point );
}


extern "C" bool_t have_nlr_through_C;


Oop nativeMethod_substitute() {
    return Universe::nilObj();
}


const char * CompiledInlineCache::normalLookup( Oop recv ) {

    ResourceMark resourceMark;
    const char * entry_point;

    // The assertion below is turned into an if so we can see possible problems in the fast version as well - gri 6/21/96
    //
    // Last problem reported by Steffen (gri 7/24/96):
    // This has been called via a NativeMethod called from within a megamorphic self send in the interpreter.
    // I've added Interpreter::_last_native_called for better debugging.
    //
    st_assert( not Interpreter::contains( begin_addr() ), "should be handled in the interpreter" );
    if ( Interpreter::contains( begin_addr() ) ) {
        _console->print_cr( "NativeMethod called from interpreter reports ic miss:" );
        _console->print_cr( "interpreter call at [0x%x]", begin_addr() );
        _console->print_cr( "NativeMethod entry point [0x%x]", Interpreter::_last_native_called );
        InterpretedInlineCache * ic = as_InterpretedIC( next_instruction_address() );
        st_fatal( "please notify VM people" );
    }

    if ( TraceLookup ) {
        _console->print( "CompiledInlineCache lookup (" );
        recv->klass()->print_value();
        _console->print( ", " );
        selector()->print_value();
        _console->print( ")" );
        _console->cr();
    }

    KlassOop     klass  = recv->klass();
    SymbolOop    sel    = selector();
    LookupResult result = LookupCache::ic_normal_lookup( klass, sel );

    if ( result.is_empty() ) {
        // does not understand
        //
        // The following code is just a copy from the corresponding section
        // for interpreted code - especially the arguments for the message
        // object are not set up yet (0 arguments assumed). This code is
        // only here to prevent the system from crashing completely - in
        // general one should be able to recover using abort in the evaluator.
        //
        // FIX THIS (gri 6/10/96)
        BlockScavenge bs; // make sure that no scavenge happens
        KlassOop      msgKlass = KlassOop( Universe::find_global( "Message" ) );
        Oop           obj      = msgKlass->klass_part()->allocateObject();
        st_assert( obj->is_mem(), "just checkin'..." );
        MemOop         msg  = MemOop( obj );
        ObjectArrayOop args = oopFactory::new_objArray( 0 );
        // for now: assume instance variables are there...
        // later: should check this or use a VM interface:
        // msg->set_receiver(recv);
        // msg->set_selector(ic->selector());
        // msg->set_arguments(args);
        msg->raw_at_put( 2, recv );
        msg->raw_at_put( 3, selector() );
        msg->raw_at_put( 4, args );
        SymbolOop sel    = oopFactory::new_symbol( "doesNotUnderstand:" );
        if ( interpreter_normal_lookup( recv->klass(), sel ).is_empty() ) {
            // doesNotUnderstand: not found ==> process error
            {
                ResourceMark resourceMark;
                _console->print_cr( "LOOKUP ERROR" );
                sel->print_value();
                _console->print_cr( " not found" );
            }
            if ( DeltaProcess::active()->is_scheduler() ) {
                DeltaProcess::active()->trace_stack();
                st_fatal( "lookup error in scheduler" );
            } else {
                DeltaProcess::active()->suspend( ProcessState::lookup_error );
            }
            ShouldNotReachHere();
        }
        // return marked result of doesNotUnderstand: message
        Oop       result = Delta::call( recv, sel, msg );

        if ( not have_nlr_through_C ) {
            Unimplemented();
        }
        // return a substitute NativeMethod so that stub routine doesn't crash
        return ( const char * ) nativeMethod_substitute;

        /* Old code - keep around till completely fixed

        LookupKey key(klass, sel);

        _console->print("Compiled lookup failed for: ");
        key.print_on(_console);
        _console->cr();

        DeltaProcess::active()->trace_stack();
        warning("Lookup error: DoesNotUnderstand semantics not implemented for compiled code");
        evaluator::read_eval_loop();
        Unimplemented();
        */
    }

    if ( isOptimized() and result.is_method() ) {
        // If this call site is optimized only nativeMethods should be called.
        st_assert( not isMegamorphic(), "megamorphic ICs should not be optimized" );
        LookupKey         key( klass, sel );
        VM_OptimizeMethod op( &key, result.method() );
        VMProcess::execute( &op );
        result = LookupCache::ic_normal_lookup( klass, sel );
        st_assert( result.is_entry(), "result must contain a jump table entry" );
    }

    bool_t empty = is_empty();
    if ( empty )
        setDirty();
    if ( not empty or result.is_method() ) {
        PolymorphicInlineCache * pic = PolymorphicInlineCache::allocate( this, klass, result );
        if ( pic == nullptr ) {
            // PolymorphicInlineCache too large & MICs (megamorphic PICs) turned off
            // => start with empty InlineCache again
            st_assert( not UseMICs, "should return a MIC" );
            clear();
            if ( result.is_entry() ) {
                entry_point = isReceiverStatic() ? result.entry()->method()->verifiedEntryPoint() : result.entry()->destination();
            } else {
                pic = PolymorphicInlineCache::allocate( this, klass, result );
                st_assert( pic not_eq nullptr, "pic must be present" );
                entry_point = pic->entry();
            }
        } else {
            entry_point = pic->entry();
            if ( pic->is_megamorphic() ) {
                setMegamorphic(); // even when UseInlineCaching is turned off? (gri 5/24/96) - check this
            }
        }
    } else {
        st_assert( empty and result.is_entry(), "just checking" );

        // result is a jump table entry for an NativeMethod
        if ( TraceLookup2 )
            lprintf( "NativeMethod found, f = 0x%x\n", result.get_nativeMethod() );
        // fetch the destination of the jump table entry to avoid the indirection

        // is the receiver is static we will use the verified entry point
        entry_point = isReceiverStatic() ? result.entry()->method()->verifiedEntryPoint() : result.entry()->destination();
    }
    st_assert( isDirty(), "must be dirty now" );    // important invariant for type feedback: non-empty InlineCache must be dirty
    if ( UseInlineCaching )
        set_call_destination( entry_point );
    if ( TraceLookup2 )
        print();
    LOG_EVENT3( "CompiledICLookup (%#x, %#x) --> %#x", klass, sel, entry_point );
    return entry_point;
}


extern "C" const char * icSuperLookup( Oop recv, CompiledInlineCache * ic ) {
    // As soon as the lookup routine handles 'message not understood' correctly, allocation may take place.
    // Then we have to fix the lookup stub as well. (receiver cannot be saved/restored within the C frame).
    VerifyNoScavenge vna;
    return ic->superLookup( recv );
}


extern "C" const char * zombie_nativeMethod( const char * return_addr ) {

    // Called from zombie nativeMethods. Determines if called from interpreted
    // or compiled code, does cleanup of the corresponding inline caches
    // and restarts the send.
    //
    // Note: If the NativeMethod is called from within the interpreter, the
    //       send restart entry point is computed via a 2nd ic_info word
    //       that follows the ordinary ic_info associated with the call.
    //       (the same method as for NonLocalReturns used, the target is the send
    //       restart entry point).
    VerifyNoScavenge vna;
    if ( Interpreter::contains( return_addr ) ) {
        // NativeMethod called from interpreted code
        Frame f = DeltaProcess::active()->last_frame();
        InterpretedInlineCache * ic = f.current_interpretedIC();
        LOG_EVENT1( "zombie NativeMethod called => interpreted InlineCache 0x%x cleared", ic );
        ic->cleanup();
        // reset instruction pointer => next instruction beeing executed is the same send
        f.set_hp( ic->send_code_addr() );
        // restart send entry point for interpreted sends
        return Interpreter::redo_send_entry();

    } else {
        // NativeMethod called from compiled code
        CompiledInlineCache * ic = CompiledIC_from_return_addr( return_addr );
        LOG_EVENT1( "zombie NativeMethod called => compiled InlineCache 0x%x cleaned up", ic );
        ic->cleanup();
        // restart send entry point is call address
        return ic->begin_addr();
    }
}


KlassOop CompiledInlineCache::targetKlass() const {
    NativeMethod * nm = target();
    if ( nm ) {
        return nm->_lookupKey.klass();
    } else {
        Unimplemented();
        return nullptr;
    }
}


KlassOop CompiledInlineCache::sending_method_holder() {
    char                     * addr   = begin_addr();
    NativeMethod             * nm     = findNativeMethod( addr );
    ProgramCounterDescriptor * pcdesc = nm->containingProgramCounterDescriptor( addr );
    ScopeDescriptor          * scope  = pcdesc->containingDesc( nm );
    return scope->selfKlass()->klass_part()->lookup_method_holder_for( scope->method() );
}


const char * CompiledInlineCache::superLookup( Oop recv ) {
    ResourceMark resourceMark;
    const char * entry_point;
    st_assert( not Interpreter::contains( begin_addr() ), "should be handled in the interpreter" );

    KlassOop  recv_klass = recv->klass();
    KlassOop  mhld_klass = sending_method_holder();
    SymbolOop sel        = selector();

    if ( TraceLookup ) {
        _console->print( "CompiledInlineCache super lookup (" );
        recv_klass->print_value();
        _console->print( ", " );
        mhld_klass->print_value();
        _console->print( ", " );
        selector()->print_value();
        _console->print( ")" );
        _console->cr();
    }

    // The inline cache for super sends looks like the inline cache for normal sends.

    LookupResult result = LookupCache::ic_super_lookup( recv_klass, mhld_klass->klass_part()->superKlass(), sel );
    st_assert( not result.is_empty(), "lookup cache error" );
    if ( result.is_method() ) {
        // a methodOop
        if ( TraceLookup2 )
            lprintf( "methodOop found, m = 0x%x\n", result.method() );
        //result = (char*)&interpreter_call;
        //if (UseInlineCaching) set_call_destination(result);
        warning( "CompiledInlineCache::superLookup didn't find a NativeMethod - check this" );
        Unimplemented();
    } else {
        // result is a jump table entry for an NativeMethod
        if ( TraceLookup2 )
            lprintf( "NativeMethod %#x found\n", result.get_nativeMethod() );
        // fetch the destination of the jump table entry to avoid the indirection
        entry_point = result.entry()->destination();
    }
    if ( UseInlineCaching )
        set_call_destination( entry_point );
    if ( TraceLookup2 )
        print();
    LOG_EVENT3( "SuperLookup (%#x, %#x) --> %#x", recv_klass, sel, entry_point );
    return entry_point;
}


bool_t CompiledInlineCache::is_monomorphic() const {
    if ( target() not_eq nullptr )
        return true;
    PolymorphicInlineCache * p = pic();
    return p not_eq nullptr and p->is_monomorphic();
}


bool_t CompiledInlineCache::is_polymorphic() const {
    PolymorphicInlineCache * p = pic();
    return p not_eq nullptr and p->is_polymorphic();
}


bool_t CompiledInlineCache::is_megamorphic() const {
    PolymorphicInlineCache * p = pic();
    return p not_eq nullptr and p->is_megamorphic();
}


void CompiledInlineCache::replace( NativeMethod * nm ) {
    st_assert( selector() == nm->_lookupKey.selector(), "mismatched selector" );
    LOG_EVENT3( "compiled InlineCache at 0x%x: new NativeMethod 0x%x for klass 0x%x replaces old entry", this, nm, nm->_lookupKey.klass() );

    // MONO
    if ( is_monomorphic() ) {
        if ( pic() ) {
            st_assert( pic()->klasses()->at( 0 ) == nm->_lookupKey.klass(), "mismatched klass" );
        } else {
            // verify the key in the old NativeMethod matches the new
            st_assert( findNativeMethod( destination() )->_lookupKey.equal( &nm->_lookupKey ), "keys must match" );
        }
        // set_call_destination deallocates the pic if necessary.
        set_call_destination( nm->entryPoint() );
        return;
    }
    // POLY or MEGA
    PolymorphicInlineCache * p = pic();
    if ( p not_eq nullptr ) {
        PolymorphicInlineCache * new_pic = p->replace( nm );
        if ( new_pic not_eq p ) {
            set_call_destination( new_pic->entry() );
        }
        return;
    }
    // EMPTY
    ShouldNotReachHere();
}


void CompiledInlineCache::clear() {
    // Fix this when compiler is more flexible
    st_assert( not isSuperSend() or UseNewBackend, "We cannot yet have super sends in nativeMethods" );

    // Clear destination
    set_call_destination( isSuperSend() ? superLookupRoutine() : normalLookupRoutine() );

    // Q: Are there any flags to be reset and if so, which ones?
    // A: No, they are "invariant" properties of the InlineCache.  Even the dirty bit should not be
    //    reset, otherwise the compiler may think it was never executed.	  -Urs 7/96
}


void CompiledInlineCache::cleanup() {
    // Convert all entries using the following rules:
    //
    //  NativeMethod   -> NativeMethod   (nothing changed)
    //            or NativeMethod'  (new NativeMethod has been compiled)
    //            or methodOop (old NativeMethod is invalid)
    //
    //  methodOop -> methodOop (nothing changed)
    //            or NativeMethod   (new NativeMethod has been compiled)

    // EMPTY
    if ( is_empty() )
        return;

    // MONOMORPHIC
    if ( is_monomorphic() ) {
        if ( pic() ) {
            // Must be interpreter send
            PolymorphicInlineCacheIterator it( pic() );
            st_assert( it.is_interpreted(), "must be interpreted send in monomorphic case" );
            // Since it is impossible to retrieve the sending method for a methodOop
            // we leave the InlineCache unchanged if we're in a super send.
            if ( isSuperSend() )
                return;
            LookupKey    key( it.get_klass(), selector() );
            LookupResult result = LookupCache::lookup( &key );
            // Nothing to do if lookup result is the same
            if ( result.matches( it.interpreted_method() ) )
                return;
            // Otherwise update InlineCache depending on lookup result
            if ( result.is_empty() ) {
                clear();
            } else if ( result.is_method() ) {
                it.set_methodOop( result.method() );
            } else {
                st_assert( result.is_entry(), "lookup result should be a jump table entry" );
                set_call_destination( result.get_nativeMethod()->entryPoint() );
            }
        } else {
            // compiled target
            NativeMethod * old_nm = findNativeMethod( destination() );
            LookupResult result = LookupCache::lookup( &old_nm->_lookupKey );
            // Nothing to do if lookup result is the same
            if ( result.matches( old_nm ) )
                return;
            // Otherwise update InlineCache depending on lookup result
            if ( result.is_empty() ) {
                clear();
            } else if ( result.is_method() ) {
                // don't set to interpreted method -- may be "compiled only" send
                clear();
            } else {
                st_assert( result.is_entry(), "lookup result should be a jump table entry" );
                set_call_destination( result.get_nativeMethod()->entryPoint() );
            }
            /* Old code for compiled target - remove if new version works (gri 7/17/96)

            NativeMethod* nm = findNativeMethod(destination());
            LookupResult result = LookupCache::lookup(&nm->key);
            assert(nm->isZombie() or result.is_entry(), "lookup cache is wrong");
            // Nothing to do if lookup result is the same
            if (result.matches(nm)) return;
            assert(nm->isZombie(), "NativeMethod should be zombie");
            NativeMethod* newNM = result.get_nativeMethod();
            if (newNM not_eq nullptr) {
          set_call_destination(newNM->entryPoint());
            } else {
          clear();    // don't set to interpreted method -- may be "compiled only" send
            }
            */
        }
        return;
    }

    // POLYMORPHIC
    PolymorphicInlineCache * p = pic();
    if ( p ) {
        NativeMethod           * nm;
        PolymorphicInlineCache * result = p->cleanup( &nm );
        if ( result not_eq p ) {
            if ( p not_eq nullptr ) {
                // still polymorphic
                set_call_destination( result->entry() );
            } else {
                if ( nm ) {
                    // monomorphic
                    set_call_destination( nm->entryPoint() );
                } else {
                    // anamorphic
                    clear();
                }
            }
        }
        return;
    }

    // IMPOSSIBLE STATE
    ShouldNotReachHere();
}


void CompiledInlineCache::print() {
    ResourceMark resourceMark;    // so we can print from debugger
    lprintf( "\t((CompiledInlineCache*)%#x) ", this );
    if ( is_empty() ) {
        lprintf( "(empty) " );
    } else {
        lprintf( "(filled: 0x%08x targets) ", ntargets() );
    }
    if ( isReceiverStatic() )
        lprintf( "static " );
    if ( isDirty() )
        lprintf( "dirty " );
    if ( isOptimized() )
        lprintf( "optimized " );
    if ( isUninlinable() )
        lprintf( "uninlinable " );
    if ( isSuperSend() )
        lprintf( "super " );
    if ( isMegamorphic() )
        lprintf( "megamorphic " );
    lprintf( "\n" );

    lprintf( "\t- selector    : " );
    selector()->print_symbol_on();
    lprintf( "\n" );

    CompiledInlineCacheIterator it( this );
    while ( not it.at_end() ) {
        lprintf( "\t- klass       : " );
        it.klass()->print_value();
        if ( it.is_compiled() ) {
            lprintf( ";\tNativeMethod %#x\n", it.compiled_method() );
        } else {
            lprintf( ";\tmethod %#x\n", it.interpreted_method() );
        }
        it.advance();
    }

    lprintf( "\t- call address: " );
    char * dest = destination();
    if ( dest == normalLookupRoutine() ) {
        lprintf( "normalLookupRoutine\n" );
    } else if ( dest == superLookupRoutine() ) {
        lprintf( "superLookupRoutine\n" );
    } else {
        // non-empty icache
        lprintf( "0x%x\n", destination() );
    }

    lprintf( "\t- NonLocalReturn testcode: 0x%x\n", NonLocalReturn_testcode() );
}


InterpretedInlineCache * CompiledInlineCache::inlineCache() const {
    // return interpreter inline cache in corresponding source method
    char                     * addr   = begin_addr();
    NativeMethod             * nm     = findNativeMethod( addr );
    ProgramCounterDescriptor * pcdesc = nm->containingProgramCounterDescriptor( addr );
    ScopeDescriptor          * scope  = pcdesc->containingDesc( nm );
    CodeIterator iter = CodeIterator( scope->method(), pcdesc->_byteCodeIndex );
    return iter.ic();
}


SymbolOop CompiledInlineCache::selector() const {
    return inlineCache()->selector();
}


NativeMethod * CompiledInlineCache::target() const {
    char * dest = destination();
    if ( Universe::code->contains( dest ) ) {
        // linked to an NativeMethod
        NativeMethod * m = nativeMethod_from_insts( dest );
        st_assert( m == findNativeMethod( dest ), "wrong NativeMethod start" );
        return m;
    } else {
        return nullptr;
    }
}


KlassOop CompiledInlineCache::get_klass( int i ) const {
    PolymorphicInlineCache * p = pic();
    if ( p ) {
        PolymorphicInlineCacheIterator it( p );
        for ( int                      j = 0; j < i; j++ )
            it.advance();
        return it.get_klass();
    } else {
        st_assert( i == 0, "have max. 1 target method" );
        return target()->_lookupKey.klass();
    }
}


PolymorphicInlineCache * CompiledInlineCache::pic() const {
    char * dest = destination();
    return PolymorphicInlineCache::find( dest );
}


LookupKey * CompiledInlineCache::key( int i, bool_t is_normal_send ) const {
    if ( is_normal_send ) {
        return LookupKey::allocate( get_klass( i ), selector() );
    } else {
        CompiledInlineCacheIterator it( ( CompiledInlineCache * ) this );
        it.goto_elem( i );
        return LookupKey::allocate( it.klass(), it.interpreted_method() );
    }
}


bool_t CompiledInlineCache::wasNeverExecuted() const {
    return is_empty() and not isDirty();
}


PrimitiveDescriptor * PrimitiveInlineCache::primitive() {
    return Primitives::lookup( ( primitiveFunctionType ) destination() );
}


char * PrimitiveInlineCache::end_addr() {
    PrimitiveDescriptor * pd = primitive();
    int offset = pd->can_perform_NonLocalReturn() ? InlineCacheInfo::instruction_size : 0;
    return next_instruction_address() + offset;
}


void PrimitiveInlineCache::print() {
    lprintf( "\tPrimitive inline cache\n" );
    PrimitiveDescriptor * pd = primitive();
    lprintf( "\t- name        : %s\n", pd->name() );
    if ( pd->can_perform_NonLocalReturn() ) {
        lprintf( "\t- NonLocalReturn testcode: 0x%x\n", NonLocalReturn_testcode() );
    }
}


CompiledInlineCache * CompiledIC_from_return_addr( const char * return_addr ) {
    return ( CompiledInlineCache * ) nativeCall_from_return_address( return_addr );
}


CompiledInlineCache * CompiledIC_from_relocInfo( const char * displacement_address ) {
    return ( CompiledInlineCache * ) nativeCall_from_relocInfo( displacement_address );
}


PrimitiveInlineCache * PrimitiveIC_from_return_addr( const char * return_addr ) {
    return ( PrimitiveInlineCache * ) nativeCall_from_return_address( return_addr );
}


PrimitiveInlineCache * PrimitiveIC_from_relocInfo( const char * displacement_address ) {
    return ( PrimitiveInlineCache * ) nativeCall_from_relocInfo( displacement_address );
}
