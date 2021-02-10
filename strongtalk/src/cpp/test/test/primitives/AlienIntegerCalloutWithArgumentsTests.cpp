
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/primitives/ByteArrayPrimitives.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"

#include <gtest/gtest.h>


extern "C" std::int32_t __CALLING_CONVENTION returnFirst2( std::int32_t a, std::int32_t b ) {
    static_cast<void>(b); // unused
    return a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFirstPointer2( std::int32_t *a, std::int32_t b ) {
    static_cast<void>(b); // unused
    return *a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecond2( std::int32_t a, std::int32_t b ) {
    static_cast<void>(a); // unused
    return b;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecondPointer2( std::int32_t a, std::int32_t *b ) {
    static_cast<void>(a); // unused
    return *b;
}

extern "C" std::int32_t __CALLING_CONVENTION forceScavengeWA( std::int32_t a, std::int32_t b ) {
    static_cast<void>(a); // unused
    static_cast<void>(b); // unused
    Universe::scavenge();
    return -1;
}

extern "C" std::int32_t __CALLING_CONVENTION argAlignment2( std::int32_t a, std::int32_t b ) {
    static_cast<void>(b); // unused
    return ( (std::int32_t) &a ) & 0xF;
}


class AlienIntegerCalloutWithArgumentsTests : public ::testing::Test {

public:
    AlienIntegerCalloutWithArgumentsTests() :
        ::testing::Test(),
        rm{ nullptr },
        handles{ nullptr },
        resultAlien{ nullptr },
        addressAlien{ nullptr },
        pointerAlien{ nullptr },
        functionAlien{ nullptr },
        directAlien{ nullptr },
        invalidFunctionAlien{ nullptr },
        smi0{},
        smi1{},
        smim1{},
        intCalloutFunctions{},
        intPointerCalloutFunctions{},
        address{} {}


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
        while ( !handles->isEmpty() ) {
            release( handles->pop() );
        }

        free( handles );
        handles = nullptr;
        delete rm;
        rm = nullptr;
    }


    HeapResourceMark                   *rm;
    GrowableArray<PersistentHandle **> *handles;
    PersistentHandle                   *resultAlien, *addressAlien, *pointerAlien, *functionAlien;
    PersistentHandle                   *directAlien, *invalidFunctionAlien;
    SmallIntegerOop                    smi0, smi1, smim1;
    static const std::int32_t          argCount = 2;
    std::array<void *, argCount>       intCalloutFunctions;
    std::array<void *, argCount>       intPointerCalloutFunctions;
    std::array<Oop, argCount>          zeroes;
    std::array<char, 16>               address;


    void allocateAlien( PersistentHandle *&alienHandle, std::int32_t arraySize, std::int32_t alienSize, void *ptr = nullptr ) {
        ByteArrayOop alien = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( arraySize ) );
        ByteArrayPrimitives::alienSetSize( smiOopFromValue( alienSize ), alien );
        if ( ptr ) {
            ByteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) ptr ), alien );
        }
        alienHandle = new PersistentHandle( alien );
        handles->append( &alienHandle );
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->isMarkOop() ) << "Should be marked";
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void checkIntResult( const char *message, std::int32_t expected, PersistentHandle *alien ) {
        static_cast<void>(message); // unused

        char         text[200];
        bool         ok;
        std::int32_t actual = asInt( ok, ByteArrayPrimitives::alienSignedLongAt( smi1, alien->as_oop() ) );

        EXPECT_TRUE( ok ) << "not an integer result";
        sprintf( text, "Should be: %d, was: %d", expected, actual );
        EXPECT_TRUE( actual == expected ) << text;
    }


    std::int32_t asInt( bool &ok, Oop intOop ) {
        ok = true;
        if ( intOop->isSmallIntegerOop() ) {
            return SmallIntegerOop( intOop )->value();
        }

        if ( !intOop->isByteArray() ) {
            ok = false;
            return 0;
        }

        return ByteArrayOop( intOop )->number().as_int32_t( ok );
    }


    Oop asOop( std::int32_t value ) {
        std::int32_t size     = IntegerOps::int_to_Integer_result_size_in_bytes( value );
        ByteArrayOop valueOop = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( size ) );
        IntegerOps::int_to_Integer( value, valueOop->number() );
        bool ok;

        Oop result = valueOop->number().as_SmallIntegerOop( ok );
        return ok ? result : valueOop;
    }


    void release( PersistentHandle **handle ) {
        delete *handle;
        *handle = nullptr;
    }


    void setAddress( PersistentHandle *handle, void *argument ) {
        ByteArrayPrimitives::alienSetAddress( asOop( (std::int32_t) argument ), handle->as_oop() );
    }


    Oop callout( std::array<Oop, argCount> arg, Oop result, Oop address ) {

        ObjectArrayOop argOops = OopFactory::new_objectArray( 2 );

        for ( std::size_t index = 0; index < 2; index++ ) {
            argOops->obj_at_put( index + 1, arg[ index ] );
        }

        return ByteArrayPrimitives::alienCallResultWithArguments( argOops, result, address );
    }


    Oop callout( std::array<Oop, argCount> arg ) {
        return callout( arg, resultAlien->as_oop(), functionAlien->as_oop() );
    }


    void checkArgnPassed( std::int32_t argIndex, std::int32_t argValue, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        std::array<Oop, argCount> arg;

        for ( std::size_t index = 0; index < argCount; index++ ) {
            arg[ index ] = argIndex == index ? asOop( argValue ) : smi0;
        }

        Oop result = callout( arg );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", argValue, resultAlien );
    }


    void checkArgnPtrPassed( std::int32_t argIndex, Oop pointer, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        std::array<Oop, argCount> arg;

        for ( std::size_t index = 0; index < argCount; index++ ) {
            arg[ index ] = argIndex == index ? pointer : smi0;
        }

        Oop result = callout( arg );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", -1, resultAlien );
    }


    void checkIllegalArgnPassed( std::int32_t argIndex, Oop pointer ) {
        std::array<Oop, argCount> arg;

        for ( std::size_t index = 0; index < argCount; index++ ) {
            arg[ index ] = argIndex == index ? pointer : smi0;
        }

        Oop result = callout( arg );

        checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
    }


};


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldCallIntArgFunction ) {
    for ( std::int32_t arg = 0; arg < argCount; arg++ ) {
        checkArgnPassed( arg, -1, intCalloutFunctions );
    }
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResult1Should16ByteAlignArgs ) {
    Oop fnAddress = asOop( (std::int32_t) &argAlignment2 );
    ByteArrayPrimitives::alienSetAddress( fnAddress, functionAlien->as_oop() );
    std::array<Oop, argCount> arg;
    arg[ 1 ] = smi0;
    for ( std::int32_t size = -4; size > -20; size -= 4 ) {
        arg[ 0 ] = addressAlien->as_oop();
        ByteArrayPrimitives::alienSetSize( smiOopFromValue( size ), addressAlien->as_oop() );
        callout( arg );
        checkIntResult( "wrong result", 0, resultAlien );
    }
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldCallIntPointerArgFunction ) {
    ByteArrayPrimitives::alienSignedLongAtPut( asOop( -1 ), smi1, pointerAlien->as_oop() );
    for ( std::int32_t arg = 0; arg < argCount; arg++ ) {
        checkArgnPtrPassed( arg, pointerAlien->as_oop(), intPointerCalloutFunctions );
    }
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldCallFunctionAndIgnoreResultWhenResultAlienNil ) {
    Oop result = callout( zeroes, nilObject, functionAlien->as_oop() );
    EXPECT_TRUE( !result->isMarkOop() ) << "should not be marked";
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsWithScavengeShouldReturnCorrectResult ) {
    setAddress( functionAlien, reinterpret_cast <void *>(&forceScavengeWA) );
    checkIntResult( "incorrect initialization", 0, resultAlien );
    callout( zeroes, resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "result alien not updated", -1, resultAlien );
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultForNonAlien ) {
    Oop result = callout( zeroes, resultAlien->as_oop(), smi0 );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultForDirectAlien ) {
    Oop result = callout( zeroes, resultAlien->as_oop(), resultAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultForNullFunctionPointer ) {
    Oop result = callout( zeroes, resultAlien->as_oop(), invalidFunctionAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultWhenResultNotAlienOrNil ) {
    Oop result = callout( zeroes, trueObject, functionAlien->as_oop() );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCalloutWithArgumentsTests, alienCallResultWithArgumentsShouldReturnMarkedResultWhenFunctionParameterNotAlienOrSMI ) {
    for ( std::int32_t arg = 0; arg < argCount; arg++ ) {
        checkIllegalArgnPassed( arg, trueObject );
    }
}
