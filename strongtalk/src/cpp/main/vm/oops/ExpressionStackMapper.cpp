
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ExpressionStackMapper.hpp"


void ExpressionStackMapper::if_node( IfNode *node ) {
    if ( node->includes( _targetByteCodeIndex ) ) {
        if ( node->then_code()->includes( _targetByteCodeIndex ) ) {
            map_pop();
            MethodIterator i( node->then_code(), this );
        } else if ( node->else_code() and node->else_code()->includes( _targetByteCodeIndex ) ) {
            map_pop();
            MethodIterator i( node->else_code(), this );
        }
        abort();
    } else {
        map_pop();
        if ( node->produces_result() )
            map_push( node->begin_byteCodeIndex() );
    }
}


void ExpressionStackMapper::cond_node( CondNode *node ) {
    if ( node->includes( _targetByteCodeIndex ) ) {
        if ( node->expr_code()->includes( _targetByteCodeIndex ) ) {
            map_pop();
            MethodIterator i( node->expr_code(), this );
        }
        abort();
    } else {
        map_pop();
        map_push( node->begin_byteCodeIndex() );
    }
}


void ExpressionStackMapper::while_node( WhileNode *node ) {
    if ( node->includes( _targetByteCodeIndex ) ) {
        if ( node->expr_code()->includes( _targetByteCodeIndex ) )
            MethodIterator i( node->expr_code(), this );
        else if ( node->body_code() and node->body_code()->includes( _targetByteCodeIndex ) )
            MethodIterator i( node->body_code(), this );
        abort();
    }
}


void ExpressionStackMapper::primitive_call_node( PrimitiveCallNode *node ) {
    std::int32_t nofArgsToPop = node->number_of_parameters();

    for ( std::int32_t i = 0; i < nofArgsToPop; i++ ) {
        map_pop();
    }

    map_push();
    if ( node->failure_code() and node->failure_code()->includes( _targetByteCodeIndex ) ) {
        MethodIterator i( node->failure_code(), this );
    }
}


void ExpressionStackMapper::dll_call_node( DLLCallNode *node ) {

    for ( std::int32_t i = 0; i < node->nofArgs(); i++ )
        map_pop();

}
