//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "test/utilities/testUtils.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"

#include <gtest/gtest.h>

extern "C" std::int32_t expansion_count;


class ByteArrayPrimsTests : public ::testing::Test {

protected:
    void SetUp() override {
        byteArrayClass = Universe::byteArrayKlassObject();
        alien          = ByteArrayOop( Universe::byteArrayKlassObject()->klass_part()->allocateObjectSize( 8 ) );
        byteArrayPrimitives::alienSetSize( smiOopFromValue( 4 ), alien );
        memset( alien_byte_region, 0, 16 );
    }


    void TearDown() override {

    }


    KlassOop     byteArrayClass;
    ByteArrayOop alien;
    std::uint8_t alien_byte_region[16];


    void checkAlienContents( ByteArrayOop alien ) {
        EXPECT_TRUE( 255 == SMIOop( byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 1 ), alien ) )->value() ) << "Wrong byte at index 1";
        EXPECT_TRUE( 2 == SMIOop( byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 2 ), alien ) )->value() ) << "Wrong byte at index 2";
        EXPECT_TRUE( 3 == SMIOop( byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 3 ), alien ) )->value() ) << "Wrong byte at index 3";
        EXPECT_TRUE( 4 == SMIOop( byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 4 ), alien ) )->value() ) << "Wrong byte at index 4";
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        ResourceMark resourceMark;
        char         text[200];
        EXPECT_TRUE( result->is_mark() ) << "Should be marked";
        sprintf( text, "%s. Should be: %s, was: %s", message, expected->as_string(), unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void setUnsignedContents( std::uint8_t *contents ) {
        contents[ 0 ] = 255;
        contents[ 1 ] = 2;
        contents[ 2 ] = 3;
        contents[ 3 ] = 4;
    }


    std::int32_t asInteger( Oop largeInteger, bool &ok ) {
        Integer *number = &ByteArrayOop( largeInteger )->number();
        return number->as_int32_t( ok );
    }


};

TEST_F( ByteArrayPrimsTests, allocateSize2ShouldAllocateByteArrayOfCorrectSize ) {
    HandleMark                                                                                handles;
    Handle                                                                                    byteArrayClassHandle( byteArrayClass );
    Oop                                                                                       result = byteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( 10 ), byteArrayClass );
    ASSERT_TRUE( result->is_byteArray() );
    ASSERT_EQ( 10, ByteArrayOop                                                               (result) -> length() );
    ASSERT_EQ( (const char *) byteArrayClassHandle.as_klass(), (const char *) result->klass() );
    for ( std::int32_t                                                                        index  = 10; index > 0; index-- )ASSERT_EQ( std::uint8_t( 0 ), ByteArrayOop( result ) -> byte_at(index) );
}


TEST_F( ByteArrayPrimsTests, allocateSize2ShouldAllocateTenuredWhenRequested ) {
    HandleMark                                                                              handles;
    Handle                                                                                  byteArrayClassHandle( byteArrayClass );
    std::int32_t                                                                            size   = Universe::new_gen.eden()->free() + 1;
    Oop                                                                                     result = byteArrayPrimitives::allocateSize2( trueObject, smiOopFromValue( size ), byteArrayClass );
    ASSERT_TRUE( result->is_byteArray() );
    ASSERT_TRUE( Universe::old_gen.contains( result ) );
    ASSERT_EQ( size, ByteArrayOop                                                           ( result ) -> length() );
    ASSERT_EQ( (const char *) byteArrayClassHandle.as_klass(), (const char *) result->klass() );
}


TEST_F( ByteArrayPrimsTests, allocateSize2ShouldFailWithNonByteArray ) {
    Oop result = byteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( 10 ), Universe::find_global( "Object" ) );
    checkMarkedSymbol( "wrong class", result, vmSymbols::invalid_klass() );
}


TEST_F( ByteArrayPrimsTests, allocateSize2ShouldFailWithNonInteger ) {
    Oop result = byteArrayPrimitives::allocateSize2( falseObject, byteArrayClass, byteArrayClass );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, allocateSize2ShouldFailWithNegativeSize ) {
    Oop result = byteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( -1 ), byteArrayClass );
    checkMarkedSymbol( "negative size", result, vmSymbols::negative_size() );
}


TEST_F( ByteArrayPrimsTests, allocateSize2ShouldFailWhenTenuredNotBoolean ) {
    Oop result = byteArrayPrimitives::allocateSize2( Universe::nilObject(), smiOopFromValue( 10 ), byteArrayClass );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, allocateSize2ShouldFailWhenInsufficientSpace ) {
    std::int32_t size   = Universe::new_gen.eden()->free();
    Oop          result = byteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( size + 1 ), byteArrayClass );
    checkMarkedSymbol( "failed allocation", result, vmSymbols::failed_allocation() );
}


TEST_F( ByteArrayPrimsTests, allocateSize2ShouldFailWhenTooBigForOldGen ) {
    std::int32_t size   = Universe::old_gen.free();
    Oop          result = byteArrayPrimitives::allocateSize2( trueObject, smiOopFromValue( size + 1 ), byteArrayClass );
    checkMarkedSymbol( "failed allocation", result, vmSymbols::failed_allocation() );
}


TEST_F( ByteArrayPrimsTests, alienSizeShouldReturnCorrectSize ) {
    std::uint8_t *bytes = alien->bytes();
    ( (std::int32_t *) bytes )[ 0 ] = 4;
    EXPECT_EQ( 4, SMIOop( byteArrayPrimitives::alienGetSize( alien ) )->value() ) << "wrong size";
}


TEST_F( ByteArrayPrimsTests, alienAddressShouldReturnCorrectAddress ) {
    std::uint8_t *bytes = alien->bytes();
    ( (std::int32_t *) bytes )[ 0 ]  = -16;
    ( (std::uint8_t **) bytes )[ 1 ] = alien_byte_region;
    EXPECT_EQ( (std::int32_t) alien_byte_region, SMIOop( byteArrayPrimitives::alienGetAddress( alien ) )->value() ) << "wrong address";
}


TEST_F( ByteArrayPrimsTests, alienSetAddressShouldAssignCorrectAddress ) {
    std::uint8_t *bytes  = alien->bytes();
    std::int32_t address = (std::int32_t) alien_byte_region;
    ( (std::int32_t *) bytes )[ 0 ] = -16;
    EXPECT_TRUE( alien == byteArrayPrimitives::alienSetAddress( smiOopFromValue( address ), alien ) ) << "Should return alien";
    EXPECT_EQ( address, SMIOop( byteArrayPrimitives::alienGetAddress( alien ) )->value() ) << "Address should match";
}


TEST_F( ByteArrayPrimsTests, alienSetAddressShouldAssignCorrectAddressFromLargeInteger ) {
    PersistentHandle address( as_large_integer( (std::int32_t) alien_byte_region ) );
    byteArrayPrimitives::alienSetSize( smiOopFromValue( -16 ), alien );
    EXPECT_TRUE( alien == byteArrayPrimitives::alienSetAddress( address.as_oop(), alien ) ) << "Should return alien";
    EXPECT_EQ( (std::int32_t) alien_byte_region, SMIOop( byteArrayPrimitives::alienGetAddress( alien ) )->value() ) << "Address should match";
}


TEST_F( ByteArrayPrimsTests, alienSetAddressShouldReturnMarkedSymbolForAddressOutOfRange ) {
    BlockScavenge bs;
    byteArrayPrimitives::alienSetSize( smiOopFromValue( 0 ), alien );
    Oop largeInteger = as_large_integer( 256 * 256 * 256 );
    Oop tooBig       = byteArrayPrimitives::largeIntegerMultiply( largeInteger, largeInteger );
    Oop result       = byteArrayPrimitives::alienSetAddress( tooBig, alien );
    checkMarkedSymbol( "invalid argument", result, vmSymbols::argument_is_invalid() );
}


TEST_F( ByteArrayPrimsTests, alienSetAddressShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienSetAddress( SMIOop( 0 ), SMIOop( 0 ) );
    checkMarkedSymbol( "wrong receiver", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienSetAddressShouldReturnMarkedSymbolWhenAlienIsDirect ) {
    //std::uint8_t *bytes = alien->bytes();
    byteArrayPrimitives::alienSetSize( SMIOop( 4 ), alien );
    Oop result = byteArrayPrimitives::alienSetAddress( SMIOop( 0 ), alien );
    checkMarkedSymbol( "iillegal state", result, vmSymbols::illegal_state() );
}


TEST_F( ByteArrayPrimsTests, alienSetAddressShouldReturnMarkedSymbolWhenArgumentNotInteger ) {
    byteArrayPrimitives::alienSetSize( SMIOop( -16 ), alien );
    Oop result = byteArrayPrimitives::alienSetAddress( trueObject, alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienAddressShouldReturnLargeIntegerAddress ) {
    std::uint8_t *bytes  = alien->bytes();
    std::int32_t address = -128 * 256 * 256 * 256;
    ( (std::int32_t *) bytes )[ 0 ]  = 0;
    ( (std::uint8_t **) bytes )[ 1 ] = (std::uint8_t *) address;

    Oop result = byteArrayPrimitives::alienGetAddress( alien );
    EXPECT_TRUE( result->is_byteArray() ) << "should be large integer";

    Integer *number = &ByteArrayOop( result )->number();
    bool    ok;

    std::int32_t resultAddress = number->as_int32_t( ok );
    EXPECT_TRUE( ok ) << "too large for std::int32_t";
    EXPECT_EQ( address, resultAddress ) << "wrong address";
}


TEST_F( ByteArrayPrimsTests, alienAddressShouldReturnMarkedSymbolWhenNotByteArray ) {
    Oop result = byteArrayPrimitives::alienGetAddress( smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienSizeShouldReturnMarkedSymbolWhenNotByteArray ) {
    Oop result = byteArrayPrimitives::alienGetSize( smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienAddressShouldReturnMarkedSymbolWhenDirect ) {
    std::uint8_t *bytes = alien->bytes();
    ( (std::int32_t *) bytes )[ 0 ] = 4;
    Oop result = byteArrayPrimitives::alienGetAddress( alien );
    checkMarkedSymbol( "illegal state", result, vmSymbols::illegal_state() );
}


TEST_F( ByteArrayPrimsTests, alienSetSizeShouldReturnMarkedResultWhenReceiverNotByteArray ) {
    Oop result = byteArrayPrimitives::alienSetSize( smiOopFromValue( 4 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienSetSizeShouldReturnMarkedResultWhenSizeNotSMI ) {
    Oop result = byteArrayPrimitives::alienSetSize( vmSymbols::abs(), alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienSetSizeShouldSetCorrectSize ) {
    std::uint8_t *bytes = alien->bytes();
    EXPECT_TRUE( alien == byteArrayPrimitives::alienSetSize( smiOopFromValue( 4 ), alien ) ) << "should return receiver";
    EXPECT_EQ( 4, ( (std::int32_t *) bytes )[ 0 ] ) << "wrong size";
}


TEST_F( ByteArrayPrimsTests, unsignedByteAtWithDirectAlienShouldReturnCorrectByte ) {
    std::uint8_t *bytes = alien->bytes();
    ( (std::int32_t *) bytes )[ 0 ] = 4;
    setUnsignedContents( &bytes[ 4 ] );
    checkAlienContents( alien );
}


TEST_F( ByteArrayPrimsTests, unsignedByteAtWithIndirectAlienShouldReturnCorrectByteFromIndirectAlien ) {
    std::uint8_t *bytes    = alien->bytes();
    //std::uint8_t *contents = bytes + 4;
    ( (std::int32_t *) bytes )[ 0 ]  = -16;
    ( (std::uint8_t **) bytes )[ 1 ] = alien_byte_region;
    setUnsignedContents( alien_byte_region );
    checkAlienContents( alien );
}


TEST_F( ByteArrayPrimsTests, unsignedByteAtWithPointerAlienShouldReturnCorrectByteFromPointerAlien ) {
    std::uint8_t *bytes = alien->bytes();
    ( (std::int32_t *) bytes )[ 0 ]  = 0;
    ( (std::uint8_t **) bytes )[ 1 ] = alien_byte_region;
    setUnsignedContents( alien_byte_region );
    checkAlienContents( alien );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtShouldReturnMarkedSymbolWhenNotByteArray ) {
    Oop result = byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenNotByteArray ) {
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenIndexNotSmallInteger ) {
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 1 ), alien, alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenValueNotSmallInteger ) {
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( alien, smiOopFromValue( 1 ), alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    byteArrayPrimitives::alienSetAddress( smiOopFromValue( (std::int32_t) &alien_byte_region ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 0 ), smiOopFromValue( 0 ), alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenValueTooSmall ) {
    byteArrayPrimitives::alienSetSize( smiOopFromValue( 4 ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), alien );
    checkMarkedSymbol( "argument invalid", result, vmSymbols::argument_is_invalid() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenValueTooLarge ) {
    byteArrayPrimitives::alienSetSize( smiOopFromValue( 4 ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 256 ), smiOopFromValue( 1 ), alien );
    checkMarkedSymbol( "argument invalid", result, vmSymbols::argument_is_invalid() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    byteArrayPrimitives::alienSetSize( smiOopFromValue( 4 ), alien );
    Oop result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 0 ), smiOopFromValue( 5 ), alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    std::uint8_t *bytes = alien->bytes();
    ( (std::int32_t *) bytes )[ 0 ] = 4;
    setUnsignedContents( &bytes[ 4 ] );
    Oop result = byteArrayPrimitives::alienUnsignedByteAt( vmSymbols::abs(), alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( ByteArrayPrimsTests, alienUnsignedByteAtShouldReturnMarkedSymbolWhenIndexNotInRange ) {
    std::uint8_t *bytes = alien->bytes();
    ( (std::int32_t *) bytes )[ 0 ] = 4;
    setUnsignedContents( &bytes[ 4 ] );
    Oop result = byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 5 ), alien );
    checkMarkedSymbol( "invalid argument", result, vmSymbols::index_not_valid() );
    result = byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 0 ), alien );
    checkMarkedSymbol( "invalid argument", result, vmSymbols::index_not_valid() );
}
