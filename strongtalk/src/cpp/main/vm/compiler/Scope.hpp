//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/compiler/Expression.hpp"
#include "vm/compiler/NodeBuilder.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"


// Scopes represent the source-level scopes compiled by the compiler.
// Compilation starts with a method or a block.
// All subsequently in lined methods/blocks are described via InlinedScopes.
// If and only if compilation starts with a block, there are parent scopes described via OutlinedScopes.
//
// OutlinedScopes represent scopes outside the current compilation; i.e., at runtime, OutlinedScopes will be in different frames
// than all InlinedScopes (which are part of the same frame).
// Two OutlinedScopes may or may not be in different frames depending on previous inlining (a real stack
// frame may consist of several virtual frames / OutlinedScopes).
// Thus the scopes up to the OutlinedScope representing the method enclosing the block method may be in one or several stack frames.

class Scope;

class InlinedScope;

class MethodScope;

class BlockScope;

class OutlinedScope;

class OutlinedMethodScope;

class OutlinedBlockScope;

class CompiledLoop;

class RecompilationScope;


// SendInfo holds various data about a send before/while it is being inlined

class SendInfo : public PrintableResourceObject {
public:
    InlinedScope   *_senderScope;      //
    Expression     *_receiver;         //
    LookupKey      *_lookupKey;        //
    PseudoRegister *_resultRegister;   // register where result should end up
    SymbolOop      _selector;                //
    bool           _needRealSend;            // need a real (non-inlined) send
    bool           _counting;                // count # sends? (for non-inlined send)
    std::int32_t   _sendCount;               // estimated # of invocations (< 0 == unknown)
    bool           _predicted;               // was receiver type-predicted?
    bool           uninlinable;              // was send considered uninlinable?
    bool           _receiverStatic;          // receiver type is statically known
    bool           _inPrimitiveFailure;      // sent from within prim. failure block

protected:
    void init();

public:
    SendInfo( InlinedScope *sen, Expression *r, SymbolOop s ) {
        _senderScope = sen;
        _receiver    = r;
        _selector    = s;
        _lookupKey   = nullptr;
        init();
    }


    SendInfo( InlinedScope *senderScope, LookupKey *lookupKey, Expression *r );

    void computeNSends( RecompilationScope *rscope, std::int32_t byteCodeIndex );

    void print();
};


// Scope definitions

class Scope : public PrintableResourceObject {
private:
    static smi_t _currentScopeID;            // for scope descs

public:
    // scopes are numbered starting at 0
    static void initialize() {
        _currentScopeID = 0;
    }


    smi_t currentScopeID() {
        return _currentScopeID++;
    }


    virtual smi_t scopeID() const = 0;


    // test functions
    virtual bool isInlinedScope() const {
        return false;
    }


    virtual bool isMethodScope() const {
        return false;
    }


    virtual bool isBlockScope() const {
        return false;
    }


    virtual bool isOutlinedScope() const {
        return false;
    }


    virtual bool isOutlinedMethodScope() const {
        return false;
    }


    virtual bool isOutlinedBlockScope() const {
        return false;
    }


    virtual KlassOop selfKlass() const = 0;

    virtual SymbolOop selector() const = 0;

    virtual MethodOop method() const = 0;

    virtual Scope *parent() const = 0;    // lexically enclosing scope or nullptr (if MethodScope)
    virtual InlinedScope *sender() const = 0;    // caller scope
    virtual KlassOop methodHolder() const = 0;    // for super sends

    virtual bool allocatesInterpretedContext() const = 0;// true if the scope allocates its own context in the interpreter
    virtual bool allocatesCompiledContext() const = 0;    // true if the scope allocates a context in the compiled code
    virtual bool expectsContext() const = 0;    // true if the scope has an incoming context in the interpreter
    bool needsContextZapping() const {
        return ( parent() == nullptr ) and allocatesCompiledContext();
    }


    virtual Scope *home() const = 0;    // the home scope
    bool isTop() const {
        return sender() == nullptr;
    }


    bool isInlined() const {
        return sender() not_eq nullptr;
    }


    virtual bool isSenderOf( InlinedScope *s ) const {
        return false;
    } // isSenderOf = this is a proper caller of s
    bool isSenderOrSame( InlinedScope *s ) {
        return (Scope *) s == this or isSenderOf( s );
    }


    virtual bool isRecursiveCall( MethodOop method, KlassOop rcvrKlass, std::int32_t n ) = 0;


    virtual void genCode() {
        ShouldNotCallThis();
    }
};


class InlinedScope : public Scope {

protected:
    std::int32_t _scopeID;                   //
    InlinedScope *_sender;                    // nullptr for top scope
    std::int32_t _senderByteCodeIndex;       // call position in sender (if inlined)
    ScopeInfo    _scopeInfo;                 // for debugging information (see scopeDescRecoder.hpp)
    LookupKey    *_key;                       //
    KlassOop     _methodHolder;              // not_eq receiver klass only for methods invoked by super sends
    MethodOop    _method;
    std::int32_t _nofSends;                  // no. of non-inlined sends, cumulative (incl. subScopes)
    std::int32_t _nofInterruptPoints;        // no. of interrupt points, cumulative (incl. subScopes) (_nofInterruptPoints == 0 => needs no debug info)
    bool         _primFailure;               // true if in a primitive call failure branch
    bool         _endsDead;                  // true if method ends with dead code
    Expression   *_self;                      // the receiver
    NodeBuilder  _gen;                       // the generator of the intermediate representation

    PseudoRegister                  *_context;            // context (either passed in or created here), if any
    GrowableArray<Expression *>     *_arguments;          // the arguments
    GrowableArray<Expression *>     *_temporaries;        // the (originally) stack-allocated temporaries
    GrowableArray<Expression *>     *_floatTemporaries;   // the (originally) stack-allocated float temporaries
    GrowableArray<Expression *>     *_contextTemporaries; // the (originally) heap-allocated temporaries
    GrowableArray<Expression *>     *_exprStackElems;     // the expression stack elems for debugging (indexed by byteCodeIndex)
    GrowableArray<InlinedScope *>   *_subScopes;          // the inlined scopes
    GrowableArray<CompiledLoop *>   *_loops;              // loops contained in this scope
    GrowableArray<NonTrivialNode *> *_typeTests;          // type test-like nodes contained in this scope
    GrowableArray<PseudoRegister *> *_pseudoRegistersBegSorted;     // the scope's PseudoRegisters sorted with begByteCodeIndex (used for regAlloc)
    GrowableArray<PseudoRegister *> *_pseudoRegistersEndSorted;     // the scope's PseudoRegisters sorted with endByteCodeIndex (used for regAlloc)

    // float temporaries
    std::int32_t _firstFloatIndex;    // the (stack) float temporary index for the first float

    // for node builders
    MergeNode       *_returnPoint;           // starting point for shared return code
    MergeNode       *_NonLocalReturneturnPoint;         // starting point for shared non-local return code
    MergeNode       *_nlrTestPoint;          // where NonLocalReturns coming from callees will jump to (or nullptr)
    ContextInitNode *_contextInitializer;    // node initializing context (if any)
    bool            _hasBeenGenerated;      // true iff genCode() was called

public:
    // for node builders
    MergeNode *returnPoint() {
        return _returnPoint;
    }


    MergeNode *nlrPoint() {
        return _NonLocalReturneturnPoint;
    }


    MergeNode *nlrTestPoint();        // returns a lazily generated nlrTestPoint
    ContextInitNode *contextInitializer() {
        return _contextInitializer;
    }


    bool has_nlrTestPoint() {
        return _nlrTestPoint not_eq nullptr;
    }


    void set_contextInitializer( ContextInitNode *n ) {
        _contextInitializer = n;
    }


public:
    RecompilationScope *rscope;         // equiv. scope in recompilee (if any) - used for type feedback
    bool               predicted;      // was receiver type-predicted?
    std::int32_t       depth;          // call nesting level (top = 0)
    std::int32_t       loopDepth;      // loop nesting level (top = 0)
    Expression         *result;         // result of normal return (nullptr if none)
    Expression         *nlrResult;      // NonLocalReturn result (non-nullptr only for blocks)
    PseudoRegister     *resultPR;       // pseudo register containing result

protected:
    InlinedScope();

    void initialize( MethodOop method, KlassOop methodHolder, InlinedScope *sender, RecompilationScope *rs, SendInfo *info );

public:
    smi_t scopeID() const {
        return _scopeID;
    }


    InlinedScope *sender() const {
        return _sender;
    }


    std::int32_t senderByteCodeIndex() const {
        return _senderByteCodeIndex;
    }


    ScopeInfo getScopeInfo() const {
        return _scopeInfo;
    }


    virtual std::int32_t byteCodeIndex() const {
        return gen()->byteCodeIndex();
    }


    bool isInlinedScope() const {
        return true;
    }


    std::int32_t nofArguments() const {
        return _arguments->length();
    }


    bool hasTemporaries() const {
        return _temporaries not_eq nullptr;
    }


    std::int32_t nofTemporaries() const {
        return _temporaries->length();
    }


    bool hasFloatTemporaries() const {
        return _floatTemporaries not_eq nullptr;
    }


    std::int32_t nofFloatTemporaries() const {
        return _floatTemporaries->length();
    }


    std::int32_t firstFloatIndex() const {
        st_assert( _firstFloatIndex >= 0, "not yet computed" );
        return _firstFloatIndex;
    }


    bool allocatesInterpretedContext() const {
        return _method->allocatesInterpretedContext();
    }


    bool allocatesCompiledContext() const;


    bool expectsContext() const {
        return _method->expectsContext();
    }


    bool isSenderOf( InlinedScope *s ) const;


    GrowableArray<Expression *> *contextTemporaries() const {
        return _contextTemporaries;
    }


    std::int32_t nofBytes() const {
        return _method->end_byteCodeIndex() - 1;
    }


    std::int32_t nofSends() const {
        return _nofSends;
    }


    bool containsNonLocalReturn() const {
        return _method->containsNonLocalReturn();
    }


    bool primFailure() const {
        return _primFailure;
    }


    Expression *self() const {
        return _self;
    }


    void set_self( Expression *e );


    NodeBuilder *gen() const {
        return static_cast<NodeBuilder *>( const_cast< NodeBuilder * >( &_gen ));
    }


    GrowableArray<Expression *> *exprStackElems() const {
        return _exprStackElems;
    }


    void addSubScope( InlinedScope *s );


    KlassOop selfKlass() const {
        return _key->klass();
    }


    LookupKey *key() const {
        return _key;
    }


    KlassOop methodHolder() const {
        return _methodHolder;
    }


    MethodOop method() const {
        return _method;
    }


    SymbolOop selector() const {
        return _key->selector();
    }


    bool isLite() const;


    Expression *argument( std::int32_t no ) const {
        return _arguments->at( no );
    }


    Expression *temporary( std::int32_t no ) const {
        return _temporaries->at( no );
    }


    Expression *floatTemporary( std::int32_t no ) const {
        return _floatTemporaries->at( no );
    }


    void set_temporary( std::int32_t no, Expression *t ) {
        _temporaries->at_put( no, t );
    }


    Expression *contextTemporary( std::int32_t no ) const {
        return _contextTemporaries->at( no );
    }


    PseudoRegister *context() const {
        return _context;
    }


    virtual void setContext( PseudoRegister *ctx ) {
        _context = ctx;
    }


    ExpressionStack *exprStack() const {
        return gen()->exprStack();
    }


    Node *current() const {
        return gen()->current();
    }


    virtual bool is_self_initialized() const {
        return true;
    }


    virtual void set_self_initialized() {
        ShouldNotCallThis();
    }


    bool hasBeenGenerated() const {
        return _hasBeenGenerated;
    }


    void createTemporaries( std::int32_t nofTemps );

    void createFloatTemporaries( std::int32_t nofFloats );

    void createContextTemporaries( std::int32_t nofTemps );

    void contextTemporariesAtPut( std::int32_t no, Expression *e );

    std::int32_t homeContext() const;            // the home context level
    InlinedScope *find_scope( std::int32_t context, std::int32_t &nofIndirections, OutlinedScope *&out );    // find enclosing scope
    void addResult( Expression *e );

    void genCode();

    void addSend( GrowableArray<PseudoRegister *> *exprStack, bool isSend );


    GrowableArray<NonTrivialNode *> *typeTests() const {
        return _typeTests;
    }


    GrowableArray<CompiledLoop *> *loops() const {
        return _loops;
    }


    void addTypeTest( NonTrivialNode *t );

    CompiledLoop *addLoop();

    void setExprForByteCodeIndex( std::int32_t byteCodeIndex, Expression *expr );

    void set2ndExprForByteCodeIndex( std::int32_t byteCodeIndex, Expression *expr );

    virtual void collectContextInfo( GrowableArray<InlinedScope *> *scopeList );

    virtual void generateDebugInfo();

    void generateDebugInfoForNonInlinedBlocks();

    void optimizeLoops();

    void subScopesDo( Closure<InlinedScope *> *c );        // apply f to receiver and all subscopes

protected:
    void initializeArguments();


    virtual void initializeSelf() {
    }


    virtual void prologue();

    virtual void epilogue();

public:
    std::int32_t descOffset();

protected:
    // std::int32_t calleeSize(RecompilationScope* rs);
    void markLocalsDebugVisible( GrowableArray<PseudoRegister *> *exprStack );

public:
    // for global register allocation
    void addToPseudoRegistersBegSorted( PseudoRegister *r );

    void addToPseudoRegistersEndSorted( PseudoRegister *r );

    void allocatePseudoRegisters( IntegerFreeList *f );

    std::int32_t allocateFloatTemporaries( std::int32_t firstFloatIndex );    // returns the number of float temps allocated for
    // this and all subscopes; sets _firstFloatIndex

public:
    void print();

    void printTree();

    // see Compiler::number_of_noninlined_blocks
    std::int32_t number_of_noninlined_blocks();

    // see Compiler::copy_noninlined_block_info
    void copy_noninlined_block_info( NativeMethod *nm );

    friend class OutlinedScope;
};


class MethodScope : public InlinedScope {     // ordinary methods
protected:
    MethodScope();

    void initialize( MethodOop method, KlassOop methodHolder, InlinedScope *sen, RecompilationScope *rs, SendInfo *info );

public:
    static MethodScope *new_MethodScope( MethodOop method, KlassOop methodHolder, InlinedScope *sen, RecompilationScope *rs, SendInfo *info );


    bool isMethodScope() const {
        return true;
    }


    Scope *parent() const {
        return nullptr;
    }


    Scope *home() const {
        return (Scope *) this;
    }


    bool isRecursiveCall( MethodOop method, KlassOop rcvrKlass, std::int32_t n );

    void generateDebugInfo();

    // debugging
    void print_short();

    void print();
};


class BlockScope : public InlinedScope {        // block methods
protected:
    Scope *_parent;                // lexically enclosing scope
    bool  _self_is_initialized;            // true if self has been loaded
    void initialize( MethodOop method, KlassOop methodHolder, Scope *p, InlinedScope *s, RecompilationScope *rs, SendInfo *info );

    void initializeSelf();

    BlockScope();

public:
    static BlockScope *new_BlockScope( MethodOop method, KlassOop methodHolder, Scope *p, InlinedScope *s, RecompilationScope *rs, SendInfo *info );


    bool isBlockScope() const {
        return true;
    }


    bool is_self_initialized() const {
        return _self_is_initialized;
    }


    void set_self_initialized() {
        _self_is_initialized = true;
    }


    Scope *parent() const {
        return _parent;
    }


    Scope *home() const {
        return _parent->home();
    }


    KlassOop selfKlass() const {
        return _parent->selfKlass();
    }


    bool isRecursiveCall( MethodOop method, KlassOop rcvrKlass, std::int32_t n );

    void generateDebugInfo();

    void setContext( PseudoRegister *newContext );

    // debugging
    void print_short();

    void print();
};


class OutlinedScope : public Scope {        // abstract; a scope outside of the current compilation
protected:
    NativeMethod    *_nm;                // NativeMethod containing this scope
    ScopeDescriptor *_scope;

public:
    OutlinedScope( NativeMethod *nm, ScopeDescriptor *scope );


    bool isOutlinedScope() const {
        return true;
    }


    SymbolOop selector() const {
        return _scope->selector();
    }


    Expression *receiverExpression( PseudoRegister *p ) const;

    MethodOop method() const;


    NativeMethod *nm() const {
        return _nm;
    }


    InlinedScope *sender() const {
        return nullptr;
    }


    ScopeDescriptor *scope() const {
        return _scope;
    }


    bool allocatesInterpretedContext() const {
        return method()->allocatesInterpretedContext();
    }


    bool allocatesCompiledContext() const {
        return _scope->allocates_compiled_context();
    }


    bool expectsContext() const {
        return method()->expectsContext();
    }


    // debugging
    void print_short( const char *name );

    void print( const char *name );
};

OutlinedScope *new_OutlinedScope( NativeMethod *nm, ScopeDescriptor *sc );


class OutlinedMethodScope : public OutlinedScope {
public:
    OutlinedMethodScope( NativeMethod *nm, ScopeDescriptor *s ) :
        OutlinedScope( nm, s ) {
    }


    bool isOutlinedMethodScope() const {
        return true;
    }


    bool isRecursiveCall( MethodOop method, KlassOop rcvrKlass, std::int32_t n ) {
        ShouldNotCallThis();
        return false;
    }


    KlassOop selfKlass() const {
        return _scope->selfKlass();
    }


    smi_t scopeID() const {
        return _scope->scopeID();
    }


    Scope *parent() const {
        return nullptr;
    }


    Scope *home() const {
        return (Scope *) this;
    }


    KlassOop methodHolder() const;

    // debugging
    void print_short();

    void print();
};


class OutlinedBlockScope : public OutlinedScope {
protected:
    OutlinedScope *_parent;            // parent or nullptr (if non-LIFO)

public:
    OutlinedBlockScope( NativeMethod *nm, ScopeDescriptor *sen );


    bool isOutlinedBlockScope() const {
        return true;
    }


    bool isRecursiveCall( MethodOop method, KlassOop rcvrKlass, std::int32_t n ) {
        ShouldNotCallThis();
        return false;
    }


    Scope *parent() const {
        return _parent;
    }


    Scope *home() const {
        return _parent ? _parent->home() : nullptr;
    }


    KlassOop selfKlass() const {
        return _parent->selfKlass();
    }


    smi_t scopeID() const {
        return _scope->scopeID();
    }


    KlassOop methodHolder() const {
        return _parent->methodHolder();
    }


    // debugging
    void print_short();

    void print();
};
