
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of thisr source tree for complete licence and copyright terms
//

#include "vm/system/macros.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/utilities/OutputStream.hpp"

#include <gtest/gtest.h>


class IntegerOpsTests : public testing::Test {
};


// -----------------------------------------------------------------------------

TEST( IntegerOpsTest, test_axpy ) {
    Digit a, x, y, carry;
    for ( a = 12345; a <= 112345; a += 1234 ) {
        for ( x = 10; x <= 20000; x += 1234 ) {
            y             = 10;
            carry         = 500;
            Digit answer0 = ( ( a * x ) + y + carry ) % oneB;
            Digit answer1 = ( ( a * x ) + y + carry ) / oneB;
            Digit result  = IntegerOps::axpy( a, x, y, carry );
            EXPECT_EQ( result, answer0 );
            EXPECT_EQ( carry, answer1 );
        }
    }
}


TEST( IntegerOpsTest, string_to_Integer ) {

    bool    ok = true;
    Integer z6;
    IntegerOps::string_to_Integer( "0", 10, z6 );
    EXPECT_TRUE( 0 == z6.as_int32_t( ok ) ) << "failed to convert std::int32_t 0 to Integer ";

    Integer z7;
    IntegerOps::string_to_Integer( "1", 10, z7 );
    EXPECT_TRUE( 1 == z7.as_int32_t( ok ) ) << "failed to convert std::int32_t 1 to Integer ";

    Integer z8;
    IntegerOps::string_to_Integer( "1234", 10, z8 );
    EXPECT_TRUE( 1234 == z8.as_int32_t( ok ) ) << "failed to convert std::int32_t 1234 to Integer ";

    Integer z9;
    IntegerOps::string_to_Integer( "-1", 10, z9 );
    EXPECT_TRUE( -1 == z9.as_int32_t( ok ) ) << "failed to convert std::int32_t -1 to Integer ";

    Integer z10;
    IntegerOps::string_to_Integer( "-1234", 10, z10 );
    EXPECT_TRUE( -1234 == z10.as_int32_t( ok ) ) << "failed to convert std::int32_t -1234 to Integer ";

    Integer z11;
    IntegerOps::string_to_Integer( "1234567890123456789", 10, z11 );
    Integer z12;
    IntegerOps::string_to_Integer( "12345674401234567891234567890123456789", 10, z12 );
    Integer z13;
    IntegerOps::string_to_Integer( "12345678901255678912345676601234567891234567890123456789", 10, z13 );

}


TEST( IntegerOpsTest, test_integer_conversion ) {

    bool               ok = true;
    const std::int32_t n  = 10000;
    const std::int32_t l  = n * sizeof( std::int32_t );
    Integer            x, y, z;

    for ( std::int32_t i = -10; i <= 10; i++ ) {
        EXPECT_EQ( std::int32_t( ( i == 0 ? 1 : 2 ) * sizeof( std::int32_t ) ), IntegerOps::int_to_Integer_result_size_in_bytes( i ) ) << "int_to_Integer_result_size failed";
        IntegerOps::int_to_Integer( i, z );
        EXPECT_TRUE( i == z.as_int32_t( ok ) ) << "int_to_Integer/Integer_to_int failed";
    }

}


TEST( IntegerOpsTest, test_string_conversion ) {

    bool    ok = true;
    Integer z1;
    char *s1_in = "123456";
    IntegerOps::string_to_Integer( s1_in, 10, z1 );
    std::int32_t s1_out = z1.as_int32_t( ok );
    EXPECT_EQ( z1.as_int32_t( ok ), 123456 );

    lprintf( "ok: [%s] converted to [%d]", s1_in, s1_out );
    lprintf( "ok: z1._first_digit [%d], z._signed_length [%d]", z1._first_digit, z1._signed_length );

//    lprintf( "mul: x._first_digit [%d], x._signed_length [%d]", x._first_digit, x._signed_length );
//    lprintf( "mul: y._first_digit [%d], y._signed_length [%d]", y._first_digit, y._signed_length );

}


TEST( IntegerOpsTest, test_string_conversion_negative_input ) {

    bool    ok = true;
    Integer z2;
    char *s2_in = "-123456";
    IntegerOps::string_to_Integer( s2_in, 10, z2 );
    std::int32_t s2_out = z2.as_int32_t( ok );
    EXPECT_EQ( z2.as_int32_t( ok ), -123456 );

    SPDLOG_INFO( "ok: [{}] converted to [{}]", s2_in, s2_out );
    SPDLOG_INFO( "ok: z2._first_digit [{}], z._signed_length [{}]", z2._first_digit, z2._signed_length );

}


TEST( IntegerOpsTest, test_addition ) {

    bool    ok = true;
    Integer x, y, z;

    for ( std::int32_t i = -12345; i <= 12345; i += 1234 ) {
        for ( std::int32_t j = -12345; j <= 12345; j += 1234 ) {
            IntegerOps::int_to_Integer( i, x );
            IntegerOps::int_to_Integer( j, y );
            IntegerOps::add( x, y, z );
            EXPECT_EQ( z.as_int32_t( ok ), i + j );
        }
    }
}


TEST( IntegerOpsTest, test_subtraction ) {

    bool    ok = true;
    Integer x, y, z;

    for ( std::int32_t i = -12345; i <= 12345; i += 1234 ) {
        for ( std::int32_t j = -12345; j <= 12345; j += 1234 ) {
            x._signed_length = 0;
            x._first_digit   = 0;
            y._signed_length = 0;
            y._first_digit   = 0;
            IntegerOps::int_to_Integer( i, x );
            IntegerOps::int_to_Integer( j, y );
            IntegerOps::sub( x, y, z );
            EXPECT_EQ( z.as_int32_t( ok ), i - j );
        }
    }
}


TEST( IntegerOpsTest, test_multiplication ) {

    bool    ok = true;
    Integer x, y, z;

    for ( std::int32_t i = -12345; i <= 12345; i += 1234 ) {
        for ( std::int32_t j = -12345; j <= 12345; j += 1234 ) {
            IntegerOps::int_to_Integer( i, x );
            IntegerOps::int_to_Integer( j, y );
            IntegerOps::mul( x, y, z );
            EXPECT_EQ( z.as_int32_t( ok ), i * j );
        }
    }
}


// testing

static void check( bool p, char *s ) {
    if ( not p ) {
        st_fatal( s );
    }
}


static void factorial( std::int32_t n ) {
    Integer x, y, z;
    IntegerOps::int_to_Integer( 1, z );
    for ( std::int32_t i = 2; i <= n; i++ ) {
        IntegerOps::int_to_Integer( i, x );
        IntegerOps::copy( z, y );
        IntegerOps::mul( x, y, z );
        st_assert( z.size_in_bytes() <= sizeof( z ), "result too big" );
    };
    _console->print( "%d! = ", n );
    z.print();
    _console->cr();
}


TEST( IntegerOpsTest, test_factorial ) {

    std::int32_t i = 0;
    while ( i <= 10 ) {
        factorial( i );
        i++;
    }
    i = 20;
    while ( i <= 100 ) {
        factorial( i );
        i += 10;
    }
    factorial( 1000 );
}


static void unfactorial( std::int32_t n ) {
    Integer x, y, z;
    IntegerOps::int_to_Integer( 1, z );
    for ( std::int32_t i = 2; i <= n; i++ ) {
        IntegerOps::int_to_Integer( i, x );
        IntegerOps::copy( z, y );
        IntegerOps::mul( x, y, z );
        st_assert( z.size_in_bytes() <= sizeof( z ), "result too big" );
    };

    for ( std::int32_t j = 2; j <= n; j++ ) {
        IntegerOps::int_to_Integer( j, y );
        IntegerOps::copy( z, x );
        IntegerOps::Div( x, y, z );
        st_assert( z.size_in_bytes() <= sizeof( z ), "result too big" );
    };

    // _console->print( "%dun! = ", n );
    // z.print();
    // _console->cr();
}


TEST( IntegerOpsTest, test_unfactorial ) {

    Integer x, y, z;

    std::int32_t i = 0;
    while ( i <= 10 ) {
        unfactorial( i );
        i++;
    }

    i = 20;
    while ( i <= 100 ) {
        unfactorial( i );
        i += 10;
    }
    unfactorial( 1000 );
}


TEST( IntegerOpsTest, int_to_Integer ) {

    std::int32_t i;

    i = -10;
    while ( i <= 10 ) {
        Integer z;
        IntegerOps::int_to_Integer( i, z );
        bool ok;
        i++;
    }

    i = 20;
    while ( i <= 100 ) {
        Integer z;
        IntegerOps::int_to_Integer( i, z );
        bool ok;
        i += 10;
    }
}


TEST( IntegerOpsTest, double_to_Integer ) {

    std::int32_t i;

    i = -10;
    while ( i <= 10 ) {
        Integer z;
        IntegerOps::double_to_Integer( i, z );
        i++;
    }

    i = 20;
    while ( i <= 100 ) {
        Integer z;
        IntegerOps::double_to_Integer( i, z );
        i += 10;
    }


    Integer z1;
    IntegerOps::double_to_Integer( 0.49, z1 );

    Integer z2;
    IntegerOps::double_to_Integer( 123.0e10, z2 );

    Integer z3;
    IntegerOps::double_to_Integer( 1.2e12, z3 );

    Integer z4;
    IntegerOps::double_to_Integer( 314.159265358979323e-2, z4 );

    Integer z5;
    IntegerOps::double_to_Integer( -1.5, z5 );
}
