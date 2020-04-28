//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
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


        BlockScopeNode( MethodOop method, ScopeInfo parent, bool_t allocates_compiled_context, bool_t lite, int scopeID, int senderByteCodeIndex, bool_t visible ) :
            ScopeDescriptorNode( method, allocates_compiled_context, scopeID, lite, senderByteCodeIndex, visible ) {
            _parent = parent;
        }


        uint8_t code() {
            return BLOCK_CODE;
        }


        void generate( ScopeDescriptorRecorder * rec, int senderScopeOffset, bool_t bigHeader );

        void verify( ScopeDescriptor * sd );
};
