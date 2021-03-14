
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/utility/Integer.hpp"
#include "vm/utility/IntegerOps.hpp"
#include "vm/runtime/ResourceMark.hpp"

#include <gtest/gtest.h>


class PointerAlienPrimsTests : public ::testing::Test {

public:
    PointerAlienPrimsTests() : ::testing::Test(), rm{ nullptr }, alien{}, invalidAlien{}, largeUnsignedInteger{}, largeSignedInteger{}, alien_byte_region{}, doubleValue{} {}


protected:
    void SetUp() override {

        rm = new HeapResourceMark();
        PersistentHandle ah( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( 8 ) );
        PersistentHandle iah( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( 8 ) );
        PersistentHandle lu( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( 8 ) );
        PersistentHandle ls( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( 8 ) );

        largeUnsignedInteger = ByteArrayOop( lu.as_oop() );
        largeSignedInteger   = ByteArrayOop( ls.as_oop() );

        doubleValue = DoubleOop( Universe::doubleKlassObject()->klass_part()->allocateObject() );
        doubleValue->set_value( 1.625 );

        IntegerOps::unsigned_int_to_Integer( (std::uint32_t) 0xFFFFFFFF, ByteArrayOop( largeUnsignedInteger )->number() );
        IntegerOps::int_to_Integer( -1 << 31, ByteArrayOop( largeSignedInteger )->number() );

        alien = ByteArrayOop( ah.as_oop() );
        ByteArrayPrimitives::alienSetSize( smiOopFromValue( 0 ), alien );
        ByteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) alien_byte_region ), alien );
        memset( alien_byte_region, 0, 16 );

        invalidAlien = ByteArrayOop( iah.as_oop() );
        ByteArrayPrimitives::alienSetSize( smiOopFromValue( 0 ), invalidAlien );
        ByteArrayPrimitives::alienSetAddress( smiOopFromValue( 0 ), invalidAlien );
    }


    void TearDown() override {
        delete rm;
        rm = nullptr;
    }


    HeapResourceMark *rm;
    ByteArrayOop     alien, invalidAlien;
    ByteArrayOop     largeUnsignedInteger;
    ByteArrayOop     largeSignedInteger;
    std::uint8_t     alien_byte_region[16];
    DoubleOop        doubleValue;


    std::int32_t asInteger( Oop largeInteger, bool &ok ) {
        Integer *number = &ByteArrayOop( largeInteger )->number();
        return number->as_int32_t( ok );
    }


    void checkLargeInteger( Oop result, std::int32_t expected ) {
        char         message[200];
        EXPECT_TRUE( result->isByteArray() ) << "Should be integer";
        bool         ok;
        std::int32_t actual = asInteger( result, ok );
        EXPECT_TRUE( ok ) << "should be integer";
        sprintf( message, "wrong value. expected: 0x%08x, was: 0x%08x", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }


    void checkLargeUnsigned( Oop result, std::uint32_t expected ) {
        char          message[200];
        EXPECT_TRUE( result->isByteArray() ) << "Should be integer";
        bool          ok;
        std::uint32_t actual = ByteArrayOop( result )->number().as_uint32_t( ok );
        EXPECT_TRUE( ok ) << "should be integer";
        sprintf( message, "wrong value. expected: 0x%08x, was: 0x%08x", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }


    void checkSmallInteger( Oop result, std::int32_t expected ) {
        char         message[200];
        EXPECT_TRUE( result->isSmallIntegerOop() ) << "Should be small integer";
        std::int32_t actual = SmallIntegerOop( result )->value();
        sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->isMarkOop() ) << "Should be marked";
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


};

TEST_F( PointerAlienPrimsTests, alienUnsignedByteAtPutShouldSetUnsignedByte ) {
    ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 255 );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedByteAtPutShouldReturnAssignedByte ) {
    Oop result = ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 255 );
}


TEST_F( PointerAlienPrimsTests, alienSignedByteAtShouldReturnCorrectByte ) {
    ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( PointerAlienPrimsTests, alienSignedByteAtPutShouldSetCorrectByte ) {
    ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedShortAtShouldReturnCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), alien );
    ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 2 ), alien );
    Oop result = ByteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 65535 );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedShortAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 65535 );
}


TEST_F( PointerAlienPrimsTests, alienSignedShortAtShouldReturnCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( PointerAlienPrimsTests, alienSignedShortAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedLongAtShouldReturnCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), alien );
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 3 ), alien );
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), alien );
    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedLongAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedLongAtPut( largeUnsignedInteger, smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), alien );
    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( PointerAlienPrimsTests, alienSignedLongAtShouldReturnCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedLongAtPut( largeUnsignedInteger, smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( PointerAlienPrimsTests, alienSignedLongAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienSignedLongAtPut( largeSignedInteger, smiOopFromValue( 1 ), alien );
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), alien );
    checkLargeInteger( result, -1 << 31 );
}


TEST_F( PointerAlienPrimsTests, alienDoubleAtPutShouldSetValueAtSecondByte ) {
    ByteArrayPrimitives::alienDoubleAtPut( doubleValue, smiOopFromValue( 2 ), alien );
    Oop                        result = ByteArrayPrimitives::alienDoubleAt( smiOopFromValue( 2 ), alien );
    EXPECT_TRUE( result->isDouble() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
    EXPECT_EQ( 1.625, ( (double *) ( alien_byte_region + 1 ) )[ 0 ] ) << "value not set";
}


TEST_F( PointerAlienPrimsTests, alienFloatAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienFloatAtPut( doubleValue, smiOopFromValue( 1 ), alien );
    Oop                        result = ByteArrayPrimitives::alienFloatAt( smiOopFromValue( 1 ), alien );
    EXPECT_TRUE( result->isDouble() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
    EXPECT_EQ( 1.625F, ( (float *) ( alien_byte_region ) )[ 0 ] ) << "value not set";
}


TEST_F( PointerAlienPrimsTests, alienUnsignedByteAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienSignedByteAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienSignedShortAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienSignedLongAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienDoubleAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienDoubleAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienDoubleAtPut( doubleValue, smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienFloatAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienFloatAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( PointerAlienPrimsTests, alienFloatAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = ByteArrayPrimitives::alienFloatAtPut( doubleValue, smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}
