//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/ScopeDescriptor.hpp"


class TopLevelBlockScopeNode: public ScopeDescriptorNode {
public:
    LogicalAddress* receiver_location;
    KlassOop        receiver_klass;

    std::uint8_t code() { return TOP_LEVEL_BLOCK_CODE; }

    TopLevelBlockScopeNode(MethodOop  method, LogicalAddress* receiver_location, KlassOop receiver_klass, bool allocates_compiled_context)
        : ScopeDescriptorNode(method, allocates_compiled_context, false, 0, NULL, true) {
        this->receiver_location = receiver_location;
        this->receiver_klass    = receiver_klass;
    }

    void generate(ScopeDescriptorRecorder* rec, int senderScopeOffset, bool bigHeader) {
        ScopeDescriptorNode::generate(rec, senderScopeOffset, bigHeader);
        receiver_location->generate(rec);
        rec->genOop(receiver_klass);
    }

    void verify(ScopeDescriptor* sd) {
        ScopeDescriptorNode::verify(sd);
        if (!sd->isTopLevelBlockScope()) st_fatal("TopLevelBlockScope expected");
    }
};
