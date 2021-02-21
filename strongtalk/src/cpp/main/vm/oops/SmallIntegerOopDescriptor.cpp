//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/SmallIntegerOopDescriptor.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


void SmallIntegerOopDescriptor::print_on( ConsoleOutputStream *stream ) {
    stream->print( "%ld", (std::int32_t) value() );
}
