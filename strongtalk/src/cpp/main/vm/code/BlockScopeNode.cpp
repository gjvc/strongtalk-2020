//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/BlockScopeNode.hpp"


void BlockScopeNode::generate( ScopeDescriptorRecorder *rec, int senderScopeOffset, bool_t bigHeader ) {
    ScopeDescriptorNode::generate( rec, senderScopeOffset, bigHeader );
    rec->genValue( _offset - _parent->_offset );
}


void BlockScopeNode::verify( ScopeDescriptor *sd ) {
    ScopeDescriptorNode::verify( sd );
}
