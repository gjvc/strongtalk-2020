
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/interpreter/MethodNode.hpp"
#include "vm/interpreter/MethodIterator.hpp"


// A MethodClosure is the object handling the call backs when iterating through a MethodInterval.
// Virtuals are defined for each inline structure and each byte code not defining an inline structure.

class MethodClosure : ValueObject {
private:
    MethodOop    _method;                  //
    std::int32_t _byteCodeIndex;           //
    std::int32_t _next_byteCodeIndex;      //
    bool         _aborting;                //
    bool         _in_primitive_failure;    // currently in primitive failure block?
    std::int32_t _float0_index;            //

    void set_method( MethodOop method );


    void set_byteCodeIndex( std::int32_t byteCodeIndex ) {
        _byteCodeIndex = byteCodeIndex;
    }


    void set_next_byteCodeIndex( std::int32_t next_byteCodeIndex ) {
        _next_byteCodeIndex = next_byteCodeIndex;
    }


    std::int32_t float_at( std::int32_t index );            // converts the float byte code index into a float number

protected:

    // operations to terminate iteration
    virtual void abort() {
        _aborting = true;
    }


    virtual void un_abort() {
        _aborting = false;
    }


    friend class MethodIterator;

public:

    MethodClosure();
//    ~MethodClosure() = default;
    static void operator delete( void * ) {}


    std::int32_t byteCodeIndex() const {
        return _byteCodeIndex;
    }


    std::int32_t next_byteCodeIndex() const {
        return _next_byteCodeIndex;
    }


    MethodOop method() const {
        return _method;
    }


    bool aborting() const {
        return _aborting;
    }// was iteration aborted?  (dead code)

    // The following function are called when an inlined structure
    // is recognized. It is the functions responsibility to iterate
    // over the structures byte codes.
    virtual void if_node( IfNode *node ) = 0; // inlined if message send
    virtual void cond_node( CondNode *node ) = 0; // inlined or/and message send
    virtual void while_node( WhileNode *node ) = 0; // inlined while message send
    virtual void primitive_call_node( PrimitiveCallNode *node ) = 0; // primitive call with inlined failure block
    virtual void dll_call_node( DLLCallNode *node ) = 0; // dll call

    // primitive failure block recognition (for inlining policy of compiler)
    virtual bool in_primitive_failure_block() const {
        return _in_primitive_failure;
    }


    virtual void set_primitive_failure( bool f ) {
        _in_primitive_failure = f;
    }


public:
    // Specifies the number of additional temporaries that are allocated on the stack
    // and initialized. Note: One temporary is always there and initialized (temp0).
    virtual void allocate_temporaries( std::int32_t nofTemps ) = 0;

    // Push a value on the stack.
    virtual void push_self() = 0;

    virtual void push_tos() = 0;

    virtual void push_literal( Oop obj ) = 0;

    // Argument numbers are 0, 1, ... starting with the first argument (0).
    // Temporary numbers are 0, 1, ... starting with the first temporary (0).
    // Contexts are numbered 0, 1, ... starting with the context held in the
    // current frame (i.e., in temp0). Context i+1 is the context reached by
    // dereferencing the home field of context i. Globals are held in the value
    // field of an association (obj).
    virtual void push_argument( std::int32_t no ) = 0;

    virtual void push_temporary( std::int32_t no ) = 0;

    virtual void push_temporary( std::int32_t no, std::int32_t context ) = 0;

    virtual void push_instVar( std::int32_t offset ) = 0;

    virtual void push_instVar_name( SymbolOop name ) = 0;

    virtual void push_classVar( AssociationOop assoc ) = 0;

    virtual void push_classVar_name( SymbolOop name ) = 0;

    virtual void push_global( AssociationOop obj ) = 0;

    virtual void store_temporary( std::int32_t no ) = 0;

    virtual void store_temporary( std::int32_t no, std::int32_t context ) = 0;

    virtual void store_instVar( std::int32_t offset ) = 0;

    virtual void store_instVar_name( SymbolOop name ) = 0;

    virtual void store_classVar( AssociationOop assoc ) = 0;

    virtual void store_classVar_name( SymbolOop name ) = 0;

    virtual void store_global( AssociationOop obj ) = 0;

    virtual void pop() = 0;

    // The receiver and arguments are pushed prior to a normal send. Only the
    // arguments are pushed for self and super sends (the receiver is self).
    virtual void normal_send( InterpretedInlineCache *ic ) = 0;

    virtual void self_send( InterpretedInlineCache *ic ) = 0;

    virtual void super_send( InterpretedInlineCache *ic ) = 0;

    // Hardwired sends
    virtual void double_equal() = 0;

    virtual void double_not_equal() = 0;

    // nofArgs is the number of arguments to be popped before returning (callee popped arguments).
    virtual void method_return( std::int32_t nofArgs ) = 0;

    virtual void nonlocal_return( std::int32_t nofArgs ) = 0;

    virtual void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) = 0;

    virtual void allocate_context( std::int32_t nofTemps, bool forMethod ) = 0;

    // fp->recv = fp->context->bottom()->recv
    virtual void set_self_via_context() = 0;

    virtual void copy_self_into_context() = 0;

    virtual void copy_argument_into_context( std::int32_t argNo, std::int32_t no ) = 0;

    // fp->context->home
    virtual void zap_scope() = 0;

    // indicates the method is a pure primitive call
    virtual void predict_primitive_call( PrimitiveDescriptor *pdesc, std::int32_t failure_start ) = 0;

    // floating-point operations
    virtual void float_allocate( std::int32_t nofFloatTemps, std::int32_t nofFloatExprs ) = 0;

    virtual void float_floatify( Floats::Function f, std::int32_t fno ) = 0;

    virtual void float_move( std::int32_t fno, std::int32_t from ) = 0;

    virtual void float_set( std::int32_t fno, DoubleOop value ) = 0;

    virtual void float_nullary( Floats::Function f, std::int32_t fno ) = 0;

    virtual void float_unary( Floats::Function f, std::int32_t fno ) = 0;

    virtual void float_binary( Floats::Function f, std::int32_t fno ) = 0;

    virtual void float_unaryToOop( Floats::Function f, std::int32_t fno ) = 0;

    virtual void float_binaryToOop( Floats::Function f, std::int32_t fno ) = 0;
};


class CustomizedMethodClosure : public MethodClosure {

public:

    CustomizedMethodClosure() = default;
    ~CustomizedMethodClosure() = default;
    static void operator delete( void * ) {}

    // virtuals from MethodClosure
    void push_instVar_name( SymbolOop name );

    void store_instVar_name( SymbolOop name );

    void push_classVar_name( SymbolOop name );

    void push_classVar( AssociationOop assoc );

    void store_classVar_name( SymbolOop name );

    void store_classVar( AssociationOop assoc );
};


// A SpecializedMethodClosure is a MethodClosure that provides default implementations for
// all inlined structures as well as empty implementations for methods corresponding to
// individual bytecodes.

// A SpecializedMethodClosure should be used when only a few methods need to be specialized.

class SpecializedMethodClosure : public CustomizedMethodClosure {

public:

    SpecializedMethodClosure() = default;
    ~SpecializedMethodClosure() = default;
    static void operator delete( void * ) {}

    virtual void if_node( IfNode *node );

    virtual void cond_node( CondNode *node );

    virtual void while_node( WhileNode *node );

    virtual void primitive_call_node( PrimitiveCallNode *node );

    virtual void dll_call_node( DLLCallNode *node );

public:
    // Customize this method to get uniform behavior for all instructions.
    virtual void instruction() {
    }


public:
    virtual void allocate_temporaries( std::int32_t nofTemps ) {
        static_cast<void>(nofTemps); // unused
        instruction();
    }


    virtual void push_self() {
        instruction();
    }


    virtual void push_tos() {
        instruction();
    }


    virtual void push_literal( Oop obj ) {
        static_cast<void>(obj); // unused
        instruction();
    }


    virtual void push_argument( std::int32_t no ) {
        static_cast<void>(no); // unused
        instruction();
    }


    virtual void push_temporary( std::int32_t no ) {
        static_cast<void>(no); // unused
        instruction();
    }


    virtual void push_temporary( std::int32_t no, std::int32_t context ) {
        static_cast<void>(no); // unused
        static_cast<void>(context); // unused
        instruction();
    }


    virtual void push_instVar( std::int32_t offset ) {
        static_cast<void>(offset); // unused
        instruction();
    }


    virtual void push_global( AssociationOop obj ) {
        static_cast<void>(obj); // unused
        instruction();
    }


    virtual void store_temporary( std::int32_t no ) {
        static_cast<void>(no); // unused
        instruction();
    }


    virtual void store_temporary( std::int32_t no, std::int32_t context ) {
        static_cast<void>(no); // unused
        static_cast<void>(context); // unused
        instruction();
    }


    virtual void store_instVar( std::int32_t offset ) {
        static_cast<void>(offset); // unused
        instruction();
    }


    virtual void store_global( AssociationOop obj ) {
        static_cast<void>(obj); // unused
        instruction();
    }


    virtual void pop() {
        instruction();
    }


    virtual void normal_send( InterpretedInlineCache *ic ) {
        static_cast<void>(ic); // unused
        instruction();
    }


    virtual void self_send( InterpretedInlineCache *ic ) {
        static_cast<void>(ic); // unused
        instruction();
    }


    virtual void super_send( InterpretedInlineCache *ic ) {
        static_cast<void>(ic); // unused
        instruction();
    }


    virtual void double_equal() {
        instruction();
    }


    virtual void double_not_equal() {
        instruction();
    }


    virtual void method_return( std::int32_t nofArgs ) {
        static_cast<void>(nofArgs); // unused
        instruction();
    }


    virtual void nonlocal_return( std::int32_t nofArgs ) {
        static_cast<void>(nofArgs); // unused
        instruction();
    }


    virtual void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) {
        static_cast<void>(type); // unused
        static_cast<void>(nofArgs); // unused
        static_cast<void>(meth); // unused
        instruction();
    }


    virtual void allocate_context( std::int32_t nofTemps, bool forMethod ) {
        static_cast<void>(nofTemps); // unused
        static_cast<void>(forMethod); // unused
        instruction();
    }


    virtual void set_self_via_context() {
        instruction();
    }


    virtual void copy_self_into_context() {
        instruction();
    }


    virtual void copy_argument_into_context( std::int32_t argNo, std::int32_t no ) {
        static_cast<void>(argNo); // unused
        static_cast<void>(no); // unused
        instruction();
    }


    virtual void zap_scope() {
        instruction();
    }


    virtual void predict_primitive_call( PrimitiveDescriptor *pdesc, std::int32_t failure_start ) {
        static_cast<void>(pdesc); // unused
        static_cast<void>(failure_start); // unused
        instruction();
    }


    virtual void float_allocate( std::int32_t nofFloatTemps, std::int32_t nofFloatExprs ) {
        static_cast<void>(nofFloatTemps); // unused
        static_cast<void>(nofFloatExprs); // unused
        instruction();
    }


    virtual void float_floatify( Floats::Function f, std::int32_t fno ) {
        static_cast<void>(f); // unused
        static_cast<void>(fno); // unused
        instruction();
    }


    virtual void float_move( std::int32_t fno, std::int32_t from ) {
        static_cast<void>(fno); // unused
        static_cast<void>(from); // unused
        instruction();
    }


    virtual void float_set( std::int32_t fno, DoubleOop value ) {
        static_cast<void>(fno); // unused
        static_cast<void>(value); // unused
        instruction();
    }


    virtual void float_nullary( Floats::Function f, std::int32_t fno ) {
        static_cast<void>(f); // unused
        static_cast<void>(fno); // unused
        instruction();
    }


    virtual void float_unary( Floats::Function f, std::int32_t fno ) {
        static_cast<void>(f); // unused
        static_cast<void>(fno); // unused
        instruction();
    }


    virtual void float_binary( Floats::Function f, std::int32_t fno ) {
        static_cast<void>(f); // unused
        static_cast<void>(fno); // unused
        instruction();
    }


    virtual void float_unaryToOop( Floats::Function f, std::int32_t fno ) {
        static_cast<void>(f); // unused
        static_cast<void>(fno); // unused
        instruction();
    }


    virtual void float_binaryToOop( Floats::Function f, std::int32_t fno ) {
        static_cast<void>(f); // unused
        static_cast<void>(fno); // unused
        instruction();
    }
};
