
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/interpreter/MethodClosure.hpp"
#include "vm/interpreter/MethodNode.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"


MethodClosure::MethodClosure() :
    _method{ nullptr },
    _byteCodeIndex{ PrologueByteCodeIndex },
    _next_byteCodeIndex{ PrologueByteCodeIndex },
    _aborting{ false },
    _in_primitive_failure{ false },
    _float0_index{ 0 } {
}


void MethodClosure::set_method( MethodOop method ) {
    _method = method;
    st_assert( method->number_of_stack_temporaries() % 2 == 0 or not method->has_float_temporaries(), "inconsistency" );
    _float0_index = 256 - method->number_of_stack_temporaries() / 2;
}


std::int32_t MethodClosure::float_at( std::int32_t index ) {
    std::int32_t fno = _float0_index - index;
    st_assert( 0 <= fno and fno < _method->total_number_of_floats(), "illegal float number" );
    return fno;
}


void CustomizedMethodClosure::push_instVar_name( SymbolOop name ) {
    st_unused( name ); // unused
    st_fatal( "instance variable not resolved" );
}


void CustomizedMethodClosure::store_instVar_name( SymbolOop name ) {
    st_unused( name ); // unused
    st_fatal( "instance variable not resolved" );
}


void CustomizedMethodClosure::push_classVar_name( SymbolOop name ) {
    st_unused( name ); // unused
    st_fatal( "class variable not resolved" );
}


void CustomizedMethodClosure::store_classVar_name( SymbolOop name ) {
    st_unused( name ); // unused
    st_fatal( "class variable not resolved" );
}


void CustomizedMethodClosure::push_classVar( AssociationOop assoc ) {
    push_global( assoc );
}


void CustomizedMethodClosure::store_classVar( AssociationOop assoc ) {
    store_global( assoc );
}

// SpecializedMethodClosure

void SpecializedMethodClosure::if_node( IfNode *node ) {
    MethodIterator iter( node->then_code(), this );
    if ( node->else_code() not_eq nullptr ) {
        MethodIterator iter( node->else_code(), this );
    }
}


void SpecializedMethodClosure::cond_node( CondNode *node ) {
    MethodIterator iter( node->expr_code(), this );
}


void SpecializedMethodClosure::while_node( WhileNode *node ) {
    MethodIterator iter( node->expr_code(), this );
    if ( node->body_code() not_eq nullptr ) {
        MethodIterator iter( node->body_code(), this );
    }
}


void SpecializedMethodClosure::primitive_call_node( PrimitiveCallNode *node ) {
    if ( node->failure_code() not_eq nullptr ) {
        MethodIterator iter( node->failure_code(), this );
    }
}


void SpecializedMethodClosure::dll_call_node( DLLCallNode *node ) {
    if ( node->failure_code() not_eq nullptr ) {
        MethodIterator iter( node->failure_code(), this );
    }
}
