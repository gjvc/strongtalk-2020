//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/ResourceMark.hpp"

#include <gtest/gtest.h>


extern "C" std::int32_t expansion_count;


class IndirectAlienPrimsTests : public ::testing::Test {

protected:
    void SetUp() override {
        heapResourceMark                = new HeapResourceMark();
        KlassOop         byteArrayClass = Universe::byteArrayKlassObject();
        PersistentHandle ah( byteArrayClass->klass_part()->allocateObjectSize( 8 ) );
        PersistentHandle iah( byteArrayClass->klass_part()->allocateObjectSize( 8 ) );
        PersistentHandle lu( byteArrayClass->klass_part()->allocateObjectSize( 8 ) );
        PersistentHandle ls( byteArrayClass->klass_part()->allocateObjectSize( 8 ) );

        doubleValue = DoubleOop( Universe::doubleKlassObject()->klass_part()->allocateObject() );
        doubleValue->set_value( 1.625 );

        largeUnsignedInteger = ByteArrayOop( lu.as_oop() );
        largeSignedInteger   = ByteArrayOop( ls.as_oop() );

        IntegerOps::unsigned_int_to_Integer( (std::uint32_t) 0xFFFFFFFF, ByteArrayOop( largeUnsignedInteger )->number() );
        IntegerOps::int_to_Integer( -1 << 31, ByteArrayOop( largeSignedInteger )->number() );

        alien = ByteArrayOop( ah.as_oop() );
        byteArrayPrimitives::alienSetSize( smiOopFromValue( -16 ), alien );
        byteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) alien_byte_region ), alien );
        memset( alien_byte_region, 0, 16 );

        invalidAlien = ByteArrayOop( iah.as_oop() );
        byteArrayPrimitives::alienSetSize( smiOopFromValue( -16 ), invalidAlien );
        byteArrayPrimitives::alienSetAddress( smiOopFromValue( 0 ), invalidAlien );
    }


    void TearDown() override {
        delete heapResourceMark;
        heapResourceMark = nullptr;
    }


    HeapResourceMark *heapResourceMark;
    ByteArrayOop     alien, invalidAlien;
    ByteArrayOop     largeUnsignedInteger;
    ByteArrayOop     largeSignedInteger;
    std::uint8_t     alien_byte_region[16];
    DoubleOop        doubleValue;


    std::int32_t asInteger( Oop largeInteger, bool &ok ) {
        Integer *number = &ByteArrayOop( largeInteger )->number();
        return number->as_int( ok );
    }


    void checkLargeInteger( Oop result, std::int32_t expected ) {
        char         message[200];
        EXPECT_TRUE( result->is_byteArray() ) << "Should be integer";
        bool       ok;
        std::int32_t actual = asInteger( result, ok );
        EXPECT_TRUE( ok ) << "should be integer";
        sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }


    void checkLargeUnsigned( Oop result, std::uint32_t expected ) {
        char          message[200];
        EXPECT_TRUE( result->is_byteArray() ) << "Should be integer";
        bool        ok;
        std::uint32_t actual = ByteArrayOop( result )->number().as_unsigned_int( ok );
        EXPECT_TRUE( ok ) << "should be integer";
        sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }


    void checkSmallInteger( Oop result, std::int32_t expected ) {
        char         message[200];
        EXPECT_TRUE( result->is_smi() ) << "Should be small integer";
        std::int32_t actual = SMIOop( result )->value();
        sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->is_mark() ) << "Should be marked";
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


};

TEST_F( IndirectAlienPrimsTests, alienUnsignedByteAtPutShouldSetUnsignedByte ) {
    byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 255 );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedByteAtPutShouldReturnAssignedByte ) {
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 255 );
}


TEST_F( IndirectAlienPrimsTests, alienSignedByteAtShouldReturnCorrectByte ) {
    byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( IndirectAlienPrimsTests, alienSignedByteAtPutShouldReturnCorrectByte ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( IndirectAlienPrimsTests, alienSignedByteAtPutShouldSetCorrectByte ) {
    byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( IndirectAlienPrimsTests, alienSignedByteAtPutShouldSetMaxValue ) {
    byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 127 ), smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 127 );
}


TEST_F( IndirectAlienPrimsTests, alienSignedByteAtPutShouldSetMinValue ) {
    byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -128 ), smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -128 );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedShortAtShouldReturnCorrectValue ) {
    byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), alien );
    byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 2 ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 65535 );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedShortAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, 65535 );
}


TEST_F( IndirectAlienPrimsTests, alienSignedShortAtShouldReturnCorrectValue ) {
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( IndirectAlienPrimsTests, alienSignedShortAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedLongAtShouldReturnCorrectValue ) {
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), alien );
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 3 ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), alien );
    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedLongAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienUnsignedLongAtPut( largeUnsignedInteger, smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), alien );
    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( IndirectAlienPrimsTests, alienSignedLongAtShouldReturnCorrectValue ) {
    byteArrayPrimitives::alienUnsignedLongAtPut( largeUnsignedInteger, smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), alien );
    checkSmallInteger( result, -1 );
}


TEST_F( IndirectAlienPrimsTests, alienSignedLongAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienSignedLongAtPut( largeSignedInteger, smiOopFromValue( 1 ), alien );
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), alien );
    checkLargeInteger( result, -1 << 31 );
}


TEST_F( IndirectAlienPrimsTests, alienDoubleAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienDoubleAtPut( doubleValue, smiOopFromValue( 1 ), alien );
    Oop                        result = byteArrayPrimitives::alienDoubleAt( smiOopFromValue( 1 ), alien );
    EXPECT_TRUE( result->is_double() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
    EXPECT_EQ( 1.625, ( (double *) alien_byte_region )[ 0 ] ) << "value not set";
}


TEST_F( IndirectAlienPrimsTests, alienDoubleAtPutShouldSetValueAtSecondByte ) {
    byteArrayPrimitives::alienDoubleAtPut( doubleValue, smiOopFromValue( 2 ), alien );
    Oop                        result = byteArrayPrimitives::alienDoubleAt( smiOopFromValue( 2 ), alien );
    EXPECT_TRUE( result->is_double() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
    EXPECT_EQ( 1.625, ( (double *) ( alien_byte_region + 1 ) )[ 0 ] ) << "value not set";
}


TEST_F( IndirectAlienPrimsTests, alienFloatAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienFloatAtPut( doubleValue, smiOopFromValue( 1 ), alien );
    Oop                        result = byteArrayPrimitives::alienFloatAt( smiOopFromValue( 1 ), alien );
    EXPECT_TRUE( result->is_double() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
    EXPECT_EQ( 1.625F, ( (float *) ( alien_byte_region ) )[ 0 ] ) << "value not set";
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedByteAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienSignedByteAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienSignedShortAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienSignedLongAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienDoubleAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienDoubleAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienDoubleAtPut( doubleValue, smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienFloatAtShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienFloatAt( smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}


TEST_F( IndirectAlienPrimsTests, alienFloatAtPutShouldReturnMarkedSymbolWhenAddressInvalid ) {
    Oop result = byteArrayPrimitives::alienFloatAtPut( doubleValue, smiOopFromValue( 1 ), invalidAlien );
    checkMarkedSymbol( "invalid address", result, vmSymbols::illegal_state() );
}
