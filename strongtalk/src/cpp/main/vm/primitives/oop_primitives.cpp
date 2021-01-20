//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitives/oop_primitives.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/memory/Reflection.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/oops/WeakArrayKlass.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/SlidingSystemAverage.hpp"
#include "vm/utilities/objectIDTable.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/runtime/ResourceMark.hpp"


TRACE_FUNC( TraceOopPrims, "Oop" )


std::size_t oopPrimitives::number_of_calls;

//void swapPointers(Oop* p) {
//  Oop object = *p;
//  if (object == becomeTarget) {
//    *p = becomeReplacement;
//  } else if (object == becomeReplacement) {
//    *p = becomeTarget;
//  }
//}

class TwoWayBecomeClosure : public ObjectClosure, public OopClosure {
private:
    Oop target;
    Oop replacement;
public:
    TwoWayBecomeClosure( Oop target, Oop replacement ) :
            target( target ), replacement( replacement ) {
    }


    void do_object( MemOop obj ) {
        obj->oop_iterate( this );
    }


    void do_oop( Oop *p ) {
        Oop object = *p;
        if ( object == target ) {
            Universe::store( p, replacement, Universe::is_heap( p ) );
        } else if ( object == replacement ) {
            Universe::store( p, target, Universe::is_heap( p ) );
        }
    }
};


PRIM_DECL_2( oopPrimitives::become, Oop receiver, Oop argument ) {
    PROLOGUE_2( "become", receiver, argument )
    if ( receiver->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( argument->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    {
        ResourceMark mark;
        Processes::deoptimize_all();
    }
    //Universe::code->clear();
    TwoWayBecomeClosure closure( receiver, argument );
    Universe::new_gen.object_iterate( &closure );
    Universe::old_gen.object_iterate( &closure );
    Universe::root_iterate( &closure );
    Processes::oop_iterate( &closure );
    return receiver;
}


PRIM_DECL_2( oopPrimitives::instVarAt, Oop receiver, Oop index ) {
    PROLOGUE_2( "instVarAt", receiver, index )
    if ( not receiver->is_mem() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    int raw_index = SMIOop( index )->value() - 1;

    if ( not MemOop( receiver )->is_within_instVar_bounds( raw_index ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    return MemOop( receiver )->instVarAt( raw_index );
}


PRIM_DECL_2( oopPrimitives::instance_variable_name_at, Oop obj, Oop index ) {
    PROLOGUE_2( "instance_variable_name_at", obj, index )
    if ( not obj->is_mem() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    int raw_index = SMIOop( index )->value() - 1;

    if ( not MemOop( obj )->is_within_instVar_bounds( raw_index ) )
        markSymbol( vmSymbols::out_of_bounds() );

    return obj->blueprint()->inst_var_name_at( raw_index );
}


PRIM_DECL_3( oopPrimitives::instVarAtPut, Oop receiver, Oop index, Oop value ) {
    PROLOGUE_3( "instVarAtPut", receiver, index, value )
    if ( not receiver->is_mem() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    int raw_index = SMIOop( index )->value() - 1;

    if ( not MemOop( receiver )->is_within_instVar_bounds( raw_index ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    return MemOop( receiver )->instVarAtPut( raw_index, value );
}


PRIM_DECL_1( oopPrimitives::hash, Oop receiver ) {
    PROLOGUE_1( "hash", receiver )
    return smiOopFromValue( receiver->identity_hash() );
}


PRIM_DECL_1( oopPrimitives::hash_of, Oop obj ) {
    PROLOGUE_1( "hash_of", obj )
    return smiOopFromValue( obj->identity_hash() );
}


PRIM_DECL_1( oopPrimitives::shallowCopy, Oop receiver ) {
    PROLOGUE_1( "shallowCopy", receiver )
    return receiver->shallow_copy( false );
}


PRIM_DECL_1( oopPrimitives::copy_tenured, Oop receiver ) {
    PROLOGUE_1( "copy_tenured", receiver )
    return receiver->shallow_copy( true );
}


PRIM_DECL_2( oopPrimitives::equal, Oop receiver, Oop argument ) {
    PROLOGUE_2( "equal", receiver, argument )
    return receiver == argument ? trueObj : falseObj;
}


PRIM_DECL_2( oopPrimitives::not_equal, Oop receiver, Oop argument ) {
    PROLOGUE_2( "not_equal", receiver, argument )
    return receiver not_eq argument ? trueObj : falseObj;
}


PRIM_DECL_1( oopPrimitives::oop_size, Oop receiver ) {
    PROLOGUE_1( "oop_size", receiver )
    return smiOopFromValue( receiver->is_mem() ? MemOop( receiver )->size() : 0 );
}


PRIM_DECL_1( oopPrimitives::klass, Oop receiver ) {
    PROLOGUE_1( "klass", receiver )
    return receiver->klass();
}


PRIM_DECL_1( oopPrimitives::klass_of, Oop obj ) {
    PROLOGUE_1( "klass_of", obj )
    return obj->klass();
}


PRIM_DECL_1( oopPrimitives::print, Oop receiver ) {
    PROLOGUE_1( "print", receiver )
    ResourceMark resourceMark;
    receiver->print();
    return receiver;
}


PRIM_DECL_1( oopPrimitives::printValue, Oop receiver ) {
    PROLOGUE_1( "printString", receiver )
    ResourceMark resourceMark;
    receiver->print_value();
    return receiver;
}


PRIM_DECL_1( oopPrimitives::asObjectID, Oop receiver ) {
    PROLOGUE_1( "asObjectID", receiver )
    return SMIOop( objectIDTable::insert( receiver ) );
}


PRIM_DECL_2( oopPrimitives::perform, Oop receiver, Oop selector ) {
    PROLOGUE_2( "perform", receiver, selector );
    return Delta::call( receiver, selector );
}


PRIM_DECL_3( oopPrimitives::performWith, Oop receiver, Oop selector, Oop arg1 ) {
    PROLOGUE_3( "performWith", receiver, selector, arg1 );
    return Delta::call( receiver, selector, arg1 );
}


PRIM_DECL_4( oopPrimitives::performWithWith, Oop receiver, Oop selector, Oop arg1, Oop arg2 ) {
    PROLOGUE_4( "performWithWith", receiver, selector, arg1, arg2 );
    return Delta::call( receiver, selector, arg1, arg2 );
}


PRIM_DECL_5( oopPrimitives::performWithWithWith, Oop receiver, Oop selector, Oop arg1, Oop arg2, Oop arg3 ) {
    PROLOGUE_5( "performWithWithWith", receiver, selector, arg1, arg2, arg3 );
    return Delta::call( receiver, selector, arg1, arg2, arg3 );
}


PRIM_DECL_3( oopPrimitives::performArguments, Oop receiver, Oop selector, Oop args ) {
    PROLOGUE_3( "performArguments", receiver, selector, args );
    if ( not args->is_objArray() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );
    return Delta::call( receiver, selector, ObjectArrayOop( args ) );
}
