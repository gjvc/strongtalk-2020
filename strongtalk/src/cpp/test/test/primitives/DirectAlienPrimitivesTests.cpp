
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"

#include <gtest/gtest.h>

extern "C" int expansion_count;


class DirectAlienPrimitivesTests : public ::testing::Test {

    protected:

        HeapResourceMark * _heapResourceMark;
        ByteArrayOop _alien;
        ByteArrayOop _largeUnsignedInteger;
        ByteArrayOop _largeUnsignedInteger2;
        ByteArrayOop _veryLargeUnsignedInteger;
        ByteArrayOop _largeSignedInteger;
        DoubleOop    _doubleValue;


        void SetUp() override {
            _heapResourceMark = new HeapResourceMark();
            PersistentHandle bac( Universe::byteArrayKlassObj() );

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
            _doubleValue              = DoubleOop( Universe::doubleKlassObj()->klass_part()->allocateObject() );
            _doubleValue->set_value( 1.625 );

            IntegerOps::unsigned_int_to_Integer( ( uint32_t ) 0xFFFFFFFF, ByteArrayOop( _largeUnsignedInteger )->number() );
            IntegerOps::int_to_Integer( 1 << 30, ByteArrayOop( _largeUnsignedInteger2 )->number() );
            IntegerOps::int_to_Integer( -1 << 31, ByteArrayOop( _largeSignedInteger )->number() );
//            IntegerOps::mul( _largeUnsignedInteger->number(), _largeUnsignedInteger->number(), _veryLargeUnsignedInteger->number() );
            byteArrayPrimitives::alienSetSize( smiOopFromValue( 8 ), _alien );
        }


        void TearDown() override {
            MarkSweep::collect();
            delete _heapResourceMark;
            _heapResourceMark = nullptr;
        }


        void checkMarkedSymbol( const char * message, Oop result, SymbolOop expected ) {
            char text[200];
            EXPECT_TRUE( result->is_mark() ) << "Should be marked";
            sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
            EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
        }


        void checkLargeInteger( Oop result, int expected ) {
            char   message[200];
            EXPECT_TRUE( result->is_byteArray() ) << "Should be integer";
            bool_t ok;
            int    actual = ByteArrayOop( result )->number().as_int( ok );
            EXPECT_TRUE( ok ) << "should be integer";
            sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
            EXPECT_EQ( expected, actual ) << message;
        }


        void checkLargeUnsigned( Oop result, uint32_t expected ) {
            char     message[200];
            EXPECT_TRUE( result->is_byteArray() ) << "Should be integer";
            bool_t   ok;
            uint32_t actual = ByteArrayOop( result )->number().as_unsigned_int( ok );
            EXPECT_TRUE( ok ) << "should be integer";
            sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
            EXPECT_EQ( expected, actual ) << message;
        }


        void checkSmallInteger( Oop result, int expected ) {
            char message[200];
            EXPECT_TRUE( result->is_smi() ) << "Should be small integer";
            int  actual = SMIOop( result )->value();
            sprintf( message, "wrong value. expected: %d, was: %d", expected, actual );
            EXPECT_EQ( expected, actual ) << message;
        }

};


TEST_F( DirectAlienPrimitivesTests, alienUnsignedByteAtPutShouldSetUnsignedByte ) {
    byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienUnsignedByteAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "result should be SmallInteger";
    EXPECT_EQ( 255, SMIOop( result )->value() ) << "Should set value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedByteAtPutShouldReturnAssignedByte ) {
    Oop                   result = byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->is_smi() ) << "result should be SmallInteger";
    EXPECT_EQ( 255, SMIOop( result )->value() ) << "Should return value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnSignedValue ) {
    byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "result should be SmallInteger";
    EXPECT_EQ( -1, SMIOop( result )->value() ) << "Should return correct value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnMarkedSymbolWhenIndexNotSmallInteger ) {
    Oop result = byteArrayPrimitives::alienSignedByteAt( _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 9 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnSignedValue ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "result should be SmallInteger";
    EXPECT_EQ( -1, SMIOop( result )->value() ) << "Should return correct value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "result should be SmallInteger";
    EXPECT_EQ( -1, SMIOop( result )->value() ) << "Should return correct value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldSetCorrectValueWhenPositive ) {
    byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienSignedByteAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "result should be SmallInteger";
    EXPECT_EQ( 1, SMIOop( result )->value() ) << "Should return correct value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenIndexNotSmallInteger ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 1 ), _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenValieNotSmallInteger ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( _alien, smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 0 ), smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 0 ), smiOopFromValue( 9 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenValueTooSmall ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( -129 ), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "invalid value", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedByteAtPutShouldReturnMarkedSymbolWhenValueTooBig ) {
    Oop result = byteArrayPrimitives::alienSignedByteAtPut( smiOopFromValue( 128 ), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "invalid value", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnCorrectValue ) {
    byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 1 ), _alien );
    byteArrayPrimitives::alienUnsignedByteAtPut( smiOopFromValue( 255 ), smiOopFromValue( 2 ), _alien );

    Oop                     result = byteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->is_smi() ) << "should be smi_t";
    EXPECT_EQ( 65535, SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAt( _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 8 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnCorrectValue ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "should be smi_t";
    EXPECT_EQ( 65535, SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienUnsignedShortAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "should be smi_t";
    EXPECT_EQ( 65535, SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenValueNotSMI ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( _alien, smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 8 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenValueTooSmall ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "value too small", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedShortAtPutShouldReturnMarkedSymbolWhenValueTooBig ) {
    Oop result = byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65536 ), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "value too small", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnCorrectValue ) {
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), _alien );

    Oop                  result = byteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), _alien );
    EXPECT_TRUE( result->is_smi() ) << "should be smi_t";
    EXPECT_EQ( -1, SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienSignedShortAt( _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = byteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 8 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnCorrectValue ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "should be smi_t";
    EXPECT_EQ( -1, SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( -1 ), smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienSignedShortAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "should be smi_t";
    EXPECT_EQ( -1, SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenValueNotSMI ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( _alien, smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 8 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenValueTooSmall ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( -32769 ), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "value too small", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedShortAtPutShouldReturnMarkedSymbolWhenValueTooBig ) {
    Oop result = byteArrayPrimitives::alienSignedShortAtPut( smiOopFromValue( 32768 ), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "value too small", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnCorrectValue ) {
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 1 ), _alien );
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 65535 ), smiOopFromValue( 3 ), _alien );

    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), _alien );

    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnSmallIntegerWhenResultSmall ) {
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    byteArrayPrimitives::alienUnsignedShortAtPut( smiOopFromValue( 0 ), smiOopFromValue( 3 ), _alien );

    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "Should be small integer";
    EXPECT_EQ( 1, SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtShouldReturnMarkedSymbolWhenIndexTooBig ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 6 ), _alien );

    checkMarkedSymbol( "index invalid", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnCorrectValue ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAtPut( _largeUnsignedInteger, smiOopFromValue( 1 ), _alien );

    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienUnsignedLongAtPut( _largeUnsignedInteger, smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), _alien );

    checkLargeUnsigned( result, 0xFFFFFFFF );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenValueTooLarge ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAtPut( _veryLargeUnsignedInteger, smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "invalid value", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldSetCorrectValueWithSMI ) {
    byteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienUnsignedLongAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "Should be small integer";
    EXPECT_TRUE( 1 == SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenValueNotInteger ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAtPut( Universe::byteArrayKlassObj(), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienUnsignedLongAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = byteArrayPrimitives::alienUnsignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 6 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnCorrectValue ) {
    byteArrayPrimitives::alienUnsignedLongAtPut( _largeUnsignedInteger, smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_smi() ) << "Should be integer";
    EXPECT_EQ( -1, SMIOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnLargeInteger ) {
    byteArrayPrimitives::alienUnsignedLongAtPut( _largeUnsignedInteger2, smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), _alien );

    checkLargeInteger( result, 1 << 30 );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienSignedLongAt( _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 6 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


//TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnCorrectValue ) {
//    Oop result = byteArrayPrimitives::alienSignedLongAtPut( largeSignedInteger, smiOopFromValue( 1 ), alien );
//
//    checkLargeInteger( result, -1 << 31 );
//}
//

//TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldSetCorrectValue ) {
//    byteArrayPrimitives::alienSignedLongAtPut( largeSignedInteger, smiOopFromValue( 1 ), alien );
//    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), alien );
//
//    checkLargeInteger( result, -1 << 31 );
//}
//

TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutWithSMIShouldSetCorrectValue ) {
    byteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienSignedLongAt( smiOopFromValue( 1 ), _alien );

    checkSmallInteger( result, 1 );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenValueNotInteger ) {
    Oop result = byteArrayPrimitives::alienSignedLongAtPut( Universe::byteArrayKlassObj(), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = byteArrayPrimitives::alienSignedLongAtPut( smiOopFromValue( 1 ), smiOopFromValue( 6 ), _alien );

    checkMarkedSymbol( "invalid index", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienSignedLongAtPutShouldReturnMarkedSymbolWhenValueTooLarge ) {
    Oop result = byteArrayPrimitives::alienSignedLongAtPut( _veryLargeUnsignedInteger, smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "invalid value", result, vmSymbols::argument_is_invalid() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnValue ) {
    Oop result = byteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_double() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienDoubleAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_double() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result )->value() ) << "wrong value";
    EXPECT_EQ( 1.625, ( ( double * ) ( _alien->bytes() + 4 ) )[ 0 ] ) << "value not set";
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienDoubleAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienDoubleAtPut( _doubleValue, _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienDoubleAt( _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenValueNotDouble ) {
    Oop result = byteArrayPrimitives::alienDoubleAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienDoubleAt( smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = byteArrayPrimitives::alienDoubleAtPut( _doubleValue, smiOopFromValue( 2 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienDoubleAtShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = byteArrayPrimitives::alienDoubleAt( smiOopFromValue( 2 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldSetCorrectValue ) {
    byteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 1 ), _alien );
    Oop result = byteArrayPrimitives::alienFloatAt( smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_double() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result )->value() ) << "wrong value";
    EXPECT_EQ( 1.625F, ( ( float * ) ( _alien->bytes() + 4 ) )[ 0 ] ) << "value not set";
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldSetCorrectValueAt2 ) {
    byteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 2 ), _alien );
    Oop result = byteArrayPrimitives::alienFloatAt( smiOopFromValue( 2 ), _alien );

    EXPECT_TRUE( result->is_double() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result )->value() ) << "wrong value";
    EXPECT_EQ( 1.625F, ( ( float * ) ( _alien->bytes() + 5 ) )[ 0 ] ) << "value not set";
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnCorrectValue ) {
    Oop result = byteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 1 ), _alien );

    EXPECT_TRUE( result->is_double() ) << "should be double";
    EXPECT_EQ( 1.625, DoubleOop( result )->value() ) << "wrong value";
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtShouldReturnMarkedSymbolWhenNotAlien ) {
    Oop result = byteArrayPrimitives::alienFloatAt( smiOopFromValue( 1 ), smiOopFromValue( 0 ) );

    checkMarkedSymbol( "wrong type", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienFloatAtPut( _doubleValue, _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::first_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtShouldReturnMarkedSymbolWhenIndexNotSMI ) {
    Oop result = byteArrayPrimitives::alienFloatAt( _alien, _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenValueNotDouble ) {
    Oop result = byteArrayPrimitives::alienFloatAtPut( smiOopFromValue( 1 ), smiOopFromValue( 1 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::second_argument_has_wrong_type() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienFloatAt( smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenIndexTooSmall ) {
    Oop result = byteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 0 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtPutShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = byteArrayPrimitives::alienFloatAtPut( _doubleValue, smiOopFromValue( 6 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}


TEST_F( DirectAlienPrimitivesTests, alienFloatAtShouldReturnMarkedSymbolWhenIndexTooLarge ) {
    Oop result = byteArrayPrimitives::alienFloatAt( smiOopFromValue( 6 ), _alien );

    checkMarkedSymbol( "wrong type", result, vmSymbols::index_not_valid() );
}
