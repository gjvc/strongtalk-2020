//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/AgeTable.hpp"


AgeTable::AgeTable() {
    clear();
}


int AgeTable::tenure_size( int age ) {
    int total = 0;
    for ( ; age < table_size; age++ ) {
        total += _sizes[ age ];
    }
    return total;
}


int AgeTable::tenuring_threshold( int size ) {
    int total = 0;
    int age   = 1;
    for ( ; age < table_size; age++ ) {
        total += _sizes[ age ];
        if ( total > size )
            // zero will promote too much garbage
            return age == 1 ? 1 : age - 1;
    }
    return age;
}
