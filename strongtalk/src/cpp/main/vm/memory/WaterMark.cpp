
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/runtime/flags.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/memory/Space.hpp"
#include "vm/memory/WaterMark.hpp"


Oop * OldWaterMark::pseudo_allocate( int size ) {
    Oop * p = _point;
    if ( p + size < _space->end() ) {
        _point = p + size;
    } else {
        lprintf( "crossing Space\n" );
        st_fatal( "not implemented yet" );
    }
    return p;
}
