//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"


class CallBack : AllStatic {

    public:
        static void initialize( Oop receiver, SymbolOop selector );

        static void * registerPascalCall( int index, int nofArgs );

        static void * registerCCall( int index );

        static void unregister( void * block );
};
