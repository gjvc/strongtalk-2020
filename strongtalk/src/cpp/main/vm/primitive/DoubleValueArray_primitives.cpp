//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitive/DoubleValueArray_primitives.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"


TRACE_FUNC( TraceDoubleValueArrayPrims, "doubleValueArray" )


std::int32_t DoubleValueArrayPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->isDoubleValueArray(), "receiver must be double value array")


PRIM_DECL_2( DoubleValueArrayPrimitives::allocateSize, Oop receiver, Oop argument ) {
    PROLOGUE_2( "allocateSize", receiver, argument )
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oopIsDoubleValueArray(), "receiver must double byte array class" );
    if ( not argument->isSmallIntegerOop() )
        markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( SmallIntegerOop( argument )->value() < 0 )
        return markSymbol( vmSymbols::negative_size() );

    std::size_t length = SmallIntegerOop( argument )->value();

    KlassOop            k        = KlassOop( receiver );
    std::int32_t        ni_size  = k->klass_part()->non_indexable_size();
    std::int32_t        obj_size = ni_size + 1 + roundTo( length * sizeof( double ), OOP_SIZE ) / OOP_SIZE;
    // allocate
    DoubleValueArrayOop obj = as_doubleValueArrayOop( Universe::allocate( obj_size, (MemOop *) &k ) );
    // header
    MemOop( obj )->initialize_header( true, k );
    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );
    obj->set_length( length );

    for ( std::size_t i = 1; i <= length; i++ ) {
        obj->double_at_put( i, 0.0 );
    }

    return obj;
}


PRIM_DECL_1( DoubleValueArrayPrimitives::size, Oop receiver ) {
    PROLOGUE_1( "size", receiver );
    ASSERT_RECEIVER;

    // do the operation
    return smiOopFromValue( DoubleValueArrayOop( receiver )->length() );
}


PRIM_DECL_2( DoubleValueArrayPrimitives::at, Oop receiver, Oop index ) {
    PROLOGUE_2( "at", receiver, index );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check index value
    if ( not DoubleValueArrayOop( receiver )->is_within_bounds( SmallIntegerOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    return OopFactory::new_double( DoubleValueArrayOop( receiver )->double_at( SmallIntegerOop( index )->value() ) );
}


PRIM_DECL_3( DoubleValueArrayPrimitives::atPut, Oop receiver, Oop index, Oop value ) {
    PROLOGUE_3( "atPut", receiver, index, value );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check value type
    if ( not value->isDouble() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    // check index value
    if ( not DoubleValueArrayOop( receiver )->is_within_bounds( SmallIntegerOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // do the operation
    DoubleValueArrayOop( receiver )->double_at_put( SmallIntegerOop( index )->value(), DoubleOop( value )->value() );
    return receiver;
}
