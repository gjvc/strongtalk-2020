//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/utilities/GrowableArray.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/code/LogicalAddress.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"

enum {
    METHOD_CODE,            //
    BLOCK_CODE,             //
    TOP_LEVEL_BLOCK_CODE,   //
    NON_INLINED_BLOCK_CODE  //
};


//
// Class hierarchy for nodes generating ScopeDescriptor instances.
// ScopeDescriptorNode
//  - MethodScopeNode
//  - TopLevelBlockScopeNode
//  - BlockScopeNode
//

constexpr std::size_t INVALID_OFFSET = -1;

class LogicalAddress;

class ScopeDescriptorNode : public ResourceObject {

public:
    MethodOop _method;
    bool_t    _allocates_compiled_context;
    std::size_t       _scopeID;
    bool_t    _lite;
    std::size_t       _senderByteCodeIndex;
    bool_t    _visible;

    GrowableArray<LogicalAddress *> *_arg_list;
    GrowableArray<LogicalAddress *> *_temp_list;
    GrowableArray<LogicalAddress *> *_context_temp_list;
    GrowableArray<LogicalAddress *> *_expr_stack_list;

    std::size_t _offset; // byte offset to the encoded scopeDesc Initial value is  INVALID_OFFSET

    bool_t _usedInPcs;

public:
    bool_t has_args() const;

    bool_t has_temps() const;

    bool_t has_context_temps() const;

    bool_t has_expr_stack() const;

    bool_t has_context() const;

    bool_t has_nameDescs() const;

    ScopeInfo _scopesHead;
    ScopeInfo _scopesTail;
    ScopeInfo _next;

    ScopeDescriptorNode( MethodOop method, bool_t allocates_compiled_context, std::size_t scopeID, bool_t lite, std::size_t senderByteCodeIndex, bool_t visible );

    void addNested( ScopeInfo scope );

    virtual std::uint8_t code() = 0;

    virtual void generate( ScopeDescriptorRecorder *rec, std::size_t senderScopeOffset, bool_t bigHeader );

    void generateBody( ScopeDescriptorRecorder *rec, std::size_t senderScopeOffset );

    void generateNameDescs( ScopeDescriptorRecorder *rec );

    void generate_solid( GrowableArray<LogicalAddress *> *list, ScopeDescriptorRecorder *rec );

    void generate_sparse( GrowableArray<LogicalAddress *> *list, ScopeDescriptorRecorder *rec );

    bool_t computeVisibility();

    ScopeInfo find_scope( std::size_t scope_id );

    virtual void verify( ScopeDescriptor *sd );

    void verifyBody();

};


class TopLevelBlockScopeNode : public ScopeDescriptorNode {

public:
    LogicalAddress *_receiverLocation;
    KlassOop _receiverKlass;


    std::uint8_t code() {
        return TOP_LEVEL_BLOCK_CODE;
    }


public:

    TopLevelBlockScopeNode( MethodOop method, LogicalAddress *receiver_location, KlassOop receiver_klass, bool_t allocates_compiled_context ) :
            ScopeDescriptorNode( method, allocates_compiled_context, false, 0, 0, true ) {
        _receiverLocation = receiver_location;
        _receiverKlass    = receiver_klass;
    }


    void generate( ScopeDescriptorRecorder *rec, std::size_t senderScopeOffset, bool_t bigHeader ) {
        ScopeDescriptorNode::generate( rec, senderScopeOffset, bigHeader );
        _receiverLocation->generate( rec );
        rec->genOop( _receiverKlass );
    }


    void verify( ScopeDescriptor *sd ) {
        ScopeDescriptorNode::verify( sd );
        if ( not sd->isTopLevelBlockScope() ) st_fatal( "TopLevelBlockScope expected" );
    }
};
