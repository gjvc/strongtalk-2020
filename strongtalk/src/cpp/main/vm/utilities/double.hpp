
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

constexpr int sign_length     = 1;      //
constexpr int exponent_length = 11;     //
constexpr int mantissa_length = 52;     //
constexpr int double_length   = 64;     //
constexpr int exponent_bias   = 1023;   //
constexpr int max_exponent    = 2046;   //
