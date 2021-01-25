
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/double.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"
#include "vm/utilities/OutputStream.hpp"


std::int32_t Integer::length() const {
    return abs( _signed_length );
}


std::int32_t Integer::length_in_bits() const {

    if ( is_zero() ) {
        return 0;
    }

    std::int32_t i = length() - 1;
    return i * logB + ::length_in_bits( operator[]( i ) );

}


std::int32_t Integer::as_int( bool_t &ok ) const {

    ok = true;
    switch ( _signed_length ) {
        case -1:
            if ( -std::int32_t( _first_digit ) < 0 )
                return -std::int32_t( _first_digit );
            break;
        case 0:
            return 0;
        case 1:
            if ( std::int32_t( _first_digit ) > 0 )
                return std::int32_t( _first_digit );
            break;
    }
    ok = false;
    return 0;
}


std::uint32_t Integer::as_unsigned_int( bool_t &ok ) const {
    ok = true;
    switch ( _signed_length ) {
        case 0:
            return 0;
        case 1:
            return (std::uint32_t) _first_digit;
            break;
    }
    ok = false;
    return 0;
}


double Integer::as_double( bool_t &ok ) const {

    // filter out trivial result 0.0
    ok = true;
    if ( is_zero() )
        return 0.0;

    // get an n-Digit integer d[n] built from the n most significant digits of self
    // n needs to be big enough so that we have enough bits for the mantissa (note
    // that the mantissa consists of one (implicit) extra bit which is always 1).
    const std::int32_t n = ( mantissa_length + 1 ) / logB + 2;
    Digit     d[n];
    std::int32_t       l = length();
    std::int32_t       i = 1;
    while ( i <= n ) {
        d[ n - i ] = l - i >= 0 ? operator[]( l - i ) : 0;
        i++;
    }

    // shift d[n] to the left so that the most significant bit of d is shifted out
    std::int32_t left_shift_count = logB - ::length_in_bits( d[ n - 1 ] ) + 1;
    shift_left( d, n, left_shift_count );

    // shift d[n] to the right so it builds the mantissa of a double
    const std::int32_t right_shift_count = sign_length + exponent_length;
    shift_right( d, n, right_shift_count );

    // add exponent to d
    std::int32_t exponent = exponent_bias + length_in_bits() - 1;
    if ( exponent > max_exponent ) {
        // integer too large => doesn't fit into double representation
        ok = false;
        return 0.0;
    }
    st_assert( logB - right_shift_count > 0, "check this code" );
    d[ n - 1 ] = d[ n - 1 ] | ( Digit( exponent ) << ( logB - right_shift_count ) );

    // cast d into double & set sign
    double result = *( (double *) &d[ n - ( double_length / logB ) ] );
    if ( is_negative() )
        result = -result;
    return result;
}


SMIOop Integer::as_smi( bool_t &ok ) const {
    ok = true;
    switch ( _signed_length ) {
        case -1:
            if ( _first_digit <= -smi_min )
                return smiOopFromValue( -std::int32_t( _first_digit ) );
            break;
        case 0:
            return smiOopFromValue( 0 );
        case 1:
            if ( _first_digit <= smi_max )
                return smiOopFromValue( std::int32_t( _first_digit ) );
            break;
    }
    ok = false;
    return smiOopFromValue( 0 );
}


void Integer::print() {
    char s[100000]; // for the time being - FIX THIS
    IntegerOps::Integer_to_string( *this, 10, s );
    std::int32_t i = 0;
    while ( s[ i ] not_eq '\x0' ) {
        _console->print( "%c", s[ i ] );
        i++;
    }
}


void Integer::set_length( std::int32_t l ) {
    _signed_length = l;
}


Digit &Integer::operator[]( std::int32_t i ) const {
    return digits()[ i ];
}


Digit *Integer::digits() const {
    return (Digit *) &_first_digit;
}


std::int32_t Integer::length_to_size_in_bytes( std::int32_t l ) {
    return sizeof( std::int32_t ) + l * sizeof( Digit );
}


std::int32_t Integer::signum() const {
    return _signed_length;
}


bool_t Integer::is_zero() const {
    return signum() == 0;
}


bool_t Integer::is_not_zero() const {
    return signum() not_eq 0;
}


bool_t Integer::is_positive() const {
    return signum() > 0;
}


bool_t Integer::is_negative() const {
    return signum() < 0;
}


bool_t Integer::is_odd() const {
    return is_not_zero() and ( _first_digit & 1 ) == 1;
}


bool_t Integer::is_even() const {
    return not is_odd();
}


bool_t Integer::is_valid() const {
    return is_zero() or operator[]( length() - 1 ) not_eq 0;
}


std::int32_t Integer::size_in_bytes() const {
    return length_to_size_in_bytes( length() );
}
