
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/interpreter/MethodInterval.hpp"


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


    InlineSendNode() = default;
    virtual ~InlineSendNode() = default;
    InlineSendNode( const InlineSendNode & ) = default;
    InlineSendNode &operator=( const InlineSendNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

public:
    // Accessor operation
    virtual SymbolOop selector() const = 0;
};


class CondNode : public InlineSendNode {
protected:
    MethodInterval *_expr_code;

    CondNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );

    CondNode() = default;
    virtual ~CondNode() = default;
    CondNode( const CondNode & ) = default;
    CondNode &operator=( const CondNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

public:
    // Inlined block
    MethodInterval *expr_code() const {
        return _expr_code;
    }


    // Accessor operations
    virtual bool is_and() const = 0;

    virtual bool is_or() const = 0;
};


class AndNode : public CondNode {
    AndNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );


    AndNode() = default;
    virtual ~AndNode() = default;
    AndNode( const AndNode & ) = default;
    AndNode &operator=( const AndNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    friend class MethodIntervalFactory;

public:
    // Accessor operation
    SymbolOop selector() const;


    bool is_and() const {
        return true;
    }


    bool is_or() const {
        return false;
    }
};


class OrNode : public CondNode {
protected:
    OrNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );


    OrNode() = default;
    virtual ~OrNode() = default;
    OrNode( const OrNode & ) = default;
    OrNode &operator=( const OrNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    friend class MethodIntervalFactory;

public:
    // Accessor operation
    SymbolOop selector() const;


    bool is_and() const {
        return false;
    }


    bool is_or() const {
        return true;
    }
};


class WhileNode : public InlineSendNode {
protected:
    bool           _cond;
    MethodInterval *_expr_code;
    MethodInterval *_body_code;

    WhileNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t cond_offset, std::int32_t end_offset );


    WhileNode() = default;
    virtual ~WhileNode() = default;
    WhileNode( const WhileNode & ) = default;
    WhileNode &operator=( const WhileNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


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


    bool is_whileTrue() const {
        return _cond;
    }


    bool is_whileFalse() const {
        return not _cond;
    }
};


class IfNode : public InlineSendNode {
protected:
    bool           _cond;
    bool           _ignore_else_while_printing;
    bool           _produces_result;
    MethodInterval *_then_code;
    MethodInterval *_else_code;

    IfNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool cond, std::int32_t else_offset, std::uint8_t structure );


    IfNode() = default;
    virtual ~IfNode() = default;
    IfNode( const IfNode & ) = default;
    IfNode &operator=( const IfNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

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


    bool is_ifTrue() const {
        return _cond;
    }


    bool is_ifFalse() const {
        return not _cond;
    }


    bool ignore_else_while_printing() const {
        return _ignore_else_while_printing;
    }


    bool produces_result() const {
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


    ExternalCallNode() = default;
    virtual ~ExternalCallNode() = default;
    ExternalCallNode( const ExternalCallNode & ) = default;
    ExternalCallNode &operator=( const ExternalCallNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

};


class PrimitiveCallNode : public ExternalCallNode {
protected:
    PrimitiveDescriptor *_pdesc;
    bool                _has_receiver;
    SymbolOop           _name;

    // Constructors
    PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc );

    PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc, std::int32_t end_offset );

    friend class MethodIntervalFactory;

public:

    PrimitiveCallNode() = default;
    virtual ~PrimitiveCallNode() = default;
    PrimitiveCallNode( const PrimitiveCallNode & ) = default;
    PrimitiveCallNode &operator=( const PrimitiveCallNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    // Returns the primitive descriptor
    PrimitiveDescriptor *pdesc() const {
        return _pdesc;
    }


    bool has_receiver() const {
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
    SymbolOop      _dll_name;
    SymbolOop      _function_name;
    std::int32_t   _nofArgs;
    dll_func_ptr_t _function;
    bool           _async;

    void initialize( Interpreted_DLLCache *cache );

    // Constructors
    DLLCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, Interpreted_DLLCache *cache );

    friend class MethodIntervalFactory;

public:
    DLLCallNode() = default;
    virtual ~DLLCallNode() = default;
    DLLCallNode( const DLLCallNode & ) = default;
    DLLCallNode &operator=( const DLLCallNode & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

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
        _console->print( "calling DLL[{}], function[{}]", _dll_name, _function_name );
        return _function;
    }


    bool async() const {
        return _async;
    }
};
