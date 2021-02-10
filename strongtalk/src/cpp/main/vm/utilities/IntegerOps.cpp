
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"


static std::int32_t exponent( double x ) {

    // Extracts the un-biased (binary) exponent of x.
    constexpr std::int32_t n = DOUBLE_LENGTH / logB;

    Digit d[n]{};

    *( (double *) d ) = x;

    return std::int32_t( ( d[ n - 1 ] << SIGN_LENGTH ) >> ( logB - EXPONENT_LENGTH ) ) - EXPONENT_BIAS;
}


std::int32_t length_in_bits( Digit x ) {

    // Computes the index of the most significant bit + 1 (length is 0 for x = 0).
    std::int32_t i = 0;
    while ( x not_eq 0 ) {
        x >>= 1;
        i++;
    }

    return i;
}


void shift_left( Digit d[], std::size_t length, std::int32_t shift_count ) {

    // Implements d[length] << shift_count (logical bit-wise left shift).
    if ( shift_count <= 0 )
        return;

    // shift_count > 0
    if ( shift_count % logB == 0 ) {
        // no bit-shifting needed
        std::int32_t i = length - 1;
        std::int32_t k = shift_count / logB;
        while ( i > k ) {
            d[ i ] = d[ i - k ];
            i--;
        }
        while ( i > 0 ) {
            d[ i ] = 0;
            i--;
        }
    } else {
        // bit-shifting needed
        std::int32_t i = length - 1;
        std::int32_t k = shift_count / logB;
        std::int32_t h = shift_count % logB;
        std::int32_t l = logB - h;
        while ( i > k ) {
            d[ i ] = ( d[ i - k ] << h ) | ( d[ i - k - 1 ] >> l );
            i--;
        }
        d[ i ]         = d[ i - k ] << h;
        i--;
        while ( i > 0 ) {
            d[ i ] = 0;
            i--;
        }
    }
}


void shift_right( Digit d[], std::size_t length, std::int32_t shift_count ) {

    // Implements d[length] >> shift_count (logical bit-wise right shift).
    if ( shift_count <= 0 )
        return;

    // shift_count > 0
    if ( shift_count % logB == 0 ) {
        // no bit shifting needed
        std::int32_t i = 0;
        std::int32_t k = shift_count / logB;
        while ( i < length - k ) {
            d[ i ] = d[ i + k ];
            i++;
        }
        while ( i < length ) {
            d[ i ] = 0;
            i++;
        }

    } else {
        // bit-shifting needed
        std::int32_t i = 0;
        std::int32_t k = shift_count / logB;
        std::int32_t l = shift_count % logB;
        std::int32_t h = logB - l;

        while ( i < length - k - 1 ) {
            d[ i ] = ( d[ i + k + 1 ] << h ) | ( d[ i + k ] >> l );
            i++;
        }

        d[ i ] = d[ i + k ] >> l;
        i++;
        while ( i < length ) {
            d[ i ] = 0;
            i++;
        }
    }
}


// Basic 32/64bit unsigned operations

Digit IntegerOps::as_Digit( char c ) {
    if ( '0' <= c and c <= '9' )
        return Digit( c - '0' );

    if ( 'A' <= c and c <= 'Z' )
        return Digit( c - 'A' ) + 10;

    if ( 'a' <= c and c <= 'z' )
        return Digit( c - 'a' ) + 10;

    st_fatal( "illegal digit" );
    return 0;
}


char IntegerOps::as_char( std::int32_t i ) {
    st_assert( 0 <= i and i < DIGITS_BASE, "illegal digit" );
    return "0123456789abcdefghijklmnopqrstuvwxyz"[ i ];
}


Digit IntegerOps::xpy( Digit x, Digit y, Digit &carry ) {
    // returns (x + y + c) mod B; sets carry = (x + y + c) div B

    DoubleDigit lx = x;
    DoubleDigit r  = lx + y + carry;
    carry = r >> digitBitLength;
    return (Digit) ( r & oneB );
}


Digit IntegerOps::xmy( Digit x, Digit y, Digit &carry ) {
    // returns (x - y - c) mod B; sets carry = -((x - y - c) div B)
    DoubleDigit lx = x;
    DoubleDigit r  = lx - y - carry;
    carry = r >> digitBitLength & 1;
    return Digit( r & oneB );
}


Digit IntegerOps::axpy( Digit a, Digit x, Digit y, Digit &carry ) {

//    SPDLOG_INFO( "axpy: a=[%u], x=[%u], y=[%u], carry=[%u]", a, x, y, carry );
    // returns (a*x + y + c) mod B; sets carry = (a*x + y + c) div B
    DoubleDigit lx = x;
//    SPDLOG_INFO( "axpy: lx=[%llu], x=[%u]", lx, x );

    DoubleDigit r = ( lx * a ) + y + carry;
//    SPDLOG_INFO( "axpy: r=[%llu]", r );

    carry = r >> digitBitLength;
//    SPDLOG_INFO( "axpy: carry=[%lu]", carry);

    return Digit( r & oneB );
}


Digit IntegerOps::xdy( Digit x, Digit y, Digit &carry ) {
    // returns (carry*B + x) div y; sets carry = (carry*B + x) mod y
    DoubleDigit c     = carry;    // make sure that carry is used below and not &carry
    DoubleDigit total = ( ( c << digitBitLength ) + x );
    carry = total % y;
    return Digit( ( total / y ) & oneB );
}


Digit IntegerOps::power( Digit x, std::int32_t n ) {
    Digit        f = x;
    Digit        p = 1;
    std::int32_t i = n;
    while ( i > 0 ) {
        // p * f^i = x^n
        if ( ( i & 1 ) == 1 ) {
            // i is odd
            p *= f;
        }
        f *= f;
        i >>= 1;
    }
    return p;
}


Digit IntegerOps::max_power( Digit x ) {
    Digit n = 1;
    Digit p = x;
    Digit c = 0;
    while ( c == 0 ) {
        // x^n = c*B + p
        p = axpy( x, p, 0, c );
        n++;
    }
    return p == 0 ? n : n - 1;
}


// Unsigned operations

void IntegerOps::unsigned_add( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl = x.length();
    std::int32_t yl = y.length();
    std::int32_t l  = min( xl, yl );
    std::int32_t i  = 0;
    Digit        c  = 0;
    while ( i < l ) {
        z[ i ] = xpy( x[ i ], y[ i ], c );
        i++;
    }
    while ( i < xl ) {
        z[ i ] = xpy( x[ i ], 0, c );
        i++;
    }
    while ( i < yl ) {
        z[ i ] = xpy( 0, y[ i ], c );
        i++;
    }
    if ( c not_eq 0 ) {
        z[ i ] = c;
        i++;
    }
    z.set_signed_length( i );
}


void IntegerOps::unsigned_sub( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl = x.length();
    std::int32_t yl = y.length();
    std::int32_t i  = 0;
    Digit        c  = 0;
    while ( i < yl ) {
        z[ i ] = xmy( x[ i ], y[ i ], c );
        i++;
    }
    while ( i < xl ) {
        z[ i ] = xmy( x[ i ], 0, c );
        i++;
    }
    if ( c not_eq 0 ) st_fatal( "negative result" );
    while ( i > 0 and z[ i - 1 ] == 0 )
        i--;
    z.set_signed_length( i );
}


void IntegerOps::unsigned_mul( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl = x.length();
    std::int32_t yl = y.length();
    // initialize z
    std::int32_t i  = xl + yl;
    while ( i > 0 ) {
        i--;
        z[ i ] = 0;
    }
    st_assert( i == 0, "i != 0" );

    std::int32_t k = 0;
    while ( i < xl ) {
        Digit d = x[ i ];
        if ( d not_eq 0 ) {
            std::int32_t j = 0;
            k = i;
            Digit c = 0;
            while ( j < yl ) {
                z[ k ] = axpy( d, y[ j ], z[ k ], c );
                j++;
                k++;
            }
            if ( c not_eq 0 ) {
                z[ k ] = c;
                k++;
            }
            i++;
        }
    }
    z.set_signed_length( k );
}


Digit IntegerOps::scale( Digit *array, Digit factor, std::size_t length ) {
// scale multiplies an integer representation stored
// in array by factor and returns the last carry.

    std::int32_t i = 0;
    Digit        c = 0;
    while ( i < length ) {
        array[ i ] = axpy( factor, array[ i ], 0, c );
        i++;
    }
    return c;
}


Digit *IntegerOps::copyDigits( Digit *source, std::size_t length, std::int32_t toCopy ) {
    Digit              *x = new_resource_array<Digit>( length );
    for ( std::int32_t i  = toCopy - 1; i >= 0; i-- )
        x[ i ] = source[ i ];
    return x;
}


Digit *IntegerOps::qr_decomposition_single_digit( Digit *x, std::size_t length, Digit divisor ) {
    Digit              c = 0;
    for ( std::size_t i = length - 1; i >= 0; i-- )
        x[ i + 1 ] = xdy( x[ i ], divisor, c );
    x[ 0 ]         = c;
    return x;
}


Digit IntegerOps::qr_estimate_digit_quotient( Digit &xhi, Digit xlo, Digit y ) {
    if ( xhi == y )
        return oneB;
    return xdy( xlo, y, xhi );
}


Digit IntegerOps::qr_calculate_remainder( Digit *qr, Digit *divisor, Digit q, std::int32_t qrStart, std::int32_t stop ) {
    Digit        c = 0;
    std::int32_t j = qrStart;
    std::int32_t k = 0;
    Digit        b = 0;
    while ( k < stop ) {
        qr[ j ]   = xmy( qr[ j ], 0, b );
        Digit qyk = axpy( q, divisor[ k ], 0, c );
        Digit c2  = 0;
        qr[ j ] = xmy( qr[ j ], qyk, c2 );
        b = xpy( b, c2, c );
        st_assert( c == 0, "Overflow" );

        j++;
        k++;
    }
    return b;
}


Digit IntegerOps::qr_adjust_for_underflow( Digit *qr, Digit *divisor, Digit q, std::int32_t qrStart, std::int32_t stop ) {
    std::int32_t j = qrStart;
    std::int32_t k = 0;
    Digit        c = 0;
    while ( k < stop ) {
        qr[ j ] = xpy( qr[ j ], divisor[ k ], c );
        j++;
        k++;
    }
    st_assert( c == 1, "Should have carry to balance borrow" ); // see Knuth p258 D6
    return q - 1; // correct estimate
}


Digit IntegerOps::qr_adjust_for_over_estimate( Digit y1, Digit y2, Digit q, Digit xi, Digit xi2 ) {
    //check if estimate is too large
    Digit c   = 0;
    Digit y2q = axpy( y2, q, 0, c );
    if ( c > xi or ( c = xi and y2q > xi2 ) ) {
        q--; // too large by 1
        Digit c2 = 0;
        xpy( xi, y1, c2 ); // add back divisor

        if ( not c2 ) {
            c   = 0;
            y2q = axpy( y2, q, 0, c );
            if ( c > xi or ( c = xi and y2q > xi2 ) )
                q--; // too large by 2
        }
    }
    return q;
}


Digit IntegerOps::qr_scaling_factor( Digit firstDivisorDigit ) {
    Digit c = 1;
    return ( firstDivisorDigit == oneB ) ? 1 : xdy( 0, firstDivisorDigit + 1, c ); // d = B/(yn + 1)
}


void IntegerOps::qr_unscale_remainder( Digit *qr, Digit scalingFactor, std::size_t length ) {
    Digit              c = 0;
    for ( std::size_t i = length - 1; i >= 0; i-- )
        qr[ i ] = xdy( qr[ i ], scalingFactor, c ); // undo scaling to get remainder
    st_assert( c == 0, "qr scaling broken" );
}


Digit *IntegerOps::qr_decomposition( Integer &dividend, Integer &y0 ) {


    // qr_decomposition divides x by y (unsigned) and returns its decomposition into quotient (q) and remainder (r) packed into the array qr with length x.length() + 1.
    //
    //
    // The layout of the result qr is as follows:
    //
    // qr      [<--- quotient (q) ---->|<-- remainder (r) -->]
    // index   |xl  xl-1  ...  yl+1  yl|yl-1  yl-2  ...  1  0|
    //
    // length of quotient : ql = xl - yl + 1 (xl >= yl => ql >= 1)
    // length of remainder: rl = yl          (yl >   0 => rl >= 1)

    std::int32_t dividendLength = dividend.length();
    std::int32_t divisorLength  = y0.length();
    if ( dividendLength < divisorLength ) st_fatal( "division not needed" );
    if ( divisorLength == 0 ) st_fatal( "division by zero" );

    // initialize qr
    Digit *x = copyDigits( dividend.digits(), dividendLength + 1, dividendLength );

    if ( divisorLength == 1 )
        return qr_decomposition_single_digit( x, dividendLength, y0._first_digit );

    // full division
    x[ dividendLength ] = 0;
    Digit *y = y0.digits();

    Digit d = qr_scaling_factor( y[ divisorLength - 1 ] ); // d = B/(yn + 1)

    if ( d > 1 ) {
        x[ dividendLength ] = scale( x, d, dividendLength );
        y = copyDigits( y, divisorLength, divisorLength );
        Digit c = scale( y, d, divisorLength );

        if ( c not_eq 0 ) st_fatal( "qr_decomposition broken" );
    }

    Digit        y1 = y[ divisorLength - 1 ];
    Digit        y2 = y[ divisorLength - 2 ];
    std::int32_t i  = dividendLength;
    while ( i >= divisorLength ) {
        Digit xi = x[ i ]; //x[i] gets overwritten by remainder so save it for later
        Digit q  = qr_estimate_digit_quotient( x[ i ], x[ i - 1 ], y1 ); // estimate q = rem/y
        //x[i] now contains remainder - used in test below
        q = qr_adjust_for_over_estimate( y1, y2, q, x[ i ], x[ i - 2 ] );

        Digit b = qr_calculate_remainder( x, y, q, i - divisorLength, divisorLength );
        xmy( xi, 0, b );
        if ( b ) { // underflow
            x[ i ] = qr_adjust_for_underflow( x, y, q, i - divisorLength, divisorLength );
        } else {
            x[ i ] = q;
        }
        i--;
    }
    if ( d not_eq 1 ) {
        qr_unscale_remainder( x, d, divisorLength );
    }
    return x;
}


void IntegerOps::unsigned_quo( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl = x.length();
    std::int32_t yl = y.length();
    if ( xl < yl ) {
        // unsigned x < unsigned y => z = 0
        z.set_signed_length( 0 );
    } else {
        // xl >= yl
        ResourceMark resourceMark;
        Digit        *qr = qr_decomposition( x, y );
        std::int32_t i   = xl;
        while ( i >= yl and qr[ i ] == 0 )
            i--;
        // i < yl or qr[i] not_eq 0
        z.set_signed_length( i - yl + 1 );
        while ( i >= yl ) {
            z[ i - yl ] = qr[ i ];
            i--;
        }
    }
}


bool IntegerOps::sd_all_zero( Digit *digits, std::int32_t start, std::int32_t stop ) {
    for ( std::size_t i = start; i < stop; i++ )
        if ( digits[ i ] )
            return false;
    return true;
}


void IntegerOps::signed_div( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl   = x.length();
    bool         xneg = x.is_negative();
    std::int32_t yl   = y.length();
    bool         yneg = y.is_negative();

    if ( xl < yl ) {
        // unsigned x < unsigned y => z = 0
        if ( xneg == yneg )
            z.set_signed_length( 0 );
        else {
            z.set_signed_length( 1 );
            z[ 0 ] = 1;
            neg( z );
        }
    } else {
        // xl >= yl
        ResourceMark resourceMark;
        Digit        *qr = qr_decomposition( x, y );

        std::int32_t i = xl;
        while ( i >= yl and qr[ i ] == 0 )
            i--;
        // i < yl or qr[i] not_eq 0
        Digit              carry  = not sd_all_zero( qr, 0, yl ) and xneg not_eq yneg;
        std::int32_t       digits = i - yl + 1;
        for ( std::int32_t j      = 0; j < digits; j++ )
            z[ j ] = xpy( qr[ yl + j ], 0, carry );

        if ( carry ) {
            z.set_signed_length( i - yl + 2 );
            z[ i - yl + 1 ] = carry;
        } else
            z.set_signed_length( i - yl + 1 );
        if ( xneg not_eq yneg )
            neg( z );
    }
}


std::int32_t IntegerOps::last_non_zero_index( Digit *z, std::int32_t lastIndex ) {
    std::int32_t i = lastIndex;
    while ( i >= 0 and z[ i ] == 0 )
        i--;
    return i;
}


void IntegerOps::signed_mod( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl = x.length();
    std::int32_t yl = y.length();
    if ( xl < yl ) {
        // unsigned x < unsigned y => z = (y - x)
        Digit carry = 0;

        for ( std::size_t i = 0; i < xl; i++ ) {
            z[ i ] = xmy( y[ i ], x[ i ], carry );
        }

        for ( std::size_t i = xl; i < yl; i++ ) {
            z[ i ] = xmy( y[ i ], 0, carry );
        }

        st_assert( carry == 0, "Remainder too large" );

        z.set_signed_length( last_non_zero_index( z.digits(), yl - 1 ) + 1 );
    } else {
        // xl >= yl
        ResourceMark resourceMark;
        Digit        *qr = qr_decomposition( x, y );

        if ( not sd_all_zero( qr, 0, yl ) ) {

            Digit carry = 0;

            for ( std::size_t j = 0; j < yl; j++ ) {
                qr[ j ] = xmy( y[ j ], qr[ j ], carry );
            }
            st_assert( carry == 0, "Remainder too large" );
        }

        std::int32_t i = last_non_zero_index( qr, yl - 1 );
        z.set_signed_length( i + 1 );
        while ( i >= 0 ) {
            z[ i ] = qr[ i ];
            i--;
        }
    }
}


void IntegerOps::unsigned_rem( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl = x.length();
    std::int32_t yl = y.length();
    if ( xl < yl ) {
        // unsigned x < unsigned y => z = x
        copy( x, z );
    } else {
        // xl >= yl
        ResourceMark resourceMark;
        Digit        *qr = qr_decomposition( x, y );
        std::int32_t i   = yl - 1;
        while ( i >= 0 and qr[ i ] == 0 )
            i--;
        // i < 0 or qr[i] not_eq 0
        z.set_signed_length( i + 1 );
        while ( i >= 0 ) {
            z[ i ] = qr[ i ];
            i--;
        }
    }
}


std::int32_t IntegerOps::unsigned_cmp( Integer &x, Integer &y ) {
    std::int32_t xl = x.length();
    std::int32_t yl = y.length();
    if ( xl == yl and xl > 0 ) {
        std::int32_t i = xl - 1;
        while ( i > 0 and x[ i ] == y[ i ] )
            i--;
        if ( x[ i ] < y[ i ] )
            return -1;
        return x[ i ] not_eq y[ i ];
    } else {
        return xl - yl;
    }
}


Digit IntegerOps::last_digit( Integer &x, Digit b ) {
    std::int32_t xl = x.length();
    std::int32_t i  = xl;
    Digit        c  = 0;
    while ( i > 0 ) {
        i--;
        x[ i ] = xdy( x[ i ], b, c );
    }
    if ( xl > 0 and x[ xl - 1 ] == 0 )
        x.set_signed_length( xl - 1 );
    return c;
}


void IntegerOps::first_digit( Integer &x, Digit base, Digit carry ) {

    std::int32_t xl = x.length();
    std::int32_t i  = 0;
    while ( i < xl ) {
        x[ i ] = axpy( base, x[ i ], 0, carry );
        i++;
    }

    if ( carry not_eq 0 ) {
        x[ i ] = carry;
        i++;
    }

    x.set_signed_length( i );
}


// Implementation of IntegerOps

std::int32_t IntegerOps::add_result_size_in_bytes( Integer &x, Integer &y ) {
    std::int32_t l;
    if ( x.is_negative() == y.is_negative() ) {
        l = unsigned_add_result_length( x, y );
    } else if ( unsigned_cmp( x, y ) < 0 ) {
        l = unsigned_sub_result_length( y, x );
    } else {
        l = unsigned_sub_result_length( x, y );
    }
    return Integer::length_to_size_in_bytes( l );
}


std::int32_t IntegerOps::sub_result_size_in_bytes( Integer &x, Integer &y ) {
    std::int32_t l;
    if ( x.is_negative() not_eq y.is_negative() ) {
        l = unsigned_add_result_length( x, y );
    } else if ( unsigned_cmp( x, y ) < 0 ) {
        l = unsigned_sub_result_length( y, x );
    } else {
        l = unsigned_sub_result_length( x, y );
    }
    return Integer::length_to_size_in_bytes( l );
}


std::int32_t IntegerOps::mul_result_size_in_bytes( Integer &x, Integer &y ) {
    return Integer::length_to_size_in_bytes( unsigned_mul_result_length( x, y ) );
}


std::int32_t IntegerOps::quo_result_size_in_bytes( Integer &x, Integer &y ) {
    return Integer::length_to_size_in_bytes( unsigned_quo_result_length( x, y ) );
}


std::int32_t IntegerOps::rem_result_size_in_bytes( Integer &x, Integer &y ) {
    return Integer::length_to_size_in_bytes( unsigned_rem_result_length( x, y ) );
}


std::int32_t IntegerOps::div_result_size_in_bytes( Integer &x, Integer &y ) {

    std::int32_t l;
    if ( x.is_negative() == y.is_negative() ) {
        l = unsigned_quo_result_length( x, y );
    } else {
        l = unsigned_quo_result_length( x, y ) + 1;
    }

    return Integer::length_to_size_in_bytes( l );
}


std::int32_t IntegerOps::mod_result_size_in_bytes( Integer &x, Integer &y ) {
    std::int32_t digitLength = unsigned_rem_result_length( x, y );
    return Integer::length_to_size_in_bytes( digitLength );
}


std::int32_t IntegerOps::and_result_size_in_bytes( Integer &x, Integer &y ) {

    std::int32_t digitLength;
    if ( x.is_zero() or y.is_zero() ) {
        digitLength = 0;
    } else if ( x.is_positive() and y.is_positive() ) {
        digitLength = min( x.length(), y.length() );
    } else if ( x.is_positive() ) {
        digitLength = x.length();
    } else if ( y.is_positive() ) {
        digitLength = y.length();
    } else {
        digitLength = max( x.length(), y.length() );
    }
    return Integer::length_to_size_in_bytes( digitLength );
}


std::int32_t IntegerOps::or_result_size_in_bytes( Integer &x, Integer &y ) {

    std::int32_t digitLength;
    if ( not x.is_negative() and not y.is_negative() )
        digitLength = max( x.length(), y.length() );
    else if ( y.is_positive() )
        digitLength = x.length();
    else if ( x.is_positive() )
        digitLength = y.length();
    else
        digitLength = min( x.length(), y.length() );

    return Integer::length_to_size_in_bytes( digitLength );
}


std::int32_t IntegerOps::xor_result_size_in_bytes( Integer &x, Integer &y ) {
    std::int32_t digitLength;
    if ( x.is_negative() not_eq y.is_negative() )
        digitLength = max( x.length(), y.length() ) + 1;
    else
        digitLength = max( x.length(), y.length() );
    return Integer::length_to_size_in_bytes( digitLength );
}


std::int32_t IntegerOps::ash_result_size_in_bytes( Integer &x, std::int32_t n ) {

    std::int32_t digitLength;
    if ( x.is_zero() or n == 0 ) {
        digitLength = x.length();
    } else if ( n > 0 ) {
        std::int32_t rem      = n % logB;
        Digit        mask     = nthMask( logB - rem ) ^oneB;
        bool         overflow = ( mask & x[ x.length() - 1 ] ) not_eq 0;
        digitLength = x.length() + ( n / logB ) + overflow;
    } else {
//        std::int32_t rem  = ( -n ) % logB;
//        Digit        mask = nthMask( rem ) ^oneB;
        digitLength = max( x.length() + ( n / logB ), 1 );
    }

    return Integer::length_to_size_in_bytes( digitLength );
}


std::int32_t IntegerOps::copy_result_size_in_bytes( Integer &x ) {
    return x.size_in_bytes();
}


std::int32_t IntegerOps::unsigned_int_to_Integer_result_size_in_bytes( std::uint32_t i ) {
    return Integer::length_to_size_in_bytes( i not_eq 0 );
}


std::int32_t IntegerOps::int_to_Integer_result_size_in_bytes( std::int32_t i ) {
    return Integer::length_to_size_in_bytes( i not_eq 0 );
}


std::int32_t IntegerOps::double_to_Integer_result_size_in_bytes( double x ) {
    if ( x < 0.0 )
        x = -x;
    return Integer::length_to_size_in_bytes( x < 1.0 ? 0 : ( exponent( x ) + logB ) / logB );
}


std::int32_t IntegerOps::string_to_Integer_result_size_in_bytes( const char *s, std::int32_t base ) {
    // for now: implement for base 10 only, use simple heuristics
    if ( base == 10 ) {
        std::int32_t i = 0;
        while ( s[ i ] not_eq '\x0' )
            i++;
        return Integer::length_to_size_in_bytes( i / 9 + 1 );
    }
    st_fatal( "string conversion with base not equal to 10" );
    return -1;
}


std::int32_t IntegerOps::Integer_to_string_result_size_in_bytes( Integer &x, std::int32_t base ) {
    // for now: implement for base 10 only, use simple heuristics
    if ( base == 10 ) {
        return ( x.length() + 1 ) * 10; // add one for sign & zero
    }
    st_fatal( "string conversion with base not equal to 10" );
    return -1;
}


void IntegerOps::add( Integer &x, Integer &y, Integer &z ) {
    if ( x.is_negative() == y.is_negative() ) {
        unsigned_add( x, y, z );
    } else if ( unsigned_cmp( x, y ) < 0 ) {
        unsigned_sub( y, x, z );
        neg( z );
    } else {
        unsigned_sub( x, y, z );
    }
    if ( x.is_negative() )
        neg( z );
}


void IntegerOps::sub( Integer &x, Integer &y, Integer &z ) {
    if ( x.is_negative() not_eq y.is_negative() ) {
        unsigned_add( x, y, z );
    } else if ( unsigned_cmp( x, y ) < 0 ) {
        unsigned_sub( y, x, z );
        neg( z );
    } else {
        unsigned_sub( x, y, z );
    }
    if ( x.is_negative() )
        neg( z );
}


void IntegerOps::mul( Integer &x, Integer &y, Integer &z ) {

    if ( x.is_zero() || y.is_zero() ) {
        z.set_signed_length( 0 );
    } else if ( x.length() < y.length() ) {
        unsigned_mul( x, y, z );
    } else {
        unsigned_mul( y, x, z );
    }
    if ( x.is_negative() != y.is_negative() ) neg( z );

}


void IntegerOps::quo( Integer &x, Integer &y, Integer &z ) {
    unsigned_quo( x, y, z );
    if ( x.is_negative() not_eq y.is_negative() )
        neg( z );
}


void IntegerOps::rem( Integer &x, Integer &y, Integer &z ) {
    unsigned_rem( x, y, z );
    if ( x.is_negative() )
        neg( z );
}


#define copyInteger( x ) \
  (Integer*)memcpy((void*)NEW_RESOURCE_ARRAY(Digit, x.length() + 1), \
                 (void*)&x._signed_length,\
                 (x.length() + 1) * sizeof(Digit))


void IntegerOps::Div( Integer &x, Integer &y, Integer &z ) {
    ResourceMark resourceMark;

    if ( not x.is_negative() and y.is_positive() ) {
        unsigned_quo( x, y, z );
    } else {
        signed_div( x, y, z );
    }
}


void IntegerOps::Mod( Integer &x, Integer &y, Integer &z ) {
    if ( x.is_negative() == y.is_negative() ) {
        unsigned_rem( x, y, z );
    } else {
        signed_mod( x, y, z );
    }
    if ( y.is_negative() and z.is_not_zero() )
        neg( z );
}


void IntegerOps::And( Integer &x, Integer &y, Integer &z ) {
    if ( x.is_zero() or y.is_zero() ) {
        z.set_signed_length( 0 );
    } else if ( x.is_positive() and y.is_positive() ) {
        std::int32_t l = min( x.length(), y.length() );
        std::int32_t i = 0;
        while ( i < l ) {
            z[ i ] = x[ i ] & y[ i ];
            i++;
        }
        z.set_signed_length( i );
    } else if ( x.is_positive() ) {
        and_one_positive( x, y, z );
    } else if ( y.is_positive() ) {
        and_one_positive( y, x, z );
    } else {
        and_both_negative( x, y, z );
    }
}


void IntegerOps::and_both_negative( Integer &x, Integer &y, Integer &z ) {
    std::int32_t digitLength = max( x.length(), y.length() );

    Digit        xcarry = 1;
    Digit        ycarry = 1;
    Digit        zcarry = 1;
    std::int32_t i      = 0;

    while ( i < min( x.length(), y.length() ) ) {
        z[ i ] = xpy( ( xpy( x[ i ] ^ 0xffffffff, 0, xcarry ) & xpy( y[ i ] ^ 0xffffffff, 0, ycarry ) ) ^ 0xffffffff, 0, zcarry );
        i++;
    }
    while ( i < x.length() ) {
        z[ i ] = x[ i ];
        i++;
    }
    while ( i < y.length() ) {
        z[ i ] = y[ i ];
        i++;
    }
    z.set_signed_length( -digitLength );
}


void IntegerOps::and_one_positive( Integer &positive, Integer &negative, Integer &z ) {
    std::int32_t digitLength = positive.length();

    Digit        carry = 1;
    std::int32_t i     = 0;
    while ( i < min( positive.length(), negative.length() ) ) { // digits in both
        z[ i ] = positive[ i ] & xpy( ( negative[ i ] ^ oneB ), 0, carry );
        i++;
    }
    while ( i < digitLength ) { // remaining digits in positive
        z[ i ] = positive[ i ];
        i++;
    }
    z.set_signed_length( digitLength );
}


void IntegerOps::Or( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl     = x.length();
    std::int32_t yl     = y.length();
    std::int32_t l      = min( xl, yl );
    Digit        xcarry = 1;
    Digit        ycarry = 1;
    Digit        zcarry = 1;
    std::int32_t i      = 0;
    if ( not x.is_negative() and not y.is_negative() ) {
        while ( i < l ) {
            z[ i ] = x[ i ] | y[ i ];
            i++;
        }
        while ( i < xl ) {
            z[ i ] = x[ i ];
            i++;
        }
        while ( i < yl ) {
            z[ i ] = y[ i ];
            i++;
        }
        z.set_signed_length( i );
        return;
    } else if ( not x.is_negative() ) {
        while ( i < l ) {
            z[ i ] = xpy( ( x[ i ] | xpy( y[ i ] ^ oneB, 0, ycarry ) ) ^ oneB, 0, zcarry );
            i++;
        }
        while ( i < yl ) {
            z[ i ] = y[ i ];
            i++;
        }
    } else if ( not y.is_negative() ) {
        while ( i < l ) {
            z[ i ] = xpy( ( y[ i ] | xpy( x[ i ] ^ oneB, 0, xcarry ) ) ^ oneB, 0, zcarry );
            i++;
        }
        while ( i < xl ) {
            z[ i ] = x[ i ];
            i++;
        }
    } else {
        st_assert( x.is_negative() and y.is_negative(), "Error invalid integers?" );
        while ( i < l ) {
            z[ i ] = xpy( ( xpy( y[ i ] ^ oneB, 0, ycarry ) | xpy( x[ i ] ^ oneB, 0, xcarry ) ) ^ oneB, 0, zcarry );
            i++;
        }
    }
    while ( i > 0 and not z[ i - 1 ] )
        i--;
    z.set_signed_length( -i );
}


void IntegerOps::Xor( Integer &x, Integer &y, Integer &z ) {
    std::int32_t xl = x.length();
    std::int32_t yl = y.length();
    std::int32_t l  = min( xl, yl );
    std::int32_t i  = 0;
    if ( not x.is_negative() and not y.is_negative() ) {
        while ( i < l ) {
            z[ i ] = x[ i ] ^ y[ i ];
            i++;
        }
        while ( i < xl ) {
            z[ i ] = x[ i ];
            i++;
        }
        while ( i < yl ) {
            z[ i ] = y[ i ];
            i++;
        }
        while ( i > 0 and not z[ i - 1 ] )
            i--;
        z.set_signed_length( i );

    } else if ( not y.is_negative() ) {
        xor_one_positive( y, x, z );

    } else if ( not x.is_negative() ) {
        xor_one_positive( x, y, z );

    } else {
        st_assert( x.is_negative() and y.is_negative(), "Error invalid integers?" );
        Digit xcarry = 1;
        Digit ycarry = 1;
        while ( i < l ) {
            z[ i ] = xmy( x[ i ], 0, xcarry ) ^ xmy( y[ i ], 0, ycarry );
            i++;
        }
        while ( i < xl ) {
            z[ i ] = xmy( x[ i ], 0, xcarry );
            i++;
        }
        while ( i < yl ) {
            z[ i ] = xmy( y[ i ], 0, ycarry );
            i++;
        }
        while ( i > 0 and z[ i - 1 ] == 0 )
            i--;
        z.set_signed_length( i );
    }
}


void IntegerOps::xor_one_positive( Integer &positive, Integer &negative, Integer &z ) {
    std::int32_t pl     = positive.length();
    std::int32_t nl     = negative.length();
    std::int32_t l      = min( pl, nl );
    std::int32_t i      = 0;
    Digit        ncarry = 1;
    Digit        zcarry = 1;

    while ( i < l ) {
        z[ i ] = xpy( positive[ i ] ^ xmy( negative[ i ], 0, ncarry ), 0, zcarry );
        i++;
    }

    while ( i < nl ) {
        z[ i ] = xpy( xmy( negative[ i ], 0, ncarry ), 0, zcarry );
        i++;
    }

    while ( i < pl ) {
        z[ i ] = xpy( positive[ i ], 0, zcarry );
        i++;
    }

    if ( zcarry ) {
        z[ i ] = 1;
        i++;
    }
    z.set_signed_length( -i );
}


void IntegerOps::ash( Integer &x, std::int32_t n, Integer &z ) {
    if ( n > 0 ) {
        std::int32_t i          = 0;
        std::int32_t bitShift   = n % logB;
        std::int32_t digitShift = n / logB;
        while ( i < digitShift ) {
            z[ i ] = 0;
            i++;
        }
        Digit carry = 0;
        while ( i < x.length() + digitShift ) {
            z[ i ] = ( x[ i - digitShift ] << bitShift ) + carry;
            carry = bitShift ? x[ i - digitShift ] >> ( logB - bitShift ) : 0;
            i++;
        }
        if ( carry ) {
            z[ i ] = carry;
            i++;
        }
        z.set_signed_length( i );
        if ( x.is_negative() )
            neg( z );
    } else {
        std::int32_t digitShift = -n / logB;
        std::int32_t bitShift   = -n % logB;
        std::int32_t i          = x.length() - digitShift;
        Digit        carry      = 0;
        while ( i > 0 ) {
            i--;
            z[ i ] = ( x[ i + digitShift ] >> bitShift ) + carry;
            carry = bitShift ? ( x[ i + digitShift ] & nthMask( bitShift ) ) << ( logB - bitShift ) : 0;
        }
        i                       = x.length() - digitShift;
        while ( i > 0 and z[ i - 1 ] == 0 )
            i--;
        z.set_signed_length( max( i, 0 ) );
        if ( x.is_negative() )
            neg( z );
        if ( x.is_negative() and z.is_zero() ) {
            z.set_signed_length( -1 );
            z[ 0 ] = 1;
        }
    }
}


std::int32_t IntegerOps::cmp( Integer &x, Integer &y ) {
    if ( x.is_negative() == y.is_negative() ) {
        return x.is_negative() ? unsigned_cmp( y, x ) : unsigned_cmp( x, y );
    } else {
        return x.is_zero() ? -y.signed_length() : x.signed_length();
    }
}


void IntegerOps::abs( Integer &x ) {
    if ( x.is_negative() )
        neg( x );
}


void IntegerOps::neg( Integer &x ) {
    x.set_signed_length( -x._signed_length );
}


void IntegerOps::copy( const Integer &x, Integer &z ) {
    z._signed_length = x._signed_length;
    std::int32_t i = x.length();
    while ( i > 0 ) {
        i--;
        z[ i ] = x[ i ];
    }
}


void IntegerOps::unsigned_int_to_Integer( std::uint32_t i, Integer &z ) {
    if ( i == 0 ) {
        z.set_signed_length( 0 );
    } else {
        z.set_signed_length( 1 );
        z._first_digit = Digit( i );
    }
}


void IntegerOps::int_to_Integer( std::int32_t i, Integer &z ) {

    if ( i < 0 ) {
        z.set_signed_length( -1 );
        z._first_digit = Digit( -i );

    } else if ( i == 0 ) {
        z.set_signed_length( 0 );
        z._first_digit = Digit( 0 ); // no digits in this case

    } else {
        z.set_signed_length( 1 );
        z._first_digit = Digit( i );
    }

}


void IntegerOps::double_to_Integer( double x, Integer &z ) {
    // filter out sign
    bool negative = false;
    if ( x < 0.0 ) {
        negative = true;
        x        = -x;
    }

    // filter out trivial cases
    if ( x < 1.0 ) {
        z.set_signed_length( 0 );
        return;
    }

    // get an n-Digit integer d[n] built from x. n needs to be big enough
    // so that we don't loose bits after shifting (note that the mantissa
    // consists of one (implicit) extra bit which is always 1).
    const std::int32_t n = ( MANTISSA_LENGTH + 1 ) / logB + 2;
    Digit              d[n];
    std::int32_t       i = 0;
    while ( i < n ) {
        d[ i ] = 0;
        i++;
    }
    *( (double *) &d[ n - ( DOUBLE_LENGTH / logB ) ] ) = x;

    // compute length l of integer
    std::int32_t length_in_bits = exponent( x ) + 1;
    std::int32_t l              = ( length_in_bits + logB - 1 ) / logB;

    // shift sign & exponent out but keep Space for implicit 1 bit
    const std::int32_t left_shift_count = SIGN_LENGTH + EXPONENT_LENGTH - 1;
    shift_left( d, n, left_shift_count );

    // add implicit 1 bit
    const Digit mask = Digit( -1 ) << ( logB - 1 );
    d[ n - 1 ] |= mask;

    // shift right to the right
    const std::int32_t right_shift_count = logB - length_in_bits % logB;
    shift_right( d, n, right_shift_count );

    // copy most significant digits into z & fill with zeros
    i = 1;
    while ( l - i >= 0 and n - i >= 0 ) {
        z[ l - i ] = d[ n - i ];
        i++;
    }
    while ( l - i >= 0 ) {
        z[ l - i ] = 0;
        i++;
    }

    // set length & adjust sign
    z.set_signed_length( l );
    if ( negative )
        neg( z );
}


void IntegerOps::string_to_Integer( const char *s, std::int32_t base, Integer &z ) {
    int_to_Integer( 0, z );
    std::int32_t i = s[ 0 ] == '-';
    while ( s[ i ] != '\x00' ) {
        first_digit( z, base, as_Digit( s[ i ] ) );
        i++;
    }
    if ( s[ 0 ] == '-' )
        neg( z );
}


void IntegerOps::Integer_to_string( const Integer &x, std::int32_t base, char *s ) {

    st_assert( 2 <= base and base <= DIGITS_BASE, "illegal base" );
    st_assert( x.size_in_bytes() <= 10001 * sizeof( Digit ), "temporary array too small" );

    Integer t;
    copy( x, t );

    // convert t into s (destructive)
    std::int32_t i = 0;
    do {
        s[ i ] = as_char( last_digit( t, base ) );
        i++;
    } while ( t.is_not_zero() );

    if ( x.is_negative() ) {
        s[ i ] = '-';
        i++;
    }

    s[ i ] = '\0';
    // reverse string
    std::int32_t j = i - 1;
    i = 0;
    while ( i < j ) {
        char c = s[ i ];
        s[ i ] = s[ j ];
        s[ j ] = c;
        i++;
        j--;
    }

}


std::int32_t IntegerOps::hash( Integer &x ) {
    std::int32_t hash = 0;

    for ( std::size_t i = x.length() - 1; i >= 0; i-- ) {
        hash ^= x[ i ];
    }

    hash ^= x.signed_length();
    return hash >> 2;
}


std::int32_t IntegerOps::unsigned_add_result_length( Integer &x, Integer &y ) {
    return max( x.length(), y.length() ) + 1;
}


std::int32_t IntegerOps::unsigned_sub_result_length( Integer &x, Integer &y ) {
    static_cast<void>(y); // unused

    return x.length();
}


std::int32_t IntegerOps::unsigned_mul_result_length( Integer &x, Integer &y ) {
    return x.is_zero() or y.is_zero() ? 0 : x.length() + y.length();
}


std::int32_t IntegerOps::unsigned_quo_result_length( Integer &x, Integer &y ) {
    return max( x.length() - y.length() + 1, 0 );
}


std::int32_t IntegerOps::unsigned_rem_result_length( Integer &x, Integer &y ) {
    static_cast<void>(x); // unused

    return y.length();
}
