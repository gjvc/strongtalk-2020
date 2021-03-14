
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"


// An IntegerFreeList maintains a list of 'available' integers in the range [0, n[ where n is the maximum number of integers ever allocated.
// An IntegerFreeList may be used to allocate/release stack locations.

class IntegerFreeList : public PrintableResourceObject {

protected:
    std::int32_t                _first;     // the first available integer
    GrowableArray<std::int32_t> *_list;     // the list
    std::vector<std::int32_t>   _vector;    // as vector

    void grow();

public:
    IntegerFreeList( std::int32_t size );
    IntegerFreeList() = default;
    virtual ~IntegerFreeList() = default;
    IntegerFreeList( const IntegerFreeList & ) = default;
    IntegerFreeList &operator=( const IntegerFreeList & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    std::int32_t allocate();        // returns a new integer, grows the list if necessary
    std::int32_t allocated();       // returns the number of allocated integers
    void release( std::int32_t i ); // marks the integer i as 'available' again
    std::size_t length();           // the maximum number of integers ever allocated
    void print();                   //

};
