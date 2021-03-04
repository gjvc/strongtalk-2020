//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/klass/DoubleValueArrayKlass.hpp"

#include "vm/oop/DoubleValueArrayOopDescriptor.hpp"


bool DoubleValueArrayOopDescriptor::verify() {
    bool flag = MemOopDescriptor::verify();
    if ( flag ) {
        std::int32_t l = length();
        if ( l < 0 ) {
            error( "DoubleValueArrayOop 0x{0:x} has negative length", this );
            flag = false;
        }
    }
    return flag;
}


void DoubleValueArrayOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    MemOopDescriptor::bootstrap_object( stream );

    // Clear eventual padding area
    raw_at_put( size() - 1, smiOop_zero );

    stream->read_oop( length_addr() );
    for ( std::size_t i = 1; i <= length(); i++ )
        double_at_put( i, stream->read_double() );

}
