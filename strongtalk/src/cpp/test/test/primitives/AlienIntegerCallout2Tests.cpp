//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/utility/Integer.hpp"
#include "vm/utility/IntegerOps.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/compiler/Node.hpp"

#include <gtest/gtest.h>


extern "C" std::int32_t __CALLING_CONVENTION returnFirst( std::int32_t a, std::int32_t b ) {
    // a
    st_unused( b );
    return a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnFirstPointer( std::int32_t *a, std::int32_t b ) {
    // a
    st_unused( b );
    return *a;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecond( std::int32_t a, std::int32_t b ) {
    st_unused( a );
    // b
    return b;
}

extern "C" std::int32_t __CALLING_CONVENTION returnSecondPointer( std::int32_t a, std::int32_t *b ) {
    st_unused( a );
    // b
    return *b;
}

extern "C" std::int32_t __CALLING_CONVENTION forceScavenge2( std::int32_t ignore1, std::int32_t ignore2 ) {
    st_unused( ignore1 );
    st_unused( ignore2 );
    Universe::scavenge();
    return -1;
}

//extern "C" std::int32_t __CALLING_CONVENTION size5(std::int32_t ignore, char byte) {
//  return byte == -1 ? 0 : -1;
//}
//
//extern "C" Oop __CALLING_CONVENTION forceScavenge(std::int32_t ignore) {
//  Universe::scavenge();
//  return vmSymbols::completed();
//}


class AlienIntegerCallout2Tests : public ::testing::Test {
public:
    AlienIntegerCallout2Tests() :
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

        allocateAlien( functionAlien, 8, 0, reinterpret_cast<void *>(&returnFirst) );
        allocateAlien( resultAlien, 12, 8 );
        allocateAlien( directAlien, 12, 4 );
        allocateAlien( addressAlien, 8, -4, &address );
        allocateAlien( pointerAlien, 8, 0, &address );
        allocateAlien( invalidFunctionAlien, 8, 0 );

        memset( address, 0, 8 );

        intCalloutFunctions[ 0 ]        = reinterpret_cast<void *>(returnFirst);
        intCalloutFunctions[ 1 ]        = reinterpret_cast<void *>(returnSecond);
        intPointerCalloutFunctions[ 0 ] = reinterpret_cast<void *>(returnFirstPointer);
        intPointerCalloutFunctions[ 1 ] = reinterpret_cast<void *>(returnSecondPointer);
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
    PersistentHandle                   *directAlien, *invalidFunctionAlien;
    SmallIntegerOop                    smi0, smi1, smim1;
    static constexpr std::int32_t      argCount = 2;
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
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void checkIntResult( const char *message, std::int32_t expected, PersistentHandle *alien ) {
        st_unused( message );
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
        std::int32_t size     = IntegerOps::int_to_Integer_result_size_in_bytes( value );
        ByteArrayOop valueOop = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( size ) );
        IntegerOps::int_to_Integer( value, valueOop->number() );
        bool ok;
        Oop  result           = valueOop->number().as_SmallIntegerOop( ok );
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
        Oop arg0   = argIndex == 0 ? asOop( argValue ) : smi0;
        Oop arg1   = argIndex == 1 ? asOop( argValue ) : smi0;
        Oop result = ByteArrayPrimitives::alienCallResult2( arg1, arg0, resultAlien->as_oop(), functionAlien->as_oop() );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", argValue, resultAlien );
    }


    void checkArgnPtrPassed( std::int32_t argIndex, std::int32_t argValue, std::array<void *, argCount> functionArray ) {
        setAddress( functionAlien, functionArray[ argIndex ] );
        ByteArrayPrimitives::alienSignedLongAtPut( asOop( argValue ), smi1, pointerAlien->as_oop() );
        Oop arg0   = argIndex == 0 ? pointerAlien->as_oop() : smi0;
        Oop arg1   = argIndex == 1 ? pointerAlien->as_oop() : smi0;
        Oop result = ByteArrayPrimitives::alienCallResult2( arg1, arg0, resultAlien->as_oop(), functionAlien->as_oop() );

        EXPECT_TRUE( result == resultAlien->as_oop() ) << "Should return result alien";
        checkIntResult( "wrong result", argValue, resultAlien );
    }

};

TEST_F   ( AlienIntegerCallout2Tests, alienCallResult2ShouldCallIntArgFunction ) { for ( std::int32_t arg = 0; arg < argCount; arg++ )checkArgnPassed( arg, -1, intCalloutFunctions ); }


TEST_F( AlienIntegerCallout2Tests, alienCallResult2ShouldCallIntPointerArgFunctionWithArg2 ) { for ( std::int32_t arg = 0; arg < argCount; arg++ )checkArgnPtrPassed( arg, -1, intPointerCalloutFunctions ); }


TEST_F( AlienIntegerCallout2Tests, alienCallResult2ShouldCallFunctionAndIgnoreResultWhenResultAlienNil ) {
    Oop result = ByteArrayPrimitives::alienCallResult2( smi0, smim1, nilObject, functionAlien->as_oop() );
    EXPECT_TRUE( !result->isMarkOop() ) << "should not be marked";
}


TEST_F( AlienIntegerCallout2Tests, alienCallResult2WithScavengeShouldReturnCorrectResult ) {
    setAddress( functionAlien, reinterpret_cast <void *>(&forceScavenge2) );
    checkIntResult( "incorrect initialization", 0, resultAlien );
    ByteArrayPrimitives::alienCallResult2( smi0, smi0, resultAlien->as_oop(), functionAlien->as_oop() );
    checkIntResult( "result alien not updated", -1, resultAlien );
}


TEST_F( AlienIntegerCallout2Tests, alienCallResult2ShouldReturnMarkedResultForNonAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult2( smi0, smi0, resultAlien->as_oop(), smi0 );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( AlienIntegerCallout2Tests, alienCallResult2ShouldReturnMarkedResultForDirectAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult2( smi0, smi0, resultAlien->as_oop(), resultAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout2Tests, alienCallResult2ShouldReturnMarkedResultForNullFunctionPointer ) {
    Oop result = ByteArrayPrimitives::alienCallResult2( smi0, smi0, resultAlien->as_oop(), invalidFunctionAlien->as_oop() );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout2Tests, alienCallResult2ShouldReturnMarkedResultWhenResultNotAlienOrNil ) {
    Oop result = ByteArrayPrimitives::alienCallResult2( smi0, smi0, trueObject, functionAlien->as_oop() );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout2Tests, alienCallResult2ShouldReturnMarkedResultWhenFunctionParameter1NotAlienOrSMI ) {
    Oop result = ByteArrayPrimitives::alienCallResult2( smi0, trueObject, resultAlien->as_oop(), functionAlien->as_oop() );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout2Tests, alienCallResult2ShouldReturnMarkedResultWhenFunctionParameter2NotAlienOrSMI ) {
    Oop result = ByteArrayPrimitives::alienCallResult2( trueObject, smi0, resultAlien->as_oop(), functionAlien->as_oop() );
    checkMarkedSymbol( "wrong type", result, vmSymbols::third_argument_has_wrong_type() );
}
