
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"

// symbols are immutable, canonicalized byteArrays.

class SymbolOopDescriptor : public ByteArrayOopDescriptor {

public:
    friend SymbolOop as_symbolOop( void *p );

    // memory operations
    SymbolOop scavenge();

    bool verify();

    void print_symbol_on( ConsoleOutputStream *stream = nullptr );
};


inline SymbolOop as_symbolOop( void *p ) {
    return SymbolOop( as_byteArrayOop( p ) );
}
