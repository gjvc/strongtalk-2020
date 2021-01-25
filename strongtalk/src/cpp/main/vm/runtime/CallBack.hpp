//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"


class CallBack : AllStatic {

public:
    static void initialize( Oop receiver, SymbolOop selector );

    static void *registerPascalCall( std::int32_t index, std::int32_t nofArgs );

    static void *registerCCall( std::int32_t index );

    static void unregister( void *block );
};
