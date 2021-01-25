
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
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/compiler/Node.hpp"

#include <gtest/gtest.h>


extern "C" std::int32_t __CALLING_CONVENTION sum7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return ( a ) + ( b ) + ( c ) + ( d ) + ( e ) + ( f ) + ( g );
}

extern "C" std::int32_t __CALLING_CONVENTION returnFirst7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFirstPointer7( std::int32_t *a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return *a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecond7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return b;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecondPointer7( std::int32_t a, std::int32_t *b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return *b;
}

extern "C" std::int32_t __CALLING_CONVENTION returnThird7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return c;
}

extern "C" std::int32_t __CALLING_CONVENTION returnThirdPointer7( std::int32_t a, std::int32_t b, std::int32_t *c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return *c;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFourth7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return d;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFourthPointer7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t *d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return *d;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFifth7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return e;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFifthPointer7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t *e, std::int32_t f, std::int32_t g ) {
    return *e;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSixth7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return f;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSixthPointer7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t *f ) {
    return *f;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSeventh7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t g ) {
    return g;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSeventhPointer7( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e, std::int32_t f, std::int32_t *g ) {
    return *g;
}

extern "C" std::int32_t __CALLING_CONVENTION forceScavenge7( std::int32_t ignore1, std::int32_t ignore2, std::int32_t ignore3, std::int32_t d, std::int32_t e ) {
    Universe::scavenge();
    return -1;
}


class AlienIntegerCallout7Tests : public ::testing::Test {

protected:
    void SetUp() override {
        rm      = new HeapResourceMark();
        smi0    = smiOopFromValue( 0 );
        smi1    = smiOopFromValue( 1 );
        smim1   = smiOopFromValue( -1 );
        handles = new( true ) GrowableArray<PersistentHandle **>( 7 );

        allocateAlien( functionAlien, 8, 0, reinterpret_cast<void *>(&returnFirst7) );
        allocateAlien( resultAlien, 12, 8 );
        allocateAlien( directAlien, 12, 4 );
        allocateAlien( addressAlien, 8, -4, &address );
        allocateAlien( pointerAlien, 8, 0, &address );
        allocateAlien( invalidFunctionAlien, 8, 0 );

        memset( address, 0, 8 );

        intCalloutFunctions[ 0 ] = reinterpret_cast<void *>(returnFirst7);
        intCalloutFunctions[ 1 ] = reinterpret_cast<void *>(returnSecond7);
        intCalloutFunctions[ 2 ] = reinterpret_cast<void *>(returnThird7);
        intCalloutFunctions[ 3 ] = reinterpret_cast<void *>(returnFourth7);
        intCalloutFunctions[ 4 ] = reinterpret_cast<void *>(returnFifth7);
        intCalloutFunctions[ 5 ] = reinterpret_cast<void *>(returnSixth7);
        intCalloutFunctions[ 6 ] = reinterpret_cast<void *>(returnSeventh7);

        intPointerCalloutFunctions[ 0 ] = reinterpret_cast<void *>(returnFirstPointer7);
        intPointerCalloutFunctions[ 1 ] = reinterpret_cast<void *>(returnSecondPointer7);
        intPointerCalloutFunctions[ 2 ] = reinterpret_cast<void *>(returnThirdPointer7);
        intPointerCalloutFunctions[ 3 ] = reinterpret_cast<void *>(returnFourthPointer7);
        intPointerCalloutFunctions[ 4 ] = reinterpret_cast<void *>(returnFifthPointer7);
        intPointerCalloutFunctions[ 5 ] = reinterpret_cast<void *>(returnSixthPointer7);
        intPointerCalloutFunctions[ 6 ] = reinterpret_cast<void *>(returnSeventhPointer7);

        for ( std::int32_t index = 0; index < argCount; index++ )
            zeroes[ index ] = smi0;
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
    static const std::int32_t               argCount = 7;
    std::array<void *, argCount>   intCalloutFunctions;
    std::array<void *, argCount>   intPointerCalloutFunctions;
    std::array<Oop, argCount>      zeroes;
    char                           address[8];


    void allocateAlien( PersistentHandle *&alienHandle, std::int32_t arraySize, std::int32_t alienSize, void *ptr = nullptr ) {
        ByteArrayOop alien = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( arraySize ) );
        byteArrayPrimitives::alienSetSize( smiOopFromValue( alienSize ), alien );
        if ( ptr )
            byteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) ptr ), alien );
        alienHandle = new PersistentHandle( alien );
        handles->append( &alienHandle );
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->is_mark() ) << "Should be marked";
        sprintf( text, "%s. Should be: %s, was: %s", message, expected->as_string(), unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void checkIntResult( const char *message, std::int32_t expected, PersistentHandle *alien ) {
        char   text[200];
        bool_t ok;
        std::int32_t    actual = asInt( ok, byteArrayPrimitives::alienSignedLongAt( smi1, alien->as_oop() ) );
        EXPECT_TRUE( ok ) << "not an integer result";
        sprintf( text, "Should be: %d, was: %d", expected, actual );
        EXPECT_TRUE( actual == expected ) << text;
    }


    std::int32_t asInt( bool_t &ok, Oop intOop ) {
        ok = true;
        if ( intOop->is_smi() )
            return SMIOop( intOop )->value();
        if ( !intOop->is_byteArray() ) {
            ok = false;
            return 0;
        }
        return ByteArrayOop( intOop )->number().as_int( ok );
    }


    Oop asOop( std::int32_t value ) {
        std::int32_t          size     = IntegerOps::int_to_Integer_result_size_in_bytes( value );
        ByteArrayOop valueOop = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( size ) );
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
        byteArrayPrimitives::alienSetAddress( asOop( (std::int32_t) argument ), handle->as_oop() );
    }


    Oop callout( std::array<Oop, argCount> arg, Oop result, Oop address ) {
        return byteArrayPrimitives::alienCallResult7( arg[ 6 ], arg[ 5 ], arg[ 4 ], arg[ 3 ], arg[ 2 ], arg[ 1 ], arg[ 0 ], result, address );
    }


    Oop callout( std::array<Oop, argCount> arg ) {
        return callout( arg, resultAlien->as_oop(), functionAlien->as_oop() );
    }


    void checkArgnPassed( std::int32_t argIndex, std::int32_t argValue, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );

        std::array<Oop, argCount> arg;

        for ( std::int32_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? asOop( argValue ) : smi0;
        Oop       result = callout( arg );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", argValue, resultAlien );
    }


    void checkArgnPtrPassed( std::int32_t argIndex, Oop pointer, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );

        std::array<Oop, argCount> arg;

        for ( std::int32_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? pointer : smi0;

        Oop result = callout( arg );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", -1, resultAlien );
    }


    void checkIllegalArgnPassed( std::int32_t argIndex, Oop pointer ) {

        std::array<Oop, argCount> arg;

        for ( std::int32_t index = 0; index < argCount; index++ )
            arg[ index ] = argIndex == index ? pointer : smi0;

        Oop result = callout( arg );

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
            case 5:
                symbol = vmSymbols::seventh_argument_has_wrong_type();
                break;
            case 6:
                symbol = vmSymbols::eighth_argument_has_wrong_type();
                break;
            default:
                symbol = vmSymbols::argument_has_wrong_type();
        }
        checkMarkedSymbol( "wrong type", result, symbol );
    }


};


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldCallIntArgFunction
) {
for (
std::int32_t arg = 0;
    arg<argCount;
arg++ )
checkArgnPassed( arg,
-1, intCalloutFunctions );
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldCallSumFunction
) {
std::array<Oop, argCount> arg;

byteArrayPrimitives::alienSignedLongAtPut( asOop( -1 ), smi1, addressAlien
->
as_oop()
);
for (
std::int32_t index = 0;
    index<argCount;
index++ )
arg[ index ] = addressAlien->
as_oop();
byteArrayPrimitives::alienSetAddress( asOop( (std::int32_t) &sum7 ), functionAlien
->
as_oop()
);
callout( arg );
checkIntResult( "wrong result", -1 * argCount, resultAlien );
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldCallIntPointerArgFunction
) {
byteArrayPrimitives::alienSignedLongAtPut( asOop( -1 ), smi1, pointerAlien
->
as_oop()
);
for (
std::int32_t arg = 0;
    arg<argCount;
arg++ )
checkArgnPtrPassed( arg, pointerAlien
->
as_oop(), intPointerCalloutFunctions
);
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldCallFunctionAndIgnoreResultWhenResultAlienNil
) {
Oop result = callout( zeroes, nilObject, functionAlien->as_oop() );
EXPECT_TRUE( !result->
is_mark()
) << "should not be marked";
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7WithScavengeShouldReturnCorrectResult
) {
setAddress( functionAlien,
reinterpret_cast
<void *>(&forceScavenge7)
);
checkIntResult( "incorrect initialization", 0, resultAlien );
Oop result = callout( zeroes, resultAlien->as_oop(), functionAlien->as_oop() );
checkIntResult( "result alien not updated", -1, resultAlien );
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldReturnMarkedResultForNonAlien
) {
Oop result = callout( zeroes, resultAlien->as_oop(), smi0 );

checkMarkedSymbol( "wrong type", result,
vmSymbols::receiver_has_wrong_type()
);
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldReturnMarkedResultForDirectAlien
) {
Oop result = callout( zeroes, resultAlien->as_oop(), resultAlien->as_oop() );

checkMarkedSymbol( "illegal state", result,
vmSymbols::illegal_state()
);
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldReturnMarkedResultForNullFunctionPointer
) {
Oop result = callout( zeroes, resultAlien->as_oop(), invalidFunctionAlien->as_oop() );

checkMarkedSymbol( "illegal state", result,
vmSymbols::illegal_state()
);
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldReturnMarkedResultWhenResultNotAlienOrNil
) {
Oop result = callout( zeroes, trueObject, functionAlien->as_oop() );

checkMarkedSymbol( "wrong type", result,
vmSymbols::first_argument_has_wrong_type()
);
}


TEST_F( AlienIntegerCallout7Tests, alienCallResult7ShouldReturnMarkedResultWhenFunctionParameterNotAlienOrSMI
) {
for (
std::int32_t arg = 0;
    arg<argCount;
arg++ )
checkIllegalArgnPassed( arg, trueObject
);
}
