//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/code/ScopeDescriptorNode.hpp"


class MethodScopeNode : public ScopeDescriptorNode {

    public:
        LookupKey      * _lookupKey;
        LogicalAddress * _receiverLocation;


        uint8_t code() {
            return METHOD_CODE;
        }


        MethodScopeNode( LookupKey * key, MethodOop method, LogicalAddress * receiver_location, bool_t allocates_compiled_context, bool_t lite, int scopeID, int senderByteCodeIndex, bool_t visible ) :
            ScopeDescriptorNode( method, allocates_compiled_context, scopeID, lite, senderByteCodeIndex, visible ) {
            _lookupKey        = key;
            _receiverLocation = receiver_location;
        }


        void generate( ScopeDescriptorRecorder * rec, int senderScopeOffset, bool_t bigHeader );

        void verify( ScopeDescriptor * sd );
};
