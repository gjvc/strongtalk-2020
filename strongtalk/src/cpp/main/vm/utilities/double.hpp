
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once



// -----------------------------------------------------------------------------
// double layout
//
//      double fields: [s|exponent|mantissa]
//      field lengths: |1|<--11-->|<--52-->|

constexpr std::size_t sign_length     = 1;      //
constexpr std::size_t exponent_length = 11;     //
constexpr std::size_t mantissa_length = 52;     //
constexpr std::size_t double_length   = 64;     //
constexpr std::size_t exponent_bias   = 1023;   //
constexpr std::size_t max_exponent    = 2046;   //
