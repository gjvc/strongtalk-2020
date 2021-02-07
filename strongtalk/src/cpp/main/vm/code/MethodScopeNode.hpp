//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/code/ScopeDescriptorNode.hpp"


class MethodScopeNode : public ScopeDescriptorNode {

public:
    LookupKey      *_lookupKey;
    LogicalAddress *_receiverLocation;


    std::uint8_t code() {
        return METHOD_CODE;
    }


    MethodScopeNode( LookupKey *key, MethodOop method, LogicalAddress *receiver_location, bool allocates_compiled_context, bool lite, std::int32_t scopeID, std::int32_t senderByteCodeIndex, bool visible ) :
        ScopeDescriptorNode( method, allocates_compiled_context, scopeID, lite, senderByteCodeIndex, visible ),
        _lookupKey{ key },
        _receiverLocation{ receiver_location } {
    }

    MethodScopeNode() = default;
    virtual ~MethodScopeNode() = default;
    MethodScopeNode( const MethodScopeNode & ) = default;
    MethodScopeNode &operator=( const MethodScopeNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    void generate( ScopeDescriptorRecorder *rec, std::int32_t senderScopeOffset, bool bigHeader );

    void verify( ScopeDescriptor *sd );
};
