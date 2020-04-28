
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/memory/util.hpp"


// A Watermark points into a Space and is used during scavenge to keep track of progress.
class NewWaterMark : ValueObject {
    public:
        Oop * _point;

};

class OldSpace;
class OldWaterMark : ValueObject {
    public:
        OldSpace * _space;
        Oop      * _point;

        Oop * pseudo_allocate( int size );
};


inline bool_t operator==( const NewWaterMark & x, const NewWaterMark & y ) {
    return x._point == y._point;
}


inline bool_t operator!=( const NewWaterMark & x, const NewWaterMark & y ) {
    return not( x == y );
}


inline bool_t operator==( const OldWaterMark & x, const OldWaterMark & y ) {
    return ( x._space == y._space ) and ( x._point == y._point );
}


inline bool_t operator!=( const OldWaterMark & x, const OldWaterMark & y ) {
    return not( x == y );
}


