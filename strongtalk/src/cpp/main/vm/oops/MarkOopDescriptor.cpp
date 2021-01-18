//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/MarkOopDescriptor.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/Process.hpp"


void MarkOopDescriptor::print_on( ConsoleOutputStream *stream ) {
    stream->print( "mark(%c,%c,", has_tagged_contents() ? 'U' : '_', is_near_death() ? 'D' : '_' );
    stream->print( "hash %#lx,", hash() );
    stream->print( "age %d)", age() );
}


int assign_hash( MarkOop &m ) {
    m = m->set_hash( CurrentHash++ );
    return m->hash();
}
