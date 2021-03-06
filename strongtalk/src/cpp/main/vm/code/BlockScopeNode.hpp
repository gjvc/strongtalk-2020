//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/ScopeDescriptor.hpp"


class BlockScopeNode : public ScopeDescriptorNode {

public:
    ScopeInfo _parent;


    BlockScopeNode( MethodOop method, ScopeInfo parent, bool allocates_compiled_context, bool lite, std::int32_t scopeID, std::int32_t senderByteCodeIndex, bool visible ) :
        ScopeDescriptorNode( method, allocates_compiled_context, scopeID, lite, senderByteCodeIndex, visible ),
        _parent{ parent } {
    }


    BlockScopeNode() = default;
    virtual ~BlockScopeNode() = default;
    BlockScopeNode( const BlockScopeNode & ) = default;
    BlockScopeNode &operator=( const BlockScopeNode & ) = default;


    void operator delete( void *ptr ) { (void)(ptr); }


    std::uint8_t code() {
        return BLOCK_CODE;
    }


    void generate( ScopeDescriptorRecorder *rec, std::int32_t senderScopeOffset, bool bigHeader );

    void verify( ScopeDescriptor *sd );

};
