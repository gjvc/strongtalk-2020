//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/oops/DoubleKlass.hpp"


void DoubleOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    MemOopDescriptor::bootstrap_header( stream );
    set_value( stream->read_double() );
}
