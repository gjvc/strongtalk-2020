//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"


class LongInteger64 {

    private:
        uint32_t _low;
        uint32_t _high;

        LongInteger64();

    public:
        LongInteger64( uint32_t low, uint32_t high );

        LongInteger64( double value );

        LongInteger64 operator-( const LongInteger64 & rhs );

        LongInteger64 operator+( const LongInteger64 & rhs );

        bool_t operator==( const LongInteger64 & rhs );

        bool_t operator!=( const LongInteger64 & rhs );

        double as_double();
};
