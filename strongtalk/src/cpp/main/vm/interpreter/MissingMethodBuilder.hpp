//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/interpreter/HeapCodeBuffer.hpp"
#include "vm/runtime/ResourceObject.hpp"


class MissingMethodBuilder : public ResourceObject {

    private:
        HeapCodeBuffer _buffer;
        SymbolOop      _selector;
        MethodOop      _method;

    public:
        MissingMethodBuilder( SymbolOop selector ) :
            _selector( selector ) {
            _method = nullptr;
        };

        void build();

        ByteArrayOop bytes();

        ObjectArrayOop oops();


        MethodOop method() {
            return _method;
        };
};

