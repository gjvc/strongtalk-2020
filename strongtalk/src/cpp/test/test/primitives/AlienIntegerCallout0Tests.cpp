//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/utility/Integer.hpp"
#include "vm/utility/IntegerOps.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/compiler/Node.hpp"

#include <ctime>
#include <gtest/gtest.h>


class AlienIntegerCallout0Tests : public ::testing::Test {

public:
    AlienIntegerCallout0Tests() :
        ::testing::Test(),
        rm{ nullptr },
        fnAlien{},
        invalidFunctionAlien{},
        resultAlien{},
        addressAlien{},
        pointerAlien{},
        argumentAlien{},
        smi0{},
        smi1{},
        address{} {}


protected:
    void SetUp() override {
        rm   = new HeapResourceMark();
        smi0 = smiOopFromValue( 0 );
        smi1 = smiOopFromValue( 1 );

        PersistentHandle ca( allocateAlien( 8, 0 ) );
        PersistentHandle ifa( allocateAlien( 8, 0 ) );
        PersistentHandle ra( allocateAlien( 12, 8 ) );
        PersistentHandle aa( allocateAlien( 8, -8 ) );
        PersistentHandle pa( allocateAlien( 8, 0 ) );

        resultAlien          = ByteArrayOop( ra.as_oop() );
        invalidFunctionAlien = ByteArrayOop( ifa.as_oop() );
        ByteArrayPrimitives::alienSetAddress( smi0, invalidFunctionAlien );

        fnAlien = ByteArrayOop( ca.as_oop() );
        ByteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) &clock ), fnAlien );

        addressAlien = ByteArrayOop( aa.as_oop() );
        ByteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) &address ), addressAlien );

        pointerAlien = ByteArrayOop( pa.as_oop() );
        ByteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) &address ), pointerAlien );
    }


    void TearDown() override {
        delete rm;
        rm = nullptr;
    }


    HeapResourceMark *rm;
    ByteArrayOop     fnAlien;
    ByteArrayOop     invalidFunctionAlien;
    ByteArrayOop     resultAlien, addressAlien, pointerAlien, argumentAlien;
    SmallIntegerOop  smi0, smi1;
    char             address[8];


    ByteArrayOop allocateAlien( std::int32_t arraySize, std::int32_t alienSize ) {
        ByteArrayOop alien = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( arraySize ) );
        ByteArrayPrimitives::alienSetSize( smiOopFromValue( alienSize ), alien );
        return alien;
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->isMarkOop() ) << "Should be marked";
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void checkIntResult( const char *message, std::int32_t expected, std::int32_t actual ) {
        static_cast<void>(message); // unused
        char text[200];
        sprintf( text, "Should be: %d, was: %d", expected, actual );
        EXPECT_TRUE( actual == expected ) << text;
    }


    std::int32_t asInt( bool &ok, Oop intOop ) {
        if ( intOop->isSmallIntegerOop() )
            return SmallIntegerOop( intOop )->value();
        if ( !intOop->isByteArray() ) {
            ok = false;
            return 0;
        }
        return ByteArrayOop( intOop )->number().as_int32_t( ok );
    }

};

TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldReturnResultAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult0( resultAlien, fnAlien );
    EXPECT_TRUE( result == resultAlien ) << "should return result alien";
}


TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldReturnMarkedResultForNonAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult0( resultAlien, smi0 );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldReturnMarkedResultForDirectAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult0( resultAlien, resultAlien );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldReturnMarkedResultForNullFunctionPointer ) {
    Oop result = ByteArrayPrimitives::alienCallResult0( resultAlien, invalidFunctionAlien );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldReturnMarkedResultWhenResultNotAlien ) {
    Oop result = ByteArrayPrimitives::alienCallResult0( smi0, fnAlien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldCallClock ) {
    std::clock_t clockResult = clock();
    ByteArrayPrimitives::alienCallResult0( resultAlien, fnAlien );
    EXPECT_TRUE( sizeof( std::clock_t ) == 4 ) << "wrong size";
    Oop alienClockResult     = ByteArrayPrimitives::alienUnsignedLongAt( smi1, resultAlien );
    EXPECT_TRUE( clockResult == SmallIntegerOop( alienClockResult )->value() ) << "wrong result";
}


TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldSetResultInPointerAlien ) {
    std::clock_t clockResult = clock();
    ByteArrayPrimitives::alienCallResult0( pointerAlien, fnAlien );
    Oop alienClockResult = ByteArrayPrimitives::alienUnsignedLongAt( smi1, pointerAlien );
    EXPECT_TRUE( clockResult == SmallIntegerOop( alienClockResult )->value() ) << "wrong result";
}


TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldSetResultInAddressAlien ) {
    std::clock_t clockResult = clock();
    ByteArrayPrimitives::alienCallResult0( addressAlien, fnAlien );
    Oop alienClockResult = ByteArrayPrimitives::alienUnsignedLongAt( smi1, addressAlien );
    EXPECT_TRUE( clockResult == SmallIntegerOop( alienClockResult )->value() ) << "wrong result";
}


TEST_F( AlienIntegerCallout0Tests, alienCallResult0ShouldIgnoreResultWhenResultArgZero ) {
    Oop result = ByteArrayPrimitives::alienCallResult0( nilObject, fnAlien );
    EXPECT_TRUE( !result->isMarkOop() ) << "Should not be marked";
}
