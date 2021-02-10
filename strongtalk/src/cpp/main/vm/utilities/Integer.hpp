
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/SMIKlass.hpp"


// -----------------------------------------------------------------------------

typedef std::uint32_t Digit;        //
typedef std::uint64_t DoubleDigit;  //


// -----------------------------------------------------------------------------
// double layout
//
//      double fields: [s|exponent|mantissa]
//      field lengths: |1|<--11-->|<--52-->|

constexpr std::int32_t SIGN_LENGTH     = 1;      //
constexpr std::int32_t EXPONENT_LENGTH = 11;     //
constexpr std::int32_t MANTISSA_LENGTH = 52;     //
constexpr std::int32_t DOUBLE_LENGTH   = 64;     //
constexpr std::int32_t EXPONENT_BIAS   = 1023;   //
constexpr std::int32_t MAX_EXPONENT    = 2046;   //


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

    std::size_t length() const;


    Digit *digits() const;

    Digit &operator[]( std::int32_t i ) const;

    static std::size_t length_to_size_in_bytes( std::size_t l );

    std::size_t length_in_bits() const;

    std::int32_t signed_length() const; // returns < 0 for x < 0; 0 for x == 0; > 0 for x > 0

    std::size_t size_in_bytes() const;

    void set_signed_length( std::int32_t l );

    void print() const;


    std::int32_t as_int32_t( bool &ok ) const;

    std::uint32_t as_uint32_t( bool &ok ) const;

    double as_double( bool &ok ) const;

    SmallIntegerOop as_smi( bool &ok ) const;


    bool is_zero() const;

    bool is_not_zero() const;

    bool is_positive() const;

    bool is_negative() const;

    bool is_odd() const;

    bool is_even() const;

    bool is_valid() const;

    friend class IntegerOps;
};
