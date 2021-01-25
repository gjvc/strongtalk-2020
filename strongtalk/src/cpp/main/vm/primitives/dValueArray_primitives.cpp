//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitives/dValueArray_primitives.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"


TRACE_FUNC( TraceDoubleValueArrayPrims, "doubleValueArray" )


std::int32_t doubleValueArrayPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->is_doubleValueArray(), "receiver must be double value array")


PRIM_DECL_2( doubleValueArrayPrimitives::allocateSize, Oop receiver, Oop argument ) {
    PROLOGUE_2( "allocateSize", receiver, argument )
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oop_is_doubleValueArray(), "receiver must double byte array class" );
    if ( not argument->is_smi() )
        markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( SMIOop( argument )->value() < 0 )
        return markSymbol( vmSymbols::negative_size() );

    std::int32_t length = SMIOop( argument )->value();

    KlassOop            k        = KlassOop( receiver );
    std::int32_t                 ni_size  = k->klass_part()->non_indexable_size();
    std::int32_t                 obj_size = ni_size + 1 + roundTo( length * sizeof( double ), oopSize ) / oopSize;
    // allocate
    doubleValueArrayOop obj      = as_doubleValueArrayOop( Universe::allocate( obj_size, (MemOop *) &k ) );
    // header
    MemOop( obj )->initialize_header( true, k );
    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );
    obj->set_length( length );

    for ( std::int32_t i = 1; i <= length; i++ ) {
        obj->double_at_put( i, 0.0 );
    }

    return obj;
}


PRIM_DECL_1( doubleValueArrayPrimitives::size, Oop receiver ) {
    PROLOGUE_1( "size", receiver );
    ASSERT_RECEIVER;

    // do the operation
    return smiOopFromValue( doubleValueArrayOop( receiver )->length() );
}


PRIM_DECL_2( doubleValueArrayPrimitives::at, Oop receiver, Oop index ) {
    PROLOGUE_2( "at", receiver, index );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check index value
    if ( not doubleValueArrayOop( receiver )->is_within_bounds( SMIOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    return oopFactory::new_double( doubleValueArrayOop( receiver )->double_at( SMIOop( index )->value() ) );
}


PRIM_DECL_3( doubleValueArrayPrimitives::atPut, Oop receiver, Oop index, Oop value ) {
    PROLOGUE_3( "atPut", receiver, index, value );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check value type
    if ( not value->is_double() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    // check index value
    if ( not doubleValueArrayOop( receiver )->is_within_bounds( SMIOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // do the operation
    doubleValueArrayOop( receiver )->double_at_put( SMIOop( index )->value(), DoubleOop( value )->value() );
    return receiver;
}
