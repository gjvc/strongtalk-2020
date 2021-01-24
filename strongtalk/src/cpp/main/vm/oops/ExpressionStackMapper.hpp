
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"


class ExpressionStackMapper : public MethodClosure {

private:
    GrowableArray<std::size_t> *_mapping;
    std::size_t                _targetByteCodeIndex;


    void map_push() {
        map_push( byteCodeIndex() );
    }


    void map_push( int b ) {
        // lprintf("push(%d)", byteCodeIndex);
        if ( b >= _targetByteCodeIndex ) {
            abort();
        } else {
            _mapping->push( b );
        }
    }


    void map_pop() {
        if ( byteCodeIndex() >= _targetByteCodeIndex ) {
            abort();
        } else {
            // lprintf("pop(%d)", byteCodeIndex());
            _mapping->pop();
        }
    }


    void map_send( bool_t has_receiver, int number_of_arguments ) {
        if ( has_receiver )
            map_pop();
        for ( std::size_t i = 0; i < number_of_arguments; i++ )
            map_pop();
        map_push();
    }


public:
    ExpressionStackMapper( GrowableArray<std::size_t> *mapping, std::size_t targetByteCodeIndex ) {
        this->_mapping             = mapping;
        this->_targetByteCodeIndex = targetByteCodeIndex;
    }


    void push_self() {
        map_push();
    }


    void push_tos() {
        map_push();
    }


    void push_literal( Oop obj ) {
        map_push();
    }


    void push_argument( int no ) {
        map_push();
    }


    void push_temporary( int no ) {
        map_push();
    }


    void push_temporary( int no, int context ) {
        map_push();
    }


    void push_instVar( int offset ) {
        map_push();
    }


    void push_instVar_name( SymbolOop name ) {
        map_push();
    }


    void push_classVar( AssociationOop assoc ) {
        map_push();
    }


    void push_classVar_name( SymbolOop name ) {
        map_push();
    }


    void push_global( AssociationOop obj ) {
        map_push();
    }


    void pop() {
        map_pop();
    }


    void normal_send( InterpretedInlineCache *ic ) {
        map_send( true, ic->selector()->number_of_arguments() );
    }


    void self_send( InterpretedInlineCache *ic ) {
        map_send( false, ic->selector()->number_of_arguments() );
    }


    void super_send( InterpretedInlineCache *ic ) {
        map_send( false, ic->selector()->number_of_arguments() );
    }


    void double_equal() {
        map_send( true, 1 );
    }


    void double_not_equal() {
        map_send( true, 1 );
    }


    void method_return( int nofArgs ) {
        map_pop();
    }


    void nonlocal_return( int nofArgs ) {
        map_pop();
    }


    void allocate_closure( AllocationType type, int nofArgs, MethodOop meth ) {
        if ( type == AllocationType::tos_as_scope )
            map_pop();
        map_push();
    }


    // nodes
    void if_node( IfNode *node );

    void cond_node( CondNode *node );

    void while_node( WhileNode *node );

    void primitive_call_node( PrimitiveCallNode *node );

    void dll_call_node( DLLCallNode *node );


    // call backs to ignore
    void allocate_temporaries( int nofTemps ) {
    }


    void store_temporary( int no ) {
    }


    void store_temporary( int no, int context ) {
    }


    void store_instVar( int offset ) {
    }


    void store_instVar_name( SymbolOop name ) {
    }


    void store_classVar( AssociationOop assoc ) {
    }


    void store_classVar_name( SymbolOop name ) {
    }


    void store_global( AssociationOop obj ) {
    }


    void allocate_context( int nofTemps, bool_t forMethod = false ) {
    }


    void set_self_via_context() {
    }


    void copy_self_into_context() {
    }


    void copy_argument_into_context( int argNo, int no ) {
    }


    void zap_scope() {
    }


    void predict_primitive_call( PrimitiveDescriptor *pdesc, int failure_start ) {
    }


    void float_allocate( int nofFloatTemps, int nofFloatExprs ) {
    }


    void float_floatify( Floats::Function f, int tof ) {
        map_pop();
    }


    void float_move( int tof, int from ) {
    }


    void float_set( int tof, DoubleOop value ) {
    }


    void float_nullary( Floats::Function f, int tof ) {
    }


    void float_unary( Floats::Function f, int tof ) {
    }


    void float_binary( Floats::Function f, int tof ) {
    }


    void float_unaryToOop( Floats::Function f, int tof ) {
        map_push();
    }


    void float_binaryToOop( Floats::Function f, int tof ) {
        map_push();
    }
};
