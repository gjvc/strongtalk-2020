//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/os.hpp"
#include "vm/system/asserts.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/oops/MemOopKlass.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/system/sizes.hpp"



TRACE_FUNC( TraceByteArrayPrims, "byteArray" )


int byteArrayPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert( receiver->is_byteArray(), "receiver must be byte array" )


PRIM_DECL_2( byteArrayPrimitives::allocateSize, Oop receiver, Oop argument ) {
    PROLOGUE_2( "allocateSize", receiver, argument )
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oop_is_byteArray(), "receiver must byte array class" );
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( SMIOop( argument )->value() < 0 )
        return markSymbol( vmSymbols::negative_size() );

    KlassOop k        = KlassOop( receiver );
    int      ni_size  = k->klass_part()->non_indexable_size();
    int      obj_size = ni_size + 1 + roundTo( SMIOop( argument)->value(), oopSize ) / oopSize;

    // allocate
    ByteArrayOop obj = as_byteArrayOop( Universe::allocate( obj_size, ( MemOop * ) &k ) );

    // header
    MemOop( obj )->initialize_header( true, k );

    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );

    // indexables
    Oop * base = ( Oop * ) obj->addr();
    Oop * end  = base + obj_size;
    // %optimized 'obj->set_length(size)'
    base[ ni_size ] = argument;
    // %optimized 'for (int index = 1; index <= size; index++)
    //               obj->byte_at_put(index, '\000')'
    base = &base[ ni_size + 1 ];
    while ( base < end )
        *base++ = ( Oop ) 0;
    return obj;
}


PRIM_DECL_3( byteArrayPrimitives::allocateSize2, Oop receiver, Oop argument, Oop tenured ) {
    PROLOGUE_3( "allocateSize2", receiver, argument, tenured )

    // These should be ordinary checks in case ST code erroneously passes an invalid value.
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oop_is_byteArray(), "receiver must byte array class" );

    if ( not( receiver->is_klass() and KlassOop( receiver )->klass_part()->oop_is_byteArray() ) )
        return markSymbol( vmSymbols::invalid_klass() );

    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    if ( SMIOop( argument )->value() < 0 )
        return markSymbol( vmSymbols::negative_size() );

    if ( tenured not_eq Universe::trueObj() and tenured not_eq Universe::falseObj() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    MemOopKlass * theKlass = ( MemOopKlass * ) KlassOop( receiver )->klass_part();
    Oop         result     = theKlass->allocateObjectSize( SMIOop( argument )->value(), false, tenured == trueObj );
    if ( result == nullptr )
        return markSymbol( vmSymbols::failed_allocation() );

    return result;
}


PRIM_DECL_1( byteArrayPrimitives::size, Oop receiver ) {
    PROLOGUE_1( "size", receiver );
    ASSERT_RECEIVER;
    // do the operation
    return smiOopFromValue( ByteArrayOop( receiver )->length() );
}


PRIM_DECL_1( byteArrayPrimitives::numberOfArguments, Oop receiver ) {
    PROLOGUE_1( "numberOfArguments", receiver );
    ASSERT_RECEIVER;
    // do the operation
    return smiOopFromValue( ByteArrayOop( receiver )->number_of_arguments() );
}


PRIM_DECL_2( byteArrayPrimitives::at, Oop receiver, Oop index ) {
    PROLOGUE_2( "at", receiver, index );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check index value
    if ( not ByteArrayOop( receiver )->is_within_bounds( SMIOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // do the operation
    return smiOopFromValue( ByteArrayOop( receiver )->byte_at( SMIOop( index )->value() ) );
}


PRIM_DECL_3( byteArrayPrimitives::atPut, Oop receiver, Oop index, Oop value ) {
    PROLOGUE_3( "atPut", receiver, index, value );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check index type
    if ( not value->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    // check index value
    if ( not ByteArrayOop( receiver )->is_within_bounds( SMIOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // check value range (must be byte)
    std::uint32_t v = ( std::uint32_t ) SMIOop( value )->value();
    if ( v >= ( 1 << 8 ) )
        return markSymbol( vmSymbols::value_out_of_range() );

    // do the operation
    ByteArrayOop( receiver )->byte_at_put( SMIOop( index )->value(), v );
    return receiver;
}


PRIM_DECL_2( byteArrayPrimitives::compare, Oop receiver, Oop argument ) {
    PROLOGUE_2( "comare", receiver, argument );
    ASSERT_RECEIVER;

    if ( receiver == argument )
        return smiOopFromValue( 0 );

    if ( argument->is_byteArray() )
        return smiOopFromValue( ByteArrayOop( receiver )->compare( ByteArrayOop( argument ) ) );

    if ( argument->is_doubleByteArray() )
        return smiOopFromValue( ByteArrayOop( receiver )->compare_doubleBytes( DoubleByteArrayOop( argument ) ) );

    return markSymbol( vmSymbols::first_argument_has_wrong_type() );
}


PRIM_DECL_1( byteArrayPrimitives::intern, Oop receiver ) {
    PROLOGUE_1( "intern", receiver );
    ASSERT_RECEIVER;

    return Universe::symbol_table->lookup( ByteArrayOop( receiver )->chars(), ByteArrayOop( receiver )->length() );
}


PRIM_DECL_2( byteArrayPrimitives::characterAt, Oop receiver, Oop index ) {
    PROLOGUE_2( "characterAt", receiver, index );
    ASSERT_RECEIVER;

    // check index type
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // range check
    if ( not ByteArrayOop( receiver )->is_within_bounds( SMIOop( index )->value() ) )
        return markSymbol( vmSymbols::out_of_bounds() );

    // fetch byte
    int byte = ByteArrayOop( receiver )->byte_at( SMIOop( index )->value() );

    // return the n+1'th element in asciiCharacter
    return Universe::asciiCharacters()->obj_at( byte + 1 );
}


PRIM_DECL_2( byteArrayPrimitives::at_all_put, Oop receiver, Oop value ) {
    PROLOGUE_2( "at_all_put", receiver, value );
    ASSERT_RECEIVER;

    // check index type
    if ( not value->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // check value range (must be byte)
    std::uint32_t v = ( std::uint32_t ) SMIOop( value )->value();
    if ( v >= ( 1 << 8 ) )
        return markSymbol( vmSymbols::value_out_of_range() );

    int length = ByteArrayOop( receiver )->length();

    for ( int i = 1; i <= length; i++ ) {
        ByteArrayOop( receiver )->byte_at_put( i, v );
    }

    return receiver;
}

// LargeIntegers primitives

Oop simplified( ByteArrayOop result ) {
    // Tries to simplify result, a large integer, into a small integer if possible.
    bool_t ok;
    Oop    smi_result = result->number().as_smi( ok );
    return ok ? smi_result : result;
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerFromSmallInteger, Oop receiver, Oop number ) {
    PROLOGUE_2( "largeIntegerFromSmallInteger", receiver, number );
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oop_is_byteArray(), "just checking" );

    // Check arguments
    if ( not number->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    BlockScavenge bs;
    int          i = SMIOop( number )->value();
    ByteArrayOop z;

    z = ByteArrayOop( KlassOop( receiver )->klass_part()->allocateObjectSize( IntegerOps::int_to_Integer_result_size_in_bytes( i ) ) );
    IntegerOps::int_to_Integer( i, z->number() );

    return z;
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerFromDouble, Oop receiver, Oop number ) {
    PROLOGUE_2( "largeIntegerFromDouble", receiver, number );
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oop_is_byteArray(), "just checking" );

    if ( not number->is_double() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;
    double       x = DoubleOop( number )->value();
    ByteArrayOop z;

    z = ByteArrayOop( KlassOop( receiver )->klass_part()->allocateObjectSize( IntegerOps::double_to_Integer_result_size_in_bytes( x ) ) );
    IntegerOps::double_to_Integer( x, z->number() );

    return z;
}


PRIM_DECL_3( byteArrayPrimitives::largeIntegerFromString, Oop receiver, Oop argument, Oop base ) {
    PROLOGUE_3( "largeIntegerFromString", receiver, argument, base );
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oop_is_byteArray(), "just checking" );

    if ( not argument->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not base->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x = ByteArrayOop( argument );
    ByteArrayOop z;

    z = ByteArrayOop( x->klass()->klass_part()->allocateObjectSize( IntegerOps::string_to_Integer_result_size_in_bytes( x->chars(), SMIOop( base )->value() ) ) );
    IntegerOps::string_to_Integer( x->chars(), SMIOop( base )->value(), z->number() );

    return z;
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerAdd, Oop receiver, Oop argument ) {
    PROLOGUE_2( "largeIntegerAdd", receiver, argument );
    ASSERT_RECEIVER;

    if ( not argument->is_byteArray() )
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


PRIM_DECL_2( byteArrayPrimitives::largeIntegerSubtract, Oop receiver, Oop argument ) {
    PROLOGUE_2( "largeIntegerSubtract", receiver, argument );
    ASSERT_RECEIVER;

    if ( not argument->is_byteArray() )
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


PRIM_DECL_2( byteArrayPrimitives::largeIntegerMultiply, Oop
    receiver, Oop
                 argument ) {
    PROLOGUE_2( "largeIntegerMultiply", receiver, argument );
    ASSERT_RECEIVER;

    if ( not argument->is_byteArray() )
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
  if (not argument->is_byteArray())\
    return markSymbol(vmSymbols::first_argument_has_wrong_type());\
\
  ByteArrayOop x = ByteArrayOop(receiver);\
  ByteArrayOop y = ByteArrayOop(argument);\
\
  if (not x->number().is_valid() or not y->number().is_valid()) return markSymbol(vmSymbols::argument_is_invalid())


PRIM_DECL_2( byteArrayPrimitives::largeIntegerQuo, Oop receiver, Oop argument ) {
    DIVISION( receiver, argument, quo_result_size_in_bytes, quo, "largeIntegerQuo" );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerDiv, Oop receiver, Oop argument ) {
    DIVISION( receiver, argument, div_result_size_in_bytes, Div, "largeIntegerDiv" );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerMod, Oop receiver, Oop argument ) {
    DIVISION( receiver, argument, mod_result_size_in_bytes, Mod, "largeIntegerMod" );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerRem, Oop receiver, Oop argument ) {
    DIVISION( receiver, argument, rem_result_size_in_bytes, rem, "largeIntegerRem" );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerAnd, Oop receiver, Oop argument ) {
    BIT_OP( receiver, argument, and_result_size_in_bytes, And, "largeIntegerAnd" );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerOr, Oop receiver, Oop argument ) {
    BIT_OP( receiver, argument, or_result_size_in_bytes, Or, "largeIntegerOr" );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerXor, Oop receiver, Oop argument ) {
    BIT_OP( receiver, argument, xor_result_size_in_bytes, Xor, "largeIntegerXor" );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerShift, Oop receiver, Oop argument ) {
    PROLOGUE_2( "largeIntegerShift", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    ByteArrayOop x     = ByteArrayOop( receiver );
    int          shift = SMIOop( argument )->value();

    if ( not ByteArrayOop( receiver )->number().is_valid() )
        return markSymbol( vmSymbols::argument_is_invalid() );

    ByteArrayOop z = ByteArrayOop( x->klass()->klass_part()->allocateObjectSize( IntegerOps::ash_result_size_in_bytes( x->number(), shift ) ) );

    IntegerOps::ash( x->number(), shift, z->number() );

    return simplified( z );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerCompare, Oop receiver, Oop argument ) {
    PROLOGUE_2( "largeIntegerCompare", receiver, argument );
    ASSERT_RECEIVER;

    // Check argument
    if ( not argument->is_byteArray() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x = ByteArrayOop( receiver );
    ByteArrayOop y = ByteArrayOop( argument );

    if ( not x->number().is_valid() or not y->number().is_valid() )
        return markSymbol( vmSymbols::argument_is_invalid() );

    int res = IntegerOps::cmp( x->number(), y->number() );
    if ( res < 0 )
        return smiOopFromValue( -1 );
    if ( res > 0 )
        return smiOopFromValue( 1 );
    return smiOopFromValue( 0 );
}


PRIM_DECL_1( byteArrayPrimitives::largeIntegerToFloat, Oop receiver ) {
    PROLOGUE_1( "largeIntegerToFloat", receiver );
    ASSERT_RECEIVER;

    bool_t ok;
    double result = ByteArrayOop( receiver )->number().as_double( ok );

    if ( not ok )
        return markSymbol( vmSymbols::conversion_failed() );

    BlockScavenge bs;
    return oopFactory::new_double( result );
}


PRIM_DECL_2( byteArrayPrimitives::largeIntegerToString, Oop receiver, Oop base ) {
    PROLOGUE_1( "largeIntegerToString", receiver );
    ASSERT_RECEIVER;

    // Check argument
    if ( not base->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    BlockScavenge bs;

    ByteArrayOop x      = ByteArrayOop( receiver );
    int          length = IntegerOps::Integer_to_string_result_size_in_bytes( x->number(), SMIOop( base )->value() );

    ByteArrayOop result = oopFactory::new_byteArray( length );

    IntegerOps::Integer_to_string( x->number(), SMIOop( base )->value(), result->chars() );
    return result;
}


PRIM_DECL_1( byteArrayPrimitives::largeIntegerHash, Oop receiver ) {
    PROLOGUE_1( "largeIntegerHash", receiver );
    ASSERT_RECEIVER;
    return smiOopFromValue( IntegerOps::hash( ByteArrayOop( receiver )->number() ) );
}


PRIM_DECL_1( byteArrayPrimitives::hash, Oop
    receiver ) {
    PROLOGUE_1( "hash", receiver );
    ASSERT_RECEIVER;
    return smiOopFromValue( ByteArrayOop( receiver )->hash_value() );
}


PersistentHandle * _largeIntegerClass = nullptr;
PersistentHandle * _unsafeAlienClass  = nullptr;


KlassOop largeIntegerClass() {
    if ( _largeIntegerClass )
        return _largeIntegerClass->as_klassOop();
    Oop liKlass = Universe::find_global( "LargeInteger" );
    st_assert( liKlass and liKlass->is_klass(), "LargeInteger not found" );
    if ( not liKlass )
        return nullptr;
    _largeIntegerClass = new PersistentHandle( liKlass );
    return KlassOop( liKlass );
}


KlassOop unsafeAlienClass() {
    if ( _unsafeAlienClass )
        return _unsafeAlienClass->as_klassOop();
    Oop uaKlass = Universe::find_global( "UnsafeAlien" );
    st_assert( uaKlass and uaKlass->is_klass(), "UnsafeAlien not found" );
    if ( not uaKlass )
        return nullptr;
    _unsafeAlienClass = new PersistentHandle( uaKlass );
    return KlassOop( uaKlass );
}


Oop unsafeContents( Oop unsafeAlien ) {
    SymbolOop ivarName = oopFactory::new_symbol( "nonPointerObject" );
    int       offset   = unsafeAlienClass()->klass_part()->lookup_inst_var( ivarName );
    return MemOop( unsafeAlien )->instVarAt( offset );
}


#define checkAlienReceiver( receiver )\
  if (not receiver->is_byteArray())\
    return markSymbol(vmSymbols::receiver_has_wrong_type())

#define isUnsafe( argument )\
  (not Oop(argument)->is_smi()\
  and MemOop(argument)->klass_field() == unsafeAlienClass()\
  and unsafeContents(argument)->is_byteArray())

#define alienArg( argument )      (void*)argument

#define alienArray( receiver )    ((int*)ByteArrayOop(receiver)->bytes())

#define alienSize( receiver )     (alienArray(receiver)[0])

#define alienAddress( receiver )  ((void**)alienArray(receiver))[1]

#define alienResult( handle )     (handle.as_oop() == nilObj ? nullptr : (void*)handle.asPointer())
#define alienResult2( handle )     (handle->as_oop() == nilObj ? nullptr : (void*)handle->asPointer())

#define checkAlienCalloutReceiver( receiver ) \
  checkAlienReceiver(receiver);\
  if (/*alienSize(receiver) > 0 or */ alienAddress(receiver) == nullptr)\
    return markSymbol(vmSymbols::illegal_state())

#define checkAlienCalloutResult( argument ) \
  if (not (argument->is_byteArray() or argument == nilObj))\
    return markSymbol(vmSymbols::argument_has_wrong_type())

#define checkAlienCalloutResultArgs( argument ) \
  if (not (argument->is_byteArray() or argument == nilObj))\
    return markSymbol(vmSymbols::first_argument_has_wrong_type())

#define checkAlienCalloutArg( argument, symbol )\
  if (not ((argument->is_byteArray() and MemOop(argument)->klass() not_eq largeIntegerClass())\
      or argument->is_smi() or isUnsafe(argument)))\
    return markSymbol(symbol)

#define checkAlienCalloutArg1( argument )\
  checkAlienCalloutArg(argument, vmSymbols::second_argument_has_wrong_type())

#define checkAlienCalloutArg2( argument )\
  checkAlienCalloutArg(argument, vmSymbols::third_argument_has_wrong_type())

#define checkAlienCalloutArg3( argument )\
  checkAlienCalloutArg(argument, vmSymbols::fourth_argument_has_wrong_type())

#define checkAlienCalloutArg4( argument )\
  checkAlienCalloutArg(argument, vmSymbols::fifth_argument_has_wrong_type())

#define checkAlienCalloutArg5( argument )\
  checkAlienCalloutArg(argument, vmSymbols::sixth_argument_has_wrong_type())

#define checkAlienCalloutArg6( argument )\
  checkAlienCalloutArg(argument, vmSymbols::seventh_argument_has_wrong_type())

#define checkAlienCalloutArg7( argument )\
  checkAlienCalloutArg(argument, vmSymbols::eighth_argument_has_wrong_type())

#define checkAlienCalloutArg8( argument )\
  checkAlienCalloutArg(argument, vmSymbols::ninth_argument_has_wrong_type())

#define alienIndex( argument ) (SMIOop(argument)->value())

#define checkAlienAtIndex( receiver, argument, type )\
  if (not argument->is_smi())\
    return markSymbol(vmSymbols::argument_has_wrong_type());\
  if (alienIndex(argument) < 1 or\
      (alienSize(receiver) not_eq 0 and ((std::uint32_t)alienIndex(argument)) > abs(alienSize(receiver)) - sizeof(type) + 1))\
    return markSymbol(vmSymbols::index_not_valid())

#define checkAlienAtPutIndex( receiver, argument, type )\
  if (not argument->is_smi())\
    return markSymbol(vmSymbols::first_argument_has_wrong_type());\
  if (alienIndex(argument) < 1 or\
      (alienSize(receiver) not_eq 0 and ((std::uint32_t)alienIndex(argument)) > abs(alienSize(receiver)) - sizeof(type) + 1))\
    return markSymbol(vmSymbols::index_not_valid())

#define checkAlienAtPutValue( receiver, argument, type, min, max )\
  if (not argument->is_smi())\
    return markSymbol(vmSymbols::second_argument_has_wrong_type());\
  {\
    int value = SMIOop(argument)->value();\
    if (value < min or value > max)\
      return markSymbol(vmSymbols::argument_is_invalid());\
  }

#define checkAlienAtReceiver( receiver )\
  checkAlienReceiver(receiver);\
  if (alienSize(receiver) <= 0 and alienAddress(receiver) == nullptr)\
    return markSymbol(vmSymbols::illegal_state())

#define alienContents( receiver )\
  (alienSize(receiver) > 0\
    ? ((void*)(alienArray(receiver) + 1))\
    : (alienAddress(receiver)))

#define alienAt( receiver, argument, type )\
  *((type*)(((char*)alienContents(receiver)) + alienIndex(argument) - 1))


PRIM_DECL_2( byteArrayPrimitives::alienUnsignedByteAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienUnsignedByteAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::uint8_t );

    return smiOopFromValue( alienAt( receiver, argument, std::uint8_t ) );
}


PRIM_DECL_3( byteArrayPrimitives::alienUnsignedByteAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienUnsignedByteAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::uint8_t );
    checkAlienAtPutValue( receiver, argument2, std::uint8_t, 0, 255 );

    alienAt( receiver, argument1, std::uint8_t ) = SMIOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( byteArrayPrimitives::alienSignedByteAt, Oop
    receiver, Oop
                 argument ) {
    PROLOGUE_2( "alienSignedByteAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, char );

    return smiOopFromValue( alienAt( receiver, argument, char ) );
}


PRIM_DECL_3( byteArrayPrimitives::alienSignedByteAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienSignedByteAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, char );
    checkAlienAtPutValue( receiver, argument2, char, -128, 127 );

    alienAt( receiver, argument1, char ) = SMIOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( byteArrayPrimitives::alienUnsignedShortAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienUnsignedShortAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::uint16_t );

    return smiOopFromValue( alienAt( receiver, argument, std::uint16_t ) );
}


PRIM_DECL_3( byteArrayPrimitives::alienUnsignedShortAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienUnsignedShortAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::uint16_t );
    checkAlienAtPutValue( receiver, argument2, std::uint16_t, 0, 65535 );

    alienAt( receiver, argument1, std::uint16_t ) = SMIOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( byteArrayPrimitives::alienSignedShortAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSignedShortAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, int16_t );

    return smiOopFromValue( alienAt( receiver, argument, int16_t ) );
}


PRIM_DECL_3( byteArrayPrimitives::alienSignedShortAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienSignedShortAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, int16_t );
    checkAlienAtPutValue( receiver, argument2, int16_t, -32768, 32767 );

    alienAt( receiver, argument1, int16_t ) = SMIOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( byteArrayPrimitives::alienUnsignedLongAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienUnsignedLongAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::uint32_t );

    std::uint32_t value = alienAt( receiver, argument, std::uint32_t );

    int          resultSize   = IntegerOps::int_to_Integer_result_size_in_bytes( value );
    KlassOop     largeInteger = KlassOop( Universe::find_global( "LargeInteger" ) );
    ByteArrayOop result       = ByteArrayOop( largeInteger->klass_part()->allocateObjectSize( resultSize ) );
    IntegerOps::unsigned_int_to_Integer( value, result->number() );

    return simplified( result );
}


PRIM_DECL_3( byteArrayPrimitives::alienUnsignedLongAtPut, Oop receiver, Oop argument1, Oop argument2 ) {

    PROLOGUE_3( "alienUnsignedLongAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::uint32_t );
    if ( not argument2->is_smi() and not argument2->is_byteArray() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    std::uint32_t value;
    if ( argument2->is_smi() )
        value = SMIOop( argument2 )->value();
    else {
        bool_t ok;
        value = ByteArrayOop( argument2 )->number().as_unsigned_int( ok );
        if ( not ok )
            return markSymbol( vmSymbols::argument_is_invalid() );
    }

    alienAt( receiver, argument1, std::uint32_t ) = value;

    return argument2;
}


PRIM_DECL_2( byteArrayPrimitives::alienSignedLongAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSignedLongAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::int32_t );

    int value = alienAt( receiver, argument, std::int32_t );

    int          resultSize   = IntegerOps::int_to_Integer_result_size_in_bytes( value );
    KlassOop     largeInteger = KlassOop( Universe::find_global( "LargeInteger" ) );
    ByteArrayOop result       = ByteArrayOop( largeInteger->klass_part()->allocateObjectSize( resultSize ) );
    IntegerOps::int_to_Integer( value, result->number() );

    return simplified( result );
}


PRIM_DECL_3( byteArrayPrimitives::alienSignedLongAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienSignedLongAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::int32_t );
    if ( not argument2->is_smi() and not argument2->is_byteArray() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    int value;

    if ( argument2->is_smi() )
        value = SMIOop( argument2 )->value();
    else {
        bool_t ok;
        value = ByteArrayOop( argument2 )->number().as_int( ok );
        if ( not ok )
            return markSymbol( vmSymbols::argument_is_invalid() );
    }

    alienAt( receiver, argument1, std::int32_t ) = value;

    return argument2;
}


PRIM_DECL_2( byteArrayPrimitives::alienDoubleAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienDoubleAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, double );

    DoubleOop result = DoubleOop( Universe::doubleKlassObj()->klass_part()->allocateObject() );
    result->set_value( alienAt( receiver, argument, double ) );

    return result;
}


PRIM_DECL_3( byteArrayPrimitives::alienDoubleAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienDoubleAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, double );
    if ( not argument2->is_double() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    alienAt( receiver, argument1, double ) = DoubleOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( byteArrayPrimitives::alienFloatAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienFloatAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, float );

    DoubleOop result = DoubleOop( Universe::doubleKlassObj()->klass_part()->allocateObject() );
    result->set_value( alienAt( receiver, argument, float ) );

    return result;
}


PRIM_DECL_3( byteArrayPrimitives::alienFloatAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienFloatAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, float );
    if ( not argument2->is_double() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    alienAt( receiver, argument1, float ) = ( float ) DoubleOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_1( byteArrayPrimitives::alienGetSize, Oop receiver ) {
    PROLOGUE_1( "alienGetSize", receiver );
    checkAlienReceiver( receiver );

    return smiOopFromValue( alienSize( receiver ) );
}


PRIM_DECL_2( byteArrayPrimitives::alienSetSize, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSetSize", receiver, argument );
    checkAlienReceiver( receiver );
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::argument_has_wrong_type() );

    alienSize( receiver ) = SMIOop( argument )->value();
    return receiver;
}


PRIM_DECL_1( byteArrayPrimitives::alienGetAddress, Oop receiver ) {
    PROLOGUE_1( "alienGetAddress", receiver );
    checkAlienReceiver( receiver );

//  if (alienSize(receiver) > 0)
//    return markSymbol(vmSymbols::illegal_state());

    std::uint32_t address = ( std::uint32_t ) alienAddress( receiver );
    int      size    = IntegerOps::unsigned_int_to_Integer_result_size_in_bytes( address );

    Oop largeInteger = Universe::find_global( "LargeInteger" );
    Oop z            = KlassOop( largeInteger )->klass_part()->allocateObjectSize( size );
    IntegerOps::unsigned_int_to_Integer( address, ByteArrayOop( z )->number() );
    return simplified( ByteArrayOop( z ) );
}


PRIM_DECL_2( byteArrayPrimitives::alienSetAddress, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSetAddress", receiver, argument );
    checkAlienReceiver( receiver );
    if ( alienSize( receiver ) > 0 )
        return markSymbol( vmSymbols::illegal_state() );
    if ( not argument->is_smi() and not( argument->is_byteArray() and ByteArrayOop( argument )->number().signum() > 0 ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    std::uint32_t value;
    if ( argument->is_smi() )
        value = SMIOop( argument )->value();
    else {
        bool_t ok;
        value = ByteArrayOop( argument )->number().as_unsigned_int( ok );
        if ( not ok )
            return markSymbol( vmSymbols::argument_is_invalid() );
    }
    alienAddress( receiver ) = ( void * ) value;

    return receiver;
}


typedef void ( __CALLING_CONVENTION * call_out_func_0 )( void *, void * );

typedef void ( __CALLING_CONVENTION * call_out_func_1 )( void *, void *, void * );

typedef void ( __CALLING_CONVENTION * call_out_func_2 )( void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION * call_out_func_3 )( void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION * call_out_func_4 )( void *, void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION * call_out_func_5 )( void *, void *, void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION * call_out_func_6 )( void *, void *, void *, void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION * call_out_func_7 )( void *, void *, void *, void *, void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION * call_out_func_args)( void *, void *, Oop, Oop * );


void break_on_error( void * address, Oop result ) {
    if ( false )
        return;
    if ( not result->is_byteArray() )
        return;

    int value = alienAt( ByteArrayOop(result), smiOopFromValue( 1 ), int );

    int err = os::error_code();
    if ( value == 0 and err ) {
        ResourceMark resourceMark;
        _console->print_cr( "Last error: 0x%x %d", address, err );
        DeltaProcess::active()->trace_top( 1, 5 );
        if ( false )
            os::breakpoint();
    }
}


PRIM_DECL_2( byteArrayPrimitives::alienCallResult0, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienCallResult0", receiver, argument );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResult( argument );

    PersistentHandle * resultHandle = new PersistentHandle( argument );
    call_out_func_0  entry          = call_out_func_0( StubRoutines::alien_call_entry( 0 ) );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_3( byteArrayPrimitives::alienCallResult1, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienCallResult1", receiver, argument1, argument2 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );

    PersistentHandle * resultHandle = new PersistentHandle( argument1 );
    call_out_func_1  entry          = call_out_func_1( StubRoutines::alien_call_entry( 1 ) );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_4( byteArrayPrimitives::alienCallResult2, Oop receiver, Oop argument1, Oop argument2, Oop argument3 ) {
    PROLOGUE_4( "alienCallResult2", receiver, argument1, argument2, argument3 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );

    PersistentHandle * resultHandle = new PersistentHandle( argument1 );
    call_out_func_2  entry          = call_out_func_2( StubRoutines::alien_call_entry( 2 ) );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_5( byteArrayPrimitives::alienCallResult3, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4 ) {
    PROLOGUE_5( "alienCallResult3", receiver, argument1, argument2, argument3, argument4 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );

    PersistentHandle * resultHandle = new PersistentHandle( argument1 );
    call_out_func_3  entry          = call_out_func_3( StubRoutines::alien_call_entry( 3 ) );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_6( byteArrayPrimitives::alienCallResult4, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5 ) {
    PROLOGUE_6( "alienCallResult4", receiver, argument1, argument2, argument3, argument4, argument5 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );
    checkAlienCalloutArg4( argument5 );

    PersistentHandle * resultHandle = new PersistentHandle( argument1 );
    call_out_func_4  entry          = call_out_func_4( StubRoutines::alien_call_entry( 4 ) );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ), alienArg( argument5 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_7( byteArrayPrimitives::alienCallResult5, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6 ) {
    PROLOGUE_7( "alienCallResult5", receiver, argument1, argument2, argument3, argument4, argument5, argument6 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );
    checkAlienCalloutArg4( argument5 );
    checkAlienCalloutArg5( argument6 );

    PersistentHandle * resultHandle = new PersistentHandle( argument1 );
    call_out_func_5  entry          = call_out_func_5( StubRoutines::alien_call_entry( 5 ) );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ), alienArg( argument5 ), alienArg( argument6 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_8( byteArrayPrimitives::alienCallResult6, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6, Oop argument7 ) {
    PROLOGUE_8( "alienCallResult6", receiver, argument1, argument2, argument3, argument4, argument5, argument6, argument7 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );
    checkAlienCalloutArg4( argument5 );
    checkAlienCalloutArg5( argument6 );
    checkAlienCalloutArg6( argument7 );

    PersistentHandle * resultHandle = new PersistentHandle( argument1 );
    call_out_func_6  entry          = call_out_func_6( StubRoutines::alien_call_entry( 6 ) );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ), alienArg( argument5 ), alienArg( argument6 ), alienArg( argument7 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_9( byteArrayPrimitives::alienCallResult7, Oop
    receiver, Oop
                 argument1, Oop
                 argument2, Oop
                 argument3, Oop
                 argument4, Oop
                 argument5, Oop
                 argument6, Oop
                 argument7, Oop
                 argument8 ) {
    PROLOGUE_9( "alienCallResult7", receiver, argument1, argument2, argument3, argument4, argument5, argument6, argument7, argument8 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );
    checkAlienCalloutArg4( argument5 );
    checkAlienCalloutArg5( argument6 );
    checkAlienCalloutArg6( argument7 );
    checkAlienCalloutArg7( argument8 );

    PersistentHandle * resultHandle = new PersistentHandle( argument1 );
    call_out_func_7  entry          = call_out_func_7( StubRoutines::alien_call_entry( 7 ) );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ), alienArg( argument5 ), alienArg( argument6 ), alienArg( argument7 ), alienArg( argument8 ) );
    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_3( byteArrayPrimitives::alienCallResultWithArguments, Oop
    receiver, Oop
                 argument1, Oop
                 argument2 ) {
    PROLOGUE_3( "alienCallResultWithArguments", receiver, argument1, argument2 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );

    int                length         = ObjectArrayOop( argument2 )->length();
    for ( int          index          = 1; index <= length; index++ ) {
        checkAlienCalloutArg( ObjectArrayOop(argument2)->obj_at(index), vmSymbols::argument_has_wrong_type() );
    }
    PersistentHandle   * resultHandle = new PersistentHandle( argument1 );
    call_out_func_args entry          = call_out_func_args( StubRoutines::alien_call_with_args_entry() );

    void * address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), smiOopFromValue( length ), ObjectArrayOop( argument2 )->objs( 1 ) );
    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}
