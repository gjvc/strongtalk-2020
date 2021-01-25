//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/ScopeDescriptor.hpp"

class BlockScopeNode : public ScopeDescriptorNode {

public:
    ScopeInfo _parent;


    BlockScopeNode( MethodOop method, ScopeInfo parent, bool_t allocates_compiled_context, bool_t lite, std::int32_t scopeID, std::int32_t senderByteCodeIndex, bool_t visible ) :
            ScopeDescriptorNode( method, allocates_compiled_context, scopeID, lite, senderByteCodeIndex, visible ) {
        _parent = parent;
    }


    std::uint8_t code() {
        return BLOCK_CODE;
    }


    void generate( ScopeDescriptorRecorder *rec, std::int32_t senderScopeOffset, bool_t bigHeader );

    void verify( ScopeDescriptor *sd );
};
