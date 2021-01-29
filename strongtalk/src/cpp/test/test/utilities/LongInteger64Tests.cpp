//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/LongInteger64.hpp"

#include <gtest/gtest.h>


class LongInteger64Tests : public ::testing::Test {

protected:
    void SetUp() override {
    }


    void TearDown() override {
    }


};


TEST( LongInteger64Tests, as_double
) {
LongInteger64 value( 1.0 );
double        result   = value.as_double();
EXPECT_NEAR( 1.0, result, 0.0001 );
}


TEST( LongInteger64Tests, asDoubleFromInt
) {
LongInteger64 value( 1000, 0 );
EXPECT_NEAR( 1000.0, value.
as_double(),
0.0001 );
}


TEST( LongInteger64Tests, asDoubleFromLongInt
) {
LongInteger64 value( 3, 1 );
double        expected = ( ( (std::int64_t) 1 ) << 32 ) + 3;
EXPECT_NEAR( expected, value
.
as_double(),
0.0001 );
}


TEST( LongInteger64Tests, equality
) {
LongInteger64 lhs( 5, 10 );
LongInteger64 rhs( 5, 10 );
LongInteger64 different( 0, 0 );
ASSERT_TRUE( lhs
== rhs );
ASSERT_TRUE( !( different == rhs ) );
ASSERT_TRUE( !( lhs == different ) );
}


TEST( LongInteger64Tests, inequality
) {
LongInteger64 lhs( 5, 10 );
LongInteger64 rhs( 5, 10 );
LongInteger64 different( 0, 0 );
ASSERT_TRUE( different
!= rhs );
ASSERT_TRUE( lhs
!= different );
ASSERT_TRUE( !( lhs != rhs ) );
}


TEST( LongInteger64Tests, subtraction
) {
LongInteger64 minuend( 5, 10 );
LongInteger64 subtrahend( 2, 3 );
LongInteger64 expected( 3, 7 );
ASSERT_TRUE( expected
== minuend - subtrahend );
}


TEST( LongInteger64Tests, addition
) {
LongInteger64 augend( 5, 10 );
LongInteger64 addend( 2, 3 );
LongInteger64 expected( 7, 13 );
ASSERT_TRUE( expected
== augend + addend );
}
