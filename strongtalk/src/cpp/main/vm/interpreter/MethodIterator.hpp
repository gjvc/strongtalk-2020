//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"


#include "vm/system/os.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/runtime/ResourceObject.hpp"


// The MethodIterator iterates over the byte code structures of a methodOop
// Usage:
//    MethodIterator(method, &SomeMethodClosure);

class IntervalInfo;

// A MethodInterval represents an interval of byte codes
class MethodInterval : public ResourceObject {

protected:
    MethodInterval *_parent;            // enclosing interval (or nullptr if top-level)
    MethodOop _method;
    std::int32_t       _begin_byteCodeIndex;
    std::int32_t       _end_byteCodeIndex;
    bool_t    _in_primitive_failure;            // currently in primitive failure block?
    IntervalInfo *_info;

    void initialize( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex, bool_t failBlock );


    void set_end_byteCodeIndex( std::int32_t byteCodeIndex ) {
        _end_byteCodeIndex = byteCodeIndex;
    }


    // Constructors
    MethodInterval( MethodOop method, MethodInterval *parent );

    MethodInterval( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex = -1, bool_t failureBlock = false );

    friend class MethodIntervalFactory;

public:
    // Test operations
    bool_t includes( std::int32_t byteCodeIndex ) const {
        return begin_byteCodeIndex() <= byteCodeIndex and byteCodeIndex < end_byteCodeIndex();
    }


    // Accessor operations
    MethodOop method() const {
        return _method;
    }


    std::int32_t begin_byteCodeIndex() const {
        return _begin_byteCodeIndex;
    }


    std::int32_t end_byteCodeIndex() const {
        return _end_byteCodeIndex;
    }


    MethodInterval *parent() const {
        return _parent;
    }


    // primitive failure block recognition (for inlining policy of compiler)
    bool_t in_primitive_failure_block() const {
        return _in_primitive_failure;
    }


    void set_primitive_failure( bool_t f ) {
        _in_primitive_failure = f;
    }


    // compiler support (not fully implemented/used yet)
    IntervalInfo *info() const {
        return _info;
    }


    void set_info( IntervalInfo *i ) {
        _info = i;
    }
};


// The hierarchy of structures in a method is as follow:
//  - InlineSendNode
//    - CondNode           (expr_code)
//      - AndNode
//      - OrNode
//    - WhileNode          (expr_code, ?body_code)
//    - IfNode             (then_code, ?else_code)
//  - ExternalCallNode     (?failure_code)
//    - PrimitiveCallNode
//    - DLLCallNode
// The names in the parenthesis show the inlined blocks.
// ? denotes the inline block is optional.

class InlineSendNode : public MethodInterval {
protected:
    InlineSendNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex = -1 );

public:
    // Accessor operation
    virtual SymbolOop selector() const = 0;
};


class CondNode : public InlineSendNode {
protected:
    MethodInterval *_expr_code;

    CondNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );

public:
    // Inlined block
    MethodInterval *expr_code() const {
        return _expr_code;
    }


    // Accessor operations
    virtual bool_t is_and() const = 0;

    virtual bool_t is_or() const = 0;
};


class AndNode : public CondNode {
    AndNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );

    friend class MethodIntervalFactory;

public:
    // Accessor operation
    SymbolOop selector() const;


    bool_t is_and() const {
        return true;
    }


    bool_t is_or() const {
        return false;
    }
};


class OrNode : public CondNode {
protected:
    OrNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );

    friend class MethodIntervalFactory;

public:
    // Accessor operation
    SymbolOop selector() const;


    bool_t is_and() const {
        return false;
    }


    bool_t is_or() const {
        return true;
    }
};


class WhileNode : public InlineSendNode {
protected:
    bool_t _cond;
    MethodInterval *_expr_code;
    MethodInterval *_body_code;

    WhileNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t cond_offset, std::int32_t end_offset );

    friend class MethodIntervalFactory;

public:
    // Mandatory inlined block
    MethodInterval *expr_code() const {
        return _expr_code;
    }


    // Optional inlined block
    MethodInterval *body_code() const {
        return _body_code;
    }


    // Accessor operations
    SymbolOop selector() const;


    bool_t is_whileTrue() const {
        return _cond;
    }


    bool_t is_whileFalse() const {
        return not _cond;
    }
};


class IfNode : public InlineSendNode {
protected:
    bool_t _cond;
    bool_t _ignore_else_while_printing;
    bool_t _produces_result;
    MethodInterval *_then_code;
    MethodInterval *_else_code;

    IfNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t cond, std::int32_t else_offset, std::uint8_t structure );

    friend class MethodIntervalFactory;

public:
    // Inlined block
    MethodInterval *then_code() const {
        return _then_code;
    }


    // Optional inlined block
    MethodInterval *else_code() const {
        return _else_code;
    }


    // Accessor operations
    SymbolOop selector() const;


    bool_t is_ifTrue() const {
        return _cond;
    }


    bool_t is_ifFalse() const {
        return not _cond;
    }


    bool_t ignore_else_while_printing() const {
        return _ignore_else_while_printing;
    }


    bool_t produces_result() const {
        return _produces_result;
    }
};


class ExternalCallNode : public MethodInterval {
protected:
    MethodInterval *_failure_code;

    // Constructors
    ExternalCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex );

    ExternalCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t end_offset );

public:
    // Optional inlined block
    MethodInterval *failure_code() const {
        return _failure_code;
    }
};


class PrimitiveCallNode : public ExternalCallNode {
protected:
    PrimitiveDescriptor *_pdesc;
    bool_t    _has_receiver;
    SymbolOop _name;

    // Constructors
    PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc );

    PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc, std::int32_t end_offset );

    friend class MethodIntervalFactory;

public:
    // Returns the primitive descriptor
    PrimitiveDescriptor *pdesc() const {
        return _pdesc;
    }


    bool_t has_receiver() const {
        return _has_receiver;
    }


    SymbolOop name() const {
        return _name;
    }


    std::int32_t number_of_parameters() const;
};


class Interpreted_DLLCache;

class DLLCallNode : public ExternalCallNode {
protected:
    SymbolOop _dll_name;
    SymbolOop _function_name;
    std::int32_t       _nofArgs;
    dll_func_ptr_t  _function;
    bool_t    _async;

    void initialize( Interpreted_DLLCache *cache );

    // Constructors
    DLLCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, Interpreted_DLLCache *cache );

    friend class MethodIntervalFactory;

public:
    // DLL accessor operations
    SymbolOop dll_name() const {
        return _dll_name;
    }


    SymbolOop function_name() const {
        return _function_name;
    }


    std::int32_t nofArgs() const {
        return _nofArgs;
    }


    dll_func_ptr_t function() const {
        _console->print( "calling DLL [%s], function [%s]", _dll_name, _function_name );
        return _function;
    }


    bool_t async() const {
        return _async;
    }
};


// When creating a block closure, AllocationType specifies what is used in the context field of that block closure.
// When value is sent to the block, the context field is copied into the activation frame of the block.

enum class AllocationType {
    tos_as_scope,        // top of stack is used as context (usually nil or self)
    context_as_scope    // context of current stack frame (i.e. content of temp0) is used a context
};

class MethodIterator;

// A MethodClosure is the object handling the call backs when iterating through a MethodInterval.
// Virtuals are defined for each inline structure and each byte code not defining an inline structure.

class MethodClosure : ValueObject {
private:
    MethodOop _method;                  //
    std::int32_t       _byteCodeIndex;           //
    std::int32_t       _next_byteCodeIndex;      //
    bool_t    _aborting;                //
    bool_t    _in_primitive_failure;    // currently in primitive failure block?
    std::int32_t       _float0_index;            //

    void set_method( MethodOop method );


    void set_byteCodeIndex( std::int32_t byteCodeIndex ) {
        _byteCodeIndex = byteCodeIndex;
    }


    void set_next_byteCodeIndex( std::int32_t next_byteCodeIndex ) {
        _next_byteCodeIndex = next_byteCodeIndex;
    }


    std::int32_t float_at( std::int32_t index );            // converts the float byte code index into a float number

protected:
    MethodClosure();


    // operations to terminate iteration
    virtual void abort() {
        _aborting = true;
    }


    virtual void un_abort() {
        _aborting = false;
    }


    friend class MethodIterator;

public:
    std::int32_t byteCodeIndex() const {
        return _byteCodeIndex;
    }


    std::int32_t next_byteCodeIndex() const {
        return _next_byteCodeIndex;
    }


    MethodOop method() const {
        return _method;
    }


    bool_t aborting() const {
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
    virtual bool_t in_primitive_failure_block() const {
        return _in_primitive_failure;
    }


    virtual void set_primitive_failure( bool_t f ) {
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

    virtual void allocate_context( std::int32_t nofTemps, bool_t forMethod ) = 0;

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
        instruction();
    }


    virtual void push_self() {
        instruction();
    }


    virtual void push_tos() {
        instruction();
    }


    virtual void push_literal( Oop obj ) {
        instruction();
    }


    virtual void push_argument( std::int32_t no ) {
        instruction();
    }


    virtual void push_temporary( std::int32_t no ) {
        instruction();
    }


    virtual void push_temporary( std::int32_t no, std::int32_t context ) {
        instruction();
    }


    virtual void push_instVar( std::int32_t offset ) {
        instruction();
    }


    virtual void push_global( AssociationOop obj ) {
        instruction();
    }


    virtual void store_temporary( std::int32_t no ) {
        instruction();
    }


    virtual void store_temporary( std::int32_t no, std::int32_t context ) {
        instruction();
    }


    virtual void store_instVar( std::int32_t offset ) {
        instruction();
    }


    virtual void store_global( AssociationOop obj ) {
        instruction();
    }


    virtual void pop() {
        instruction();
    }


    virtual void normal_send( InterpretedInlineCache *ic ) {
        instruction();
    }


    virtual void self_send( InterpretedInlineCache *ic ) {
        instruction();
    }


    virtual void super_send( InterpretedInlineCache *ic ) {
        instruction();
    }


    virtual void double_equal() {
        instruction();
    }


    virtual void double_not_equal() {
        instruction();
    }


    virtual void method_return( std::int32_t nofArgs ) {
        instruction();
    }


    virtual void nonlocal_return( std::int32_t nofArgs ) {
        instruction();
    }


    virtual void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) {
        instruction();
    }


    virtual void allocate_context( std::int32_t nofTemps, bool_t forMethod ) {
        instruction();
    }


    virtual void set_self_via_context() {
        instruction();
    }


    virtual void copy_self_into_context() {
        instruction();
    }


    virtual void copy_argument_into_context( std::int32_t argNo, std::int32_t no ) {
        instruction();
    }


    virtual void zap_scope() {
        instruction();
    }


    virtual void predict_primitive_call( PrimitiveDescriptor *pdesc, std::int32_t failure_start ) {
        instruction();
    }


    virtual void float_allocate( std::int32_t nofFloatTemps, std::int32_t nofFloatExprs ) {
        instruction();
    }


    virtual void float_floatify( Floats::Function f, std::int32_t fno ) {
        instruction();
    }


    virtual void float_move( std::int32_t fno, std::int32_t from ) {
        instruction();
    }


    virtual void float_set( std::int32_t fno, DoubleOop value ) {
        instruction();
    }


    virtual void float_nullary( Floats::Function f, std::int32_t fno ) {
        instruction();
    }


    virtual void float_unary( Floats::Function f, std::int32_t fno ) {
        instruction();
    }


    virtual void float_binary( Floats::Function f, std::int32_t fno ) {
        instruction();
    }


    virtual void float_unaryToOop( Floats::Function f, std::int32_t fno ) {
        instruction();
    }


    virtual void float_binaryToOop( Floats::Function f, std::int32_t fno ) {
        instruction();
    }
};


// factory to parameterize construction of nodes
class AbstractMethodIntervalFactory : StackAllocatedObject {
public:
    virtual MethodInterval *new_MethodInterval( MethodOop method, MethodInterval *parent ) = 0;

    virtual MethodInterval *new_MethodInterval( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex = -1, bool_t failureBlock = false ) = 0;

    virtual AndNode *new_AndNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) = 0;

    virtual OrNode *new_OrNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) = 0;

    virtual WhileNode *new_WhileNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t cond_offset, std::int32_t end_offset ) = 0;

    virtual IfNode *new_IfNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t cond, std::int32_t else_offset, std::uint8_t structure ) = 0;

    virtual PrimitiveCallNode *new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc ) = 0;

    virtual PrimitiveCallNode *new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc, std::int32_t end_offset ) = 0;

    virtual DLLCallNode *new_DLLCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, Interpreted_DLLCache *cache ) = 0;
};


// default factory (used by everyone except the compiler)
class MethodIntervalFactory : public AbstractMethodIntervalFactory {
public:
    MethodInterval *new_MethodInterval( MethodOop method, MethodInterval *parent );

    MethodInterval *new_MethodInterval( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex = -1, bool_t failureBlock = false );

    AndNode *new_AndNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );

    OrNode *new_OrNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );

    WhileNode *new_WhileNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t cond_offset, std::int32_t end_offset );

    IfNode *new_IfNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t cond, std::int32_t else_offset, std::uint8_t structure );

    PrimitiveCallNode *new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc );

    PrimitiveCallNode *new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool_t has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc, std::int32_t end_offset );

    DLLCallNode *new_DLLCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, Interpreted_DLLCache *cache );
};


// A MethodIterator iterates over a MethodInterval and dispatches calls to the provided MethodClosure
class MethodIterator : StackAllocatedObject {

private:
    void dispatch( MethodClosure *blk );

    void unknown_code( std::uint8_t code );

    void should_never_encounter( std::uint8_t code );

    MethodInterval *_interval;
    static MethodIntervalFactory defaultFactory;      // default factory

public:
    static AbstractMethodIntervalFactory *factory;      // used to build nodes

    MethodIterator( MethodOop m, MethodClosure *blk, AbstractMethodIntervalFactory *f = &defaultFactory );

    MethodIterator( MethodInterval *interval, MethodClosure *blk, AbstractMethodIntervalFactory *f = &defaultFactory );


    MethodInterval *interval() const {
        return _interval;
    }
};
