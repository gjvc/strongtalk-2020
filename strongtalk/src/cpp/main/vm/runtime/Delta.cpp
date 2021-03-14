
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Delta.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/code/StubRoutines.hpp"
#include "VMSymbol.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/lookup/LookupResult.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/runtime/DeltaCallCache.hpp"



DeltaCallCache *DeltaCallCache::_root = nullptr;    // anchor of all DeltaCallCaches


typedef Oop (call_delta_func)( void *method, Oop receiver, std::int32_t nofArgs, Oop *args );


Oop Delta::call_generic( DeltaCallCache *ic, Oop receiver, Oop selector, std::int32_t nofArgs, Oop *args ) {


    //
    call_delta_func *_call_delta = (call_delta_func *) StubRoutines::call_delta();

    if ( ic->match( receiver->klass(), SymbolOop( selector ) ) ) { // use inline cache entry - but first make sure it's not a zombie NativeMethod


        JumpTableEntry *entry = ic->result().entry();
        if ( entry not_eq nullptr and entry->method()->isZombie() ) { // is a zombie NativeMethod => do a new lookup
            LookupResult result = ic->lookup( receiver->klass(), SymbolOop( selector ) );
            if ( result.is_empty() ) {
                st_fatal( "lookup failure - not treated" );
            }
            return _call_delta( result.value(), receiver, nofArgs, args );
        }
        return _call_delta( ic->result().value(), receiver, nofArgs, args );
    }

    if ( not selector->isSymbol() ) {
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    }

    LookupResult result = ic->lookup( receiver->klass(), SymbolOop( selector ) );
    if ( result.is_empty() ) {
        //can't patch ic as wrong arguments and argument types
        return does_not_understand( receiver, SymbolOop( selector ), nofArgs, args );
        //fatal("lookup failure - not treated");
    }

    return _call_delta( result.value(), receiver, nofArgs, args );
}


Oop Delta::does_not_understand( Oop receiver, SymbolOop selector, std::int32_t nofArgs, Oop *argArray ) {
    MemOop    msg;
    SymbolOop sel;
    {
        // message not understood...
        BlockScavenge      bs; // make sure that no scavenge happens
        KlassOop           msgKlass = KlassOop( Universe::find_global( "Message" ) );
        Oop                obj      = msgKlass->klass_part()->allocateObject();
        ObjectArrayOop     args     = OopFactory::new_objectArray( nofArgs );
        for ( std::int32_t index    = 0; index < nofArgs; index++ )
            args->obj_at_put( index + 1, argArray[ index ] );

        st_assert( obj->isMemOop(), "just checkin'..." );
        msg = MemOop( obj );
        // for now: assume instance variables are there...
        // later: should check this or use a VM interface:
        // msg->set_receiver(recv);
        // msg->set_selector(ic->selector());
        // msg->set_arguments(args);
        msg->raw_at_put( 2, receiver );
        msg->raw_at_put( 3, selector );
        msg->raw_at_put( 4, args );
        sel = OopFactory::new_symbol( "doesNotUnderstand:" );
        if ( interpreter_normal_lookup( receiver->klass(), sel ).is_empty() ) {
            // doesNotUnderstand: not found ==> process error
            {
                ResourceMark resourceMark;
                SPDLOG_INFO( "LOOKUP ERROR" );
                sel->print_value();
                SPDLOG_INFO( " not found" );
            }
            if ( DeltaProcess::active()->is_scheduler() ) {
                DeltaProcess::active()->trace_stack();
                st_fatal( "lookup error in scheduler" );
            } else {
                DeltaProcess::active()->suspend( ProcessState::lookup_error );
            }
            ShouldNotReachHere();
        }
    }

    // return marked result of doesNotUnderstand: message
    return Delta::call( receiver, sel, msg );
}


Oop Delta::call( Oop receiver, Oop selector ) {
    static DeltaCallCache cache;
    return call_generic( &cache, receiver, selector, 0, nullptr );
}


Oop Delta::call( Oop receiver, Oop selector, Oop arg1 ) {
    static DeltaCallCache cache;
    return call_generic( &cache, receiver, selector, 1, &arg1 );
}


Oop Delta::call( Oop receiver, Oop selector, Oop arg1, Oop arg2 ) {
    st_unused( arg2 ); // unused

    static DeltaCallCache cache;
    return call_generic( &cache, receiver, selector, 2, &arg1 );
}


Oop Delta::call( Oop receiver, Oop selector, Oop arg1, Oop arg2, Oop arg3 ) {
    st_unused( arg2 ); // unused
    st_unused( arg3 ); // unused

    static DeltaCallCache cache;
    return call_generic( &cache, receiver, selector, 3, &arg1 );
}


Oop Delta::call( Oop receiver, Oop selector, ObjectArrayOop args ) {
    static DeltaCallCache cache;
    return call_generic( &cache, receiver, selector, args->length(), args->objs( 1 ) );
}
