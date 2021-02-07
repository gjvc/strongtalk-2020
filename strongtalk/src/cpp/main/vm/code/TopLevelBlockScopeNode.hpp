//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/ScopeDescriptor.hpp"


class TopLevelBlockScopeNode : public ScopeDescriptorNode {

public:
    LogicalAddress *_receiverLocation;
    KlassOop       _receiverKlass;


    std::uint8_t code() {
        return TOP_LEVEL_BLOCK_CODE;
    }


public:

    TopLevelBlockScopeNode( MethodOop method, LogicalAddress *receiver_location, KlassOop receiver_klass, bool allocates_compiled_context ) :
        ScopeDescriptorNode( method, allocates_compiled_context, false, 0, 0, true ),
        _receiverLocation{ receiver_location },
        _receiverKlass{ receiver_klass } {
    }


    void generate( ScopeDescriptorRecorder *rec, std::int32_t senderScopeOffset, bool bigHeader ) {
        ScopeDescriptorNode::generate( rec, senderScopeOffset, bigHeader );
        _receiverLocation->generate( rec );
        rec->genOop( _receiverKlass );
    }


    void verify( ScopeDescriptor *sd ) {
        ScopeDescriptorNode::verify( sd );
        if ( not sd->isTopLevelBlockScope() ) st_fatal( "TopLevelBlockScope expected" );
    }
};
