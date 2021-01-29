//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/LongInteger64.hpp"


LongInteger64::LongInteger64() {
}


LongInteger64::LongInteger64( std::uint32_t low, std::uint32_t high ) {
    _low  = low;
    _high = high;
}


LongInteger64::LongInteger64( double value ) {
    *(int64_t *) &_low = value;
}


LongInteger64 LongInteger64::operator+( const LongInteger64 &rhs ) {
    return LongInteger64( *(int64_t *) &_low + *(int64_t *) &rhs._low );
}


LongInteger64 LongInteger64::operator-( const LongInteger64 &rhs ) {
    LongInteger64 result = *(int64_t *) &_low - *(int64_t *) &rhs._low;
    return result;
}


bool LongInteger64::operator==( const LongInteger64 &rhs ) {
    return *(int64_t *) &_low == *(int64_t *) &rhs._low;
}


bool LongInteger64::operator!=( const LongInteger64 &rhs ) {
    return *(int64_t *) &_low not_eq *(int64_t *) &rhs._low;
}


double LongInteger64::as_double() {
    return double( *(int64_t *) &_low );
}
