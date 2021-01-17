//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/code/LogicalAddress.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/memory/Array.hpp"


constexpr int INITIAL_ARG_SIZE          = 5;
constexpr int INITIAL_TEMP_SIZE         = 5;
constexpr int INITIAL_CONTEXT_TEMP_SIZE = 5;
constexpr int INITIAL_EXPR_STACK_SIZE   = 10;


ScopeDescriptorNode::ScopeDescriptorNode( MethodOop method, bool_t allocates_compiled_context, int scopeID, bool_t lite, int senderByteCodeIndex, bool_t visible ) {

    _scopeID                    = scopeID;
    _method                     = method;
    _lite                       = lite;
    _senderByteCodeIndex        = senderByteCodeIndex;
    _visible                    = visible;
    _allocates_compiled_context = allocates_compiled_context;

    _arg_list          = new GrowableArray <LogicalAddress *>( INITIAL_ARG_SIZE );
    _temp_list         = new GrowableArray <LogicalAddress *>( INITIAL_TEMP_SIZE );
    _context_temp_list = new GrowableArray <LogicalAddress *>( INITIAL_CONTEXT_TEMP_SIZE );
    _expr_stack_list   = new GrowableArray <LogicalAddress *>( INITIAL_EXPR_STACK_SIZE );

    _offset     = INVALID_OFFSET;
    _scopesHead = nullptr;
    _scopesTail = nullptr;
    _usedInPcs  = false;
}


void ScopeDescriptorNode::addNested( ScopeInfo scope ) {
    scope->_next = nullptr;
    if ( _scopesHead == nullptr ) {
        _scopesHead = _scopesTail = scope;
    } else {
        _scopesTail->_next = scope;
        _scopesTail = scope;
    }
}


void ScopeDescriptorNode::generate( ScopeDescriptorRecorder * rec, int senderScopeOffset, bool_t bigHeader ) {
    _offset = rec->_codes->size();

    rec->genScopeDescHeader( code(), _lite, has_args(), has_temps(), has_context_temps(), has_expr_stack(), has_context(), bigHeader );
    if ( _offset not_eq 0 ) {
        // Save the sender
        rec->genValue( _offset - senderScopeOffset );
        rec->genValue( _senderByteCodeIndex );
    }
    rec->genOop( _method );
    rec->genValue( _scopeID );
}


void ScopeDescriptorNode::generateNameDescs( ScopeDescriptorRecorder * rec ) {
    st_assert( has_nameDescs(), "must have nameDescs" );
    if ( has_args() )
        generate_solid( _arg_list, rec );
    if ( has_temps() )
        generate_solid( _temp_list, rec );
    if ( has_context_temps() )
        generate_solid( _context_temp_list, rec );
    if ( has_expr_stack() )
        generate_sparse( _expr_stack_list, rec );
}


void ScopeDescriptorNode::generateBody( ScopeDescriptorRecorder * rec, int senderScopeOffset ) {
    if ( has_nameDescs() ) {
        generateNameDescs( rec );
        if ( not rec->updateScopeDescHeader( _offset, rec->_codes->size() ) ) {
            // the offsets in the encoded scopeDesc header could not be
            // written in the pre-allocated two bytes.
            // Make the header large enough for extended index.
            rec->_codes->setTop( _offset );
            generate( rec, senderScopeOffset, true );
            generateNameDescs( rec );
            rec->updateExtScopeDescHeader( _offset, rec->_codes->size() );
        }
    }

    for ( ScopeInfo p = _scopesHead; p not_eq nullptr; p = p->_next ) {
        if ( p->_visible ) {
            p->generate( rec, _offset, false );
            p->generateBody( rec, _offset );
        }
    }
}


void ScopeDescriptorNode::verify( ScopeDescriptor * sd ) {
    if ( _senderByteCodeIndex not_eq IllegalByteCodeIndex and _senderByteCodeIndex not_eq sd->senderByteCodeIndex() ) st_fatal( "senderByteCodeIndex is wrong" );
}


// XXX
ScopeDescriptor * _getNextScopeDescriptor();


void ScopeDescriptorNode::verifyBody() {
    for ( ScopeInfo p = _scopesHead; p not_eq nullptr; p = p->_next ) {
        if ( p->_visible ) {
            p->verify( _getNextScopeDescriptor() );
            p->verifyBody();
        }
    }
}


bool_t ScopeDescriptorNode::computeVisibility() {
    _visible = false;
    for ( ScopeInfo p = _scopesHead; p not_eq nullptr; p = p->_next ) {
        _visible = p->computeVisibility() or _visible;
    }

    _visible = _visible or ( _usedInPcs and GenerateLiteScopeDescs ) or not _lite;
    return _visible;
}


ScopeInfo ScopeDescriptorNode::find_scope( int scope_id ) {
    if ( _scopeID == scope_id )
        return this;
    for ( ScopeInfo p = _scopesHead; p not_eq nullptr; p = p->_next ) {
        ScopeInfo result = p->find_scope( scope_id );
        if ( result )
            return result;
    }
    return nullptr;
}


void ScopeDescriptorNode::generate_solid( GrowableArray <LogicalAddress *> * list, ScopeDescriptorRecorder * rec ) {
    // Dump all the elements
    for ( int i = 0; i < list->length(); i++ ) {
        st_assert( list->at( i ), "must be a solid array" );
        list->at( i )->generate( rec );
    }
    // Terminate the list
    rec->emit_termination_node();
}


void ScopeDescriptorNode::generate_sparse( GrowableArray <LogicalAddress *> * list, ScopeDescriptorRecorder * rec ) {
    // Dump all the elements
    for ( int i = 0; i < list->length(); i++ ) {
        if ( list->at( i ) ) {
            list->at( i )->generate( rec );
            rec->genValue( i );
        }
    }
    // Terminate the list
    rec->emit_termination_node();
}


bool_t ScopeDescriptorNode::has_args() const {
    return not _lite and not _arg_list->isEmpty();
}


bool_t ScopeDescriptorNode::has_temps() const {
    return not _lite and not _temp_list->isEmpty();
}


bool_t ScopeDescriptorNode::has_context_temps() const {
    return not _lite and not _context_temp_list->isEmpty();
}


bool_t ScopeDescriptorNode::has_expr_stack() const {
    return not _lite and not _expr_stack_list->isEmpty();
}


bool_t ScopeDescriptorNode::has_context() const {
    return _allocates_compiled_context;
}


bool_t ScopeDescriptorNode::has_nameDescs() const {
    return has_args() or has_temps() or has_context_temps() or has_expr_stack();
}
