
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <array>
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/MarkOopDescriptor.hpp"
#include "allocation.hpp"


// Age table for feedback-mediated tenuring (generation scavenging).
// Note: all sizes are in oopSize
class AgeTable : public CHeapAllocatedObject {

public:

    static constexpr std::size_t table_size = MarkOopDescriptor::max_age + 1;

    std::array<int, table_size> _sizes;
    AgeTable();


    // operations
    void clear() {
        set_words( &_sizes[ 0 ], table_size, 0 );
    }


    void add( MemOop p, int oop_size ) {
        int age = p->mark()->age();
        st_assert( age >= 0 and age < table_size, "invalid age of object" );
        _sizes[ age ] += oop_size;
    }


    int tenure_size( int age );

    int tenuring_threshold( int oop_size );
};
