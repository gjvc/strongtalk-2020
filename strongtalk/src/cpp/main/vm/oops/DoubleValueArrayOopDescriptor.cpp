//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/oops/DoubleValueArrayKlass.hpp"

#include "vm/oops/DoubleValueArrayOopDescriptor.hpp"


bool_t DoubleValueArrayOopDescriptor::verify() {
    bool_t flag = MemOopDescriptor::verify();
    if ( flag ) {
        int l = length();
        if ( l < 0 ) {
            error( "doubleValueArrayOop %#lx has negative length", this );
            flag = false;
        }
    }
    return flag;
}


void DoubleValueArrayOopDescriptor::bootstrap_object( Bootstrap * stream ) {
    MemOopDescriptor::bootstrap_object( stream );

    // Clear eventual padding area
    raw_at_put( size() - 1, smiOop_zero );

    stream->read_oop( length_addr() );
    for ( int i = 1; i <= length(); i++ )
        double_at_put( i, stream->read_double() );

}


