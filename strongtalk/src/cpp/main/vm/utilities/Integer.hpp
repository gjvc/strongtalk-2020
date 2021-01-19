
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/SMIKlass.hpp"


// -----------------------------------------------------------------------------

typedef std::uint32_t Digit;
typedef std::uint64_t DoubleDigit;


// -----------------------------------------------------------------------------

//
// Integer is the representation for arbitrary length integers.
// A non-zero Integer x is represented by n Digits to a base B such that:
//
//  x = x[n-1]*B^(n-1) + x[n-2]*B^(n-2) + ... + x[1]*B + x[0]
//
// with 0 <= x[i] < B for 0 <= i < n and x[n-1] > 0.
//
// n is the length of x.
// x = 0 is represented by the length n = 0 and no digits.
//

class Integer : ValueObject {

public:
    int   _signed_length;
    Digit _first_digit;

    int length() const;

    void set_length( int l );

    Digit *digits() const;

    Digit &operator[]( std::size_t i ) const;

    static int length_to_size_in_bytes( int l );

    std::size_t length_in_bits() const;

    int signum() const;
    // returns < 0 for x < 0; 0 for x == 0; > 0 for x > 0

    std::size_t size_in_bytes() const;


    void print();


    int as_int( bool_t &ok ) const;

    std::uint32_t as_unsigned_int( bool_t &ok ) const;

    double as_double( bool_t &ok ) const;

    SMIOop as_smi( bool_t &ok ) const;


    bool_t is_zero() const;

    bool_t is_not_zero() const;

    bool_t is_positive() const;

    bool_t is_negative() const;

    bool_t is_odd() const;

    bool_t is_even() const;

    bool_t is_valid() const;

    friend class IntegerOps;
};
