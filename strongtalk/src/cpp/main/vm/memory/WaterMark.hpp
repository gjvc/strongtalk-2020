
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/memory/util.hpp"


// A Watermark points into a Space and is used during scavenge to keep track of progress.
class NewWaterMark : ValueObject {
public:
    Oop *_point;


    NewWaterMark() :
        ValueObject(),
        _point{ nullptr } {
    }

    virtual ~NewWaterMark() = default;
    NewWaterMark( const NewWaterMark & ) = default;
    NewWaterMark &operator=( const NewWaterMark & ) = default;

};


class OldSpace;

class OldWaterMark : ValueObject {
public:
    Oop      *_point;
    OldSpace *_space;

    Oop *pseudo_allocate( std::int32_t size );


    OldWaterMark() :
        ValueObject(),
        _point{ nullptr },
        _space{ nullptr } {
    }


    virtual ~OldWaterMark() = default;
    OldWaterMark( const OldWaterMark & ) = default;
    OldWaterMark &operator=( const OldWaterMark & ) = default;

};


inline bool operator==( const NewWaterMark &x, const NewWaterMark &y ) {
    return x._point == y._point;
}


inline bool operator!=( const NewWaterMark &x, const NewWaterMark &y ) {
    return not( x == y );
}


inline bool operator==( const OldWaterMark &x, const OldWaterMark &y ) {
    return ( x._space == y._space ) and ( x._point == y._point );
}


inline bool operator!=( const OldWaterMark &x, const OldWaterMark &y ) {
    return not( x == y );
}
