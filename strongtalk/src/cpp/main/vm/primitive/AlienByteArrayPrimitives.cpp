
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/oop/ProxyOopDescriptor.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/utility/IntegerOps.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/platform/os.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/primitive/primitive_tracing.hpp"
#include "vm/primitive/alien_macros.hpp"


PersistentHandle *_largeIntegerClass = nullptr;
PersistentHandle *_unsafeAlienClass  = nullptr;


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
    SymbolOop    ivarName = OopFactory::new_symbol( "nonPointerObject" );
    std::int32_t offset   = unsafeAlienClass()->klass_part()->lookup_inst_var( ivarName );
    return MemOop( unsafeAlien )->instVarAt( offset );
}


TRACE_FUNC( TraceByteArrayPrims, "byteArray" )


PRIM_DECL_2( ByteArrayPrimitives::alienUnsignedByteAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienUnsignedByteAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::uint8_t );

    return smiOopFromValue( alienAt( receiver, argument, std::uint8_t ) );
}


PRIM_DECL_3( ByteArrayPrimitives::alienUnsignedByteAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienUnsignedByteAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::uint8_t );
    checkAlienAtPutValue( receiver, argument2, std::uint8_t, 0, 255 );

    alienAt( receiver, argument1, std::uint8_t ) = SmallIntegerOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( ByteArrayPrimitives::alienSignedByteAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSignedByteAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, char );

    return smiOopFromValue( alienAt( receiver, argument, char ) );
}


PRIM_DECL_3( ByteArrayPrimitives::alienSignedByteAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienSignedByteAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, char );
    checkAlienAtPutValue( receiver, argument2, char, -128, 127 );

    alienAt( receiver, argument1, char ) = SmallIntegerOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( ByteArrayPrimitives::alienUnsignedShortAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienUnsignedShortAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::uint16_t );

    return smiOopFromValue( alienAt( receiver, argument, std::uint16_t ) );
}


PRIM_DECL_3( ByteArrayPrimitives::alienUnsignedShortAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienUnsignedShortAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::uint16_t );
    checkAlienAtPutValue( receiver, argument2, std::uint16_t, 0, 65535 );

    alienAt( receiver, argument1, std::uint16_t ) = SmallIntegerOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( ByteArrayPrimitives::alienSignedShortAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSignedShortAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::int16_t );

    return smiOopFromValue( alienAt( receiver, argument, std::int16_t ) );
}


PRIM_DECL_3( ByteArrayPrimitives::alienSignedShortAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienSignedShortAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::int16_t );
    checkAlienAtPutValue( receiver, argument2, std::int16_t, -32768, 32767 );

    alienAt( receiver, argument1, std::int16_t ) = SmallIntegerOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( ByteArrayPrimitives::alienUnsignedLongAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienUnsignedLongAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::uint32_t );

    std::uint32_t value = alienAt( receiver, argument, std::uint32_t );

    std::int32_t resultSize   = IntegerOps::int_to_Integer_result_size_in_bytes( value );
    KlassOop     largeInteger = KlassOop( Universe::find_global( "LargeInteger" ) );
    ByteArrayOop result       = ByteArrayOop( largeInteger->klass_part()->allocateObjectSize( resultSize ) );
    IntegerOps::unsigned_int_to_Integer( value, result->number() );

    return simplified( result );
}


PRIM_DECL_3( ByteArrayPrimitives::alienUnsignedLongAtPut, Oop receiver, Oop argument1, Oop argument2 ) {

    PROLOGUE_3( "alienUnsignedLongAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::uint32_t );
    if ( not argument2->isSmallIntegerOop() and not argument2->isByteArray() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    std::uint32_t value;
    if ( argument2->isSmallIntegerOop() )
        value = SmallIntegerOop( argument2 )->value();
    else {
        bool ok;
        value = ByteArrayOop( argument2 )->number().as_uint32_t( ok );
        if ( not ok )
            return markSymbol( vmSymbols::argument_is_invalid() );
    }

    alienAt( receiver, argument1, std::uint32_t ) = value;

    return argument2;
}


PRIM_DECL_2( ByteArrayPrimitives::alienSignedLongAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSignedLongAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, std::int32_t );

    std::int32_t value = alienAt( receiver, argument, std::int32_t );

    std::int32_t resultSize   = IntegerOps::int_to_Integer_result_size_in_bytes( value );
    KlassOop     largeInteger = KlassOop( Universe::find_global( "LargeInteger" ) );
    ByteArrayOop result       = ByteArrayOop( largeInteger->klass_part()->allocateObjectSize( resultSize ) );
    IntegerOps::int_to_Integer( value, result->number() );

    return simplified( result );
}


PRIM_DECL_3( ByteArrayPrimitives::alienSignedLongAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienSignedLongAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, std::int32_t );
    if ( not argument2->isSmallIntegerOop() and not argument2->isByteArray() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    std::int32_t value;

    if ( argument2->isSmallIntegerOop() )
        value = SmallIntegerOop( argument2 )->value();
    else {
        bool ok;
        value = ByteArrayOop( argument2 )->number().as_int32_t( ok );
        if ( not ok )
            return markSymbol( vmSymbols::argument_is_invalid() );
    }

    alienAt( receiver, argument1, std::int32_t ) = value;

    return argument2;
}


PRIM_DECL_2( ByteArrayPrimitives::alienDoubleAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienDoubleAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, double );

    DoubleOop result = DoubleOop( Universe::doubleKlassObject()->klass_part()->allocateObject() );
    result->set_value( alienAt( receiver, argument, double ) );

    return result;
}


PRIM_DECL_3( ByteArrayPrimitives::alienDoubleAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienDoubleAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, double );
    if ( not argument2->isDouble() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    alienAt( receiver, argument1, double ) = DoubleOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_2( ByteArrayPrimitives::alienFloatAt, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienFloatAt", receiver, argument );
    checkAlienAtReceiver( receiver );
    checkAlienAtIndex( receiver, argument, float );

    DoubleOop result = DoubleOop( Universe::doubleKlassObject()->klass_part()->allocateObject() );
    result->set_value( alienAt( receiver, argument, float ) );

    return result;
}


PRIM_DECL_3( ByteArrayPrimitives::alienFloatAtPut, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienFloatAtPut", receiver, argument1, argument2 );
    checkAlienAtReceiver( receiver );
    checkAlienAtPutIndex( receiver, argument1, float );
    if ( not argument2->isDouble() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    alienAt( receiver, argument1, float ) = (float) DoubleOop( argument2 )->value();

    return argument2;
}


PRIM_DECL_1( ByteArrayPrimitives::alienGetSize, Oop receiver ) {
    PROLOGUE_1( "alienGetSize", receiver );
    checkAlienReceiver( receiver );

    return smiOopFromValue( alienSize( receiver ) );
}


PRIM_DECL_2( ByteArrayPrimitives::alienSetSize, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSetSize", receiver, argument );
    checkAlienReceiver( receiver );
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::argument_has_wrong_type() );

    alienSize( receiver ) = SmallIntegerOop( argument )->value();
    return receiver;
}


PRIM_DECL_1( ByteArrayPrimitives::alienGetAddress, Oop receiver ) {
    PROLOGUE_1( "alienGetAddress", receiver );
    checkAlienReceiver( receiver );

//  if (alienSize(receiver) > 0)
//    return markSymbol(vmSymbols::illegal_state());

    std::uint32_t address = (std::uint32_t) alienAddress( receiver );
    std::int32_t  size    = IntegerOps::unsigned_int_to_Integer_result_size_in_bytes( address );

    Oop largeInteger = Universe::find_global( "LargeInteger" );
    Oop z            = KlassOop( largeInteger )->klass_part()->allocateObjectSize( size );
    IntegerOps::unsigned_int_to_Integer( address, ByteArrayOop( z )->number() );
    return simplified( ByteArrayOop( z ) );
}


PRIM_DECL_2( ByteArrayPrimitives::alienSetAddress, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienSetAddress", receiver, argument );
    checkAlienReceiver( receiver );
    if ( alienSize( receiver ) > 0 )
        return markSymbol( vmSymbols::illegal_state() );
    if ( not argument->isSmallIntegerOop() and not( argument->isByteArray() and ByteArrayOop( argument )->number().signed_length() > 0 ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    std::uint32_t value;
    if ( argument->isSmallIntegerOop() )
        value = SmallIntegerOop( argument )->value();
    else {
        bool ok;
        value = ByteArrayOop( argument )->number().as_uint32_t( ok );
        if ( not ok )
            return markSymbol( vmSymbols::argument_is_invalid() );
    }
    alienAddress( receiver ) = (void *) value;


    return receiver;
}


typedef void ( __CALLING_CONVENTION *call_out_func_0 )( void *, void * );

typedef void ( __CALLING_CONVENTION *call_out_func_1 )( void *, void *, void * );

typedef void ( __CALLING_CONVENTION *call_out_func_2 )( void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION *call_out_func_3 )( void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION *call_out_func_4 )( void *, void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION *call_out_func_5 )( void *, void *, void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION *call_out_func_6 )( void *, void *, void *, void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION *call_out_func_7 )( void *, void *, void *, void *, void *, void *, void *, void *, void * );

typedef void ( __CALLING_CONVENTION *call_out_func_args)( void *, void *, Oop, Oop * );


void break_on_error( void *address, Oop result ) {
    if ( false )
        return;
    if ( not result->isByteArray() )
        return;

    std::int32_t value = alienAt( ByteArrayOop(result), smiOopFromValue( 1 ), std::int32_t );

    std::int32_t err = os::error_code();
    if ( value == 0 and err ) {
        ResourceMark resourceMark;
        SPDLOG_INFO( "Last error: 0x{0:x} {}", address, err );
        DeltaProcess::active()->trace_top( 1, 5 );
        if ( false )
            os::breakpoint();
    }
}


PRIM_DECL_2( ByteArrayPrimitives::alienCallResult0, Oop receiver, Oop argument ) {
    PROLOGUE_2( "alienCallResult0", receiver, argument );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResult( argument );

    PersistentHandle *resultHandle = new PersistentHandle( argument );
    call_out_func_0  entry         = call_out_func_0( StubRoutines::alien_call_entry( 0 ) );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_3( ByteArrayPrimitives::alienCallResult1, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienCallResult1", receiver, argument1, argument2 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );

    PersistentHandle *resultHandle = new PersistentHandle( argument1 );
    call_out_func_1  entry         = call_out_func_1( StubRoutines::alien_call_entry( 1 ) );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_4( ByteArrayPrimitives::alienCallResult2, Oop receiver, Oop argument1, Oop argument2, Oop argument3 ) {
    PROLOGUE_4( "alienCallResult2", receiver, argument1, argument2, argument3 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );

    PersistentHandle *resultHandle = new PersistentHandle( argument1 );
    call_out_func_2  entry         = call_out_func_2( StubRoutines::alien_call_entry( 2 ) );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_5( ByteArrayPrimitives::alienCallResult3, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4 ) {
    PROLOGUE_5( "alienCallResult3", receiver, argument1, argument2, argument3, argument4 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );

    PersistentHandle *resultHandle = new PersistentHandle( argument1 );
    call_out_func_3  entry         = call_out_func_3( StubRoutines::alien_call_entry( 3 ) );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_6( ByteArrayPrimitives::alienCallResult4, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5 ) {
    PROLOGUE_6( "alienCallResult4", receiver, argument1, argument2, argument3, argument4, argument5 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );
    checkAlienCalloutArg4( argument5 );

    PersistentHandle *resultHandle = new PersistentHandle( argument1 );
    call_out_func_4  entry         = call_out_func_4( StubRoutines::alien_call_entry( 4 ) );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ), alienArg( argument5 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_7( ByteArrayPrimitives::alienCallResult5, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6 ) {
    PROLOGUE_7( "alienCallResult5", receiver, argument1, argument2, argument3, argument4, argument5, argument6 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );
    checkAlienCalloutArg4( argument5 );
    checkAlienCalloutArg5( argument6 );

    PersistentHandle *resultHandle = new PersistentHandle( argument1 );
    call_out_func_5  entry         = call_out_func_5( StubRoutines::alien_call_entry( 5 ) );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ), alienArg( argument5 ), alienArg( argument6 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_8( ByteArrayPrimitives::alienCallResult6, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6, Oop argument7 ) {
    PROLOGUE_8( "alienCallResult6", receiver, argument1, argument2, argument3, argument4, argument5, argument6, argument7 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );
    checkAlienCalloutArg1( argument2 );
    checkAlienCalloutArg2( argument3 );
    checkAlienCalloutArg3( argument4 );
    checkAlienCalloutArg4( argument5 );
    checkAlienCalloutArg5( argument6 );
    checkAlienCalloutArg6( argument7 );

    PersistentHandle *resultHandle = new PersistentHandle( argument1 );
    call_out_func_6  entry         = call_out_func_6( StubRoutines::alien_call_entry( 6 ) );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ), alienArg( argument5 ), alienArg( argument6 ), alienArg( argument7 ) );

    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_9( ByteArrayPrimitives::alienCallResult7, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6, Oop argument7, Oop argument8 ) {
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

    PersistentHandle *resultHandle = new PersistentHandle( argument1 );
    call_out_func_7  entry         = call_out_func_7( StubRoutines::alien_call_entry( 7 ) );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), alienArg( argument2 ), alienArg( argument3 ), alienArg( argument4 ), alienArg( argument5 ), alienArg( argument6 ), alienArg( argument7 ), alienArg( argument8 ) );
    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}


PRIM_DECL_3( ByteArrayPrimitives::alienCallResultWithArguments, Oop receiver, Oop argument1, Oop argument2 ) {
    PROLOGUE_3( "alienCallResultWithArguments", receiver, argument1, argument2 );
    checkAlienCalloutReceiver( receiver );
    checkAlienCalloutResultArgs( argument1 );

    std::int32_t length = ObjectArrayOop( argument2 )->length();

    for ( std::int32_t index         = 1; index <= length; index++ ) {
        checkAlienCalloutArg( ObjectArrayOop(argument2)->obj_at(index), vmSymbols::argument_has_wrong_type() );
    }
    PersistentHandle   *resultHandle = new PersistentHandle( argument1 );
    call_out_func_args entry         = call_out_func_args( StubRoutines::alien_call_with_args_entry() );

    void *address = alienAddress( receiver );
    entry( address, alienResult2( resultHandle ), smiOopFromValue( length ), ObjectArrayOop( argument2 )->objs( 1 ) );
    Oop result = resultHandle->as_oop();
    break_on_error( address, result );
    delete resultHandle;
    return result;
}
