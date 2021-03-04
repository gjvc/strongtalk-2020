
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <array>
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/oop/MarkOopDescriptor.hpp"
#include "allocation.hpp"


// Age table for feedback-mediated tenuring (generation scavenging).
// Note: all sizes are in OOP_SIZE
class AgeTable : public CHeapAllocatedObject {

public:
    AgeTable();
    virtual ~AgeTable() = default;
    auto operator<=>( const AgeTable & ) const = default;

    AgeTable( const AgeTable & ) = default;

    AgeTable &operator=( const AgeTable & ) = default;


    static constexpr std::int32_t table_size = MarkOopDescriptor::max_age + 1;

    std::array<std::int32_t, table_size> _sizes;




    // operations
    void clear() {
        set_words( &_sizes[ 0 ], table_size, 0 );
    }


    void add( MemOop p, std::int32_t oop_size ) {
        std::int32_t age = p->mark()->age();
        st_assert( age >= 0 and age < table_size, "invalid age of object" );
        _sizes[ age ] += oop_size;
    }


    std::int32_t tenure_size( std::int32_t age );

    std::int32_t tenuring_threshold( std::int32_t oop_size );
};
