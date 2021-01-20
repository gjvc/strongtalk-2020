//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"


// ReservedSpace is a data structure for reserving a contiguous chunk of memory.

class ReservedSpace : public ValueObject {
private:
    const char *_base;
    std::size_t _size;
public:
    ReservedSpace( std::size_t size );


    ReservedSpace( const char *base, std::size_t size ) {
        _base = base;
        _size = size;
    }


    // Accessors
    const char *base() {
        return _base;
    }


    std::size_t size() {
        return _size;
    }


    bool_t is_reserved() {
        return _base not_eq nullptr;
    }


    // Splitting
    ReservedSpace first_part( int partition_size );

    ReservedSpace last_part( int partition_size );

    // Alignment
    static std::size_t page_align_size( std::size_t size );

    static std::size_t align_size( std::size_t size, int page_size );
};
