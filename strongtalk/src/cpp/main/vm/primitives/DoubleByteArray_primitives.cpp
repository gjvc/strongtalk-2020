//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/primitives/DoubleByteArray_primitives.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/DoubleByteArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/SymbolTable.hpp"


TRACE_FUNC( TraceDoubleByteArrayPrims, "doubleByteArray" )


std::int32_t DoubleByteArrayPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->isDoubleByteArray(), "receiver must be double byte array")


PRIM_DECL_2( DoubleByteArrayPrimitives::allocateSize, Oop receiver, Oop argument ) {
    PROLOGUE_2( "allocateSize", receiver, argument )
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oopIsDoubleByteArray(), "receiver must double byte array class" );
    if ( not argument->isSmallIntegerOop() )
        markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( SmallIntegerOop( argument )->value() < 0 )
        return markSymbol( vmSymbols::negative_size() );

    KlassOop     k        = KlassOop( receiver );
    std::int32_t ni_size  = k->klass_part()->non_indexable_size();
    std::int32_t obj_size = ni_size + 1 + roundTo( SmallIntegerOop( argument )->value() * 2, OOP_SIZE ) / OOP_SIZE;

    // allocate
    DoubleByteArrayOop obj = as_doubleByteArrayOop( Universe::allocate( obj_size, (MemOop *) &k ) );

    // header
    MemOop( obj )->initialize_header( true, k );

    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );

    // indexables
    Oop *base = (Oop *) obj->addr();
    Oop *end  = base + obj_size;
    // %optimized 'obj->set_signed_length(size)'
    base[ ni_size ] = argument;
    // %optimized 'for (std::int32_t index = 1; index <= size; index++)
    //               obj->doubleByte_at_put(index, 0)'
    base = &base[ ni_size + 1 ];
    while ( base < end )
        *base++ = (Oop) 0;

    return obj;
}


PRIM_DECL_3( DoubleByteArrayPrimitives::allocateSize2, Oop receiver, Oop argument, Oop tenured ) {
    PROLOGUE_2( "allocateSize", receiver, argument )
    if ( not receiver->is_klass() or not KlassOop( receiver )->klass_part()->oopIsDoubleByteArray() )
        return markSymbol( vmSymbols::invalid_klass() );

    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( SmallIntegerOop( argument )->value() < 0 )
        return markSymbol( vmSymbols::negative_size() );

    if ( tenured not_eq Universe::trueObject() and tenured not_eq Universe::falseObject() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    Oop result = KlassOop( receiver )->klass_part()->allocateObjectSize( SmallIntegerOop( argument )->value(), false, Universe::trueObject() == tenured );
    if ( result == nullptr )
        return markSymbol( vmSymbols::failed_allocation() );

    return result;
}


PRIM_DECL_1( DoubleByteArrayPrimitives::size, Oop receiver ) {
    PROLOGUE_1( "size", receiver );
    ASSERT_RECEIVER;

    // do the operation
    return smiOopFromValue( DoubleByteArrayOop( receiver )->length() );
}


PRIM_DECL_2( DoubleByteArrayPrimitives::at, Oop receiver, Oop index ) {
    PROLOGUE_2( "at", receiver, index );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check index value
    if ( not DoubleByteArrayOop( receiver )->is_within_bounds( SmallIntegerOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    return smiOopFromValue( DoubleByteArrayOop( receiver )->doubleByte_at( SmallIntegerOop( index )->value() ) );
}


PRIM_DECL_3( DoubleByteArrayPrimitives::atPut, Oop receiver, Oop index, Oop value ) {
    PROLOGUE_3( "atPut", receiver, index, value );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check value type
    if ( not value->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    // check index value
    if ( not DoubleByteArrayOop( receiver )->is_within_bounds( SmallIntegerOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // check value as double byte
    std::uint32_t v = (std::uint32_t) SmallIntegerOop( value )->value();
    if ( v >= ( 1 << 16 ) )
        return markSymbol( vmSymbols::value_out_of_range() );

    // do the operation
    DoubleByteArrayOop( receiver )->doubleByte_at_put( SmallIntegerOop( index )->value(), v );
    return receiver;
}


PRIM_DECL_2( DoubleByteArrayPrimitives::compare, Oop receiver, Oop argument ) {
    PROLOGUE_2( "compare", receiver, argument );
    ASSERT_RECEIVER;

    if ( receiver == argument )
        return smiOopFromValue( 0 );

    if ( argument->isDoubleByteArray() )
        return smiOopFromValue( DoubleByteArrayOop( receiver )->compare( DoubleByteArrayOop( argument ) ) );

    if ( argument->isByteArray() )
        return smiOopFromValue( -ByteArrayOop( argument )->compare_doubleBytes( DoubleByteArrayOop( receiver ) ) );

    return markSymbol( vmSymbols::first_argument_has_wrong_type() );
}


PRIM_DECL_1( DoubleByteArrayPrimitives::intern, Oop receiver ) {
    PROLOGUE_1( "intern", receiver );
    ASSERT_RECEIVER;

    ResourceMark resourceMark;
    std::int32_t len     = DoubleByteArrayOop( receiver )->length();
    char         *buffer = new_resource_array<char>( len );

    for ( std::size_t i = 0; i < len; i++ ) {
        std::int32_t c = DoubleByteArrayOop( receiver )->doubleByte_at( i + 1 );
        if ( c >= ( 1 << 8 ) ) {
            return markSymbol( vmSymbols::value_out_of_range() );
        }
        buffer[ i ] = c;
    }

    SymbolOop sym = Universe::symbol_table->lookup( buffer, len );
    return sym;
}


PRIM_DECL_2( DoubleByteArrayPrimitives::characterAt, Oop receiver, Oop index ) {
    PROLOGUE_2( "characterAt", receiver, index );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // range check
    if ( not DoubleByteArrayOop( receiver )->is_within_bounds( SmallIntegerOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // fetch double byte
    std::uint16_t byte = DoubleByteArrayOop( receiver )->doubleByte_at( SmallIntegerOop( index )->value() );

    if ( byte < 256 ) {
        // return the byte+1'th element in asciiCharacter
        return Universe::asciiCharacters()->obj_at( byte + 1 );
    } else
        return markSymbol( vmSymbols::out_of_bounds() );
}


PRIM_DECL_1( DoubleByteArrayPrimitives::hash, Oop receiver ) {
    PROLOGUE_1( "intern", receiver );
    ASSERT_RECEIVER;
    return smiOopFromValue( DoubleByteArrayOop( receiver )->hash_value() );
}
