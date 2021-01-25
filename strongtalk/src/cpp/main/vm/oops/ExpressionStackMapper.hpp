
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
    GrowableArray<std::int32_t> *_mapping;
    std::int32_t                _targetByteCodeIndex;


    void map_push() {
        map_push( byteCodeIndex() );
    }


    void map_push( std::int32_t b ) {
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


    void map_send( bool_t has_receiver, std::int32_t number_of_arguments ) {
        if ( has_receiver )
            map_pop();
        for ( std::int32_t i = 0; i < number_of_arguments; i++ )
            map_pop();
        map_push();
    }


public:
    ExpressionStackMapper( GrowableArray<std::int32_t> *mapping, std::int32_t targetByteCodeIndex ) {
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


    void push_argument( std::int32_t no ) {
        map_push();
    }


    void push_temporary( std::int32_t no ) {
        map_push();
    }


    void push_temporary( std::int32_t no, std::int32_t context ) {
        map_push();
    }


    void push_instVar( std::int32_t offset ) {
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


    void method_return( std::int32_t nofArgs ) {
        map_pop();
    }


    void nonlocal_return( std::int32_t nofArgs ) {
        map_pop();
    }


    void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) {
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
    void allocate_temporaries( std::int32_t nofTemps ) {
    }


    void store_temporary( std::int32_t no ) {
    }


    void store_temporary( std::int32_t no, std::int32_t context ) {
    }


    void store_instVar( std::int32_t offset ) {
    }


    void store_instVar_name( SymbolOop name ) {
    }


    void store_classVar( AssociationOop assoc ) {
    }


    void store_classVar_name( SymbolOop name ) {
    }


    void store_global( AssociationOop obj ) {
    }


    void allocate_context( std::int32_t nofTemps, bool_t forMethod = false ) {
    }


    void set_self_via_context() {
    }


    void copy_self_into_context() {
    }


    void copy_argument_into_context( std::int32_t argNo, std::int32_t no ) {
    }


    void zap_scope() {
    }


    void predict_primitive_call( PrimitiveDescriptor *pdesc, std::int32_t failure_start ) {
    }


    void float_allocate( std::int32_t nofFloatTemps, std::int32_t nofFloatExprs ) {
    }


    void float_floatify( Floats::Function f, std::int32_t tof ) {
        map_pop();
    }


    void float_move( std::int32_t tof, std::int32_t from ) {
    }


    void float_set( std::int32_t tof, DoubleOop value ) {
    }


    void float_nullary( Floats::Function f, std::int32_t tof ) {
    }


    void float_unary( Floats::Function f, std::int32_t tof ) {
    }


    void float_binary( Floats::Function f, std::int32_t tof ) {
    }


    void float_unaryToOop( Floats::Function f, std::int32_t tof ) {
        map_push();
    }


    void float_binaryToOop( Floats::Function f, std::int32_t tof ) {
        map_push();
    }
};
