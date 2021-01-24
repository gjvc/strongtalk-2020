//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/NativeMethodScopes.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/MethodScopeNode.hpp"


void MethodScopeNode::generate( ScopeDescriptorRecorder *rec, std::size_t senderScopeOffset, bool_t bigHeader ) {
    ScopeDescriptorNode::generate( rec, senderScopeOffset, bigHeader );
    rec->genOop( _lookupKey->klass() );
    rec->genOop( _lookupKey->selector_or_method() );
    _receiverLocation->generate( rec );
}


void MethodScopeNode::verify( ScopeDescriptor *sd ) {
    ScopeDescriptorNode::verify( sd );
    if ( not sd->isMethodScope() ) st_fatal( "MethodScope expected" );
}
