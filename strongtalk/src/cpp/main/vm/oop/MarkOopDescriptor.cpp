//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oop/MarkOopDescriptor.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


void MarkOopDescriptor::print_on( ConsoleOutputStream *stream ) {
    stream->print( "mark(%c,%c,", has_tagged_contents() ? 'U' : '_', is_near_death() ? 'D' : '_' );
    stream->print( "hash 0x{0:x},", hash() );
    stream->print( "age %d)", age() );
}


std::int32_t assign_hash( MarkOop &m ) {
    m = m->set_hash( CurrentHash++ );
    return m->hash();
}
