
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/NameDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


constexpr std::int16_t IllegalByteCodeIndex  = -1;       //
constexpr std::int16_t PrologueByteCodeIndex = 0;        //
constexpr std::int16_t EpilogueByteCodeIndex = 32766;    //


class NameDescriptorClosure {

public:
    virtual void arg( std::int32_t no, NameDescriptor *a, const char *pc ) {
        static_cast<void>(no); // unused
        static_cast<void>(a); // unused
        static_cast<void>(pc); // unused
    }


    virtual void temp( std::int32_t no, NameDescriptor *t, const char *pc ) {
        static_cast<void>(no); // unused
        static_cast<void>(t); // unused
        static_cast<void>(pc); // unused
    }


    virtual void context_temp( std::int32_t no, NameDescriptor *c, const char *pc ) {
        static_cast<void>(no); // unused
        static_cast<void>(c); // unused
        static_cast<void>(pc); // unused
    }


    virtual void stack_expr( std::int32_t no, NameDescriptor *e, const char *pc ) {
        static_cast<void>(no); // unused
        static_cast<void>(e); // unused
        static_cast<void>(pc); // unused
    }


    virtual ~NameDescriptorClosure() = default;
};


// use the following functions to compare byteCodeIndexs; they handle PrologueByteCodeIndex et al.
// negative if byteCodeIndex1 is before byteCodeIndex2, 0 if same, positive if after
std::int32_t compareByteCodeIndex( std::int32_t byteCodeIndex1, std::int32_t byteCodeIndex2 );


inline bool byteCodeIndexLT( std::int32_t byteCodeIndex1, std::int32_t byteCodeIndex2 ) {
    return compareByteCodeIndex( byteCodeIndex1, byteCodeIndex2 ) < 0;
}


inline bool byteCodeIndexLE( std::int32_t byteCodeIndex1, std::int32_t byteCodeIndex2 ) {
    return compareByteCodeIndex( byteCodeIndex1, byteCodeIndex2 ) <= 0;
}


inline bool byteCodeIndexGT( std::int32_t byteCodeIndex1, std::int32_t byteCodeIndex2 ) {
    return compareByteCodeIndex( byteCodeIndex1, byteCodeIndex2 ) > 0;
}


inline bool byteCodeIndexGE( std::int32_t byteCodeIndex1, std::int32_t byteCodeIndex2 ) {
    return compareByteCodeIndex( byteCodeIndex1, byteCodeIndex2 ) >= 0;
}


// Blocks belonging to scopes that aren't described (because they can't possibly be visible to the user) get IllegalDescOffset as their desc offset
constexpr std::int32_t IllegalDescOffset = -2;

//
//
// ScopeDescriptor classes contain the information that makes source-level debugging of nativeMethods possible; each ScopeDescriptor describes a method activation
//
// Class Hierarchy:
//  - ScopeDescriptor          (abstract)
//    - MethodScopeDescriptor
//    - BlockScopeDescriptor
//    - TopLevelScopeDesc
//  - NonInlinedBlockScopeDescriptor
//
//

class NativeMethodScopes;

class PseudoRegister;

class Expression;

class ScopeDescriptor : public PrintableResourceObject {
    // abstract

protected:
    // Creation information
    const NativeMethodScopes *_scopes;
    std::int32_t             _offset;
    const char               *_pc;

    ScopeDescriptor() = default;
    virtual ~ScopeDescriptor() = default;
    ScopeDescriptor( const ScopeDescriptor & ) = default;
    ScopeDescriptor &operator=( const ScopeDescriptor & ) = default;


    void operator delete( void *ptr ) { (void) ptr; }


protected:
    // Cached information
    bool         _hasTemporaries;
    bool         _hasContextTemporaries;
    bool         _hasExpressionStack;
    MethodOop    _method;
    std::int32_t _scopeID;
    bool         _lite;
    std::int32_t _senderScopeOffset;
    std::int16_t _senderByteCodeIndex;
    bool         _allocatesCompiledContext;
    std::int32_t _name_desc_offset;
    std::int32_t _next;

    // If the pc of a ScopeDescriptor is equal to invalid_pc, the scopeDesc is pc independent.
    // A pc independent ScopeDescriptor prints out all the locations for its nameDescs.
    // NB: Only relevant for new backend.
    static char *invalid_pc;

public:
    void iterate( NameDescriptorClosure *blk );    // info for given pc()
    void iterate_all( NameDescriptorClosure *blk );    // info for all pc's

    NameDescriptor *nameDescAt( std::int32_t &offset ) const;

    std::int32_t valueAt( std::int32_t &offset ) const;

    ScopeDescriptor( const NativeMethodScopes *scopes, std::int32_t offset, const char *pc );


    std::int32_t offset() const {
        return std::int32_t( _offset );
    }


    const char *pc() const {
        return _pc;
    }


    bool is_equal( ScopeDescriptor *s ) const {
        return _scopes == s->_scopes and _offset == s->_offset;
    }


    // A lite scopeDesc has no information saved on temporaries or expression stack.
    bool is_lite() const {
        return _lite;
    }


    bool allocates_interpreted_context() const;


    MethodOop method() const {
        return _method;
    }


    // Tells whether a compiled context is allocated for this scope.
    bool allocates_compiled_context() const {
        return _allocatesCompiledContext;
    }


    // Returns the name desc for the allocated compiled context.
    NameDescriptor *compiled_context();


    std::int32_t scopeID() const {
        return _scopeID;
    }


    const NativeMethodScopes *scopes() const {
        return _scopes;
    }


    SymbolOop selector() const {
        return method()->selector();
    }


    ScopeDescriptor *sender() const;

    virtual KlassOop selfKlass() const = 0;

    // Returns the parent scope
    // If cross_NativeMethod_boundary is false parent will return nullptr if
    // the parent is located in another compilation unit (NativeMethod).
    virtual ScopeDescriptor *parent( bool cross_NativeMethod_boundary = false ) const = 0;

    ScopeDescriptor *home( bool cross_NativeMethod_boundary = false ) const;


    // Returns the byteCodeIndex of the calling method if this scopeDesc is inlined.
    // For a root scopeDesc IllegalByteCodeIndex is returned.
    std::int32_t senderByteCodeIndex() const {
        st_assert( _senderByteCodeIndex not_eq IllegalByteCodeIndex, "need to ask calling byte code" );
        return _senderByteCodeIndex;
    }


    // Root scope?
    bool isTop() const {
        return _senderScopeOffset == 0;
    }


    // Lookup key
    virtual LookupKey *key() const = 0;

    // scope equivalence -- for compiler
    virtual bool s_equivalent( ScopeDescriptor *s ) const;

    virtual bool l_equivalent( LookupKey *s ) const;


    // types test operations
    virtual bool isMethodScope() const {
        return false;
    }


    virtual bool isBlockScope() const {
        return false;
    }


    virtual bool isTopLevelBlockScope() const {
        return false;
    }


    virtual bool isNonInlinedBlockScope() const {
        return false;
    }


    virtual NameDescriptor *self() const = 0;

    NameDescriptor *temporary( std::int32_t index, bool canFail = false );

    NameDescriptor *contextTemporary( std::int32_t index, bool canFail = false );

    NameDescriptor *exprStackElem( std::int32_t byteCodeIndex );

public:
    std::int32_t next_offset() const {
        return _next;
    }


    std::int32_t sender_scope_offset() const {
        return _offset - _senderScopeOffset;
    }


    bool verify();

    void verify_expression_stack( std::int32_t byteCodeIndex );

    // printing support
    void print( std::int32_t indent, bool all_pcs );        // print info for current/all pc's
    void print() {
        print( 0, false );
    }


    virtual void print_value_on( ConsoleOutputStream *stream ) const;

protected:
    virtual bool shallow_verify() {
        return true;
    }


    virtual void printName() = 0;


    virtual void printSelf() {
    }


public:
    Expression *selfExpression( PseudoRegister *p ) const;        // Type information for the optimizing compiler

    friend class NativeMethodScopes;

    friend class NonInlinedBlockScopeDescriptor;
};


class MethodScopeDescriptor : public ScopeDescriptor {
protected:
    // Cached information
    LookupKey      _key;
    NameDescriptor *_self_name;

public:
    MethodScopeDescriptor( NativeMethodScopes *scopes, std::int32_t offset, const char *pc );
    MethodScopeDescriptor() = default;
    virtual ~MethodScopeDescriptor() = default;
    MethodScopeDescriptor( const MethodScopeDescriptor & ) = default;
    MethodScopeDescriptor &operator=( const MethodScopeDescriptor & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool s_equivalent( ScopeDescriptor *s ) const;

    bool l_equivalent( LookupKey *s ) const;


    bool isMethodScope() const {
        return true;
    }


    NameDescriptor *self() const {
        return _self_name;
    }


    LookupKey *key() const {
        return (LookupKey *) &_key;
    }


    KlassOop selfKlass() const {
        return _key.klass();
    }


    ScopeDescriptor *parent( bool cross_NativeMethod_boundary = false ) const {
        static_cast<void>(cross_NativeMethod_boundary); // unused
        return nullptr;
    }


    // printing support
    void printName();

    void printSelf();

    void print_value_on( ConsoleOutputStream *stream ) const;
};

// an inlined block whose parent scope is in the same NativeMethod
class BlockScopeDescriptor : public ScopeDescriptor {
protected:
    // Cached information
    std::int32_t _parentScopeOffset;

public:
    BlockScopeDescriptor( const NativeMethodScopes *scopes, std::int32_t offset, const char *pc );

    // NB: the next three operations may return nullptr (no context)
    KlassOop selfKlass() const;

    NameDescriptor *self() const;

    ScopeDescriptor *parent( bool cross_NativeMethod_boundary = false ) const;

    LookupKey *key() const;

    bool s_equivalent( ScopeDescriptor *s ) const;


    bool isBlockScope() const {
        return true;
    }


    // print operations
    void printSelf();

    void printName();

    void print_value_on( ConsoleOutputStream *stream ) const;
};

// A block method whose enclosing scope isn't in the same NativeMethod.
// Must be a root scope.
class TopLevelBlockScopeDescriptor : public ScopeDescriptor {
protected:
    // Cached information
    NameDescriptor *_self_name;
    KlassOop       _self_klass;

public:
    TopLevelBlockScopeDescriptor( const NativeMethodScopes *scopes, std::int32_t offset, const char *pc );
    TopLevelBlockScopeDescriptor() = default;
    virtual ~TopLevelBlockScopeDescriptor() = default;
    TopLevelBlockScopeDescriptor( const TopLevelBlockScopeDescriptor & ) = default;
    TopLevelBlockScopeDescriptor &operator=( const TopLevelBlockScopeDescriptor & ) = default;


    void operator delete( void *ptr ) { (void) ptr; }


    // type test operations
    bool isBlockScope() const {
        return true;
    }


    bool isTopLevelBlockScope() const {
        return true;
    }


    KlassOop selfKlass() const {
        return _self_klass;
    }


    NameDescriptor *self() const {
        return _self_name;
    }


    // NB: parent() may return nullptr for clean blocks
    ScopeDescriptor *parent( bool cross_NativeMethod_boundary = false ) const;

    LookupKey *key() const;

    bool s_equivalent( ScopeDescriptor *s ) const;

    // print operations
    void printSelf();

    void printName();

    void print_value_on( ConsoleOutputStream *stream ) const;
};


class NonInlinedBlockScopeDescriptor : public PrintableResourceObject {
protected:
    // Creation information
    const NativeMethodScopes *_scopes;
    std::int32_t             _offset;

protected:
    // Cached information
    MethodOop    _method;
    std::int32_t _parentScopeOffset;

public:
    NonInlinedBlockScopeDescriptor( const NativeMethodScopes *scopes, std::int32_t offset );
    NonInlinedBlockScopeDescriptor() = default;
    virtual ~NonInlinedBlockScopeDescriptor() = default;
    NonInlinedBlockScopeDescriptor( const NonInlinedBlockScopeDescriptor & ) = default;
    NonInlinedBlockScopeDescriptor &operator=( const NonInlinedBlockScopeDescriptor & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    MethodOop method() const {
        return _method;
    }


    ScopeDescriptor *parent() const;

    void print();
};
