//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/oops/OopDescriptor.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"

// symbols are immutable, canonicalized byteArrays.

class SymbolOopDescriptor : public ByteArrayOopDescriptor {

public:
    friend SymbolOop as_symbolOop( void *p );

    // memory operations
    SymbolOop scavenge();

    bool_t verify();

    void print_symbol_on( ConsoleOutputStream *stream = nullptr );
};


inline SymbolOop as_symbolOop( void *p ) {
    return SymbolOop( as_byteArrayOop( p ) );
}
