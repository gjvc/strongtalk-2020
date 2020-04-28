//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/smiOopDescriptor.hpp"
#include "vm/utilities/OutputStream.hpp"


void smiOopDescriptor::print_on( ConsoleOutputStream * stream ) {
    stream->print( "%ld", ( int32_t ) value() );
}

