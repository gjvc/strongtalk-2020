
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
    ssize_t _signed_length;
    Digit   _first_digit;

    std::int32_t length() const;

    void set_length( std::int32_t l );

    Digit *digits() const;

    Digit &operator[]( std::int32_t i ) const;

    static std::int32_t length_to_size_in_bytes( std::int32_t l );

    std::int32_t length_in_bits() const;

    std::int32_t signum() const;
    // returns < 0 for x < 0; 0 for x == 0; > 0 for x > 0

    std::int32_t size_in_bytes() const;


    void print();


    std::int32_t as_int( bool &ok ) const;

    std::uint32_t as_unsigned_int( bool &ok ) const;

    double as_double( bool &ok ) const;

    SMIOop as_smi( bool &ok ) const;


    bool is_zero() const;

    bool is_not_zero() const;

    bool is_positive() const;

    bool is_negative() const;

    bool is_odd() const;

    bool is_even() const;

    bool is_valid() const;

    friend class IntegerOps;
};
