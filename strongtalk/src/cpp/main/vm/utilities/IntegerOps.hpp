
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/utilities/Integer.hpp"


// -----------------------------------------------------------------------------

constexpr int   maxD           = 36;                    //
constexpr int   logB           = sizeof( Digit ) * 8;   //
constexpr Digit hlfB           = 0x80000000;            //
constexpr Digit oneB           = 0xFFFFFFFF;            //
constexpr int   digitBitLength = sizeof( Digit ) * 8;   //


// -----------------------------------------------------------------------------


// IntegerOps provides arithmetic & logic operations on Integers.
// Integer arguments are usually denoted by x & y, the result Integer is z.
//
// The Space for the resulting integer must be allocated before performing the operation.
// The amount of space required can be computed via the corresponding functions.

class IntegerOps : AllStatic {

public:
    static Digit as_Digit( char c );

    static char as_char( int i );

    static Digit xpy( Digit x, Digit y, Digit &carry );

    static Digit xmy( Digit x, Digit y, Digit &carry );

    static Digit axpy( Digit a, Digit x, Digit y, Digit &carry );

    static Digit xdy( Digit x, Digit y, Digit &carry );

    static Digit power( Digit x, int n );       // returns x^n
    static Digit max_power( Digit x );          // returns the largest y with x^y <= B

    static int unsigned_add_result_length( Integer &x, Integer &y );

    static int unsigned_sub_result_length( Integer &x, Integer &y );

    static int unsigned_mul_result_length( Integer &x, Integer &y );

    static int unsigned_quo_result_length( Integer &x, Integer &y );

    static int unsigned_rem_result_length( Integer &x, Integer &y );

    static void unsigned_add( Integer &x, Integer &y, Integer &z );

    static void unsigned_sub( Integer &x, Integer &y, Integer &z );

    static void unsigned_mul( Integer &x, Integer &y, Integer &z );

    static void unsigned_quo( Integer &x, Integer &y, Integer &z );

    static void unsigned_rem( Integer &x, Integer &y, Integer &z );

    static int unsigned_cmp( Integer &x, Integer &y );

    static void signed_div( Integer &x, Integer &y, Integer &z );

    static void signed_mod( Integer &x, Integer &y, Integer &z );

    static int last_non_zero_index( Digit *z, int lastIndex );

    static Digit scale( Digit *array, Digit factor, int length );

    static bool_t sd_all_zero( Digit *digits, int start, int stop );

    static Digit *copyDigits( Digit *source, int length, int toCopy );

    static Digit *qr_decomposition( Integer &x, Integer &y );

    static Digit qr_estimate_digit_quotient( Digit &xhi, Digit xlo, Digit y );

    static Digit *qr_decomposition_single_digit( Digit *qr, int length, Digit divisor );

    static Digit qr_calculate_remainder( Digit *qr, Digit *divisor, Digit q, int qrStart, int stop );

    static Digit qr_adjust_for_underflow( Digit *qr, Digit *divisor, Digit q, int qrStart, int stop );

    static Digit qr_adjust_for_over_estimate( Digit y1, Digit y2, Digit q, Digit xi, Digit xi2 );

    static Digit qr_scaling_factor( Digit firstDivisorDigit );

    static void qr_unscale_remainder( Digit *qr, Digit scalingFactor, int length );

    static Digit last_digit( Integer &x, Digit b );            // divides x by b and returns x mod b
    static void first_digit( Integer &x, Digit base, Digit carry );        // multiplies x by b and adds c

    // the following functions return the maximum result size
    // in bytes for the operation specified in the function name
    static int add_result_size_in_bytes( Integer &x, Integer &y );

    static int sub_result_size_in_bytes( Integer &x, Integer &y );

    static int mul_result_size_in_bytes( Integer &x, Integer &y );

    static int quo_result_size_in_bytes( Integer &x, Integer &y );

    static int rem_result_size_in_bytes( Integer &x, Integer &y );

    static int div_result_size_in_bytes( Integer &x, Integer &y );

    static int mod_result_size_in_bytes( Integer &x, Integer &y );

    static int and_result_size_in_bytes( Integer &x, Integer &y );

    static int or_result_size_in_bytes( Integer &x, Integer &y );

    static int xor_result_size_in_bytes( Integer &x, Integer &y );

    static int ash_result_size_in_bytes( Integer &x, int n );

    static void and_both_negative( Integer &x, Integer &y, Integer &z );

    static void and_one_positive( Integer &x, Integer &y, Integer &z );

    static void xor_one_positive( Integer &positive, Integer &negative, Integer &z );

    static int copy_result_size_in_bytes( Integer &x );

    static int int_to_Integer_result_size_in_bytes( int i );

    static int unsigned_int_to_Integer_result_size_in_bytes( std::uint32_t i );

    static int double_to_Integer_result_size_in_bytes( double x );

    static int string_to_Integer_result_size_in_bytes( const char *s, int base );

    static int Integer_to_string_result_size_in_bytes( Integer &x, int base );

    // arithmetic/binary operations & tests
    static void add( Integer &x, Integer &y, Integer &z );    // z := x + y
    static void sub( Integer &x, Integer &y, Integer &z );    // z := x - y
    static void mul( Integer &x, Integer &y, Integer &z );    // z := x * y
    static void quo( Integer &x, Integer &y, Integer &z );    // z := x quo y
    static void rem( Integer &x, Integer &y, Integer &z );    // z := x rem y
    static void Div( Integer &x, Integer &y, Integer &z );    // z := x div y
    static void Mod( Integer &x, Integer &y, Integer &z );    // z := x mod y

    static void And( Integer &x, Integer &y, Integer &z );   // z := x and y, bitwise, assuming 2's complement representation
    static void Or( Integer &x, Integer &y, Integer &z );    // z := x or  y, bitwise, assuming 2's complement representation
    static void Xor( Integer &x, Integer &y, Integer &z );   // z := x xor y, bitwise, assuming 2's complement representation
    static void ash( Integer &x, int n, Integer &z );         // z := x * 2^n

    static int cmp( Integer &x, Integer &y );    // returns < 0 for x < y; 0 for x = y; > 0 for x > y

    static void abs( Integer &x );                // x := |x|
    static void neg( Integer &x );                // x := -x
    static int hash( Integer &x );

    // copy & conversion operations
    static void copy( Integer &x, Integer &z );

    static void int_to_Integer( int i, Integer &z );

    static void unsigned_int_to_Integer( std::uint32_t i, Integer &z );

    static void double_to_Integer( double x, Integer &z );

    static void string_to_Integer( const char *s, int base, Integer &z );

    static void Integer_to_string( Integer &x, int base, char *s );
};

int length_in_bits( Digit x );

void shift_left( Digit d[], int length, int shift_count );

void shift_right( Digit d[], int length, int shift_count );
