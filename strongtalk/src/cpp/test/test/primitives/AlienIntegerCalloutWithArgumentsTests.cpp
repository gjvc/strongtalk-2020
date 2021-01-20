
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"

#include <gtest/gtest.h>


extern "C" int __CALLING_CONVENTION returnFirst2( int a, int b ) {
    return a;
}

extern "C" int __CALLING_CONVENTION returnFirstPointer2( int *a, int b ) {
    return *a;
}

extern "C" int __CALLING_CONVENTION returnSecond2( int a, int b ) {
    return b;
}

extern "C" int __CALLING_CONVENTION returnSecondPointer2( int a, int *b ) {
    return *b;
}

extern "C" int __CALLING_CONVENTION forceScavengeWA( int a, int b ) {
    Universe::scavenge();
    return -1;
}

extern "C" int __CALLING_CONVENTION argAlignment2( int a, int b ) {
    return ( (int) &a ) & 0xF;
}


class AlienIntegerCalloutWithArgumentsTests : public ::testing::Test {

protected:
    void SetUp() override {
        rm      = new HeapResourceMark();
        smi0    = smiOopFromValue( 0 );
        smi1    = smiOopFromValue( 1 );
        smim1   = smiOopFromValue( -1 );
        handles = new( true ) GrowableArray<PersistentHandle **>( 5 );

        allocateAlien( functionAlien, 8, 0, reinterpret_cast<void *>(&returnFirst2) );
        allocateAlien( resultAlien, 12, 8 );
        allocateAlien( directAlien, 12, 4 );
        allocateAlien( addressAlien, 8, -4, &address );
        allocateAlien( pointerAlien, 8, 0, &address );
        allocateAlien( invalidFunctionAlien, 8, 0 );

        std::fill( &address[ 0 ], &address[ 8 ], '\0' );

        intCalloutFunctions[ 0 ]        = reinterpret_cast<void *>(returnFirst2);
        intCalloutFunctions[ 1 ]        = reinterpret_cast<void *>(returnSecond2);
        intPointerCalloutFunctions[ 0 ] = reinterpret_cast<void *>(returnFirstPointer2);
        intPointerCalloutFunctions[ 1 ] = reinterpret_cast<void *>(returnSecondPointer2);
    }


    void TearDown() override {
        while ( !handles->isEmpty() )
            release( handles->pop() );
        free( handles );
        handles = nullptr;
        delete rm;
        rm = nullptr;
    }


    HeapResourceMark *rm;
    GrowableArray<PersistentHandle **> *handles;
    PersistentHandle *resultAlien, *addressAlien, *pointerAlien, *functionAlien;
    PersistentHandle *directAlien, *invalidFunctionAlien;
    SMIOop                         smi0, smi1, smim1;
    static const int               argCount = 2;
    std::array<void *, argCount>   intCalloutFunctions;
    std::array<void *, argCount>   intPointerCalloutFunctions;
    std::array<Oop, argCount>      zeroes;
    std::array<char, 16>           address;


    void allocateAlien( PersistentHandle *&alienHandle, int arraySize, int alienSize, void *ptr = nullptr ) {
        ByteArrayOop alien = ByteArrayOop( Universe::byteArrayKlassObj()->klass_part()->allocateObjectSize( arraySize ) );
        byteArrayPrimitives::alienSetSize( smiOopFromValue( alienSize ), alien );
        if ( ptr )
            byteArrayPrimitives::alienSetAddress( smiOopFromValue( (int) ptr ), alien );
        alienHandle = new PersistentHandle( alien );
        handles->append( &alienHandle );
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->is_mark() ) << "Should be marked";
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void checkIntResult( const char *message, int expected, PersistentHandle *alien ) {
        char   text[200];
        bool_t ok;
        int    actual = asInt( ok, byteArrayPrimitives::alienSignedLongAt( smi1, alien->as_oop() ) );
        EXPECT_TRUE( ok ) << "not an integer result";
        sprintf( text, "Should be: %d, was: %d", expected, actual );
        EXPECT_TRUE( actual == expected ) << text;
    }


    int asInt( bool_t &ok, Oop intOop ) {
        ok = true;
        if ( intOop->is_smi() )
            return SMIOop( intOop )->value();
        if ( !intOop->is_byteArray() ) {
            ok = false;
            return 0;
        }
        return ByteArrayOop( intOop )->number().as_int( ok );
    }


    Oop asOop( int value ) {
        int          size     = IntegerOps::int_to_Integer_result_size_in_bytes( value );
        ByteArrayOop valueOop = ByteArrayOop( Universe::byteArrayKlassObj()->klass_part()->allocateObjectSize( size ) );
        IntegerOps::int_to_Integer( value, valueOop->number() );
        bool_t ok;
        Oop    result         = valueOop->number().as_smi( ok );
        return ok ? result : valueOop;
    }


    void release( PersistentHandle **handle ) {
        delete *handle;
        *handle = nullptr;
    }


    void setAddress( PersistentHandle *handle, void *argument ) {
        byteArrayPrimitives::alienSetAddress( asOop( (int) argument ), handle->as_oop() );
    }


    Oop callout( std::array<Oop, argCount> arg, Oop result, Oop address ) {

        ObjectArrayOop argOops = oopFactory::new_objArray( 2 );

        for ( std::size_t index = 0; index < 2; index++ )
            argOops->obj_at_put( index + 1, arg[ index ] );

        return byteArrayPrimitives::alienCallResultWithArguments( argOops, result, address );
    }


    Oop callout( std::array<Oop, argCount> arg ) {
        return callout( arg, resultAlien->as_oop(), functionAlien->as_oop() );
    }


    void checkArgnPassed( int argIndex, int argValue, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        std::array<Oop, argCount> arg;

        for ( std::size_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? asOop( argValue ) : smi0;

        Oop result = callout( arg );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", argValue, resultAlien );
    }


    void checkArgnPtrPassed( int argIndex, Oop pointer, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        std::array<Oop, argCount> arg;

        for ( std::size_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? pointer : smi0;

        Oop result = callout( arg );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", -1, resultAlien );
    }


    void checkIllegalArgnPassed( int argIndex, Oop pointer ) {
        std::array<Oop, argCount> arg;

        for ( std::size_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? pointer : smi0;

        Oop result = callout( arg );

        checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
    }


};


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldCallIntArgFunction
) {
for (
int arg = 0;
    arg<argCount;
arg++ )
checkArgnPassed( arg,
-1, intCalloutFunctions );
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResult1Should16ByteAlignArgs
) {
Oop fnAddress = asOop( (int) &argAlignment2 );
byteArrayPrimitives::alienSetAddress( fnAddress, functionAlien
->
as_oop()
);

std::array<Oop, argCount> arg;

arg[ 1 ] =
smi0;
for (
std::size_t size = -4;
size > -20; size -= 4 ) {
arg[ 0 ] = addressAlien->
as_oop();
byteArrayPrimitives::alienSetSize( smiOopFromValue( size ), addressAlien
->
as_oop()
);
callout( arg );
checkIntResult( "wrong result", 0, resultAlien );
}
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldCallIntPointerArgFunction
) {
byteArrayPrimitives::alienSignedLongAtPut( asOop( -1 ), smi1, pointerAlien
->
as_oop()
);
for (
int arg = 0;
    arg<argCount;
arg++ )
checkArgnPtrPassed( arg, pointerAlien
->
as_oop(), intPointerCalloutFunctions
);
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldCallFunctionAndIgnoreResultWhenResultAlienNil
) {
Oop result = callout( zeroes, nilObj, functionAlien->as_oop() );
EXPECT_TRUE( !result->
is_mark()
) << "should not be marked";
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsWithScavengeShouldReturnCorrectResult
) {
setAddress( functionAlien,
reinterpret_cast
<void *>(&forceScavengeWA)
);
checkIntResult( "incorrect initialization", 0, resultAlien );
Oop result = callout( zeroes, resultAlien->as_oop(), functionAlien->as_oop() );
checkIntResult( "result alien not updated", -1, resultAlien );
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultForNonAlien
) {
Oop result = callout( zeroes, resultAlien->as_oop(), smi0 );

checkMarkedSymbol( "wrong type", result,
vmSymbols::receiver_has_wrong_type()
);
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultForDirectAlien
) {
Oop result = callout( zeroes, resultAlien->as_oop(), resultAlien->as_oop() );

checkMarkedSymbol( "illegal state", result,
vmSymbols::illegal_state()
);
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultForNullFunctionPointer
) {
Oop result = callout( zeroes, resultAlien->as_oop(), invalidFunctionAlien->as_oop() );

checkMarkedSymbol( "illegal state", result,
vmSymbols::illegal_state()
);
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultWhenResultNotAlienOrNil
) {
Oop result = callout( zeroes, trueObj, functionAlien->as_oop() );

checkMarkedSymbol( "wrong type", result,
vmSymbols::first_argument_has_wrong_type()
);
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultWhenFunctionParameterNotAlienOrSMI
) {
for (
int arg = 0;
    arg<argCount;
arg++ )
checkIllegalArgnPassed( arg, trueObj
);
}
