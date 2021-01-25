//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/AgeTable.hpp"


AgeTable::AgeTable() {
    clear();
}


std::int32_t AgeTable::tenure_size( std::int32_t age ) {
    std::int32_t total = 0;
    for ( ; age < table_size; age++ ) {
        total += _sizes[ age ];
    }
    return total;
}


std::int32_t AgeTable::tenuring_threshold( std::int32_t size ) {
    std::int32_t total = 0;
    std::int32_t age   = 1;
    for ( ; age < table_size; age++ ) {
        total += _sizes[ age ];
        if ( total > size )
            // zero will promote too much garbage
            return age == 1 ? 1 : age - 1;
    }
    return age;
}
