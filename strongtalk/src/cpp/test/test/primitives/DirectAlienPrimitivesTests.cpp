
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/primitives/ByteArrayPrimitives.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"

#include <gtest/gtest.h>

extern "C" std::int32_t expansion_count;


class DirectAlienPrimitivesTests : public ::testing::Test {

public:
    DirectAlienPrimitivesTests() : ::testing::Test() {}

protected:

    HeapResourceMark *_heapResourceMark;
    ByteArrayOop     _alien;
    ByteArrayOop     _largeUnsignedInteger;
    ByteArrayOop     _largeUnsignedInteger2;
    ByteArrayOop     _veryLargeUnsignedInteger;
    ByteArrayOop     _largeSignedInteger;
    DoubleOop        _doubleValue;


    void SetUp() override {
        _heapResourceMark = new HeapResourceMark();
        PersistentHandle bac( Universe::byteArrayKlassObject() );

        PersistentHandle ah( KlassOop( bac.as_oop() )->klass_part()->allocateObjectSize( 12 ) );
        PersistentHandle lu( KlassOop( bac.as_oop() )->klass_part()->allocateObjectSize( 8 ) );
        PersistentHandle lu2( KlassOop( bac.as_oop() )->klass_part()->allocateObjectSize( 8 ) );
        PersistentHandle vlu( KlassOop( bac.as_oop() )->klass_part()->allocateObjectSize( 12 ) );
        PersistentHandle ls( KlassOop( bac.as_oop() )->klass_part()->allocateObjectSize( 8 ) );

        _alien                    = ByteArrayOop( ah.as_oop() );
        _largeUnsignedInteger     = ByteArrayOop( lu.as_oop() );
        _largeUnsignedInteger2    = ByteArrayOop( lu2.as_oop() );
        _veryLargeUnsignedInteger = ByteArrayOop( vlu.as_oop() );
        _largeSignedInteger       = ByteArrayOop( ls.as_oop() );
        _doubleValue              = DoubleOop( Universe::doubleKlassObject()->klass_part()->allocateObject() );
        _doubleValue->set_value( 1.625 );

        IntegerOps::unsigned_int_to_Integer( (std::uint32_t) 0xFFFFFFFF, ByteArrayOop( _largeUnsignedInteger )->number() );
        IntegerOps::int_to_Integer( +( 1 << 30 ), ByteArrayOop( _largeUnsignedInteger2 )->number() );
        IntegerOps::int_to_Integer( -( 1 << 31 ), ByteArrayOop( _largeSignedInteger )->number() );
//            IntegerOps::mul( _largeUnsignedInteger->number(), _largeUnsignedInteger->number(), _veryLargeUnsignedInteger->number() );
        ByteArrayPrimitives::alienSetSize( smiOopFromValue( 8 ), _alien );
    }


    void TearDown() override {
        MarkSweep::collect();
        delete _heapResourceMark;
        _heapResourceMark = nullptr;
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->isMarkOop() ) << "Should be marked";
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


    void checkLargeInteger( Oop result, std::int32_t expected ) {
        char         message[200];
        EXPECT_TRUE( result->isByteArray() ) << "Should be integer";
        bool         ok;
        std::int32_t actual = ByteArrayOop( result )->number().as_int32_t( ok );
        EXPECT_TRUE( ok ) << "should be integer";
        sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }


    void checkLargeUnsigned( Oop result, std::uint32_t expected ) {
        char          message[200];
        EXPECT_TRUE( result->isByteArray() ) << "Should be integer";
        bool          ok;
        std::uint32_t actual = ByteArrayOop( result )->number().as_uint32_t( ok );
        EXPECT_TRUE( ok ) << "should be integer";
        sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }


    void checkSmallInteger( Oop result, std::int32_t expected ) {
        char         message[200];
        EXPECT_TRUE( result->isSmallIntegerOop() ) << "Should be small integer";
        std::int32_t actual = SmallIntegerOop( result )->value();
        sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
        EXPECT_EQ( expected, actual ) << message;
    }

};


TEST_F( DirectAlienPrimitivesTests, alienUnsignedByteAtPutShouldSetUnsignedByte ) {
    ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), _alien );
    Oop                            result = ByteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "result should be SmallInteger";
    EXPECT_EQ( 255, SmallIntegerOop( result ) -> value() ) << "Should set value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedByteAtPutShouldReturnAssignedByte ) {

    Oop result = ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->isSmallIntegerOop() ) << "result should be SmallInteger";
    EXPECT_EQ( 255, SmallIntegerOop( result ) -> value() ) << "Should return value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnSignedValue ) {
    ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), _alien );
    Oop                           result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "result should be SmallInteger";
    EXPECT_EQ( -1, SmallIntegerOop( result ) -> value() ) << "Should return correct value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnMarkedSymbolWhenIndexNotSmallInteger ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAt( _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 9 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnSignedValue ) {
    Oop                                                                                          result = ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "result should be SmallInteger";
    EXPECT_EQ( -1, SmallIntegerOop                                                               ( result ) -> value() ) << "Should return correct value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );
    Oop                           result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "result should be SmallInteger";
    EXPECT_EQ( -1, SmallIntegerOop( result ) -> value() ) << "Should return correct value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldSetCorrectValueWhenPositive ) {
    ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    Oop                          result = ByteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "result should be SmallInteger";
    EXPECT_EQ( 1, SmallIntegerOop( result ) -> value() ) << "Should return correct value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenIndexNotSmallInteger ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 1 ), _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenValieNotSmallInteger ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAtPut( _alien, smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 0 ), smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 0 ), smiOopFromValue( 9 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenValueTooSmall ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -129 ), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "invalid value", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenValueTooBig ) {
    Oop result = ByteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 128 ), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "invalid value", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), _alien );
    ByteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 2 ), _alien );
    Oop                              result = ByteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "should be small_int_t";
    EXPECT_EQ( 65535, SmallIntegerOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAt( _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 8 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnCorrectValue ) {
    Oop                                                                                              result = ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "should be small_int_t";
    EXPECT_EQ( 65535, SmallIntegerOop                                                                ( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), _alien );
    Oop                              result = ByteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "should be small_int_t";
    EXPECT_EQ( 65535, SmallIntegerOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenValueNotSMI ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAtPut( _alien, smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 8 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenValueTooSmall ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "value too small", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenValueTooBig ) {
    Oop result = ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65536 ), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "value too small", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), _alien );
    Oop                           result = ByteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "should be small_int_t";
    EXPECT_EQ( -1, SmallIntegerOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAt( _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 8 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnCorrectValue ) {

    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->isSmallIntegerOop() ) << "should be small_int_t";
    EXPECT_EQ( -1, SmallIntegerOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );
    Oop result = ByteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->isSmallIntegerOop() ) << "should be small_int_t";
    EXPECT_EQ( -1, SmallIntegerOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenValueNotSMI ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( _alien, smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 8 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenValueTooSmall ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( -32769 ), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "value too small", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenValueTooBig ) {
    Oop result = ByteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 32768 ), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "value too small", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), _alien );
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 3 ), _alien );
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), _alien );
    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnSmallIntegerWhenResultSmall ) {
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    ByteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 0 ), smiOopFromValue( 3 ), _alien );
    Oop                          result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "Should be small integer";
    EXPECT_EQ( 1, SmallIntegerOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 6 ), _alien );
    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnCorrectValue ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAtPut( _largeUnsignedInteger, smiOopFromValue( 1 ), _alien );
    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedLongAtPut( _largeUnsignedInteger, smiOopFromValue( 1 ), _alien );
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), _alien );
    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenValueTooLarge ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAtPut( _veryLargeUnsignedInteger, smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "invalid value", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldSetCorrectValueWithSMI ) {
    ByteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    Oop result = ByteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "Should be small integer";
    EXPECT_TRUE( 1 == SmallIntegerOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenValueNotInteger ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAtPut( Universe::byteArrayKlassObject(), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = ByteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 6 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnCorrectValue ) {
    ByteArrayPrimitives::alienUnsignedLongAtPut( _largeUnsignedInteger, smiOopFromValue( 1 ), _alien );
    Oop                           result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isSmallIntegerOop() ) << "Should be integer";
    EXPECT_EQ( -1, SmallIntegerOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnLargeInteger ) {
    ByteArrayPrimitives::alienUnsignedLongAtPut( _largeUnsignedInteger2, smiOopFromValue( 1 ), _alien );
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), _alien );
    checkLargeInteger( result, 1 << 30 );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAt( _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 6 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnCorrectValue ) {

    Oop largeSignedInteger{ nullptr };
    Oop result = ByteArrayPrimitives::alienSignedLongAtPut( largeSignedInteger, smiOopFromValue( 1 ), _alien );

    checkLargeInteger( result, -1 << 31 );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldSetCorrectValue ) {
    Oop largeSignedInteger{ nullptr };
    ByteArrayPrimitives::alienSignedLongAtPut( largeSignedInteger, smiOopFromValue( 1 ), _alien );
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), _alien );

    checkLargeInteger( result, -1 << 31 );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutWithSMIShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    Oop result = ByteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), _alien );
    checkSmallInteger( result, 1 );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenValueNotInteger ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAtPut( Universe::byteArrayKlassObject(), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 6 ), _alien );
    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenValueTooLarge ) {
    Oop result = ByteArrayPrimitives::alienSignedLongAtPut( _veryLargeUnsignedInteger, smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "invalid value", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnValue ) {
    Oop result = ByteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->isDouble() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 1 ), _alien );
    Oop                        result = ByteArrayPrimitives::alienDoubleAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->isDouble() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
    EXPECT_EQ( 1.625, ( (double *) ( _alien->bytes() + 4 ) )[ 0 ] ) << "value not set";
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienDoubleAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienDoubleAtPut( _doubleValue, _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienDoubleAt( _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenValueNotDouble ) {
    Oop result = ByteArrayPrimitives::alienDoubleAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienDoubleAt( smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = ByteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 2 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = ByteArrayPrimitives::alienDoubleAt( smiOopFromValue( 2 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldSetCorrectValue ) {
    ByteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 1 ), _alien );

    Oop result = ByteArrayPrimitives::alienFloatAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->isDouble() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
    EXPECT_EQ( 1.625F, ( (float *) ( _alien->bytes() + 4 ) )[ 0 ] ) << "value not set";
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldSetCorrectValueAt2 ) {
    ByteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 2 ), _alien );
    Oop                        result = ByteArrayPrimitives::alienFloatAt( smiOopFromValue( 2 ), _alien );
    EXPECT_TRUE( result->isDouble() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
    EXPECT_EQ( 1.625F, ( (float *) ( _alien->bytes() + 5 ) )[ 0 ] ) << "value not set";
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnCorrectValue ) {

    Oop result = ByteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->isDouble() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result ) -> value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = ByteArrayPrimitives::alienFloatAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );
    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienFloatAtPut( _doubleValue, _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = ByteArrayPrimitives::alienFloatAt( _alien, _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenValueNotDouble ) {
    Oop result = ByteArrayPrimitives::alienFloatAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienFloatAt( smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = ByteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 0 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = ByteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 6 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = ByteArrayPrimitives::alienFloatAt( smiOopFromValue( 6 ), _alien );
    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}
