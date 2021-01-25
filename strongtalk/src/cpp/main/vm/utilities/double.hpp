
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

constexpr std::int32_t sign_length     = 1;      //
constexpr std::int32_t exponent_length = 11;     //
constexpr std::int32_t mantissa_length = 52;     //
constexpr std::int32_t double_length   = 64;     //
constexpr std::int32_t exponent_bias   = 1023;   //
constexpr std::int32_t max_exponent    = 2046;   //
