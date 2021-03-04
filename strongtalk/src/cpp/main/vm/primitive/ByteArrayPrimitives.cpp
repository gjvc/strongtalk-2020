
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/oop/ProxyOopDescriptor.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/utility/IntegerOps.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/system/os.hpp"
#include "vm/code/StubRoutines.hpp"



TRACE_FUNC( TraceByteArrayPrims, "byteArray" )


std::int32_t ByteArrayPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert( receiver->isByteArray(), "receiver must be byte array" )


PRIM_DECL_2( ByteArrayPrimitives::allocateSize, Oop receiver, Oop argument ) {
    PROLOGUE_2( "allocateSize", receiver, argument )
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oopIsByteArray(), "receiver must byte array class" );
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( SmallIntegerOop( argument )->value() < 0 )
        return markSymbol( vmSymbols::negative_size() );

    KlassOop     k        = KlassOop( receiver );
    std::int32_t ni_size  = k->klass_part()->non_indexable_size();
    std::int32_t obj_size = ni_size + 1 + roundTo( SmallIntegerOop( argument )->value(), OOP_SIZE ) / OOP_SIZE;

    // allocate
    ByteArrayOop obj = as_byteArrayOop( Universe::allocate( obj_size, (MemOop *) &k ) );

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
    //               obj->byte_at_put(index, '\000')'
    base = &base[ ni_size + 1 ];
    while ( base < end )
        *base++ = (Oop) 0;
    return obj;
}


PRIM_DECL_3( ByteArrayPrimitives::allocateSize2, Oop receiver, Oop argument, Oop tenured ) {
    PROLOGUE_3( "allocateSize2", receiver, argument, tenured )

    // These should be ordinary checks in case ST code erroneously passes an invalid value.
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oopIsByteArray(), "receiver must byte array class" );

    if ( not( receiver->is_klass() and KlassOop( receiver )->klass_part()->oopIsByteArray() ) )
        return markSymbol( vmSymbols::invalid_klass() );

    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( SmallIntegerOop( argument )->value() < 0 )
        return markSymbol( vmSymbols::negative_size() );

    if ( tenured not_eq Universe::trueObject() and tenured not_eq Universe::falseObject() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    MemOopKlass *theKlass = (MemOopKlass *) KlassOop( receiver )->klass_part();
    Oop         result    = theKlass->allocateObjectSize( SmallIntegerOop( argument )->value(), false, tenured == trueObject );
    if ( result == nullptr )
        return markSymbol( vmSymbols::failed_allocation() );

    return result;
}


PRIM_DECL_1( ByteArrayPrimitives::size, Oop receiver ) {
    PROLOGUE_1( "size", receiver );
    ASSERT_RECEIVER;
    // do the operation
    return smiOopFromValue( ByteArrayOop( receiver )->length() );
}


PRIM_DECL_1( ByteArrayPrimitives::numberOfArguments, Oop receiver ) {
    PROLOGUE_1( "numberOfArguments", receiver );
    ASSERT_RECEIVER;
    // do the operation
    return smiOopFromValue( ByteArrayOop( receiver )->number_of_arguments() );
}


PRIM_DECL_2( ByteArrayPrimitives::at, Oop receiver, Oop index ) {
    PROLOGUE_2( "at", receiver, index );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check index value
    if ( not ByteArrayOop( receiver )->is_within_bounds( SmallIntegerOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // do the operation
    return smiOopFromValue( ByteArrayOop( receiver )->byte_at( SmallIntegerOop( index )->value() ) );
}


PRIM_DECL_3( ByteArrayPrimitives::atPut, Oop receiver, Oop index, Oop value ) {
    PROLOGUE_3( "atPut", receiver, index, value );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check index type
    if ( not value->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    // check index value
    if ( not ByteArrayOop( receiver )->is_within_bounds( SmallIntegerOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // check value range (must be byte)
    std::uint32_t v = (std::uint32_t) SmallIntegerOop( value )->value();
    if ( v >= ( 1 << 8 ) )
        return markSymbol( vmSymbols::value_out_of_range() );

    // do the operation
    ByteArrayOop( receiver )->byte_at_put( SmallIntegerOop( index )->value(), v );
    return receiver;
}


PRIM_DECL_2( ByteArrayPrimitives::compare, Oop receiver, Oop argument ) {
    PROLOGUE_2( "comare", receiver, argument );
    ASSERT_RECEIVER;

    if ( receiver == argument )
        return smiOopFromValue( 0 );

    if ( argument->isByteArray() )
        return smiOopFromValue( ByteArrayOop( receiver )->compare( ByteArrayOop( argument ) ) );

    if ( argument->isDoubleByteArray() )
        return smiOopFromValue( ByteArrayOop( receiver )->compare_doubleBytes( DoubleByteArrayOop( argument ) ) );

    return markSymbol( vmSymbols::first_argument_has_wrong_type() );
}


PRIM_DECL_1( ByteArrayPrimitives::intern, Oop receiver ) {
    PROLOGUE_1( "intern", receiver );
    ASSERT_RECEIVER;

    return Universe::symbol_table->lookup( ByteArrayOop( receiver )->chars(), ByteArrayOop( receiver )->length() );
}


PRIM_DECL_2( ByteArrayPrimitives::characterAt, Oop receiver, Oop index ) {
    PROLOGUE_2( "characterAt", receiver, index );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // range check
    if ( not ByteArrayOop( receiver )->is_within_bounds( SmallIntegerOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // fetch byte
    std::int32_t byte = ByteArrayOop( receiver )->byte_at( SmallIntegerOop( index )->value() );

    // return the n+1'th element in asciiCharacter
    return Universe::asciiCharacters()->obj_at( byte + 1 );
}


PRIM_DECL_2( ByteArrayPrimitives::at_all_put, Oop receiver, Oop value ) {
    PROLOGUE_2( "at_all_put", receiver, value );
    ASSERT_RECEIVER;

    // check index type
    if ( not value->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check value range (must be byte)
    std::uint32_t v = (std::uint32_t) SmallIntegerOop( value )->value();
    if ( v >= ( 1 << 8 ) )
        return markSymbol( vmSymbols::value_out_of_range() );

    std::size_t length = ByteArrayOop( receiver )->length();

    for ( std::size_t i = 1; i <= length; i++ ) {
        ByteArrayOop( receiver )->byte_at_put( i, v );
    }

    return receiver;
}

// LargeIntegers primitives

Oop simplified( ByteArrayOop result ) {
    // Tries to simplify result, a large integer, into a small integer if possible.
    bool ok;
    Oop  smi_result = result->number().as_SmallIntegerOop( ok );
    return ok ? smi_result : result;
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerFromSmallInteger, Oop receiver, Oop number ) {
    PROLOGUE_2( "largeIntegerFromSmallInteger", receiver, number );
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oopIsByteArray(), "just checking" );

    // Check arguments
    if ( not number->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    BlockScavenge bs;
    std::int32_t  i = SmallIntegerOop( number )->value();
    ByteArrayOop  z;

    z = ByteArrayOop( KlassOop( receiver )->klass_part()->allocateObjectSize( IntegerOps::int_to_Integer_result_size_in_bytes( i ) ) );
    IntegerOps::int_to_Integer( i, z->number() );

    return z;
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerFromDouble, Oop receiver, Oop number ) {
    PROLOGUE_2( "largeIntegerFromDouble", receiver, number );
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oopIsByteArray(), "just checking" );

    if ( not number->isDouble() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;
    double        x = DoubleOop( number )->value();
    ByteArrayOop  z;

    z = ByteArrayOop( KlassOop( receiver )->klass_part()->allocateObjectSize( IntegerOps::double_to_Integer_result_size_in_bytes( x ) ) );
    IntegerOps::double_to_Integer( x, z->number() );

    return z;
}


PRIM_DECL_3( ByteArrayPrimitives::largeIntegerFromString, Oop receiver, Oop argument, Oop base ) {
    PROLOGUE_3( "largeIntegerFromString", receiver, argument, base );
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oopIsByteArray(), "just checking" );

    if ( not argument->isByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not base->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x = ByteArrayOop( argument );
    ByteArrayOop z;

    z = ByteArrayOop( x->klass()->klass_part()->allocateObjectSize( IntegerOps::string_to_Integer_result_size_in_bytes( x->chars(), SmallIntegerOop( base )->value() ) ) );
    IntegerOps::string_to_Integer( x->chars(), SmallIntegerOop( base )->value(), z->number() );

    return z;
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerAdd, Oop receiver, Oop argument ) {
    PROLOGUE_2( "largeIntegerAdd", receiver, argument );
    ASSERT_RECEIVER;

    if ( not argument->isByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x = ByteArrayOop( receiver );
    ByteArrayOop y = ByteArrayOop( argument );
    ByteArrayOop z;

    if ( not x->number().is_valid() or not y->number().is_valid() )
        return markSymbol( vmSymbols::argument_is_invalid() );

    z = ByteArrayOop( x->klass()->klass_part()->allocateObjectSize( IntegerOps::add_result_size_in_bytes( x->number(), y->number() ) ) );
    x = ByteArrayOop( receiver );
    y = ByteArrayOop( argument );
    IntegerOps::add( x->number(), y->number(), z->number() );

    return simplified( z );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerSubtract, Oop receiver, Oop argument ) {
    PROLOGUE_2( "largeIntegerSubtract", receiver, argument );
    ASSERT_RECEIVER;

    if ( not argument->isByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x = ByteArrayOop( receiver );
    ByteArrayOop y = ByteArrayOop( argument );
    ByteArrayOop z;

    if ( not x->number().is_valid() or not y->number().is_valid() )
        return markSymbol( vmSymbols::argument_is_invalid() );

    z = ByteArrayOop( x->klass()->klass_part()->allocateObjectSize( IntegerOps::sub_result_size_in_bytes( x->number(), y->number() ) ) );
    x = ByteArrayOop( receiver );
    y = ByteArrayOop( argument );
    IntegerOps::sub( x->number(), y->number(), z->number() );
    return simplified( z );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerMultiply, Oop
    receiver, Oop
                 argument ) {
    PROLOGUE_2( "largeIntegerMultiply", receiver, argument );
    ASSERT_RECEIVER;

    if ( not argument->isByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x = ByteArrayOop( receiver );
    ByteArrayOop y = ByteArrayOop( argument );
    ByteArrayOop z;

    if ( not x->number().is_valid() or not y->number().is_valid() )
        return markSymbol( vmSymbols::argument_is_invalid() );

    z = ByteArrayOop( x->klass()->klass_part()->allocateObjectSize( IntegerOps::mul_result_size_in_bytes( x->number(), y->number() ) ) );
    x = ByteArrayOop( receiver );
    y = ByteArrayOop( argument );
    IntegerOps::mul( x->number(), y->number(), z->number() );
    return simplified( z );
}


#define BIT_OP( receiver, argument, sizeFn, opFn, label )\
  ARG_CHECK(receiver, argument, label);\
  BlockScavenge bs;\
  ByteArrayOop z = ByteArrayOop(x->klass()->klass_part()->allocateObjectSize(IntegerOps::sizeFn(x->number(), y->number())));\
\
  IntegerOps::opFn(x->number(), y->number(), z->number());\
  return simplified(z)

#define DIVISION( receiver, argument, sizeFn, divFn, label )\
  ARG_CHECK(receiver, argument, label);\
  if (y->number().is_zero()) return markSymbol(vmSymbols::division_by_zero   ());\
\
  BlockScavenge bs;\
  ByteArrayOop z = ByteArrayOop(x->klass()->klass_part()->allocateObjectSize(IntegerOps::sizeFn(x->number(), y->number())));\
\
  IntegerOps::divFn(x->number(), y->number(), z->number());\
  return simplified(z)
#define ARG_CHECK( receiver, argument, label )\
  PROLOGUE_2(label, receiver, argument);\
  ASSERT_RECEIVER;\
\
  if (not argument->isByteArray())\
    return markSymbol(vmSymbols::first_argument_has_wrong_type());\
\
  ByteArrayOop x = ByteArrayOop(receiver);\
  ByteArrayOop y = ByteArrayOop(argument);\
\
  if (not x->number().is_valid() or not y->number().is_valid()) return markSymbol(vmSymbols::argument_is_invalid())


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerQuo, Oop receiver, Oop argument ) {
    DIVISION( receiver, argument, quo_result_size_in_bytes, quo, "largeIntegerQuo" );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerDiv, Oop receiver, Oop argument ) {
    DIVISION( receiver, argument, div_result_size_in_bytes, Div, "largeIntegerDiv" );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerMod, Oop receiver, Oop argument ) {
    DIVISION( receiver, argument, mod_result_size_in_bytes, Mod, "largeIntegerMod" );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerRem, Oop receiver, Oop argument ) {
    DIVISION( receiver, argument, rem_result_size_in_bytes, rem, "largeIntegerRem" );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerAnd, Oop receiver, Oop argument ) {
    BIT_OP( receiver, argument, and_result_size_in_bytes, And, "largeIntegerAnd" );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerOr, Oop receiver, Oop argument ) {
    BIT_OP( receiver, argument, or_result_size_in_bytes, Or, "largeIntegerOr" );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerXor, Oop receiver, Oop argument ) {
    BIT_OP( receiver, argument, xor_result_size_in_bytes, Xor, "largeIntegerXor" );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerShift, Oop receiver, Oop argument ) {
    PROLOGUE_2( "largeIntegerShift", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    ByteArrayOop x     = ByteArrayOop( receiver );
    std::int32_t shift = SmallIntegerOop( argument )->value();

    if ( not ByteArrayOop( receiver )->number().is_valid() )
        return markSymbol( vmSymbols::argument_is_invalid() );

    ByteArrayOop z = ByteArrayOop( x->klass()->klass_part()->allocateObjectSize( IntegerOps::ash_result_size_in_bytes( x->number(), shift ) ) );

    IntegerOps::ash( x->number(), shift, z->number() );

    return simplified( z );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerCompare, Oop receiver, Oop argument ) {
    PROLOGUE_2( "largeIntegerCompare", receiver, argument );
    ASSERT_RECEIVER;

    // Check argument
    if ( not argument->isByteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x = ByteArrayOop( receiver );
    ByteArrayOop y = ByteArrayOop( argument );

    if ( not x->number().is_valid() or not y->number().is_valid() )
        return markSymbol( vmSymbols::argument_is_invalid() );

    std::int32_t res = IntegerOps::cmp( x->number(), y->number() );
    if ( res < 0 )
        return smiOopFromValue( -1 );
    if ( res > 0 )
        return smiOopFromValue( 1 );
    return smiOopFromValue( 0 );
}


PRIM_DECL_1( ByteArrayPrimitives::largeIntegerToFloat, Oop receiver ) {
    PROLOGUE_1( "largeIntegerToFloat", receiver );
    ASSERT_RECEIVER;

    bool   ok;
    double result = ByteArrayOop( receiver )->number().as_double( ok );

    if ( not ok )
        return markSymbol( vmSymbols::conversion_failed() );

    BlockScavenge bs;
    return OopFactory::new_double( result );
}


PRIM_DECL_2( ByteArrayPrimitives::largeIntegerToString, Oop receiver, Oop base ) {
    PROLOGUE_1( "largeIntegerToString", receiver );
    ASSERT_RECEIVER;

    // Check argument
    if ( not base->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x      = ByteArrayOop( receiver );
    std::size_t length = IntegerOps::Integer_to_string_result_size_in_bytes( x->number(), SmallIntegerOop( base )->value() );

    ByteArrayOop result = OopFactory::new_byteArray( length );

    IntegerOps::Integer_to_string( x->number(), SmallIntegerOop( base )->value(), result->chars() );
    return result;
}


PRIM_DECL_1( ByteArrayPrimitives::largeIntegerHash, Oop receiver ) {
    PROLOGUE_1( "largeIntegerHash", receiver );
    ASSERT_RECEIVER;
    return smiOopFromValue( IntegerOps::hash( ByteArrayOop( receiver )->number() ) );
}


PRIM_DECL_1( ByteArrayPrimitives::hash, Oop
    receiver ) {
    PROLOGUE_1( "hash", receiver );
    ASSERT_RECEIVER;
    return smiOopFromValue( ByteArrayOop( receiver )->hash_value() );
}
