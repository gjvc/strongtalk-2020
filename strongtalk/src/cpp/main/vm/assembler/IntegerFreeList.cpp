
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include <array>

#include "vm/assembler/IntegerFreeList.hpp"


IntegerFreeList::IntegerFreeList( std::int32_t size ) :
    _first{ -1 },
    _list{ new GrowableArray<std::int32_t>( 2 ) },
    _vector{} {
    st_assert( _list->length() == 0, "should be zero" );
}


std::int32_t IntegerFreeList::allocate() {

    //
    if ( _first < 0 ) {
        grow();
    }

    //
    std::int32_t i = _first;
    _first = _list->at( i );
    _list->at_put( i, -1 ); // for debugging only

    return i;
}


void IntegerFreeList::grow() {
    _list->append( _first );
    _first = _list->length() - 1;
}


std::int32_t IntegerFreeList::allocated() {
    std::int32_t n = length();
    std::int32_t i = _first;
    while ( i >= 0 ) {
        i = _list->at( i );
        n--;
    }

    st_assert( n >= 0, "should be >= 0" );
    return n;
}


void IntegerFreeList::release( std::int32_t i ) {
    st_assert( _list->at( i ) == -1, "should have been allocated before" );
    _list->at_put( i, _first );
    _first = i;
}


std::size_t IntegerFreeList::length() {
    return _list->length();
}


void IntegerFreeList::print() {
    SPDLOG_INFO( "FreeList 0x{0:x}:", static_cast<const void *>(this) );
    _list->print();
}
