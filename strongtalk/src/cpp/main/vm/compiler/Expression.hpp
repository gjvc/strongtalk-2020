//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/compiler/PseudoRegister.hpp"
#include "vm/runtime/ResourceObject.hpp"

// Expression objects represent the results of expressions in the compiler.
// Their main purpose is to annotate a PseudoRegister with type information.
// Another way of looking at Expression objects is that they represent values in the compiler, whereas PseudoRegisters denote locations (names).
// Thus, many Expression objects may point to the same PseudoRegister.

#define BASIC_FLAG_DEF( name, prot ) \
 protected:                                      \
    static const int CONC(name,Bit);                          \
 prot                                              \
 bool_t CONC(is,name)() const { return flags & CONC(name,Bit) ? true : false; }              \
  void CONC(set,name)(bool_t b) {                              \
    if (b) flags |= CONC(name,Bit); else flags &= ~CONC(name,Bit); }          \


#define FLAG_DEF( name )          BASIC_FLAG_DEF(name, public:)
#define PFLAG_DEF( name )      BASIC_FLAG_DEF(name, protected:)

class ConstantExpression;

class MergeExpression;

class KlassExpression;

class UnknownExpression;


class Expression : public PrintableResourceObject {    // abstract base class

    protected:
        PseudoRegister * _pseudoRegister;        // PseudoRegister holding it
        Node           * _node;                  // defining node or nullptr if unknown
        InlinedScope   * _unlikelyScope;         // scope/byteCodeIndex making unknown unlikely
        int _unlikelyByteCodeIndex;  // (only set if isUnknownUnlikely())

    public:
        Expression * next;                    // used for splittable MergeExprs
        int flags;

        Expression( PseudoRegister * p, Node * n );


        virtual bool_t isUnknownExpression() const {
            return false;
        }


        virtual bool_t isNoResultExpression() const {
            return false;
        }


        virtual bool_t isKlassExpression() const {
            return false;
        }


        virtual bool_t isBlockExpression() const {
            return false;
        }


        virtual bool_t isConstantExpression() const {
            return false;
        }


        virtual bool_t isMergeExpression() const {
            return false;
        }


        virtual bool_t isContextExpression() const {
            return false;
        }


        virtual bool_t hasKlass() const {
            return false;
        }


        virtual int nklasses() const = 0;    // number of klasses contained in expr
        virtual bool_t really_hasKlass( InlinedScope * s ) const {
            return hasKlass();
        }


        virtual KlassOop klass() const {
            ShouldNotCallThis();
            return 0;
        }


        virtual bool_t hasConstant() const {
            return false;
        }


        virtual Oop constant() const {
            ShouldNotCallThis();
            return 0;
        }


        virtual bool_t containsUnknown() = 0;


        virtual Expression * makeUnknownUnlikely( InlinedScope * s ) {
            ShouldNotCallThis();
            return 0;
        }


        virtual bool_t isUnknownUnlikely() const {
            return false;
        }


        virtual bool_t needsStoreCheck() const {
            return true;
        }


        virtual UnknownExpression * findUnknown() const {
            return nullptr;
        }


        virtual Expression * findKlass( KlassOop map ) const {
            return nullptr;
        }


        virtual Expression * asReceiver() const;


        virtual KlassExpression * asKlassExpression() const {
            ShouldNotCallThis();
            return nullptr;
        }


        virtual ConstantExpression * asConstantExpression() const {
            ShouldNotCallThis();
            return nullptr;
        }


        virtual MergeExpression * asMergeExpression() const {
            ShouldNotCallThis();
            return nullptr;
        }


        virtual Expression * shallowCopy( PseudoRegister * p, Node * n ) const = 0;  // return a shallow copy
        virtual Expression * copyWithout( Expression * e ) const = 0;  // return receiver w/o expr case
        virtual Expression * mergeWith( Expression * other, Node * n ) = 0;  // return receiver merged with other
        virtual Expression * convertToKlass( PseudoRegister * p, Node * n ) const = 0;  // convert constants to klasses
        virtual bool_t equals( Expression * other ) const = 0;


        Node * node() const {
            return _node;
        }


        PseudoRegister * preg() const {
            return _pseudoRegister;
        }


        void setNode( Node * n, PseudoRegister * p ) {
            _node           = n;
            _pseudoRegister = p;
        }


        bool_t is_smi() const {
            return hasKlass() and klass() == smiKlassObj;
        }


        InlinedScope * scope() const;

        virtual NameNode * nameNode( bool_t mustBeLegal = true ) const;

        virtual void verify() const;

    protected:
        void print_helper( const char * type );
};

// an expression whose type is unknown
class UnknownExpression : public Expression {
    public:
        UnknownExpression( PseudoRegister * p, Node * n = nullptr, bool_t u = false ) :
            Expression( p, n ) {
            setUnlikely( u );
        }


        bool_t isUnknownExpression() const {
            return true;
        }


        bool_t containsUnknown() {
            return true;
        }


    FLAG_DEF( Unlikely );            // true e.g. if this is the "unknown" branch of a
        // type-predicted receiver, or result of prim. failure
        UnknownExpression * findUnknown() const {
            return ( UnknownExpression * ) this;
        }


        bool_t isUnknownUnlikely() const {
            return isUnlikely();
        }


        int nklasses() const {
            return 0;
        }


        Expression * shallowCopy( PseudoRegister * p, Node * n ) const;


        Expression * copyWithout( Expression * e ) const {
            return ( Expression * ) this;
        }


        Expression * mergeWith( Expression * other, Node * n );


        Expression * convertToKlass( PseudoRegister * p, Node * n ) const {
            return shallowCopy( p, n );
        };

        Expression * makeUnknownUnlikely( InlinedScope * s );

        bool_t equals( Expression * other ) const;

        void print();
};

// an expression that has no value, i.e., will never exist at runtime
// example: the return value of a block method that ends with a non-local return
// used mainly for compiler debugging and to avoid generating unreachable code
class NoResultExpression : public Expression {
    public:
        NoResultExpression( Node * n = nullptr );


        bool_t isNoResultExpression() const {
            return true;
        }


        int nklasses() const {
            return 0;
        }


        bool_t containsUnknown() {
            return false;
        }


        Expression * shallowCopy( PseudoRegister * p, Node * n ) const;


        Expression * copyWithout( Expression * e ) const {
            return ( Expression * ) this;
        }


        Expression * mergeWith( Expression * other, Node * n );


        Expression * convertToKlass( PseudoRegister * p, Node * n ) const {
            return shallowCopy( p, n );
        };

        bool_t equals( Expression * other ) const;

        void print();
};


// an expression whose klass is known
class KlassExpression : public Expression {
    protected:
        KlassOop _klass;
    public:
        KlassExpression( KlassOop m, PseudoRegister * p, Node * n );


        bool_t isKlassExpression() const {
            return true;
        }


        bool_t containsUnknown() {
            return false;
        }


        KlassExpression * asKlassExpression() const {
            return ( KlassExpression * ) this;
        }


        bool_t hasKlass() const {
            return true;
        }


        int nklasses() const {
            return 1;
        }


        KlassOop klass() const {
            return _klass;
        }


        virtual bool_t needsStoreCheck() const;

        Expression * shallowCopy( PseudoRegister * p, Node * n ) const;

        Expression * copyWithout( Expression * e ) const;

        Expression * mergeWith( Expression * other, Node * n );


        Expression * convertToKlass( PseudoRegister * p, Node * n ) const {
            return shallowCopy( p, n );
        };


        Expression * findKlass( KlassOop map ) const {
            return _klass == map ? ( Expression * ) this : nullptr;
        }


        bool_t equals( Expression * other ) const;

        void print();

        virtual void verify() const;
};

// a cloned block literal (result of BlockClone node)
class BlockExpression : public KlassExpression {
    protected:
        InlinedScope * _blockScope;        // block's parent scope
    public:
        BlockExpression( BlockPseudoRegister * p, Node * n );


        bool_t isBlockExpression() const {
            return true;
        }


        InlinedScope * blockScope() const {
            return _blockScope;
        }


        BlockPseudoRegister * preg() const {
            return ( BlockPseudoRegister * ) _pseudoRegister;
        }


        int nklasses() const {
            return 1;
        }


        Expression * shallowCopy( PseudoRegister * p, Node * n ) const;

        Expression * mergeWith( Expression * other, Node * n );

        bool_t equals( Expression * other ) const;

        void print();

        virtual void verify() const;
};

// an expression whose exact runtime value is known
class ConstantExpression : public Expression {

    private:
        Oop _c;

    public:
        ConstantExpression( Oop c, PseudoRegister * p, Node * n ) :
            Expression( p, n ) {
            _c = c;
        }


        bool_t isConstantExpression() const {
            return true;
        }


        bool_t containsUnknown() {
            return false;
        }


        bool_t hasKlass() const {
            return true;
        }


        KlassExpression * asKlassExpression() const;


        KlassOop klass() const {
            return _c->klass();
        }


        int nklasses() const {
            return 1;
        }


        ConstantExpression * asConstantExpression() const {
            return ( ConstantExpression * ) this;
        }


        bool_t hasConstant() const {
            return true;
        }


        Oop constant() const {
            return _c;
        }


        virtual bool_t needsStoreCheck() const;

        NameNode * nameNode( bool_t mustBeLegal = true ) const;

        Expression * shallowCopy( PseudoRegister * p, Node * n ) const;

        Expression * copyWithout( Expression * e ) const;

        Expression * mergeWith( Expression * other, Node * n );

        Expression * convertToKlass( PseudoRegister * p, Node * n ) const;

        Expression * findKlass( KlassOop map ) const;

        bool_t equals( Expression * other ) const;

        void print();

        virtual void verify() const;
};


// an expression that is the result of merging two or more other expressions
// when control flow merges
// example: the result of foo ifTrue: [ 1 ] ifFalse: [ i + j ] could be
// MergeExpression(ConstantExpression(1), KlassExpression(IntegerKlass))
// merge expressions have the following properties:
// - they never have more than MaxPICSize+1 elements
// - they're flat (i.e don't contain other MergeExprs)

class MergeExpression : public Expression {
    public:
        GrowableArray <Expression *> * exprs;

        MergeExpression( Expression * e1, Expression * e2, PseudoRegister * p, Node * n );

        MergeExpression( PseudoRegister * p, Node * n );
        // A MergeExpression's PseudoRegister says where the merged result is (i.e., it may be different from
        // the PseudoRegisters of the individual expressions); typically, there's an assigmnent from the
        // subexpression's PseudoRegister to the MergeExpression's PseudoRegister just before the control flow merge.
        // The node n (if non-nullptr) is considered the defining node of the MergeExpression; it is usually
        // one of the first nodes after the control flow merge.  It is always legal to pass
        // nullptr for n, but doing so prevents splitting.

        bool_t isMergeExpression() const {
            return true;
        }


    FLAG_DEF( Splittable );
    PFLAG_DEF( UnknownSet );
    PFLAG_DEF( ContainingUnknown );
    public:
        bool_t containsUnknown();


        MergeExpression * asMergeExpression() const {
            return ( MergeExpression * ) this;
        }


        bool_t hasKlass() const;

        int nklasses() const;

        KlassExpression * asKlassExpression() const;

        bool_t really_hasKlass( InlinedScope * s ) const;

        KlassOop klass() const;

        Expression * asReceiver() const;

        bool_t hasConstant() const;

        Oop constant() const;

        Expression * shallowCopy( PseudoRegister * p, Node * n ) const;

        Expression * copyWithout( Expression * e ) const;

        Expression * mergeWith( Expression * other, Node * n );

        Expression * convertToKlass( PseudoRegister * p, Node * n ) const;

        Expression * makeUnknownUnlikely( InlinedScope * s );

        bool_t equals( Expression * other ) const;

        void print();

        virtual void verify() const;

        UnknownExpression * findUnknown() const;    // returns nullptr if no expression found
        Expression * findKlass( KlassOop map ) const;

    protected:
        void initialize();

        void mergeInto( Expression * other, Node * n );

        void add( Expression * s );
};


// an expression for a context pointer
// used only for compiler debugging; it should never appear where a normal expression is expected
class ContextExpression : public Expression {
    public:
        ContextExpression( PseudoRegister * r );


        bool_t isContextExpression() const {
            return true;
        }


        bool_t containsUnknown() {
            ShouldNotCallThis();
            return false;
        }


        int nklasses() const {
            ShouldNotCallThis();
            return 1;
        }


        Expression * shallowCopy( PseudoRegister * p, Node * n ) const {
            ShouldNotCallThis();
            return ( Expression * ) this;
        }


        Expression * copyWithout( Expression * e ) const {
            ShouldNotCallThis();
            return ( Expression * ) this;
        }


        Expression * mergeWith( Expression * other, Node * n ) {
            ShouldNotCallThis();
            return ( Expression * ) this;
        }


        Expression * convertToKlass( PseudoRegister * p, Node * n ) const {
            return shallowCopy( p, n );
        };


        bool_t equals( Expression * other ) const {
            ShouldNotCallThis();
            return false;
        }


        void print();

        virtual void verify() const;
};


class ExpressionStack : public GrowableArray <Expression *> {
        // an ExpressionStack simulates the run-time expression stack during compilation
        // it keeps track of the live ranges by recording the byteCodeIndex on push/pop
    private:
        InlinedScope * _scope;                // scope that generates the pushes and pops

    public:
        ExpressionStack( InlinedScope * scope, int size );

        void push( Expression * expr, InlinedScope * currentScope, int byteCodeIndex );

        void push2nd( Expression * expr, InlinedScope * currentScope, int byteCodeIndex ); // allows a 2nd expr to be pushed for the same byteCodeIndex
        void assign_top( Expression * expr );

        Expression * pop();

        void pop( int nofExprsToPop );

        void print();
};

#undef BASIC_FLAG_DEF
#undef FLAG_DEF
#undef PFLAG_DEF
