
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

constexpr std::int32_t SIGN_LENGTH     = 1;      //
constexpr std::int32_t EXPONENT_LENGTH = 11;     //
constexpr std::int32_t MANTISSA_LENGTH = 52;     //
constexpr std::int32_t DOUBLE_LENGTH   = 64;     //
constexpr std::int32_t EXPONENT_BIAS   = 1023;   //
constexpr std::int32_t MAX_EXPONENT    = 2046;   //
