//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/memory/SymbolTable.hpp"
#include "vm/utilities/OutputStream.hpp"


SymbolOop SymbolOopDescriptor::scavenge() {
    ShouldNotCallThis(); // shouldn't be scavenging canonical symbols
    // (should be tenured);
    return nullptr;
}


bool_t SymbolOopDescriptor::verify() {
    bool_t flag = ByteArrayOopDescriptor::verify();
    if ( flag ) {
        if ( not is_old() ) {
            error( "SymbolOop %#lx isn't tenured", this );
            flag = false;
        }
        SymbolOop s = Universe::symbol_table->lookup( (const char *) bytes(), length() );
        if ( s not_eq this ) {
            error( "SymbolOop %#lx isn't canonical", this );
            flag = false;
        }
    }
    return flag;
}


void SymbolOopDescriptor::print_symbol_on( ConsoleOutputStream *stream ) {
    stream = stream ? stream : _console;
    for ( std::size_t i = 1; i <= length(); i++ )
        stream->put( byte_at( i ) );
}
