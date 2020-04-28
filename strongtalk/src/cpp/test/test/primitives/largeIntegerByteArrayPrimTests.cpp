
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/oops/smiOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/memory/vmSymbols.hpp"

#include <gtest/gtest.h>

extern "C" int expansion_count;

typedef Oop(__CALLING_CONVENTION divfn)( Oop, Oop );


#define ASSERT_TRUE_M( expression, message ) \
    EXPECT_TRUE( expression ) << message;

#define ASSERT_EQ_M( expected, actual, message ) \
    EXPECT_EQ( expected, actual ) << message;

#define ASSERT_EQ_M2( expected, actual, prefix ) \
  ASSERT_EQ_M( expected, actual, report(prefix, expected, actual ) )

#define ASSERT_EQ_MH( expected, actual, prefix ) \
  ASSERT_EQ_M( expected, actual, reportHex(prefix, expected, actual ) )

#define ASSERT_EQ_MS( expected, actual, prefix ) \
  ASSERT_TRUE_M( (const char*)!strcmp( expected, actual ), report( prefix, expected, actual ) )

#define CHECK_DIV_WITH_SMI( fn, xstring, ystring, expected ) \
  IntegerOps::string_to_Integer(xstring, 16, as_Integer(x)); \
  IntegerOps::string_to_Integer(ystring, 16, as_Integer(y)); \
  SMIOop result = SMIOop(byteArrayPrimitives::fn(y->as_oop(), x->as_oop())); \
  ASSERT_TRUE_M(result->is_smi(), "Should be small integer"); \
  ASSERT_EQ_MH(expected, result->value(), "Wrong result")

#define CHECK_DIV_WITH_LRG( fn, xstring, ystring, expected ) \
  IntegerOps::string_to_Integer(xstring, 16, as_Integer(x)); \
  IntegerOps::string_to_Integer(ystring, 16, as_Integer(y)); \
  ByteArrayOop result = ByteArrayOop(byteArrayPrimitives::fn(y->as_oop(), x->as_oop())); \
  ASSERT_TRUE_M(result->is_byteArray(), "Should be byteArray"); \
  ASSERT_EQ_MS(expected, as_Hex(result->number()), "Wrong result")

#define CHECK_ARG_TYPE( fn ) \
  IntegerOps::string_to_Integer("123456781234567812345678", 16, as_Integer(x)); \
  SymbolOop result = unmarkSymbol(byteArrayPrimitives::fn(smiOopFromValue(10), x->as_oop())); \
  ASSERT_TRUE_M(result->is_symbol(), "Should be symbol"); \
  ASSERT_EQ_MS(vmSymbols::first_argument_has_wrong_type()->chars(), result->chars(), "Wrong result")

#define CHECK_ARG_TYPE2( fn ) \
  IntegerOps::string_to_Integer("123456781234567812345678", 16, as_Integer(x)); \
  SymbolOop result = unmarkSymbol(byteArrayPrimitives::fn(x->as_oop(), x->as_oop())); \
  ASSERT_TRUE_M(result->is_symbol(), "Should be symbol"); \
  ASSERT_EQ_MS(vmSymbols::first_argument_has_wrong_type()->chars(), result->chars(), "Wrong result")

#define CHECK_INVALID( fn, x, y, errorSymbol ) \
  SymbolOop result = unmarkSymbol(byteArrayPrimitives::fn(y->as_oop(), x->as_oop())); \
  ASSERT_TRUE_M(result->is_symbol(), "Should be symbol"); \
  ASSERT_EQ_MS(vmSymbols::errorSymbol()->chars(), result->chars(), "Wrong result")

#define CHECK_X_INVALID( fn ) \
  ((Digit*)ByteArrayOop(x->as_oop())->bytes())[0] = 1; \
  IntegerOps::string_to_Integer("123456781234567812345678", 16, as_Integer(y)); \
  CHECK_INVALID(fn, x, y, argument_is_invalid)

#define CHECK_Y_INVALID( fn ) \
  ((Digit*)ByteArrayOop(y->as_oop())->bytes())[0] = 1; \
  IntegerOps::string_to_Integer("123456781234567812345678", 16, as_Integer(x)); \
  CHECK_INVALID(fn, x, y, argument_is_invalid)

#define CHECK_Y_ZERO( fn ) \
  IntegerOps::string_to_Integer("123456781234567812345678", 16, as_Integer(x)); \
  CHECK_INVALID(fn, x, y, division_by_zero)


class LargeIntegerByteArrayPrimsTests : public ::testing::Test {

    protected:
        void SetUp() override {
            rm = new HeapResourceMark();
            x  = new PersistentHandle( oopFactory::new_byteArray( 24 ) );
            y  = new PersistentHandle( oopFactory::new_byteArray( 24 ) );
        }


        void TearDown() override {
            delete x;
            delete y;
            delete rm;
            rm = nullptr;
        }


        HeapResourceMark * rm;
        PersistentHandle * x, * y;
        char                  resultString[100];


        char message[100];


        char * reportHex( const char * prefix, int expected, int actual ) {
            sprintf( message, "%s. Expected: 0x%x, but was: 0x%x", prefix, expected, actual );
            return message;
        }


        char * report( const char * prefix, int expected, int actual ) {
            sprintf( message, "%s. Expected: %d, but was: %d", prefix, expected, actual );
            return message;
        }


        char * report( const char * prefix, const char * expected, const char * actual ) {
            sprintf( message, "%s. Expected: %s, but was: %s", prefix, expected, actual );
            return message;
        }


        char * as_Hex( Integer & number ) {
            IntegerOps::Integer_to_string( number, 16, resultString );
            return resultString;
        }


        Integer & as_Integer( PersistentHandle * handle ) {
            return ByteArrayOop( handle->as_oop() )->number();
        }


};


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerQuoShouldReturnCorrectResultForTwoPositive ) {
    CHECK_DIV_WITH_LRG( largeIntegerQuo, "123456781234567812345678", "1234567812345678", "100000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerQuoShouldReturnCorrectResultForTwoNegative ) {
    CHECK_DIV_WITH_LRG( largeIntegerQuo, "-123456781234567812345678", "-1234567812345678", "100000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerQuoShouldReturnCorrectResultForNegativeDivisor ) {
    CHECK_DIV_WITH_LRG( largeIntegerQuo, "123456781234567812345678", "-1234567812345678", "-100000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerQuoShouldReturnCorrectResultForNegativeDividend ) {
    CHECK_DIV_WITH_LRG( largeIntegerQuo, "-123456781234567812345678", "1234567812345678", "-100000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerDivShouldReturnCorrectResultForTwoPositive ) {
    CHECK_DIV_WITH_LRG( largeIntegerDiv, "123456781234567812345678", "1234567812345678", "100000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerDivShouldReturnCorrectResultForTwoNegative ) {
    CHECK_DIV_WITH_LRG( largeIntegerDiv, "-123456781234567812345678", "-1234567812345678", "100000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerDivShouldReturnCorrectResultForOneNegative ) {
    CHECK_DIV_WITH_LRG( largeIntegerDiv, "123456781234567812345678", "-1234567812345678", "-100000001" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerModShouldReturnCorrectResultForTwoPositive ) {
    CHECK_DIV_WITH_SMI( largeIntegerMod, "123456781234567812345678", "1234567812345678", 0x12345678 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerModShouldReturnCorrectResultForTwoNegative ) {
    CHECK_DIV_WITH_SMI( largeIntegerMod, "-123456781234567812345678", "-1234567812345678", -0x12345678 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerModShouldReturnCorrectResultForNegativeDivisor ) {
    CHECK_DIV_WITH_LRG( largeIntegerMod, "123456781234567812345678", "-1234567812345678", "-1234567800000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerAndShouldReturnCorrectResult ) {
    CHECK_DIV_WITH_LRG( largeIntegerAnd, "100000000000000000000000", "-1", "100000000000000000000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerAndShouldReturnSmallInteger ) {
    CHECK_DIV_WITH_SMI( largeIntegerAnd, "-100000000000000000000001", "1", 1 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerOrShouldReturnCorrectResult ) {
    CHECK_DIV_WITH_LRG( largeIntegerOr, "100000000000000000000000", "1", "100000000000000000000001" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerOrShouldReturnSmallInteger ) {
    CHECK_DIV_WITH_SMI( largeIntegerOr, "2", "1", 3 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerXorShouldReturnCorrectResult ) {
    CHECK_DIV_WITH_LRG( largeIntegerXor, "100000000000000000000001", "1", "100000000000000000000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerShiftShouldReturnSmallInteger ) {
    IntegerOps::string_to_Integer( "2", 16, as_Integer( x ) );
    SMIOop result = SMIOop( byteArrayPrimitives::largeIntegerShift( smiOopFromValue( -1 ), x->as_oop() ) );
    EXPECT_TRUE( result->is_smi() ) << "Should be small integer";;
    ASSERT_EQ_MH( 1, result->value(), "Wrong result" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerShiftShouldReturnLargeInteger ) {
    IntegerOps::string_to_Integer( "1", 16, as_Integer( x ) );
    ByteArrayOop result = ByteArrayOop( byteArrayPrimitives::largeIntegerShift( smiOopFromValue( 32 ), x->as_oop() ) );
    EXPECT_TRUE( result->is_byteArray() ) << "Should be byteArray";;
    ASSERT_EQ_MS( "100000000", as_Hex( result->number() ), "Wrong result" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerShiftWithUnderflowShouldReturnSmallInteger ) {
    IntegerOps::string_to_Integer( "100000000", 16, as_Integer( x ) );
    SMIOop result = SMIOop( byteArrayPrimitives::largeIntegerShift( smiOopFromValue( -4 ), x->as_oop() ) );
    EXPECT_TRUE( result->is_smi() ) << "Should be small integer";;
    ASSERT_EQ_MH( 0x10000000, result->value(), "Wrong result" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerXorShouldReturnSmallInteger ) {
    CHECK_DIV_WITH_SMI( largeIntegerXor, "3", "1", 2 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerModShouldReturnCorrectResultForNegativeDividend ) {
    CHECK_DIV_WITH_LRG( largeIntegerMod, "-123456781234567812345678", "1234567812345678", "1234567800000000" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerRemShouldReturnCorrectResultForTwoPositive ) {
    CHECK_DIV_WITH_SMI( largeIntegerRem, "123456781234567812345678", "1234567812345678", 0x12345678 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerRemShouldReturnCorrectResultForTwoNegative ) {
    CHECK_DIV_WITH_SMI( largeIntegerRem, "-123456781234567812345678", "-1234567812345678", -0x12345678 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerRemShouldReturnCorrectResultForNegativeDivisor ) {
    CHECK_DIV_WITH_SMI( largeIntegerRem, "123456781234567812345678", "-1234567812345678", 0x12345678 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerRemShouldReturnCorrectResultForNegativeDividend ) {
    CHECK_DIV_WITH_SMI( largeIntegerRem, "-123456781234567812345678", "1234567812345678", -0x12345678 );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerShiftShouldReportErrorWhenArgWrongType ) {
    CHECK_ARG_TYPE2( largeIntegerShift );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerXorShouldReportErrorWhenArgWrongType ) {
    CHECK_ARG_TYPE( largeIntegerXor );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerOrShouldReportErrorWhenArgWrongType ) {
    CHECK_ARG_TYPE( largeIntegerOr );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerAndShouldReportErrorWhenArgWrongType ) {
    CHECK_ARG_TYPE( largeIntegerAnd );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerRemShouldReportErrorWhenArgWrongType ) {
    CHECK_ARG_TYPE( largeIntegerRem );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerModShouldReportErrorWhenArgWrongType ) {
    CHECK_ARG_TYPE( largeIntegerMod );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerDivShouldReportErrorWhenArgWrongType ) {
    CHECK_ARG_TYPE( largeIntegerDiv );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerQuoShouldReportErrorWhenArgWrongType ) {
    CHECK_ARG_TYPE( largeIntegerQuo );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerShiftShouldReportErrorWhenXInvalid ) {

    ( ( Digit * ) ByteArrayOop( x->as_oop() )->bytes() )[ 0 ] = 1;
    SymbolOop result = unmarkSymbol( byteArrayPrimitives::largeIntegerShift( 0, x->as_oop() ) );
    EXPECT_TRUE( result->is_symbol() ) << "Should be symbol";;
    ASSERT_EQ_MS( vmSymbols::argument_is_invalid()->chars(), result->chars(), "Wrong result" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerAndShouldReportErrorWhenXInvalid ) {
    CHECK_X_INVALID( largeIntegerAnd );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerXorShouldReportErrorWhenXInvalid ) {
    CHECK_X_INVALID( largeIntegerXor );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerOrShouldReportErrorWhenXInvalid ) {
    CHECK_X_INVALID( largeIntegerOr );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerQuoShouldReportErrorWhenXInvalid ) {
    CHECK_X_INVALID( largeIntegerQuo );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerDivShouldReportErrorWhenXInvalid ) {
    CHECK_X_INVALID( largeIntegerDiv );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerRemShouldReportErrorWhenXInvalid ) {
    CHECK_X_INVALID( largeIntegerRem );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerModShouldReportErrorWhenXInvalid ) {
    CHECK_X_INVALID( largeIntegerMod );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerAndShouldReportErrorWhenYInvalid ) {
    CHECK_Y_INVALID( largeIntegerAnd );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerXorShouldReportErrorWhenYInvalid ) {
    CHECK_Y_INVALID( largeIntegerXor );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerOrShouldReportErrorWhenYInvalid ) {
    CHECK_Y_INVALID( largeIntegerOr );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerModShouldReportErrorWhenYInvalid ) {
    CHECK_Y_INVALID( largeIntegerMod );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerRemShouldReportErrorWhenYInvalid ) {
    CHECK_Y_INVALID( largeIntegerRem );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerDivShouldReportErrorWhenYInvalid ) {
    CHECK_Y_INVALID( largeIntegerDiv );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerQuoShouldReportErrorWhenYInvalid ) {
    CHECK_Y_INVALID( largeIntegerQuo );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerDivShouldReportErrorWhenYZero ) {
    CHECK_Y_ZERO( largeIntegerDiv );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerQuoShouldReportErrorWhenYZero ) {
    CHECK_Y_ZERO( largeIntegerQuo );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerRemShouldReportErrorWhenYZero ) {
    CHECK_Y_ZERO( largeIntegerRem );
}


TEST_F( LargeIntegerByteArrayPrimsTests, largeIntegerModShouldReportErrorWhenYZero ) {
    CHECK_Y_ZERO( largeIntegerMod );
}


TEST_F( LargeIntegerByteArrayPrimsTests, hash ) {
    IntegerOps::string_to_Integer( "-12345678ffffffff", 16, as_Integer( x ) );
    SMIOop result = SMIOop( byteArrayPrimitives::largeIntegerHash( x->as_oop() ) );
    EXPECT_TRUE( result->is_smi() ) << "Should be SMI";;
    ASSERT_EQ_M2( ( 0x12345678 ^ 0xffffffff ^ -2 ) >> 2, result->value(), "Wrong hash" );
}


TEST_F( LargeIntegerByteArrayPrimsTests, unsignedCmpShouldBeLessThanZeroWhenFirstSmaller ) {
    IntegerOps::string_to_Integer( "00000001", 16, as_Integer( x ) );
    IntegerOps::string_to_Integer( "ffffffff", 16, as_Integer( y ) );
    int result = IntegerOps::unsigned_cmp( as_Integer( x ), as_Integer( y ) );
    EXPECT_TRUE( result < 0 ) << "Wrong cmp";;
}


TEST_F( LargeIntegerByteArrayPrimsTests, unsignedCmpShouldBeGreaterThanZeroWhenFirstSmaller ) {
    IntegerOps::string_to_Integer( "ffffffff", 16, as_Integer( x ) );
    IntegerOps::string_to_Integer( "00000001", 16, as_Integer( y ) );
    int result = IntegerOps::unsigned_cmp( as_Integer( x ), as_Integer( y ) );
    EXPECT_TRUE( result > 0 ) << "Wrong cmp";;
}


TEST_F( LargeIntegerByteArrayPrimsTests, unsignedCmpShouldBeZeroWhenEqual ) {
    IntegerOps::string_to_Integer( "ffffffff", 16, as_Integer( x ) );
    IntegerOps::string_to_Integer( "ffffffff", 16, as_Integer( y ) );
    int result = IntegerOps::unsigned_cmp( as_Integer( x ), as_Integer( y ) );
    EXPECT_TRUE( result == 0 ) << "Wrong cmp";;
}

