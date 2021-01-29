
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


extern "C" std::int32_t __CALLING_CONVENTION returnFirst4( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d ) {
    return a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFirstPointer4( std::int32_t *a, std::int32_t b, std::int32_t c, std::int32_t d ) {
    return *a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecond4( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d ) {
    return b;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecondPointer4( std::int32_t a, std::int32_t *b, std::int32_t c, std::int32_t d ) {
    return *b;
}

extern "C" std::int32_t __CALLING_CONVENTION returnThird4( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d ) {
    return c;
}

extern "C" std::int32_t __CALLING_CONVENTION returnThirdPointer4( std::int32_t a, std::int32_t b, std::int32_t *c, std::int32_t d ) {
    return *c;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFourth4( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d ) {
    return d;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFourthPointer4( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t *d ) {
    return *d;
}

extern "C" std::int32_t __CALLING_CONVENTION forceScavenge4( std::int32_t ignore1, std::int32_t ignore2, std::int32_t ignore3, std::int32_t d ) {
    Universe::scavenge();
    return -1;
}


class AlienIntegerCallout4Tests : public ::testing::Test {

protected:
    void SetUp() override {
        rm      = new HeapResourceMark();
        smi0    = smiOopFromValue( 0 );
        smi1    = smiOopFromValue( 1 );
        smim1   = smiOopFromValue( -1 );
        handles = new( true ) GrowableArray<PersistentHandle **>( 5 );

        allocateAlien( functionAlien, 8, 0, reinterpret_cast<void *>(&returnFirst4) );
        allocateAlien( resultAlien, 12, 8 );
        allocateAlien( directAlien, 12, 4 );
        allocateAlien( addressAlien, 8, -4, &address );
        allocateAlien( pointerAlien, 8, 0, &address );
        allocateAlien( invalidFunctionAlien, 8, 0 );

        memset( address, 0, 8 );

        intCalloutFunctions[ 0 ]        = reinterpret_cast<void *>(returnFirst4);
        intCalloutFunctions[ 1 ]        = reinterpret_cast<void *>(returnSecond4);
        intCalloutFunctions[ 2 ]        = reinterpret_cast<void *>(returnThird4);
        intCalloutFunctions[ 3 ]        = reinterpret_cast<void *>(returnFourth4);
        intPointerCalloutFunctions[ 0 ] = reinterpret_cast<void *>(returnFirstPointer4);
        intPointerCalloutFunctions[ 1 ] = reinterpret_cast<void *>(returnSecondPointer4);
        intPointerCalloutFunctions[ 2 ] = reinterpret_cast<void *>(returnThirdPointer4);
        intPointerCalloutFunctions[ 3 ] = reinterpret_cast<void *>(returnFourthPointer4);
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
    static const std::int32_t               argCount = 4;
    std::array<void *, argCount>   intCalloutFunctions;
    std::array<void *, argCount>   intPointerCalloutFunctions;
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
        bool ok;
        std::int32_t    actual = asInt( ok, byteArrayPrimitives::alienSignedLongAt( smi1, alien->as_oop() ) );
        EXPECT_TRUE( ok ) << "not an integer result";
        sprintf( text, "Should be: %d, was: %d", expected, actual );
        EXPECT_TRUE( actual == expected ) << text;
    }


    std::int32_t asInt( bool &ok, Oop intOop ) {
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
        bool ok;
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


    void checkArgnPassed( std::int32_t argIndex, std::int32_t argValue, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        Oop arg0   = argIndex == 0 ? asOop( argValue ) : smi0;
        Oop arg1   = argIndex == 1 ? asOop( argValue ) : smi0;
        Oop arg2   = argIndex == 2 ? asOop( argValue ) : smi0;
        Oop arg3   = argIndex == 3 ? asOop( argValue ) : smi0;
        Oop result = byteArrayPrimitives::alienCallResult4( arg3, arg2, arg1, arg0, resultAlien->as_oop(), functionAlien->as_oop() );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", argValue, resultAlien );
    }


    void checkArgnPtrPassed( std::int32_t argIndex, Oop pointer, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        Oop arg0   = argIndex == 0 ? pointer : smi0;
        Oop arg1   = argIndex == 1 ? pointer : smi0;
        Oop arg2   = argIndex == 2 ? pointer : smi0;
        Oop arg3   = argIndex == 3 ? pointer : smi0;
        Oop result = byteArrayPrimitives::alienCallResult4( arg3, arg2, arg1, arg0, resultAlien->as_oop(), functionAlien->as_oop() );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", -1, resultAlien );
    }


    void checkIllegalArgnPassed( std::int32_t argIndex, Oop pointer ) {
        Oop arg0   = argIndex == 0 ? pointer : smi0;
        Oop arg1   = argIndex == 1 ? pointer : smi0;
        Oop arg2   = argIndex == 2 ? pointer : smi0;
        Oop arg3   = argIndex == 3 ? pointer : smi0;
        Oop result = byteArrayPrimitives::alienCallResult4( arg3, arg2, arg1, arg0, resultAlien->as_oop(), functionAlien->as_oop() );

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
            default:
                symbol = vmSymbols::argument_has_wrong_type();
        }
        checkMarkedSymbol( "wrong type", result, symbol );
    }


};

TEST_F( AlienIntegerCallout4Tests, alienCallResult4ShouldCallIntArgFunction ) { for ( std::int32_t arg = 0; arg < argCount; arg++ )checkArgnPassed( arg, -1, intCalloutFunctions ); }


TEST_F( AlienIntegerCallout4Tests, alienCallResult4ShouldCallIntPointerArgFunction ) {
    byteArrayPrimitives::alienSignedLongAtPut( asOop( -1 ), smi1, pointerAlien->as_oop() );
    for ( std::int32_t arg = 0; arg < argCount; arg++ )checkArgnPtrPassed( arg, pointerAlien->as_oop(), intPointerCalloutFunctions );
}


TEST_F( AlienIntegerCallout4Tests, alienCallResult4ShouldCallFunctionAndIgnoreResultWhenResultAlienNil ) {
    Oop result = byteArrayPrimitives::alienCallResult4( smi0, smi0, smi0, smim1, nilObject, functionAlien->as_oop() );
    EXPECT_TRUE( !result->is_mark() ) << "should not be marked";
}


TEST_F( AlienIntegerCallout4Tests, alienCallResult4WithScavengeShouldReturnCorrectResult ) {
    setAddress( functionAlien, reinterpret_cast <void *>(&forceScavenge4) );
    checkIntResult( "incorrect initialization", 0, resultAlien );
    byteArrayPrimitives::alienCallResult4( smi0, smi0, smi0, smi0, resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "result alien not updated", -1, resultAlien );
}


TEST_F( AlienIntegerCallout4Tests, alienCallResult4ShouldReturnMarkedResultForNonAlien ) {
    Oop result = byteArrayPrimitives::alienCallResult4( smi0, smi0, smi0, smi0, resultAlien->as_oop(), smi0 );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( AlienIntegerCallout4Tests, alienCallResult4ShouldReturnMarkedResultForDirectAlien ) {
    Oop result = byteArrayPrimitives::alienCallResult4( smi0, smi0, smi0, smi0, resultAlien->as_oop(), resultAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout4Tests, alienCallResult4ShouldReturnMarkedResultForNullFunctionPointer ) {
    Oop result = byteArrayPrimitives::alienCallResult4( smi0, smi0, smi0, smi0, resultAlien->as_oop(), invalidFunctionAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout4Tests, alienCallResult4ShouldReturnMarkedResultWhenResultNotAlienOrNil ) {
    Oop result = byteArrayPrimitives::alienCallResult4( smi0, smi0, smi0, smi0, trueObject, functionAlien->as_oop() );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout4Tests, alienCallResult4ShouldReturnMarkedResultWhenFunctionParameterNotAlienOrSMI ) { for ( std::int32_t arg = 0; arg < argCount; arg++ )checkIllegalArgnPassed( arg, trueObject ); }
