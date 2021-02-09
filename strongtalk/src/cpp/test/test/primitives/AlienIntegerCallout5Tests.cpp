//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/primitives/ByteArrayPrimitives.hpp"
#include "vm/compiler/Node.hpp"

#include <gtest/gtest.h>


extern "C" std::int32_t __CALLING_CONVENTION returnFirst5( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e ) {
    // a
    static_cast<void>(b); // unused
    static_cast<void>(c); // unused
    static_cast<void>(d); // unused
    static_cast<void>(e); // unused
    return a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFirstPointer5( std::int32_t *a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e ) {
    // a
    static_cast<void>(b); // unused
    static_cast<void>(c); // unused
    static_cast<void>(d); // unused
    static_cast<void>(e); // unused
    return *a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecond5( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e ) {
    static_cast<void>(a); // unused
    // b
    static_cast<void>(c); // unused
    static_cast<void>(d); // unused
    static_cast<void>(e); // unused
    return b;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecondPointer5( std::int32_t a, std::int32_t *b, std::int32_t c, std::int32_t d, std::int32_t e ) {
    static_cast<void>(a); // unused
    // b
    static_cast<void>(c); // unused
    static_cast<void>(d); // unused
    static_cast<void>(e); // unused
    return *b;
}

extern "C" std::int32_t __CALLING_CONVENTION returnThird5( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e ) {
    static_cast<void>(a); // unused
    static_cast<void>(b); // unused
    // c
    static_cast<void>(d); // unused
    static_cast<void>(e); // unused
    return c;
}

extern "C" std::int32_t __CALLING_CONVENTION returnThirdPointer5( std::int32_t a, std::int32_t b, std::int32_t *c, std::int32_t d, std::int32_t e ) {
    static_cast<void>(a); // unused
    static_cast<void>(b); // unused
    // c
    static_cast<void>(d); // unused
    static_cast<void>(e); // unused
    return *c;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFourth5( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e ) {
    static_cast<void>(a); // unused
    static_cast<void>(b); // unused
    static_cast<void>(c); // unused
    // d
    static_cast<void>(e); // unused
    return d;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFourthPointer5( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t *d, std::int32_t e ) {
    static_cast<void>(a); // unused
    static_cast<void>(b); // unused
    static_cast<void>(c); // unused
    // d
    static_cast<void>(e); // unused
    return *d;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFifth5( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e ) {
    static_cast<void>(a); // unused
    static_cast<void>(b); // unused
    static_cast<void>(c); // unused
    static_cast<void>(d); // unused
    // e
    return e;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFifthPointer5( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t *e ) {
    static_cast<void>(a); // unused
    static_cast<void>(b); // unused
    static_cast<void>(c); // unused
    static_cast<void>(d); // unused
    // e
    return *e;
}

extern "C" std::int32_t __CALLING_CONVENTION forceScavenge5( std::int32_t ignore1, std::int32_t ignore2, std::int32_t ignore3, std::int32_t d, std::int32_t e ) {
    static_cast<void>(ignore1); // unused
    static_cast<void>(ignore2); // unused
    static_cast<void>(ignore3); // unused
    static_cast<void>(d); // unused
    static_cast<void>(e); // unused
    Universe::scavenge();
    return -1;
}


class AlienIntegerCallout5Tests : public ::testing::Test {

public:
    AlienIntegerCallout5Tests() : ::testing::Test() {}


protected:
    void SetUp() override {
        rm      = new HeapResourceMark();
        smi0    = smiOopFromValue( 0 );
        smi1    = smiOopFromValue( 1 );
        smim1   = smiOopFromValue( -1 );
        handles = new( true ) GrowableArray<PersistentHandle **>( 5 );

        allocateAlien( functionAlien, 8, 0, reinterpret_cast<void *>(&returnFirst5) );
        allocateAlien( resultAlien, 12, 8 );
        allocateAlien( directAlien, 12, 4 );
        allocateAlien( addressAlien, 8, -4, &address );
        allocateAlien( pointerAlien, 8, 0, &address );
        allocateAlien( invalidFunctionAlien, 8, 0 );

        memset( address, 0, 8 );

        intCalloutFunctions[ 0 ]        = reinterpret_cast<void *>(returnFirst5);
        intCalloutFunctions[ 1 ]        = reinterpret_cast<void *>(returnSecond5);
        intCalloutFunctions[ 2 ]        = reinterpret_cast<void *>(returnThird5);
        intCalloutFunctions[ 3 ]        = reinterpret_cast<void *>(returnFourth5);
        intCalloutFunctions[ 4 ]        = reinterpret_cast<void *>(returnFifth5);
        intPointerCalloutFunctions[ 0 ] = reinterpret_cast<void *>(returnFirstPointer5);
        intPointerCalloutFunctions[ 1 ] = reinterpret_cast<void *>(returnSecondPointer5);
        intPointerCalloutFunctions[ 2 ] = reinterpret_cast<void *>(returnThirdPointer5);
        intPointerCalloutFunctions[ 3 ] = reinterpret_cast<void *>(returnFourthPointer5);
        intPointerCalloutFunctions[ 4 ] = reinterpret_cast<void *>(returnFifthPointer5);
    }


    void TearDown() override {
        while ( !handles->isEmpty() )
            release( handles->pop() );
        free( handles );
        handles = nullptr;
        delete rm;
        rm = nullptr;
    }


    HeapResourceMark                   *rm;
    GrowableArray<PersistentHandle **> *handles;
    PersistentHandle                   *resultAlien, *addressAlien, *pointerAlien, *functionAlien;
    PersistentHandle          *directAlien, *invalidFunctionAlien;
    SmallIntegerOop           smi0, smi1, smim1;
    static const std::int32_t argCount = 5;
    std::array<void *, argCount>       intCalloutFunctions;
    std::array<void *, argCount>       intPointerCalloutFunctions;
    char                               address[8];


    void allocateAlien( PersistentHandle *&alienHandle, std::int32_t arraySize, std::int32_t alienSize, void *ptr = nullptr ) {
        ByteArrayOop alien = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( arraySize ) );
        ByteArrayPrimitives::alienSetSize( smiOopFromValue( alienSize ), alien );
        if ( ptr )
            ByteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) ptr ), alien );
        alienHandle = new PersistentHandle( alien );
        handles->append( &alienHandle );
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->isMarkOop() ) << "Should be marked";
        sprintf( text, "%s. Should be: %s, was: %s", message, expected->as_string(), unmarkSymbol( result )->as_string() );
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
        if ( intOop->isSmallIntegerOop() )
            return SmallIntegerOop( intOop )->value();
        if ( !intOop->isByteArray() ) {
            ok = false;
            return 0;
        }
        return ByteArrayOop( intOop )->number().as_int32_t( ok );
    }


    Oop asOop( std::int32_t value ) {
        std::int32_t size = IntegerOps::int_to_Integer_result_size_in_bytes( value );

        ByteArrayOop valueOop = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( size ) );
        IntegerOps::int_to_Integer( value, valueOop->number() );
        bool ok;

        Oop result = valueOop->number().as_smi( ok );
        return ok ? result : valueOop;
    }


    void release( PersistentHandle **handle ) {
        delete *handle;
        *handle = nullptr;
    }


    void setAddress( PersistentHandle *handle, void *argument ) {
        ByteArrayPrimitives::alienSetAddress( asOop( (std::int32_t) argument ), handle->as_oop() );
    }


    void checkArgnPassed( std::int32_t argIndex, std::int32_t argValue, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        std::array<Oop, argCount> arg;

        for ( std::int32_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? asOop( argValue ) : smi0;

        Oop result = ByteArrayPrimitives::alienCallResult5( arg[ 4 ], arg[ 3 ], arg[ 2 ], arg[ 1 ], arg[ 0 ], resultAlien->as_oop(), functionAlien->as_oop() );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", argValue, resultAlien );
    }


    void checkArgnPtrPassed( std::int32_t argIndex, Oop pointer, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        std::array<Oop, argCount> arg;

        for ( std::int32_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? pointer : smi0;

        Oop result = ByteArrayPrimitives::alienCallResult5( arg[ 4 ], arg[ 3 ], arg[ 2 ], arg[ 1 ], arg[ 0 ], resultAlien->as_oop(), functionAlien->as_oop() );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", -1, resultAlien );
    }


    void checkIllegalArgnPassed( std::int32_t argIndex, Oop pointer ) {

        std::array<Oop, argCount> arg;

        for ( std::int32_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? pointer : smi0;

        Oop result = ByteArrayPrimitives::alienCallResult5( arg[ 4 ], arg[ 3 ], arg[ 2 ], arg[ 1 ], arg[ 0 ], resultAlien->as_oop(), functionAlien->as_oop() );

        SymbolOop symbol;
        switch ( argIndex ) {
            case 0:
                symbol = vmSymbols::second_argument_has_wrong_type();
                break;
            case 1:
                symbol = vmSymbols::third_argument_has_wrong_type();
                break;
            case 2:
                symbol = vmSymbols::fourth_argument_has_wrong_type();
                break;
            case 3:
                symbol = vmSymbols::fifth_argument_has_wrong_type();
                break;
            case 4:
                symbol = vmSymbols::sixth_argument_has_wrong_type();
                break;
            default:
                symbol = vmSymbols::argument_has_wrong_type();
        }
        checkMarkedSymbol( "wrong type", result, symbol );
    }

};

TEST_F   ( AlienIntegerCallout5Tests, alienCallResult5ShouldCallIntArgFunction ) { for ( std::int32_t arg = 0; arg < argCount; arg++ )checkArgnPassed( arg, -1, intCalloutFunctions ); }


TEST_F( AlienIntegerCallout5Tests, alienCallResult5ShouldCallIntPointerArgFunction ) {
    ByteArrayPrimitives::alienSignedLongAtPut( asOop( -1 ), smi1, pointerAlien->as_oop() );
    for ( std::int32_t arg = 0; arg < argCount; arg++ )checkArgnPtrPassed( arg, pointerAlien->as_oop(), intPointerCalloutFunctions );
}


TEST_F( AlienIntegerCallout5Tests, alienCallResult5ShouldCallFunctionAndIgnoreResultWhenResultAlienNil ) {
    Oop result = ByteArrayPrimitives::alienCallResult5( smi0, smi0, smi0, smi0, smim1, nilObject, functionAlien->as_oop() );
    EXPECT_TRUE( !result->isMarkOop() ) << "should not be marked";
}


TEST_F( AlienIntegerCallout5Tests, alienCallResult5WithScavengeShouldReturnCorrectResult ) {
    setAddress( functionAlien, reinterpret_cast <void *>(&forceScavenge5) );
    checkIntResult( "incorrect initialization", 0, resultAlien );
    ByteArrayPrimitives::alienCallResult5( smi0, smi0, smi0, smi0, smi0, resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "result alien not updated", -1, resultAlien );
}


TEST_F( AlienIntegerCallout5Tests, alienCallResult5ShouldReturnMarkedResultForNonAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult5( smi0, smi0, smi0, smi0, smi0, resultAlien->as_oop(), smi0 );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( AlienIntegerCallout5Tests, alienCallResult5ShouldReturnMarkedResultForDirectAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult5( smi0, smi0, smi0, smi0, smi0, resultAlien->as_oop(), resultAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout5Tests, alienCallResult5ShouldReturnMarkedResultForNullFunctionPointer ) {
    Oop result = ByteArrayPrimitives::alienCallResult5( smi0, smi0, smi0, smi0, smi0, resultAlien->as_oop(), invalidFunctionAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout5Tests, alienCallResult5ShouldReturnMarkedResultWhenResultNotAlienOrNil ) {
    Oop result = ByteArrayPrimitives::alienCallResult5( smi0, smi0, smi0, smi0, smi0, trueObject, functionAlien->as_oop() );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout5Tests, alienCallResult5ShouldReturnMarkedResultWhenFunctionParameterNotAlienOrSMI ) {
    for ( std::int32_t arg = 0; arg < argCount; arg++ ) {
        checkIllegalArgnPassed( arg, trueObject );
    }
}
