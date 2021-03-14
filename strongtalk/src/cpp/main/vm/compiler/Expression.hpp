
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
 protected: \
    static const std::int32_t CONC(name,Bit); \
 prot \
 bool CONC(is,name)() const { return flags & CONC(name,Bit) ? true : false; } \
  void CONC(set,name)(bool b) { \
    if (b) flags |= CONC(name,Bit); else flags &= ~CONC(name,Bit); } \


#define FLAG_DEF( name )          BASIC_FLAG_DEF(name, public:)

#define PFLAG_DEF( name )      BASIC_FLAG_DEF(name, protected:)


class ConstantExpression;

class MergeExpression;

class KlassExpression;

class UnknownExpression;


class Expression : public PrintableResourceObject {    // abstract base class

protected:
    PseudoRegister *_pseudoRegister;        // PseudoRegister holding it
    Node           *_node;                  // defining node or nullptr if unknown
    InlinedScope   *_unlikelyScope;         // scope/byteCodeIndex making unknown unlikely
    std::int32_t   _unlikelyByteCodeIndex;  // (only set if isUnknownUnlikely())

public:
    Expression   *next;                    // used for splittable MergeExprs
    std::int32_t flags;

    Expression() = default;
    Expression( PseudoRegister *p, Node *n );
    virtual ~Expression() = default;
    Expression( const Expression & ) = default;
    Expression &operator=( const Expression & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    virtual bool isUnknownExpression() const {
        return false;
    }


    virtual bool isNoResultExpression() const {
        return false;
    }


    virtual bool isKlassExpression() const {
        return false;
    }


    virtual bool isBlockExpression() const {
        return false;
    }


    virtual bool isConstantExpression() const {
        return false;
    }


    virtual bool isMergeExpression() const {
        return false;
    }


    virtual bool isContextExpression() const {
        return false;
    }


    virtual bool hasKlass() const {
        return false;
    }


    virtual std::int32_t nklasses() const = 0;    // number of klasses contained in expr

    virtual bool really_hasKlass( InlinedScope *s ) const {
        st_unused( s ); // unused
        return hasKlass();
    }


    virtual KlassOop klass() const {
        ShouldNotCallThis();
        return 0;
    }


    virtual bool hasConstant() const {
        return false;
    }


    virtual Oop constant() const {
        ShouldNotCallThis();
        return 0;
    }


    virtual bool containsUnknown() = 0;


    virtual Expression *makeUnknownUnlikely( InlinedScope *s ) {
        st_unused( s ); // unused
        ShouldNotCallThis();
        return 0;
    }


    virtual bool isUnknownUnlikely() const {
        return false;
    }


    virtual bool needsStoreCheck() const {
        return true;
    }


    virtual UnknownExpression *findUnknown() const {
        return nullptr;
    }


    virtual Expression *findKlass( KlassOop map ) const {
        st_unused( map ); // unused
        return nullptr;
    }


    virtual Expression *asReceiver() const;


    virtual KlassExpression *asKlassExpression() const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual ConstantExpression *asConstantExpression() const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual MergeExpression *asMergeExpression() const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual Expression *shallowCopy( PseudoRegister *p, Node *n ) const = 0;  // return a shallow copy
    virtual Expression *copyWithout( Expression *e ) const = 0;  // return receiver w/o expr case
    virtual Expression *mergeWith( Expression *other, Node *n ) = 0;  // return receiver merged with other
    virtual Expression *convertToKlass( PseudoRegister *p, Node *n ) const = 0;  // convert constants to klasses
    virtual bool equals( Expression *other ) const = 0;


    Node *node() const {
        return _node;
    }


    PseudoRegister *pseudoRegister() const {
        return _pseudoRegister;
    }


    void setNode( Node *n, PseudoRegister *p ) {
        _node           = n;
        _pseudoRegister = p;
    }


    bool isSmallIntegerOop() const {
        return hasKlass() and klass() == smiKlassObject;
    }


    InlinedScope *scope() const;

    virtual NameNode *nameNode( bool mustBeLegal = true ) const;

    virtual void verify() const;

protected:
    void print_helper( const char *type );
};


// an expression whose type is unknown
class UnknownExpression : public Expression {
public:
    UnknownExpression( PseudoRegister *p, Node *n = nullptr, bool u = false ) :
        Expression( p, n ) {
        setUnlikely( u );
    }


    bool isUnknownExpression() const {
        return true;
    }


    bool containsUnknown() {
        return true;
    }


FLAG_DEF( Unlikely );            // true e.g. if this is the "unknown" branch of a
    // type-predicted receiver, or result of prim. failure
    UnknownExpression *findUnknown() const {
        return (UnknownExpression *) this;
    }


    bool isUnknownUnlikely() const {
        return isUnlikely();
    }


    std::int32_t nklasses() const {
        return 0;
    }


    Expression *shallowCopy( PseudoRegister *p, Node *n ) const;


    Expression *copyWithout( Expression *e ) const {
        st_unused( e ); // unused
        return (Expression *) this;
    }


    Expression *mergeWith( Expression *other, Node *n );


    Expression *convertToKlass( PseudoRegister *p, Node *n ) const {
        return shallowCopy( p, n );
    };

    Expression *makeUnknownUnlikely( InlinedScope *s );

    bool equals( Expression *other ) const;

    void print();
};

// an expression that has no value, i.e., will never exist at runtime
// example: the return value of a block method that ends with a non-local return
// used mainly for compiler debugging and to avoid generating unreachable code
class NoResultExpression : public Expression {
public:
    NoResultExpression( Node *n = nullptr );


    bool isNoResultExpression() const {
        return true;
    }


    std::int32_t nklasses() const {
        return 0;
    }


    bool containsUnknown() {
        return false;
    }


    Expression *shallowCopy( PseudoRegister *p, Node *n ) const;


    Expression *copyWithout( Expression *e ) const {
        st_unused( e ); // unused
        return (Expression *) this;
    }


    Expression *mergeWith( Expression *other, Node *n );


    Expression *convertToKlass( PseudoRegister *p, Node *n ) const {
        return shallowCopy( p, n );
    };

    bool equals( Expression *other ) const;

    void print();
};


// an expression whose klass is known
class KlassExpression : public Expression {
protected:
    KlassOop _klass;
public:
    KlassExpression( KlassOop m, PseudoRegister *p, Node *n );

    KlassExpression() = default;
    virtual ~KlassExpression() = default;
    KlassExpression( const KlassExpression & ) = default;
    KlassExpression &operator=( const KlassExpression & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    bool isKlassExpression() const {
        return true;
    }


    bool containsUnknown() {
        return false;
    }


    KlassExpression *asKlassExpression() const {
        return (KlassExpression *) this;
    }


    bool hasKlass() const {
        return true;
    }


    std::int32_t nklasses() const {
        return 1;
    }


    KlassOop klass() const {
        return _klass;
    }


    virtual bool needsStoreCheck() const;

    Expression *shallowCopy( PseudoRegister *p, Node *n ) const;

    Expression *copyWithout( Expression *e ) const;

    Expression *mergeWith( Expression *other, Node *n );


    Expression *convertToKlass( PseudoRegister *p, Node *n ) const {
        return shallowCopy( p, n );
    };


    Expression *findKlass( KlassOop map ) const {
        return _klass == map ? (Expression *) this : nullptr;
    }


    bool equals( Expression *other ) const;

    void print();

    virtual void verify() const;
};


// a cloned block literal (result of BlockClone node)
class BlockExpression : public KlassExpression {
protected:
    InlinedScope *_blockScope;        // block's parent scope
public:
    BlockExpression( BlockPseudoRegister *p, Node *n );
    
    BlockExpression() = default;
    virtual ~BlockExpression() = default;
    BlockExpression( const BlockExpression & ) = default;
    BlockExpression &operator=( const BlockExpression & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    bool isBlockExpression() const {
        return true;
    }


    InlinedScope *blockScope() const {
        return _blockScope;
    }


    BlockPseudoRegister *pseudoRegister() const {
        return (BlockPseudoRegister *) _pseudoRegister;
    }


    std::int32_t nklasses() const {
        return 1;
    }


    Expression *shallowCopy( PseudoRegister *p, Node *n ) const;

    Expression *mergeWith( Expression *other, Node *n );

    bool equals( Expression *other ) const;

    void print();

    virtual void verify() const;
};

// an expression whose exact runtime value is known
class ConstantExpression : public Expression {

private:
    Oop _c;

public:
    ConstantExpression( Oop c, PseudoRegister *p, Node *n ) :
        Expression( p, n ),
        _c{ c } {
    }

    ConstantExpression() = default;
    virtual ~ConstantExpression() = default;
    ConstantExpression( const ConstantExpression & ) = default;
    ConstantExpression &operator=( const ConstantExpression & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    
    bool isConstantExpression() const {
        return true;
    }


    bool containsUnknown() {
        return false;
    }


    bool hasKlass() const {
        return true;
    }


    KlassExpression *asKlassExpression() const;


    KlassOop klass() const {
        return _c->klass();
    }


    std::int32_t nklasses() const {
        return 1;
    }


    ConstantExpression *asConstantExpression() const {
        return (ConstantExpression *) this;
    }


    bool hasConstant() const {
        return true;
    }


    Oop constant() const {
        return _c;
    }


    virtual bool needsStoreCheck() const;

    NameNode *nameNode( bool mustBeLegal = true ) const;

    Expression *shallowCopy( PseudoRegister *p, Node *n ) const;

    Expression *copyWithout( Expression *e ) const;

    Expression *mergeWith( Expression *other, Node *n );

    Expression *convertToKlass( PseudoRegister *p, Node *n ) const;

    Expression *findKlass( KlassOop map ) const;

    bool equals( Expression *other ) const;

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
    GrowableArray<Expression *> *exprs;

    MergeExpression( Expression *e1, Expression *e2, PseudoRegister *p, Node *n );

    MergeExpression( PseudoRegister *p, Node *n );

    MergeExpression() = default;
    virtual ~MergeExpression() = default;
    MergeExpression( const MergeExpression & ) = default;
    MergeExpression &operator=( const MergeExpression & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    // A MergeExpression's PseudoRegister says where the merged result is (i.e., it may be different from
    // the PseudoRegisters of the individual expressions); typically, there's an assigmnent from the
    // subexpression's PseudoRegister to the MergeExpression's PseudoRegister just before the control flow merge.
    // The node n (if non-nullptr) is considered the defining node of the MergeExpression; it is usually
    // one of the first nodes after the control flow merge.  It is always legal to pass
    // nullptr for n, but doing so prevents splitting.

    bool isMergeExpression() const {
        return true;
    }


FLAG_DEF( Splittable );
PFLAG_DEF( UnknownSet );
PFLAG_DEF( ContainingUnknown );
public:
    bool containsUnknown();


    MergeExpression *asMergeExpression() const {
        return (MergeExpression *) this;
    }


    bool hasKlass() const;

    std::int32_t nklasses() const;

    KlassExpression *asKlassExpression() const;

    bool really_hasKlass( InlinedScope *s ) const;

    KlassOop klass() const;

    Expression *asReceiver() const;

    bool hasConstant() const;

    Oop constant() const;

    Expression *shallowCopy( PseudoRegister *p, Node *n ) const;

    Expression *copyWithout( Expression *e ) const;

    Expression *mergeWith( Expression *other, Node *n );

    Expression *convertToKlass( PseudoRegister *p, Node *n ) const;

    Expression *makeUnknownUnlikely( InlinedScope *s );

    bool equals( Expression *other ) const;

    void print();

    virtual void verify() const;

    UnknownExpression *findUnknown() const;    // returns nullptr if no expression found
    Expression *findKlass( KlassOop map ) const;

protected:
    void initialize();

    void mergeInto( Expression *other, Node *n );

    void add( Expression *s );
};


// an expression for a context pointer
// used only for compiler debugging; it should never appear where a normal expression is expected
class ContextExpression : public Expression {
public:
    ContextExpression( PseudoRegister *r );


    bool isContextExpression() const {
        return true;
    }


    bool containsUnknown() {
        ShouldNotCallThis();
        return false;
    }


    std::int32_t nklasses() const {
        ShouldNotCallThis();
        return 1;
    }


    Expression *shallowCopy( PseudoRegister *p, Node *n ) const {
        st_unused( p ); // unused
        st_unused( n ); // unused
        ShouldNotCallThis();
        return (Expression *) this;
    }


    Expression *copyWithout( Expression *e ) const {
        st_unused( e ); // unused
        ShouldNotCallThis();
        return (Expression *) this;
    }


    Expression *mergeWith( Expression *other, Node *n ) {
        st_unused( other ); // unused
        st_unused( n ); // unused
        ShouldNotCallThis();
        return (Expression *) this;
    }


    Expression *convertToKlass( PseudoRegister *p, Node *n ) const {
        return shallowCopy( p, n );
    };


    bool equals( Expression *other ) const {
        st_unused( other ); // unused
        ShouldNotCallThis();
        return false;
    }


    void print();

    virtual void verify() const;
};


class ExpressionStack : public GrowableArray<Expression *> {
    // an ExpressionStack simulates the run-time expression stack during compilation
    // it keeps track of the live ranges by recording the byteCodeIndex on push/pop
private:
    InlinedScope *_scope;                // scope that generates the pushes and pops

public:
    ExpressionStack( InlinedScope *scope, std::int32_t size );

    ExpressionStack() = default;
    virtual ~ExpressionStack() = default;
    ExpressionStack( const ExpressionStack & ) = default;
    ExpressionStack &operator=( const ExpressionStack & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    void push( Expression *expr, InlinedScope *currentScope, std::int32_t byteCodeIndex );

    void push2nd( Expression *expr, InlinedScope *currentScope, std::int32_t byteCodeIndex ); // allows a 2nd expr to be pushed for the same byteCodeIndex
    void assign_top( Expression *expr );

    Expression *pop();

    void pop( std::int32_t nofExprsToPop );

    void print();
};

#undef BASIC_FLAG_DEF
#undef FLAG_DEF
#undef PFLAG_DEF
