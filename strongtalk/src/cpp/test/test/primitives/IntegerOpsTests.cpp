
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/ResourceArea.hpp"

#include <gtest/gtest.h>


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


#define CHECK_SIZE( op, first, second, expected )\
  IntegerOps::string_to_Integer(first, 16, *x);\
  IntegerOps::string_to_Integer(second, 16, *y);\
  ASSERT_EQ_M2(expected, IntegerOps::op(*x, *y), "Wrong size")

#define CHECK_AND_SIZE( first, second, expected )\
  CHECK_SIZE(and_result_size_in_bytes, first, second, expected)

#define CHECK_OR_SIZE( first, second, expected )\
  CHECK_SIZE(or_result_size_in_bytes, first, second, expected)

#define CHECK_XOR_SIZE( first, second, expected )\
  CHECK_SIZE(xor_result_size_in_bytes, first, second, expected)

#define CHECK_OP( op, first, second, expected )\
  IntegerOps::string_to_Integer(first, 16, *x);\
  IntegerOps::string_to_Integer(second, 16, *y);\
  IntegerOps::op(*x, *y, *z);\
  char result[100];\
  IntegerOps::Integer_to_string(*z, 16, result);\
  ASSERT_TRUE_M(z->is_valid(), "Not a valid Integer");\
  ASSERT_EQ_MS(expected, result , "Wrong result")

#define CHECK_AND( first, second, expected )\
  CHECK_OP(And, first, second, expected)

#define CHECK_OR( first, second, expected )\
  CHECK_OP(Or, first, second, expected)

#define CHECK_XOR( first, second, expected )\
  CHECK_OP(Xor, first, second, expected)

#define CHECK_ASH_SIZE( first, second, expected )\
  IntegerOps::string_to_Integer(first, 16, *x);\
  ASSERT_EQ_M2(expected, IntegerOps::ash_result_size_in_bytes(*x, second) , "Wrong result")

#define CHECK_ASH( first, second, expected )\
  IntegerOps::string_to_Integer(first, 16, *x);\
  IntegerOps::ash(*x, second, *z);\
  char result[100];\
  IntegerOps::Integer_to_string(*z, 16, result);\
  ASSERT_TRUE_M(z->is_valid(), "Not a valid Integer");\
  ASSERT_EQ_MS(expected, result , "Wrong result")


class IntegerOpsTests : public ::testing::Test {

    protected:
        void SetUp() override {
            rm = new HeapResourceMark();
            x  = ( Integer * ) NEW_RESOURCE_ARRAY( Digit, 5 );
            y  = ( Integer * ) NEW_RESOURCE_ARRAY( Digit, 5 );
            z  = ( Integer * ) NEW_RESOURCE_ARRAY( Digit, 5 );
        }


        void TearDown() override {
            delete rm;
            rm = nullptr;
        }


        HeapResourceMark * rm;
        Integer          * x, * y, * z;

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


};


TEST_F( IntegerOpsTests, largeIntegerDivShouldReturnZeroWhenYLargerThanX ) {
    IntegerOps::int_to_Integer( 1, *x );
    IntegerOps::int_to_Integer( 2, *y );
    IntegerOps::Div( *x, *y, *z );

    bool_t ok;
    int    result = z->as_int( ok );
    EXPECT_TRUE( ok ) << "invalid Integer";
    EXPECT_EQ( 0, result ) << "wrong result";
}


TEST_F( IntegerOpsTests, largeIntegerDivShouldReturnZeroWhenAbsYLargerThanX ) {
    IntegerOps::int_to_Integer( 1, *x );
    IntegerOps::int_to_Integer( -2, *y );
    IntegerOps::Div( *x, *y, *z );

    bool_t ok;
    int    result = z->as_int( ok );
    EXPECT_TRUE( ok ) << "invalid Integer";
    EXPECT_EQ( -1, result ) << "wrong result";
}


TEST_F( IntegerOpsTests, largeIntegerDivShouldReturnMinus1WhenYLargerThanAbsX ) {
    IntegerOps::int_to_Integer( -1, *x );
    IntegerOps::int_to_Integer( 2, *y );
    IntegerOps::Div( *x, *y, *z );

    bool_t ok;
    int    result = z->as_int( ok );
    EXPECT_TRUE( ok ) << "invalid Integer";
    EXPECT_EQ( -1, result ) << "wrong result";
}


TEST_F( IntegerOpsTests, largeIntegerDivShouldReturnZeroWhenAbsYLargerThanAbsX ) {
    IntegerOps::int_to_Integer( -1, *x );
    IntegerOps::int_to_Integer( -2, *y );
    IntegerOps::Div( *x, *y, *z );

    bool_t ok;
    int    result = z->as_int( ok );
    EXPECT_TRUE( ok ) << "invalid Integer";
    EXPECT_EQ( 0, result ) << "wrong result";
}


TEST_F( IntegerOpsTests, largeIntegerDivShouldReturnM1WhenAbsYEqualsX ) {
    IntegerOps::int_to_Integer( 2, *x );
    IntegerOps::int_to_Integer( -2, *y );
    IntegerOps::Div( *x, *y, *z );

    bool_t ok;
    int    result = z->as_int( ok );
    EXPECT_TRUE( ok ) << "invalid Integer";
    EXPECT_EQ( -1, result ) << "wrong result";
}


TEST_F( IntegerOpsTests, xpyShouldHandleAllBitsPlus1 ) {
    Digit x = 0xffffffff;
    Digit y = 1;
    Digit c = 0;
    EXPECT_EQ( 0, IntegerOps::xpy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 1, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xpyShouldHandle1PlusAllBits ) {
    Digit x = 1;
    Digit y = 0xffffffff;
    Digit c = 0;
    EXPECT_EQ( 0, IntegerOps::xpy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 1, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xpyShouldHandleAllBitsPlusAllBitsPlusAllBits ) {
    Digit x = 0xffffffff;
    Digit y = 0xffffffff;
    Digit c = 0xffffffff;
    EXPECT_EQ( 0xfffffffd, IntegerOps::xpy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 2, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xmyShouldHandle0Minus1 ) {
    Digit x = 0;
    Digit y = 1;
    Digit c = 0;
    EXPECT_EQ( 0xffffffff, IntegerOps::xmy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 1, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xmyShouldHandle0Minus0 ) {
    Digit x = 0;
    Digit y = 0;
    Digit c = 0;
    EXPECT_EQ( 0, IntegerOps::xmy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 0, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xmyShouldHandle0Minus0Carry1 ) {
    Digit x = 0;
    Digit y = 0;
    Digit c = 1;
    EXPECT_EQ( 0xffffffff, IntegerOps::xmy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 1, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xmyShouldHandle0MinusAllOnesCarry1 ) {
    Digit x = 0;
    Digit y = 0xffffffff;
    Digit c = 1;
    EXPECT_EQ( 0,
               IntegerOps::xmy( x, y, c
               ) ) << "Wrong total";
    EXPECT_EQ( 1, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, axpyShouldHandleAllOnes ) {
    Digit a = 0xffffffff;
    Digit x = 0xffffffff;
    Digit y = 0xffffffff;
    Digit c = 0xffffffff;
    EXPECT_EQ( 0xffffffff, IntegerOps::axpy( a, x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 0xffffffff, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, axpyShouldHandleAllZeroes ) {
    Digit a = 0;
    Digit x = 0;
    Digit y = 0;
    Digit c = 0;
    EXPECT_EQ( 0, IntegerOps::axpy( a, x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 0, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, axpyWithPieces ) {
    Digit a = 0xffff;
    Digit x = 0x10000;
    Digit y = 0xff00;
    Digit c = 0x00ff;
    EXPECT_EQ( 0xffffffff, IntegerOps::axpy( a, x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 0, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xdyWithZeroShouldBeZero ) {
    Digit x = 0;
    Digit y = 1;
    Digit c = 0;
    EXPECT_EQ( 0, IntegerOps::xdy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 0, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xdyByTwoShouldHaveZeroCarry ) {
    Digit x = 0;
    Digit y = 2;
    Digit c = 1;
    EXPECT_EQ( 0x80000000, IntegerOps::xdy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 0, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xdyByTwoShouldHaveCarryWhenOdd ) {
    Digit x = 1;
    Digit y = 2;
    Digit c = 1;
    EXPECT_EQ( 0x80000000, IntegerOps::xdy( x, y, c ) ) << "Wrong total";
    EXPECT_EQ( 1, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, xdyWithAllOnes ) {
    Digit x      = 0xffffffff;
    Digit y      = 0x80000000;
    Digit c      = 1;
    Digit result = IntegerOps::xdy( x, y, c );
    EXPECT_EQ( 3, result ) << report( "Wrong total", 3, result );
    EXPECT_EQ( 0x7fffffff, c ) << "Wrong carry";
}


TEST_F( IntegerOpsTests, qr_decompositionSimple ) {
    IntegerOps::unsigned_int_to_Integer( 0xffffffff, *x );
    IntegerOps::unsigned_int_to_Integer( 1, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0, result[ 0 ] ) << "Wrong remainder.";
    EXPECT_EQ( 0xffffffff, result[ 1 ] ) << "Wrong quotient.";
}


TEST_F( IntegerOpsTests, qr_decompositionSimpleWithRemainder ) {
    IntegerOps::unsigned_int_to_Integer( 0xffffffff, *x );
    IntegerOps::unsigned_int_to_Integer( 0x80000000, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0x7fffffff, result[ 0 ] ) << report( "Wrong remainder", 0x7fffffff, result[ 0 ] );
    EXPECT_EQ( 1, result[ 1 ] ) << report( "Wrong quotient", 1, result[ 1 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionTwoDigitNoRemainder ) {
    IntegerOps::string_to_Integer( "FFFFFFFF80000000", 16, *x );
    IntegerOps::unsigned_int_to_Integer( 0x80000000, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0, result[ 0 ] ) << report( "Wrong remainder", 0, result[ 0 ] );
    EXPECT_EQ( 0xffffffff, result[ 1 ] ) << report( "Wrong quotient word 1", 0xffffffff, result[ 1 ] );
    EXPECT_EQ( 1, result[ 2 ] ) << report( "Wrong quotient word 2", 1, result[ 2 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionTwoDigitWithRemainder ) {
    IntegerOps::string_to_Integer( "FFFFFFFFFFFFFFFF", 16, *x );
    IntegerOps::unsigned_int_to_Integer( 0x80000000, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0x7fffffff, result[ 0 ] ) << report( "Wrong remainder", 0x7fffffff, result[ 0 ] );
    EXPECT_EQ( 0xffffffff, result[ 1 ] ) << report( "Wrong quotient word 1", 0xffffffff, result[ 1 ] );
    EXPECT_EQ( 1, result[ 2 ] ) << report( "Wrong quotient word 2", 1, result[ 2 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionThreeDigitWithRemainder ) {
    IntegerOps::string_to_Integer( "FFFFFFFFFFFFFFFFFFFFFFFF", 16, *x );
    IntegerOps::string_to_Integer( "800000000000", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0xffffffff, result[ 0 ] ) << report( "Wrong remainder word 1", 0xffffffff, result[ 0 ] );
    EXPECT_EQ( 0x7fff, result[ 1 ] ) << report( "Wrong remainder word 2", 0x7fff, result[ 1 ] );
    EXPECT_EQ( 0xffffffff, result[ 2 ] ) << report( "Wrong quotient word 1", 0xffffffff, result[ 2 ] );
    EXPECT_EQ( 0x1ffff, result[ 3 ] ) << report( "Wrong quotient word 2", 0x1ffff, result[ 3 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionTwoDigitDivisorNoRemainder ) {
    IntegerOps::string_to_Integer( "800000000000000000000000", 16, *x );
    IntegerOps::string_to_Integer( "8000000000000000", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0, result[ 0 ] ) << report( "Wrong remainder low word", 0, result[ 0 ] );
    EXPECT_EQ( 0, result[ 1 ] ) << report( "Wrong remainder high word", 0, result[ 1 ] );
    EXPECT_EQ( 0, result[ 2 ] ) << report( "Wrong quotient low word", 0, result[ 2 ] );
    EXPECT_EQ( 1, result[ 3 ] ) << report( "Wrong quotient high word", 1, result[ 3 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionScaledTwoDigitDivisorNoRemainder ) {
    IntegerOps::string_to_Integer( "800000000000000000000000", 16, *x );
    IntegerOps::string_to_Integer( "0800000000000000", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0, result[ 0 ] ) << report( "Wrong remainder low word", 0, result[ 0 ] );
    EXPECT_EQ( 0, result[ 1 ] ) << report( "Wrong remainder high word", 0, result[ 1 ] );
    EXPECT_EQ( 0, result[ 2 ] ) << report( "Wrong quotient low word", 0, result[ 2 ] );
    EXPECT_EQ( 0x10, result[ 3 ] ) << report( "Wrong quotient high word", 0x10, result[ 3 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionTwoDigitDivisorSmallRemainder ) {
    IntegerOps::string_to_Integer( "800000000000000000000001", 16, *x );
    IntegerOps::string_to_Integer( "8000000000000000", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 1, result[ 0 ] ) << report( "Wrong remainder low word", 1, result[ 0 ] );
    EXPECT_EQ( 0, result[ 1 ] ) << report( "Wrong remainder high word", 0, result[ 1 ] );
    EXPECT_EQ( 0, result[ 2 ] ) << report( "Wrong quotient low word", 0, result[ 2 ] );
    EXPECT_EQ( 1, result[ 3 ] ) << report( "Wrong quotient high word", 1, result[ 3 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionTwoDigitDivisorSmallAllOnesRemainder ) {
    IntegerOps::string_to_Integer( "8000000000000000FFFFFFFF", 16, *x );
    IntegerOps::string_to_Integer( "8000000000000000", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0xffffffff, result[ 0 ] ) << report( "Wrong remainder low word", 0xffffffff, result[ 0 ] );
    EXPECT_EQ( 0, result[ 1 ] ) << report( "Wrong remainder high word", 0, result[ 1 ] );
    EXPECT_EQ( 0, result[ 2 ] ) << report( "Wrong quotient low word", 0, result[ 2 ] );
    EXPECT_EQ( 1, result[ 3 ] ) << report( "Wrong quotient high word", 1, result[ 3 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionTwoDigitDivisorTwoDigitRemainder ) {
    IntegerOps::string_to_Integer( "8000000000007FFFFFFFFFFF", 16, *x );
    IntegerOps::string_to_Integer( "8000000000000000", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0xffffffff, result[ 0 ] ) << report( "Wrong remainder low word", 0xffffffff, result[ 0 ] );
    EXPECT_EQ( 0x7fff, result[ 1 ] ) << report( "Wrong remainder high word", 0x7fff, result[ 1 ] );
    EXPECT_EQ( 0, result[ 2 ] ) << report( "Wrong quotient low word", 0, result[ 2 ] );
    EXPECT_EQ( 1, result[ 3 ] ) << report( "Wrong quotient high word", 1, result[ 3 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionTwoNonZeroDigitDivisorTwoDigitRemainder ) {
    IntegerOps::string_to_Integer( "FFFFFFFFFFFFFFF1FFFFFFFF", 16, *x );
    IntegerOps::string_to_Integer( "0FFFFFFFFFFFFFFF", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0xffffffff, result[ 0 ] ) << report( "Wrong remainder low word", 0xffffffff, result[ 0 ] );
    EXPECT_EQ( 0x1, result[ 1 ] ) << report( "Wrong remainder high word", 0x1, result[ 1 ] );
    EXPECT_EQ( 0, result[ 2 ] ) << report( "Wrong quotient low word", 0, result[ 2 ] );
    EXPECT_EQ( 0x10, result[ 3 ] ) << report( "Wrong quotient high word", 0x10, result[ 3 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionFirstDigitsMatch ) {
    IntegerOps::string_to_Integer( "FFFFFFFF0000000000000000", 16, *x );
    IntegerOps::string_to_Integer( "FFFFFFFF00000000", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0x00000000, result[ 0 ] ) << report( "Wrong remainder low word", 0x00000000, result[ 0 ] );
    EXPECT_EQ( 0x0, result[ 1 ] ) << report( "Wrong remainder high word", 0x0, result[ 1 ] );
    EXPECT_EQ( 0, result[ 2 ] ) << report( "Wrong quotient low word", 0, result[ 2 ] );
    EXPECT_EQ( 0x1, result[ 3 ] ) << report( "Wrong quotient high word", 0x1, result[ 3 ] );
}


TEST_F( IntegerOpsTests, qr_decompositionComplexCase ) {
    IntegerOps::string_to_Integer( "121FA00AD77D7422347E9A0F6729E011", 16, *x );
    IntegerOps::string_to_Integer( "FEDCBA9876543210", 16, *y );

    Digit * result = IntegerOps::qr_decomposition( *x, *y );
    EXPECT_EQ( 0x11111111, result[ 0 ] ) << report( "Wrong remainder low word", 0x11111111, result[ 0 ] );
    EXPECT_EQ( 0x11111111, result[ 1 ] ) << report( "Wrong remainder high word", 0x11111111, result[ 1 ] );
    EXPECT_EQ( 0x9ABCDEF0, result[ 2 ] ) << report( "Wrong quotient low word", 0x9ABCDEF0, result[ 2 ] );
    EXPECT_EQ( 0x12345678, result[ 3 ] ) << report( "Wrong quotient high word", 0x12345678, result[ 3 ] );
}


TEST_F( IntegerOpsTests, unsignedQuo ) {
    IntegerOps::string_to_Integer( "121FA00AD77D7422347E9A0F6729E011", 16, *x );
    IntegerOps::string_to_Integer( "FEDCBA9876543210", 16, *y );

    IntegerOps::unsigned_quo( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "123456789abcdef0", result ) << "Wrong quotient";
}


TEST_F( IntegerOpsTests, unsignedRem ) {
    IntegerOps::string_to_Integer( "121FA00AD77D7422347E9A0F6729E011", 16, *x );
    IntegerOps::string_to_Integer( "FEDCBA9876543210", 16, *y );

    IntegerOps::unsigned_rem( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "1111111111111111", result ) << "Wrong remainder";
}


TEST_F( IntegerOpsTests, unsignedRemWhenXIsShortThanY ) {
    IntegerOps::string_to_Integer( "12345678", 16, *x );
    IntegerOps::string_to_Integer( "FEDCBA9876543210", 16, *y );

    IntegerOps::unsigned_rem( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "12345678", result ) << "Wrong remainder";
}


TEST_F( IntegerOpsTests, unsignedRemWithNoRemainder ) {
    IntegerOps::string_to_Integer( "12345678", 16, *x );
    IntegerOps::string_to_Integer( "12345678", 16, *y );

    IntegerOps::unsigned_rem( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_TRUE( z->is_zero() ) << "Remainder should be zero";
}


TEST_F( IntegerOpsTests, divWithLargeDividendNegDivisorAndRemainder ) {
    IntegerOps::string_to_Integer( "1234567809abcdef", 16, *x );
    IntegerOps::string_to_Integer( "-12345678", 16, *y );

    IntegerOps::Div( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "-100000001", result ) << "Wrong quotient";
}


TEST_F( IntegerOpsTests, modWithLargeNegDividendAndRemainder ) {
    IntegerOps::string_to_Integer( "-1234567809abcdef", 16, *x );
    IntegerOps::string_to_Integer( "12345678", 16, *y );

    IntegerOps::Mod( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "8888889", result ) << "Wrong remainder";
}


TEST_F( IntegerOpsTests, modWithLargeNegDividendNegDivisorAndRemainder ) {
    IntegerOps::string_to_Integer( "-1234567809abcdef", 16, *x );
    IntegerOps::string_to_Integer( "-12345678", 16, *y );

    IntegerOps::Mod( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "-9abcdef", result ) << "Wrong remainder";
}


TEST_F( IntegerOpsTests, modWithLargeDividendNegDivisorAndRemainder ) {
    IntegerOps::string_to_Integer( "1234567809abcdef", 16, *x );
    IntegerOps::string_to_Integer( "-12345678", 16, *y );

    IntegerOps::Mod( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "-8888889", result ) << "Wrong remainder";
}


TEST_F( IntegerOpsTests, modWithLargeDividendNegDivisorAndNoRemainder ) {
    IntegerOps::string_to_Integer( "1234567800000000", 16, *x );
    IntegerOps::string_to_Integer( "-12345678", 16, *y );

    IntegerOps::Mod( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "0", result ) << "Wrong remainder";
}


TEST_F( IntegerOpsTests, modWithLongerDivisor ) {
    IntegerOps::string_to_Integer( "12345678", 16, *x );
    IntegerOps::string_to_Integer( "-100000000", 16, *y );

    EXPECT_TRUE( y->is_negative() ) << "Should be negative";
    IntegerOps::Mod( *x, *y, *z );
    char result[100];
    IntegerOps::Integer_to_string( *z, 16, result );

    EXPECT_EQ( "-edcba988", result ) << "Wrong remainder";
}


TEST_F( IntegerOpsTests, divResultSizeInBytesWithTwoPositiveIntegers ) {
    IntegerOps::string_to_Integer( "123456781234567812345678", 16, *x );
    IntegerOps::string_to_Integer( "12345678", 16, *y );

    int result = IntegerOps::div_result_size_in_bytes( *x, *y );

    ASSERT_EQ_M2( sizeof( int ) + ( 3 * sizeof( Digit ) ), result, "Wrong size" );
}


TEST_F( IntegerOpsTests, divResultSizeInBytesWithTwoNegativeIntegers ) {
    IntegerOps::string_to_Integer( "-123456781234567812345678", 16, *x );
    IntegerOps::string_to_Integer( "-12345678", 16, *y );

    int result = IntegerOps::div_result_size_in_bytes( *x, *y );

    ASSERT_EQ_M2( sizeof( int ) + ( 3 * sizeof( Digit ) ), result, "Wrong size" );
}


TEST_F( IntegerOpsTests, divResultSizeInBytesWithPosDividendAndNegDivisor ) {
    IntegerOps::string_to_Integer( "123456781234567812345678", 16, *x );
    IntegerOps::string_to_Integer( "-12345678", 16, *y );

    int result = IntegerOps::div_result_size_in_bytes( *x, *y );

    ASSERT_EQ_M2( sizeof( int ) + ( 4 * sizeof( Digit ) ), result, "Wrong size" );
}


TEST_F( IntegerOpsTests, modResultSizeInBytesWithTwoPositiveIntegers ) {
    IntegerOps::string_to_Integer( "123456781234567812345678", 16, *x );
    IntegerOps::string_to_Integer( "12345678", 16, *y );

    int result = IntegerOps::mod_result_size_in_bytes( *x, *y );

    ASSERT_EQ_M2( sizeof( int ) + ( 1 * sizeof( Digit ) ), result, "Wrong size" );
}


TEST_F( IntegerOpsTests, modResultSizeInBytesWithTwoNegativeIntegers ) {
    IntegerOps::string_to_Integer( "-123456781234567812345678", 16, *x );
    IntegerOps::string_to_Integer( "-1234567812345678", 16, *y );

    int result = IntegerOps::mod_result_size_in_bytes( *x, *y );

    ASSERT_EQ_M2( sizeof( int ) + ( 2 * sizeof( Digit ) ), result, "Wrong size" );
}


TEST_F( IntegerOpsTests, orWithTwoPositive ) {
    CHECK_OR( "1", "FFFFFFFF", "ffffffff" );
}


TEST_F( IntegerOpsTests, orWithFirstZero ) {
    CHECK_OR( "0", "FFFFFFFF", "ffffffff" );
}


TEST_F( IntegerOpsTests, orWithSecondNegative ) {
    CHECK_OR( "1", "-1", "-1" );
}


TEST_F( IntegerOpsTests, orWithSecondNegativeAndTwoDigits ) {
    CHECK_OR( "1000000001", "-100000001", "-100000001" );
}


TEST_F( IntegerOpsTests, orWithFirstLongerSecondNegative ) {
    CHECK_OR( "123456789a", "-1", "-1" );
}


TEST_F( IntegerOpsTests, orWithSecondLongerAndNegative ) {
    CHECK_OR( "1", "-100000001", "-100000001" );
}


TEST_F( IntegerOpsTests, orWithFirstZeroAndSecondNegative ) {
    CHECK_OR( "0", "-100000001", "-100000001" );
}


TEST_F( IntegerOpsTests, orWithSecondZero ) {
    CHECK_OR( "FFFFFFFF", "0", "ffffffff" );
}


TEST_F( IntegerOpsTests, orWithFirstNegative ) {
    CHECK_OR( "-1", "1", "-1" );
}


TEST_F( IntegerOpsTests, orWithFirstNegativeAndTwoDigits ) {
    CHECK_OR( "-100000001", "1000000001", "-100000001" );
}


TEST_F( IntegerOpsTests, orWithFirstNegativeAndLonger ) {
    CHECK_OR( "-100000002", "1", "-100000001" );
}


TEST_F( IntegerOpsTests, orWithBothNegative ) {
    CHECK_OR( "-ffff0001", "-00010000", "-1" );
}


TEST_F( IntegerOpsTests, orWithBothNegativeAndTwoDigits ) {
    CHECK_OR( "-ffffffffffff0001", "-ffffffff00010000", "-ffffffff00000001" );
}


TEST_F( IntegerOpsTests, orWithBothNegativeAndTooManyDigits ) {
    CHECK_OR( "-1000000000000", "-ffff000010000000", "-10000000" );
}


TEST_F( IntegerOpsTests, orWithBothZero ) {
    CHECK_OR( "0", "0", "0" );
}


TEST_F( IntegerOpsTests, xorWithBothZero ) {
    CHECK_XOR( "0", "0", "0" );
}


TEST_F( IntegerOpsTests, xorWithBothPositiveAndDifferent ) {
    CHECK_XOR( "2", "1", "3" );
}


TEST_F( IntegerOpsTests, xorWithBothPositiveAndTheSame ) {
    CHECK_XOR( "2", "2", "0" );
}


TEST_F( IntegerOpsTests, xorWithFirstNegative ) {
    CHECK_XOR( "-1", "1", "-2" );
}


TEST_F( IntegerOpsTests, xorWithFirstNegativeAndLonger ) {
    CHECK_XOR( "-1e9", "1", "-fffffffff" );
}


TEST_F( IntegerOpsTests, xorWithFirstNegativeAndShorter ) {
    CHECK_XOR( "-1", "1e9", "-1000000001" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegative ) {
    CHECK_XOR( "1", "-1", "-2" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeAndLonger ) {
    CHECK_XOR( "1", "-1e9", "-fffffffff" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeAndThreeDigits ) {
    CHECK_XOR( "1", "-100000000000000000", "-fffffffffffffffff" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeAndShorter ) {
    CHECK_XOR( "1e9", "-1", "-1000000001" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeAndShorterWithCarry ) {
    CHECK_XOR( "1fffffffe", "-2", "-200000000" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeAndBothTwoDigits ) {
    CHECK_XOR( "effffffff", "-100000001", "-1e9" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeAndSecondDigitsZero ) {
    CHECK_XOR( "200000000", "-100000000", "-300000000" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeAndOverflow ) {
    CHECK_XOR( "ffff0000", "-100010000", "-200000000" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeWithAllDigits ) {
    CHECK_XOR( "123456789", "-876543210", "-955115587" );
}


TEST_F( IntegerOpsTests, xorWithSecondNegativeFirstThreeDigits ) {
    CHECK_XOR( "10000000000000000", "-1", "-10000000000000001" );
}


TEST_F( IntegerOpsTests, xorWithTwoNegative ) {
    CHECK_XOR( "-2", "-1", "1" );
}


TEST_F( IntegerOpsTests, xorWithTwoNegativeOneDigitComplex ) {
    CHECK_XOR( "-f0f0f0f0", "-f0f0f0f", "ffffffe1" );
}


TEST_F( IntegerOpsTests, xorWithTwoNegativeTwoDigits ) {
    CHECK_XOR( "-1234567890ABCDEF", "-FEDCBA0987654321", "ece8ec7117ce8ece" );
}


TEST_F( IntegerOpsTests, xorWithTwoNegativeFirstThreeDigits ) {
    CHECK_XOR( "-123456780000000000000000", "-1", "12345677ffffffffffffffff" );
}


TEST_F( IntegerOpsTests, xorWithTwoNegativeSecondThreeDigits ) {
    CHECK_XOR( "-1", "-123456780000000000000000", "12345677ffffffffffffffff" );
}


TEST_F( IntegerOpsTests, xorWithTwoNegativeTwoDigitUnderflow ) {
    CHECK_XOR( "-100000000", "-ffffffff", "1" );
}


TEST_F( IntegerOpsTests, xorWithTwoNegativeFirstLonger ) {
    CHECK_XOR( "-1234567890ABCDEF", "-87654321", "1234567817ce8ece" );
}


TEST_F( IntegerOpsTests, andWithTwoPositive ) {
    CHECK_AND( "FFFFFFFF", "FFFFFFFF", "ffffffff" );
}


TEST_F( IntegerOpsTests, andWithFirstPositive ) {
    CHECK_AND( "1", "-1", "1" );
}


TEST_F( IntegerOpsTests, andWithSecondPositive ) {
    CHECK_AND( "-1", "1", "1" );
}


TEST_F( IntegerOpsTests, andWithFirstPositiveTwoDigit ) {
    CHECK_AND( "FFFFFFFFF", "-1", "fffffffff" );
}


TEST_F( IntegerOpsTests, andWithFirstPositiveTwoDigit2 ) {
    CHECK_AND( "FFFFFFFFF", "-10", "ffffffff0" );
}


TEST_F( IntegerOpsTests, andWithSecondPositiveTwoDigit ) {
    CHECK_AND( "-10", "FFFFFFFFF", "ffffffff0" );
}


TEST_F( IntegerOpsTests, andWithBothNegative ) {
    CHECK_AND( "-1", "-1", "-1" );
}


TEST_F( IntegerOpsTests, andWithBothNegativeAndTwoDigits ) {
    CHECK_AND( "-1ffffffff", "-100000001", "-1ffffffff" );
}


TEST_F( IntegerOpsTests, andWithBothNegativeAndFirstLonger ) {
    CHECK_AND( "-1ffffffff", "-1", "-1ffffffff" );
}


TEST_F( IntegerOpsTests, andWithBothNegativeAndFirstThreeLong ) {
    CHECK_AND( "-10000000000000000", "-1", "-10000000000000000" );
}


TEST_F( IntegerOpsTests, andWithBothNegativeAndSecondLonger ) {
    CHECK_AND( "-1", "-100000001", "-100000001" );
}


TEST_F( IntegerOpsTests, andWithFirstZero ) {
    CHECK_AND( "0", "-1", "0" );
}


TEST_F( IntegerOpsTests, andWithSecondZero ) {
    CHECK_AND( "-1", "0", "0" );
}


TEST_F( IntegerOpsTests, andResultSizeInBytesWithTwoPositive ) {
    CHECK_AND_SIZE( "123456781234567812345678", "1234567812345678", sizeof( int ) + ( 2 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, andResultSizeInBytesWithSecondNegativeAndShorter ) {
    CHECK_AND_SIZE( "123456781234567812345678", "-1234567812345678", sizeof( int ) + ( 3 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, andResultSizeInBytesWithFirstLongerAndNegative ) {
    CHECK_AND_SIZE( "-123456781234567812345678", "1234567812345678", sizeof( int ) + ( 2 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, andResultSizeInBytesWithFirstShorterAndPositive ) {
    CHECK_AND_SIZE( "1234567812345678", "-123456781234567812345678", sizeof( int ) + ( 2 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, andResultSizeInBytesWithFirstZero ) {
    CHECK_AND_SIZE( "0", "-123456781234567812345678", sizeof( int ) + ( 0 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, andResultSizeInBytesWithSecondZero ) {
    CHECK_AND_SIZE( "-123456781234567812345678", "0", sizeof( int ) + ( 0 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, orResultSizeInBytesWithFirstLongerAndNegative ) {
    CHECK_OR_SIZE( "-123456781234567812345678", "1234567812345678", sizeof( int ) + ( 3 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, orResultSizeInBytesWithFirstShorterAndNegative ) {
    CHECK_OR_SIZE( "-12345678", "1234567812345678", sizeof( int ) + ( 1 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, orResultSizeInBytesWithFirstShorterAndBothNegative ) {
    CHECK_OR_SIZE( "-12345678", "-1234567812345678", sizeof( int ) + ( 1 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, orResultSizeInBytesWithSecondShorterAndBothNegative ) {
    CHECK_OR_SIZE( "-1234567812345678", "-12345678", sizeof( int ) + ( 1 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, orResultSizeInBytesWithSecondShorterAndNegative ) {
    CHECK_OR_SIZE( "1234567812345678", "-12345678", sizeof( int ) + ( 1 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, xorResultSizeInBytesWithFirstLongerAndNegative ) {
    CHECK_XOR_SIZE( "-123456781234567812345678", "1234567812345678", sizeof( int ) + ( 4 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, xorResultSizeInBytesWithBothPositive ) {
    CHECK_XOR_SIZE( "123456781234567812345678", "1234567812345678", sizeof( int ) + ( 3 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, orResultSizeInBytesWithSecondZero ) {
    CHECK_OR_SIZE( "1234567812345678", "0", sizeof( int ) + ( 2 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, orResultSizeInBytesWithFirstZero ) {
    CHECK_OR_SIZE( "0", "1234567812345678", sizeof( int ) + ( 2 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, ashResultSizeInBytesWhenNoShift ) {
    CHECK_ASH_SIZE( "2", 0, sizeof( int ) + ( 1 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, ashResultSizeInBytesWhenPositive ) {
    CHECK_ASH_SIZE( "2", 1, sizeof( int ) + ( 1 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, ashResultSizeInBytesWhenPositiveWithUnderflow ) {
    CHECK_ASH_SIZE( "1", -1, sizeof( int ) + ( 1 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, ashResultSizeInBytesWhenNegativeWithUnderflow ) {
    CHECK_ASH_SIZE( "-1", -1, sizeof( int ) + ( 1 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, ashResultSizeInBytesWhenPositiveWithOverflow ) {
    CHECK_ASH_SIZE( "2", 63, sizeof( int ) + ( 3 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, ashResultSizeInBytesWhenZero ) {
    CHECK_ASH_SIZE( "0", 32, sizeof( int ) + ( 0 * sizeof( Digit ) ) );
}


TEST_F( IntegerOpsTests, ashDigitShiftSimple ) {
    CHECK_ASH( "2", 1, "4" );
}


TEST_F( IntegerOpsTests, ashDigitShift ) {
    CHECK_ASH( "1", 32, "100000000" );
}


TEST_F( IntegerOpsTests, ashDigitShiftWithNegative ) {
    CHECK_ASH( "-1", 32, "-100000000" );
}


TEST_F( IntegerOpsTests, ashTwoAndAHalfDigitShift ) {
    CHECK_ASH( "1", 80, "100000000000000000000" );
}


TEST_F( IntegerOpsTests, ashSimpleShift ) {
    CHECK_ASH( "1", 16, "10000" );
}


TEST_F( IntegerOpsTests, ashOverFlow ) {
    CHECK_ASH( "ffffffff", 16, "ffffffff0000" );
}


TEST_F( IntegerOpsTests, ashTwoDigitOverFlow ) {
    CHECK_ASH( "ffffffffffffffff", 16, "ffffffffffffffff0000" );
}


TEST_F( IntegerOpsTests, ashNegativeBitShift ) {
    CHECK_ASH( "2", -1, "1" );
}


TEST_F( IntegerOpsTests, ashNegativeBitShiftWithUnderflow ) {
    CHECK_ASH( "100000000", -1, "80000000" );
}


TEST_F( IntegerOpsTests, ashNegativeBitShiftWithTwoDigitUnderflow ) {
    CHECK_ASH( "1", -64, "0" );
}


TEST_F( IntegerOpsTests, ashNegativeBitShiftNegativeValueWithUnderflow ) {
    CHECK_ASH( "-100000000", -1, "-80000000" );
}


TEST_F( IntegerOpsTests, ashNegativeDigitShift ) {
    CHECK_ASH( "100000000", -32, "1" );
}


TEST_F( IntegerOpsTests, ashNegativeHalfDigitShift ) {
    CHECK_ASH( "ffffffff0000", -16, "ffffffff" );
}


TEST_F( IntegerOpsTests, ashNegativeUnderflowShouldResultInNegativeOne ) {
    CHECK_ASH( "-1", -1, "-1" );
}


TEST_F( IntegerOpsTests, hashShouldXorDigits ) {
    IntegerOps::string_to_Integer( "-12345678ffffffff", 16, *x );

    int result = IntegerOps::hash( *x );

    ASSERT_EQ_M2( ( 0x12345678 ^ 0xffffffff ^ -2 ) >> 2, result, "Wrong hash" );
}
