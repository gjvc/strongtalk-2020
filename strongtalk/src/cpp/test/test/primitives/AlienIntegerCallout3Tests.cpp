
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
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


extern "C" int __CALLING_CONVENTION returnFirst3( int a, int b, int c ) {
    return a;
}

extern "C" int __CALLING_CONVENTION returnFirstPointer3( int * a, int b, int c ) {
    return *a;
}

extern "C" int __CALLING_CONVENTION returnSecond3( int a, int b, int c ) {
    return b;
}

extern "C" int __CALLING_CONVENTION returnSecondPointer3( int a, int * b, int c ) {
    return *b;
}

extern "C" int __CALLING_CONVENTION returnThird3( int a, int b, int c ) {
    return c;
}

extern "C" int __CALLING_CONVENTION returnThirdPointer3( int a, int b, int * c ) {
    return *c;
}

extern "C" int __CALLING_CONVENTION forceScavenge3( int ignore1, int ignore2, int ignore3 ) {
    Universe::scavenge();
    return -1;
}


class AlienIntegerCallout3Tests : public ::testing::Test {

    protected:
        void SetUp() override {
            rm      = new HeapResourceMark();
            smi0    = smiOopFromValue( 0 );
            smi1    = smiOopFromValue( 1 );
            smim1   = smiOopFromValue( -1 );
            handles = new( true ) GrowableArray <PersistentHandle **>( 5 );

            allocateAlien( functionAlien, 8, 0, reinterpret_cast<void *>(&returnFirst3) );
            allocateAlien( resultAlien, 12, 8 );
            allocateAlien( directAlien, 12, 4 );
            allocateAlien( addressAlien, 8, -4, &address );
            allocateAlien( pointerAlien, 8, 0, &address );
            allocateAlien( invalidFunctionAlien, 8, 0 );

            memset( address, 0, 8 );

            intCalloutFunctions[ 0 ]        = reinterpret_cast<void *>(returnFirst3);
            intCalloutFunctions[ 1 ]        = reinterpret_cast<void *>(returnSecond3);
            intCalloutFunctions[ 2 ]        = reinterpret_cast<void *>(returnThird3);
            intPointerCalloutFunctions[ 0 ] = reinterpret_cast<void *>(returnFirstPointer3);
            intPointerCalloutFunctions[ 1 ] = reinterpret_cast<void *>(returnSecondPointer3);
            intPointerCalloutFunctions[ 2 ] = reinterpret_cast<void *>(returnThirdPointer3);
        }


        void TearDown() override {
            while ( !handles->isEmpty() )
                release( handles->pop() );
            free( handles );
            handles = nullptr;
            delete rm;
            rm = nullptr;
        }


        HeapResourceMark                    * rm;
        GrowableArray <PersistentHandle **> * handles;
        PersistentHandle                    * resultAlien, * addressAlien, * pointerAlien, * functionAlien;
        PersistentHandle                    * directAlien, * invalidFunctionAlien;
        SMIOop                                             smi0, smi1, smim1;
        static const int                                   argCount = 3;
        void                                * intCalloutFunctions[argCount];
        void                                * intPointerCalloutFunctions[argCount];
        char                                address[8];


        void allocateAlien( PersistentHandle *& alienHandle, int arraySize, int alienSize, void * ptr = nullptr ) {
            ByteArrayOop alien = ByteArrayOop( Universe::byteArrayKlassObj()->klass_part()->allocateObjectSize( arraySize ) );
            byteArrayPrimitives::alienSetSize( smiOopFromValue( alienSize ), alien );
            if ( ptr )
                byteArrayPrimitives::alienSetAddress( smiOopFromValue( ( int ) ptr ), alien );
            alienHandle = new PersistentHandle( alien );
            handles->append( &alienHandle );
        }


        void checkMarkedSymbol( const char * message, Oop result, SymbolOop expected ) {
            char text[200];
            EXPECT_TRUE( result->is_mark() ) << "Should be marked";
            sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
            EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
        }


        void checkIntResult( const char * message, int expected, PersistentHandle * alien ) {
            char   text[200];
            bool_t ok;
            int    actual = asInt( ok, byteArrayPrimitives::alienSignedLongAt( smi1, alien->as_oop() ) );
            EXPECT_TRUE( ok ) << "not an integer result";
            sprintf( text, "Should be: %d, was: %d", expected, actual );
            EXPECT_TRUE( actual == expected ) << text;
        }


        int asInt( bool_t & ok, Oop intOop ) {
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


        void release( PersistentHandle ** handle ) {
            delete *handle;
            *handle = nullptr;
        }


        void setAddress( PersistentHandle * handle, void * argument ) {
            byteArrayPrimitives::alienSetAddress( asOop( ( int ) argument ), handle->as_oop() );
        }


        void checkArgnPassed( int argIndex, int argValue, void ** functionArray ) {
            setAddress( functionAlien, functionArray[ argIndex ] );
            Oop arg0   = argIndex == 0 ? asOop( argValue ) : smi0;
            Oop arg1   = argIndex == 1 ? asOop( argValue ) : smi0;
            Oop arg2   = argIndex == 2 ? asOop( argValue ) : smi0;
            Oop result = byteArrayPrimitives::alienCallResult3( arg2, arg1, arg0, resultAlien->as_oop(), functionAlien->as_oop() );

            EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
            checkIntResult( "wrong result", argValue, resultAlien );
        }


        void checkArgnPtrPassed( int argIndex, int argValue, void ** functionArray ) {
            setAddress( functionAlien, functionArray[ argIndex ] );
            byteArrayPrimitives::alienSignedLongAtPut( asOop( argValue ), smi1, pointerAlien->as_oop() );
            Oop arg0   = argIndex == 0 ? pointerAlien->as_oop() : smi0;
            Oop arg1   = argIndex == 1 ? pointerAlien->as_oop() : smi0;
            Oop arg2   = argIndex == 2 ? pointerAlien->as_oop() : smi0;
            Oop result = byteArrayPrimitives::alienCallResult3( arg2, arg1, arg0, resultAlien->as_oop(), functionAlien->as_oop() );

            EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
            checkIntResult( "wrong result", argValue, resultAlien );
        }

};


TEST_F   ( AlienIntegerCallout3Tests, alienCallResult3ShouldCallIntArgFunction ) {
    for ( int arg = 0; arg < argCount; arg++ )
        checkArgnPassed( arg, -1, intCalloutFunctions );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldCallIntPointerArgFunction ) {
    for ( int arg = 0; arg < argCount; arg++ )
        checkArgnPtrPassed( arg, -1, intPointerCalloutFunctions );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldCallFunctionAndIgnoreResultWhenResultAlienNil ) {
    Oop result = byteArrayPrimitives::alienCallResult3( smi0, smi0, smim1, nilObj, functionAlien->as_oop() );
    EXPECT_TRUE( !result->is_mark() ) << "should not be marked";
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3WithScavengeShouldReturnCorrectResult ) {
    setAddress( functionAlien, reinterpret_cast
        <void *>(&forceScavenge3) );
    checkIntResult( "incorrect initialization", 0, resultAlien );
    byteArrayPrimitives::alienCallResult3( smi0, smi0, smi0, resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "result alien not updated", -1, resultAlien );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldReturnMarkedResultForNonAlien ) {
    Oop result = byteArrayPrimitives::alienCallResult3( smi0, smi0, smi0, resultAlien->as_oop(), smi0 );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldReturnMarkedResultForDirectAlien ) {
    Oop result = byteArrayPrimitives::alienCallResult3( smi0, smi0, smi0, resultAlien->as_oop(), resultAlien->as_oop() );

    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldReturnMarkedResultForNullFunctionPointer ) {
    Oop result = byteArrayPrimitives::alienCallResult3( smi0, smi0, smi0, resultAlien->as_oop(), invalidFunctionAlien->as_oop() );

    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldReturnMarkedResultWhenResultNotAlienOrNil ) {
    Oop result = byteArrayPrimitives::alienCallResult3( smi0, smi0, smi0, trueObj, functionAlien->as_oop() );

    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldReturnMarkedResultWhenFunctionParameter1NotAlienOrSMI ) {
    Oop result = byteArrayPrimitives::alienCallResult3( smi0, smi0, trueObj, resultAlien->as_oop(), functionAlien->as_oop() );

    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldReturnMarkedResultWhenFunctionParameter2NotAlienOrSMI ) {
    Oop result = byteArrayPrimitives::alienCallResult3( smi0, trueObj, smi0, resultAlien->as_oop(), functionAlien->as_oop() );

    checkMarkedSymbol( "wrong type", result, vmSymbols::third_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout3Tests, alienCallResult3ShouldReturnMarkedResultWhenFunctionParameter3NotAlienOrSMI ) {
    Oop result = byteArrayPrimitives::alienCallResult3( trueObj, smi0, smi0, resultAlien->as_oop(), functionAlien->as_oop() );

    checkMarkedSymbol( "wrong type", result, vmSymbols::fourth_argument_has_wrong_type() );
}
