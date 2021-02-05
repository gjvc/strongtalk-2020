//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/runtime/ResourceObject.hpp"


class NonInlinedBlockScopeNode : public ResourceObject {

public:
    std::int32_t             _offset;
    NonInlinedBlockScopeNode *_next;
    MethodOop                _method;
    ScopeInfo                _parent;

public:
    NonInlinedBlockScopeNode( MethodOop method, ScopeInfo parent ) :
        _method{ method },
        _parent{ parent },
        _offset{ INVALID_OFFSET },
        _next{ nullptr } {
    }


    std::uint8_t code() {
        return NON_INLINED_BLOCK_CODE;
    }


    void generate( ScopeDescriptorRecorder *rec );

};
