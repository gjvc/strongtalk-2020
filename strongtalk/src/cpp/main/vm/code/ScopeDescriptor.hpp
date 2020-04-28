//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/NameDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/runtime/ResourceObject.hpp"


constexpr int16_t IllegalByteCodeIndex  = -1;       //
constexpr int16_t PrologueByteCodeIndex = 0;        //
constexpr int16_t EpilogueByteCodeIndex = 32766;    //

class NameDescriptorClosure {

    public:
        virtual void arg( int no, NameDescriptor * a, const char * pc ) {
        }


        virtual void temp( int no, NameDescriptor * t, const char * pc ) {
        }


        virtual void context_temp( int no, NameDescriptor * c, const char * pc ) {
        }


        virtual void stack_expr( int no, NameDescriptor * e, const char * pc ) {
        }
};


// use the following functions to compare byteCodeIndexs; they handle PrologueByteCodeIndex et al.
// negative if byteCodeIndex1 is before byteCodeIndex2, 0 if same, positive if after
int compareByteCodeIndex( int byteCodeIndex1, int byteCodeIndex2 );


inline bool_t byteCodeIndexLT( int byteCodeIndex1, int byteCodeIndex2 ) {
    return compareByteCodeIndex( byteCodeIndex1, byteCodeIndex2 ) < 0;
}


inline bool_t byteCodeIndexLE( int byteCodeIndex1, int byteCodeIndex2 ) {
    return compareByteCodeIndex( byteCodeIndex1, byteCodeIndex2 ) <= 0;
}


inline bool_t byteCodeIndexGT( int byteCodeIndex1, int byteCodeIndex2 ) {
    return compareByteCodeIndex( byteCodeIndex1, byteCodeIndex2 ) > 0;
}


inline bool_t byteCodeIndexGE( int byteCodeIndex1, int byteCodeIndex2 ) {
    return compareByteCodeIndex( byteCodeIndex1, byteCodeIndex2 ) >= 0;
}


// Blocks belonging to scopes that aren't described (because they can't possibly be visible to the user) get IllegalDescOffset as their desc offset
constexpr int IllegalDescOffset = -2;

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

class ScopeDescriptor : public PrintableResourceObject {        // abstract

    protected:
        // Creation information
        const NativeMethodScopes * _scopes;
        int _offset;
        const char * _pc;

    protected:
        // Cached information
        bool_t    _hasTemporaries;
        bool_t    _hasContextTemporaries;
        bool_t    _hasExpressionStack;
        MethodOop _method;
        int       _scopeID;
        bool_t    _lite;
        int       _senderScopeOffset;
        int       _senderByteCodeIndex;
        bool_t    _allocatesCompiledContext;
        int       _name_desc_offset;
        int       _next;

        // If the pc of a ScopeDescriptor is equal to invalid_pc, the scopeDesc is pc independent.
        // A pc independent ScopeDescriptor prints out all the locations for its nameDescs.
        // NB: Only relevant for new backend.
        static char * invalid_pc;

    public:
        void iterate( NameDescriptorClosure * blk );    // info for given pc()
        void iterate_all( NameDescriptorClosure * blk );    // info for all pc's

        NameDescriptor * nameDescAt( int & offset ) const;

        int valueAt( int & offset ) const;

        ScopeDescriptor( const NativeMethodScopes * scopes, int offset, const char * pc );


        int offset() const {
            return int( _offset );
        }


        const char * pc() const {
            return _pc;
        }


        bool_t is_equal( ScopeDescriptor * s ) const {
            return _scopes == s->_scopes and _offset == s->_offset;
        }


        // A lite scopeDesc has no information saved on temporaries or expression stack.
        bool_t is_lite() const {
            return _lite;
        }


        bool_t allocates_interpreted_context() const;


        MethodOop method() const {
            return _method;
        }


        // Tells whether a compiled context is allocated for this scope.
        bool_t allocates_compiled_context() const {
            return _allocatesCompiledContext;
        }


        // Returns the name desc for the allocated compiled context.
        NameDescriptor * compiled_context();


        int scopeID() const {
            return _scopeID;
        }


        const NativeMethodScopes * scopes() const {
            return _scopes;
        }


        SymbolOop selector() const {
            return method()->selector();
        }


        ScopeDescriptor * sender() const;

        virtual KlassOop selfKlass() const = 0;

        // Returns the parent scope
        // If cross_NativeMethod_boundary is false parent will return nullptr if
        // the parent is located in another compilation unit (NativeMethod).
        virtual ScopeDescriptor * parent( bool_t cross_NativeMethod_boundary = false ) const = 0;

        ScopeDescriptor * home( bool_t cross_NativeMethod_boundary = false ) const;


        // Returns the byteCodeIndex of the calling method if this scopeDesc is inlined.
        // For a root scopeDesc IllegalByteCodeIndex is returned.
        int senderByteCodeIndex() const {
            st_assert( _senderByteCodeIndex not_eq IllegalByteCodeIndex, "need to ask calling byte code" );
            return _senderByteCodeIndex;
        }


        // Root scope?
        bool_t isTop() const {
            return _senderScopeOffset == 0;
        }


        // Lookup key
        virtual LookupKey * key() const = 0;

        // scope equivalence -- for compiler
        virtual bool_t s_equivalent( ScopeDescriptor * s ) const;

        virtual bool_t l_equivalent( LookupKey * s ) const;


        // types test operations
        virtual bool_t isMethodScope() const {
            return false;
        }


        virtual bool_t isBlockScope() const {
            return false;
        }


        virtual bool_t isTopLevelBlockScope() const {
            return false;
        }


        virtual bool_t isNonInlinedBlockScope() const {
            return false;
        }


        virtual NameDescriptor * self() const = 0;

        NameDescriptor * temporary( int index, bool_t canFail = false );

        NameDescriptor * contextTemporary( int index, bool_t canFail = false );

        NameDescriptor * exprStackElem( int byteCodeIndex );

    public:
        int next_offset() const {
            return _next;
        }


        int sender_scope_offset() const {
            return _offset - _senderScopeOffset;
        }


        bool_t verify();

        void verify_expression_stack( int byteCodeIndex );

        // printing support
        void print( int indent, bool_t all_pcs );        // print info for current/all pc's
        void print() {
            print( 0, false );
        }


        virtual void print_value_on( ConsoleOutputStream * stream ) const;

    protected:
        virtual bool_t shallow_verify() {
            return true;
        }


        virtual void printName() = 0;


        virtual void printSelf() {
        }


    public:
        Expression * selfExpression( PseudoRegister * p ) const;        // Type information for the optimizing compiler

        friend class NativeMethodScopes;

        friend class NonInlinedBlockScopeDescriptor;
};


class MethodScopeDescriptor : public ScopeDescriptor {
    protected:
        // Cached information
        LookupKey _key;
        NameDescriptor * _self_name;

    public:
        MethodScopeDescriptor( NativeMethodScopes * scopes, int offset, const char * pc );

        bool_t s_equivalent( ScopeDescriptor * s ) const;

        bool_t l_equivalent( LookupKey * s ) const;


        bool_t isMethodScope() const {
            return true;
        }


        NameDescriptor * self() const {
            return _self_name;
        }


        LookupKey * key() const {
            return ( LookupKey * ) &_key;
        }


        KlassOop selfKlass() const {
            return _key.klass();
        }


        ScopeDescriptor * parent( bool_t cross_NativeMethod_boundary = false ) const {
            return nullptr;
        }


        // printing support
        void printName();

        void printSelf();

        void print_value_on( ConsoleOutputStream * stream ) const;
};

// an inlined block whose parent scope is in the same NativeMethod
class BlockScopeDescriptor : public ScopeDescriptor {
    protected:
        // Cached information
        int _parentScopeOffset;

    public:
        BlockScopeDescriptor( const NativeMethodScopes * scopes, int offset, const char * pc );

        // NB: the next three operations may return nullptr (no context)
        KlassOop selfKlass() const;

        NameDescriptor * self() const;

        ScopeDescriptor * parent( bool_t cross_NativeMethod_boundary = false ) const;

        LookupKey * key() const;

        bool_t s_equivalent( ScopeDescriptor * s ) const;


        bool_t isBlockScope() const {
            return true;
        }


        // print operations
        void printSelf();

        void printName();

        void print_value_on( ConsoleOutputStream * stream ) const;
};

// A block method whose enclosing scope isn't in the same NativeMethod.
// Must be a root scope.
class TopLevelBlockScopeDescriptor : public ScopeDescriptor {
    protected:
        // Cached information
        NameDescriptor * _self_name;
        KlassOop _self_klass;

    public:
        TopLevelBlockScopeDescriptor( const NativeMethodScopes * scopes, int offset, const char * pc );


        // type test operations
        bool_t isBlockScope() const {
            return true;
        }


        bool_t isTopLevelBlockScope() const {
            return true;
        }


        KlassOop selfKlass() const {
            return _self_klass;
        }


        NameDescriptor * self() const {
            return _self_name;
        }


        // NB: parent() may return nullptr for clean blocks
        ScopeDescriptor * parent( bool_t cross_NativeMethod_boundary = false ) const;

        LookupKey * key() const;

        bool_t s_equivalent( ScopeDescriptor * s ) const;

        // print operations
        void printSelf();

        void printName();

        void print_value_on( ConsoleOutputStream * stream ) const;
};

class NonInlinedBlockScopeDescriptor : public PrintableResourceObject {
    protected:
        // Creation information
        const NativeMethodScopes * _scopes;
        int _offset;

    protected:
        // Cached information
        MethodOop _method;
        int       _parentScopeOffset;

    public:
        NonInlinedBlockScopeDescriptor( const NativeMethodScopes * scopes, int offset );


        MethodOop method() const {
            return _method;
        }


        ScopeDescriptor * parent() const;

        void print();
};

