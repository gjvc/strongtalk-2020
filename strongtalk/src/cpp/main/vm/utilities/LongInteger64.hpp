//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"


class LongInteger64 {

private:
    std::uint32_t _low;
    std::uint32_t _high;

    LongInteger64();

public:
    LongInteger64( std::uint32_t low, std::uint32_t high );

    LongInteger64( double value );

    LongInteger64 operator-( const LongInteger64 &rhs );

    LongInteger64 operator+( const LongInteger64 &rhs );

    bool_t operator==( const LongInteger64 &rhs );

    bool_t operator!=( const LongInteger64 &rhs );

    double as_double();
};
