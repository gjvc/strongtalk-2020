
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/compiler/BitVector.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/compiler/OpCode.hpp"
#include "vm/runtime/ResourceObject.hpp"


class NodeVisitor;  //
class InlinedScope; //

// This file defines the intermediate language used by the Compiler.
//
// For each node a code pattern is generated during code generation.
// Note: offsets in nodes are always in oops!


class Node;                     // abstract
class TrivialNode;              // abstract; has no definitions/uses; most generate no code
class MergeNode;                // nop; can have >1 predecessors
class NopNode;                  // generates no code (place holders etc)
class CommentNode;              // for compiler debugging
class FixedCodeNode;            // fixed code pattern for compiler debugging, instrumentation, etc.
class LoopHeaderNode;           // loop header (contains type tests moved out of loop)
class NonTrivialNode;           // non-trivial node, may have definitions/uses and generate code
class PrologueNode;             // entire method prologue
class LoadNode;                 // abstract
class LoadIntNode;              // load vm constant
class LoadOffsetNode;           // load slot of some object
class LoadUplevelNode;          // load up-level accessed variable
class StoreNode;                //
class AssignNode;               // general assignment (e.g. into temp)
class StoreOffsetNode;          // store into slot of object
class StoreUplevelNode;         // store up-level accessed variable
class AbstractReturnNode;

class ReturnNode;                       // method return
class NonLocalReturnSetupNode;          // setup NonLocalReturn leaving this compiled method
class InlinedReturnNode;                // inlined method return (old backend only)
class NonLocalReturnContinuationNode;   // continue NonLocalReturn going through this method
class ArithNode;                        // all computations (incl comparisons)
class AbstractBranchNode;               // nodes with (potentially) >1 sucessors
class TArithRRNode;                     // tagged arithmetic nodes
class CallNode;                         // abstract
class SendNode;                         // Delta send
class PrimitiveNode;                    // primitive call
class BlockCreateNode;                  // initial block clone (possibly memoized)
class BlockMaterializeNode;     // create block (testing for memoization)
class InterruptCheckNode;       // interrupt check / stack overflow test
class DLLNode;                  // DLL call
class ContextCreateNode;        // creates a context and copies arguments
class AbstractArrayAtNode;      //
class ArrayAtNode;              // _At: primitive
class AbstractArrayAtPutNode;   //
class ArrayAtPutNode;           // _At:Put: primitive
class InlinedPrimitiveNode;     // specialized inlined primitives
class BranchNode;               // machine-level conditional branches
class TypeTestNode;             // type tests (compare klasses, incl int/float)
class NonLocalReturnTestNode;   // tests whether NonLocalReturn has found home method
class ContextInitNode;          // initialize context fields
class ContextZapNode;           // zap context (new backend only)
class UncommonNode;             // uncommon branch

void initNodes();               // to be called before each compilation
void printNodes( Node *n );    // print n and its successors


class PrimitiveDescriptor;

// Node definitions

// Node: abstract superclass, holds common behavior (except prev/next links)

class PseudoRegisterMapping;

class BasicNode : public PrintableResourceObject {

protected:
    std::int16_t _id;                      // unique node id for debugging
    std::int16_t _num;                     // node number within basic block
    std::int16_t _byteCodeIndex;           // byteCodeIndex within the sc
    BasicBlock            *_basicBlock;            // basic block to which this instance belongs
    InlinedScope          *_scope;                 // scope to which this instance belongs
    PseudoRegisterMapping *_pseudoRegisterMapping; // the mapping at that node, if any (will be modified during code generation)

public:
    Label  _label;         // for jumps to this node -- SHOULD BE MOVED TO BasicBlock -- fix this
    bool_t _dontEliminate; // for special cases: must not eliminate this node
    bool_t _deleted;       // node has been deleted

    int id() const {
        return this == nullptr ? -1 : _id;
    }


    BasicBlock *bb() const {
        return _basicBlock;
    }


    int num() const {
        return _num;
    }


    InlinedScope *scope() const {
        return _scope;
    }


    int byteCodeIndex() const {
        return _byteCodeIndex;
    }


    void setBasicBlock( BasicBlock *b ) {
        _basicBlock = b;
    }


    void setNum( int n ) {
        _num = n;
    }


    void setScope( InlinedScope *s );

    static int       currentID;              // current node ID
    static int       currentCommentID;       // current ID for comment nodes
    static ScopeInfo lastScopeInfo;          // for programCounterDescriptor generation
    static int       lastByteCodeIndex;      //

    BasicNode();


    virtual bool_t isPrologueNode() const {
        return false;
    }


    virtual bool_t isAssignNode() const {
        return false;
    }


    virtual bool_t isTArithNode() const {
        return false;
    }


    virtual bool_t isArithNode() const {
        return false;
    }


    virtual bool_t isNonLocalReturnSetupNode() const {
        return false;
    }


    virtual bool_t isNonLocalReturnTestNode() const {
        return false;
    }


    virtual bool_t isNonLocalReturnContinuationNode() const {
        return false;
    }


    virtual bool_t isReturnNode() const {
        return false;
    }


    virtual bool_t isInlinedReturnNode() const {
        return false;
    }


    virtual bool_t isLoopHeaderNode() const {
        return false;
    }


    virtual bool_t isExitNode() const {
        return false;
    }


    virtual bool_t isMergeNode() const {
        return false;
    }


    virtual bool_t isBranchNode() const {
        return false;
    }


    virtual bool_t isBlockCreateNode() const {
        return false;
    }


    virtual bool_t isContextCreateNode() const {
        return false;
    }


    virtual bool_t isContextInitNode() const {
        return false;
    }


    virtual bool_t isContextZapNode() const {
        return false;
    }


    virtual bool_t isCommentNode() const {
        return false;
    }


    virtual bool_t isSendNode() const {
        return false;
    }


    virtual bool_t isCallNode() const {
        return false;
    }


    virtual bool_t isStoreNode() const {
        return false;
    }


    virtual bool_t isDeadEndNode() const {
        return false;
    }


    virtual bool_t isTypeTestNode() const {
        return false;
    }


    virtual bool_t isUncommonNode() const {
        return false;
    }


    virtual bool_t isUncommonSendNode() const {
        return false;
    }


    virtual bool_t isNopNode() const {
        return false;
    }


    virtual bool_t isCmpNode() const {
        return false;
    }


    virtual bool_t isArraySizeLoad() const {
        return false;
    }


    virtual bool_t isAccessingFloats() const {
        return false;
    }


    virtual bool_t isTrivial() const = 0;

protected:
    virtual Node *clone( PseudoRegister *from, PseudoRegister *to ) const {
        SubclassResponsibility();
        return nullptr;
    }


    void genProgramCounterDescriptor();

public:
    // for splitting: rough estimate of Space cost of node (in bytes)
    virtual int cost() const {
        return oopSize;
    }


    virtual bool_t hasDest() const {
        return false;
    }


    virtual bool_t canCopyPropagate() const {
        return false;
    }


    // canCopyPropagate: can node replace a use with copy-propagated PseudoRegister?
    // if true, must implement copyPropagate below
    bool_t canCopyPropagate( Node *fromNode ) const; // can copy-propagate from fromNode to receiver?
    virtual bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false ) = 0;


    virtual bool_t canCopyPropagateOop() const {
        return false;
    }


    // canCopyPropagateOop: can node replace a use with a copy-propagated Oop?
    // if true, must handle ConstPseudoRegisters; implies canCopyPropagate
    virtual bool_t isAssignmentLike() const {
        return false;
    }


    // isAssignmentLike: node copies src to dest (implies hasSrc/Dest)
    virtual bool_t shouldCopyWhenSplitting() const {
        return false;
    }


    virtual bool_t hasSrc() const {
        return false;
    }


    virtual bool_t hasConstantSrc() const {
        return false;
    }


    virtual Oop constantSrc() const {
        ShouldNotCallThis();
        return 0;
    }


    virtual bool_t canChangeDest() const {
        st_assert( hasDest(), "shouldn't call" );
        return true;
    }


    virtual bool_t endsBasicBlock() const = 0;


    virtual bool_t startsBasicBlock() const {
        return false;
    }


    int loopDepth() const {
        return _basicBlock->loopDepth();
    }


    virtual BasicBlock *newBasicBlock();


    virtual void makeUses( BasicBlock *bb ) {
    }


    virtual void removeUses( BasicBlock *bb ) {
    }


    virtual void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false ) = 0;


    virtual bool_t canBeEliminated() const {
        return not _dontEliminate;
    }


    virtual void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *lst ) {
    }


    virtual void markAllocated( int *use_count, int *def_count ) = 0;

    virtual SimpleBitVector trashedMask();

    virtual void gen();


    virtual void apply( NodeVisitor *v ) {
        ShouldNotCallThis();
    }


    virtual Node *likelySuccessor() const = 0;    // most likely code path
    virtual Node *uncommonSuccessor() const = 0;    // code path taken only in uncommon situations
    void removeUpToMerge();            // remove all nodes from this to next merge
    Node *copy( PseudoRegister *from, PseudoRegister *to ) const;    // make a copy, substituting 'to' wherever 'from' is used

    // for type test / loop optimization
    // If a node includes one or more type tests of its argument(s), it should return true for doesTypeTests
    // and implement the other four methods in this group.  It can then benefit from type test optimizations
    // (e.g., moving a test out of a loop).
    virtual bool_t doesTypeTests() const {
        return false;
    }          // does node perform any type test?

    virtual bool_t hasUnknownCode() const {
        return false;
    }          // does handle unknown cases? (with real code, not uncommon branch)

    virtual void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const {
        ShouldNotCallThis();
    }


    // return a list of pregs tested and, for each preg, a list of its types
    virtual void assert_preg_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n ) {
    } // assert that the klass of r (used by the reciver) is oneof(klasses)

    virtual void assert_in_bounds( PseudoRegister *r, LoopHeaderNode *n ) {
    }// assert that r (used by the reciver) is within array bounds

    virtual void print_short();


    virtual void print() {
        print_short();
    }


    virtual const char *print_string( const char *buf, bool_t printAddr = true ) const = 0;

    void printID() const;


    virtual void verify() const {
    }

    // Mappings
    //
    // Note: make sure, mapping at node is always unchanged, once it is set.

    PseudoRegisterMapping *mapping() const;


    bool_t hasMapping() const {
        return _pseudoRegisterMapping not_eq nullptr;
    }


    void setMapping( PseudoRegisterMapping *mapping );
};


// Linking code for Nodes
// originally known as NodeClassTemplate(Node, BasicNode)

class Node : public BasicNode {

protected:
    Node *_prev;
    Node *_next;


    Node() {
        _prev = nullptr;
        _next = nullptr;
    }


public:
    virtual bool_t endsBasicBlock() const;

    virtual Node *likelySuccessor() const;

    virtual Node *uncommonSuccessor() const;

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    virtual void verify() const;

    friend class NodeFactory;


    virtual bool_t hasSingleSuccessor() const {
        return true;
    }


    virtual bool_t hasSinglePredecessor() const {
        return true;
    }


    virtual int nPredecessors() const {
        return _prev ? 1 : 0;
    }


    virtual int nSuccessors() const {
        return _next ? 1 : 0;
    }


    virtual bool_t isPredecessor( const Node *n ) const {
        return _prev == n;
    }


    virtual bool_t isSuccessor( const Node *n ) const {
        return _next == n;
    }


    Node *next() const {
        return _next;
    }


    virtual Node *next1() const {
        return nullptr;
    }


    virtual Node *next( int i ) const {
        if ( i == 0 )
            return _next;
        else {
            st_fatal( "single next" );
            return nullptr;
        }
    }


    virtual Node *firstPrev() const {
        return _prev;
    }


    virtual Node *prev( int n ) const {
        if ( n == 0 )
            return _prev;
        else st_fatal( "single prev" );
        return nullptr;
    }


    virtual void setPrev( Node *p ) {
        st_assert( _prev == nullptr, "already set" );
        _prev = p;
    }


    virtual void movePrev( Node *from, Node *to ) {
        st_assert( _prev == from, "mismatched prev link" );
        _prev = to;
    }


    void setNext( Node *n ) {
        st_assert( _next == nullptr, "already set" );
        _next = n;
    }


    virtual void setNext1( Node *n ) {
        ShouldNotCallThis();
    }


    virtual void setNext( int i, Node *n ) {
        if ( i == 0 )
            setNext( n );
        else st_fatal( "subclass" );
    }


    virtual void moveNext( Node *from, Node *to ) {
        st_assert( _next == from, "mismatched next link" );
        _next = to;
    }


protected:
    virtual void removeMe();

    Node *endOfList() const;

public:  // really should be private, but C++ has instance-based rather than class-based privacy -- don't you love it?
    virtual void removePrev( Node *n );

public:
    virtual void removeNext( Node *n );            // cut the next link between this and n
    Node *append( Node *p ) {
        setNext( p );
        p->setPrev( this );
        return p;
    }


    Node *append1( Node *p ) {
        setNext1( p );
        p->setPrev( this );
        return p;
    }


    Node *append( int i, Node *p ) {
        setNext( i, p );
        p->setPrev( this );
        return p;
    }


    Node *appendEnd( Node *p ) {
        return endOfList()->append( p );
    }


    void insertNext( Node *n ) {
        st_assert( hasSingleSuccessor(), ">1 successors" );
        n->setNext( _next );
        n->setPrev( this );
        _next->movePrev( this, n );
        _next = n;
    }


    void insertPrev( Node *n ) {
        st_assert( n->hasSinglePredecessor(), "where to insert?" );
        n->setPrev( _prev );
        n->setNext( this );
        _prev->moveNext( this, n );
        _prev = n;
    }


    friend class NodeBuilder;
};


class TrivialNode : public Node {
public:
    bool_t isTrivial() const {
        return true;
    }


    int cost() const {
        return 0;
    }


    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false ) {
        return false;
    }


    void markAllocated( int *use_count, int *def_count ) {
    }


    // may need to implement several inherited dummy methods here -- most will do nothing
    friend class NodeFactory;
};


class NonTrivialNode : public Node {

protected:
    PseudoRegister *_dest;      // not all nodes actually have src & dest,
    PseudoRegister *_src;       // but they're declared here for convenience
    Usage          *_srcUse;    // and easier code sharing
    Definition     *_destDef;   //

    NonTrivialNode();

public:
    bool_t isTrivial() const {
        return false;
    }


    PseudoRegister *src() const;

    PseudoRegister *dest() const;


    PseudoRegister *dst() const {
        return dest();
    }


    void setDest( BasicBlock *bb, PseudoRegister *d );

    virtual bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void verify() const;

    friend class NodeFactory;
};


class PrologueNode : public NonTrivialNode {

protected:

    LookupKey *_key;
    const int _nofArgs;
    const int _nofTemps;


    PrologueNode( LookupKey *key, int nofArgs, int nofTemps ) :
            _nofArgs( nofArgs ), _nofTemps( nofTemps ) {
        _key = key;
    }


public:
    bool_t isPrologueNode() const {
        return true;
    }


    virtual bool_t canChangeDest() const {
        return false;
    }    // has no dest

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );


    void removeUses( BasicBlock *bb ) {
        ShouldNotCallThis();
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->aPrologueNode( this );
    }


    bool_t canBeEliminated() const {
        return false;
    }


    void markAllocated( int *use_count, int *def_count ) {
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class LoadNode : public NonTrivialNode {

protected:
    LoadNode( PseudoRegister *d ) {
        _dest = d;
        st_assert( d, "dest is nullptr" );
    }


public:
    bool_t hasDest() const {
        return true;
    }


    bool_t canCopyPropagate() const {
        return true;
    }


    bool_t canCopyPropagateOop() const {
        return true;
    }


    void makeUses( BasicBlock *basicBlock );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    friend class NodeFactory;
};


class LoadIntNode : public LoadNode {

protected:
    int _value;        // constant (vm-level, not an Oop) to be loaded

    LoadIntNode( PseudoRegister *dst, int value ) :
            LoadNode( dst ) {
        _value = value;
    }


public:
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    int value() {
        return _value;
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->aLoadIntNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class LoadOffsetNode : public LoadNode {

public:
    // _src is base address (e.g. object containing a slot)
    int    _offset;          // offset in words
    bool_t _isArraySize;     // is this load implementing an array size primitive?

protected:
    LoadOffsetNode( PseudoRegister *dst, PseudoRegister *b, int offs, bool_t arr ) :
            LoadNode( dst ) {
        _src         = b;
        _offset      = offs;
        _isArraySize = arr;
    }


public:
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    PseudoRegister *base() const {
        return _src;
    }


    bool_t hasSrc() const {
        return true;
    }


    bool_t isArraySizeLoad() const {
        return _isArraySize;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aLoadOffsetNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class LoadUplevelNode : public LoadNode {

private:
    Usage          *_context0Use;   //
    PseudoRegister *_context0;      // starting context
    int       _nofLevels;      // no. of indirections to follow via context home field
    int       _offset;         // offset of temporary in final context
    SymbolOop _name;           // temporary name (for printing)

protected:
    LoadUplevelNode( PseudoRegister *dst, PseudoRegister *context0, int nofLevels, int offset, SymbolOop name );

public:
    PseudoRegister *context0() const {
        return _context0;
    }


    int nofLevels() const {
        return _nofLevels;
    }


    int offset() const {
        return _offset;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aLoadUplevelNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class StoreNode : public NonTrivialNode {

protected:
    StoreNode( PseudoRegister *s ) {
        _src = s;
        st_assert( _src, "src is nullptr" );
    }


public:
    bool_t isStoreNode() const {
        return true;
    }


    bool_t canCopyPropagate() const {
        return true;
    }


    bool_t canCopyPropagateOop() const {
        return true;
    }


    bool_t hasSrc() const {
        return true;
    }


    virtual bool_t needsStoreCheck() const {
        return false;
    }


    virtual const char *action() const = 0;        // for debugging messages
    virtual void setStoreCheck( bool_t ncs ) {
    }


    void assert_preg_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );

    friend class NodeFactory;
};


class StoreOffsetNode : public StoreNode {
    // store into data slot, do check-store if necessary
    // _src is value being stored, may be a ConstPseudoRegister*
private:
    PseudoRegister *_base;              // base address (object containing the slot)
    Usage          *_baseUse;           //
    int    _offset;             // offset in words
    bool_t _needsStoreCheck;    // does store need a GC store check?

protected:
    StoreOffsetNode( PseudoRegister *s, PseudoRegister *b, int o, bool_t nsc ) :
            StoreNode( s ) {
        _base = b;
        st_assert( b, "base is nullptr" );
        _offset          = o;
        _needsStoreCheck = nsc;
    }


public:
    PseudoRegister *base() const {
        return _base;
    }


    int offset() const {
        return _offset;
    }


    bool_t needsStoreCheck() const {
        return _needsStoreCheck;
    }


    bool_t hasSrc() const {
        return true;
    }


    void setStoreCheck( bool_t ncs ) {
        _needsStoreCheck = ncs;
    }


    const char *action() const {
        return "stored into an object";
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );


    bool_t canBeEliminated() const {
        return false;
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->aStoreOffsetNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class StoreUplevelNode : public StoreNode {
    // when node is created, it is not known whether store will be to stack/reg or context
    // _src may be a ConstPseudoRegister*
private:
    Usage          *_context0Use;       //
    PseudoRegister *_context0;          // starting context
    int       _nofLevels;          // no. of indirections to follow via context home field
    int       _offset;             // offset of temporary in final context
    bool_t    _needsStoreCheck;    // generate a store check if true
    SymbolOop _name;               // temporary name (for printing)

protected:
    StoreUplevelNode( PseudoRegister *src, PseudoRegister *context0, int nofLevels, int offset, SymbolOop name, bool_t needsStoreCheck );

public:
    PseudoRegister *context0() const {
        return _context0;
    }


    int nofLevels() const {
        return _nofLevels;
    }


    int offset() const {
        return _offset;
    }


    bool_t needsStoreCheck() const {
        return _needsStoreCheck;
    }


    void setStoreCheck( bool_t ncs ) {
        _needsStoreCheck = ncs;
    }


    const char *action() const {
        return "stored into a context temporary";
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aStoreUplevelNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class AssignNode : public StoreNode {
    // _src may be a ConstPseudoRegister*
protected:
    AssignNode( PseudoRegister *s, PseudoRegister *d );

public:
    int cost() const {
        return oopSize / 2;
    }  // assume 50% eliminated
    bool_t isAccessingFloats() const;


    bool_t isAssignNode() const {
        return true;
    }


    bool_t hasDest() const {
        return true;
    }


    bool_t hasSrc() const {
        return true;
    }


    bool_t hasConstantSrc() const {
        return _src->isConstPseudoRegister();
    }


    bool_t isAssignmentLike() const {
        return true;
    }


    bool_t shouldCopyWhenSplitting() const {
        return true;
    }


    bool_t canBeEliminated() const;

    Oop constantSrc() const;


    const char *action() const {
        return _dest->isSinglyAssignedPseudoRegister() ? "passed as an argument" : "assigned to a local";
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->anAssignNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

protected:
    void genOop();

    friend class NodeFactory;
};


class AbstractReturnNode : public NonTrivialNode {

protected:
    AbstractReturnNode( int byteCodeIndex, PseudoRegister *src, PseudoRegister *dest ) {
        _byteCodeIndex = byteCodeIndex;
        _src           = src;
        _dest          = dest;
    }


public:
    bool_t canBeEliminated() const {
        return false;
    }


    bool_t isReturnNode() const {
        return true;
    }


    bool_t endsBasicBlock() const {
        return true;
    }


    bool_t canCopyPropagate() const {
        return true;
    }


    bool_t canCopyPropagateOop() const {
        return true;
    }


    bool_t hasSrc() const {
        return true;
    }


    bool_t isAssignmentLike() const {
        return true;
    }


    bool_t hasDest() const {
        return true;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );


    void markAllocated( int *use_count, int *def_count ) {
    }


    friend class NodeFactory;
};


class InlinedReturnNode : public AbstractReturnNode {

    // should replace with AssignNode + ContextZap (if needed)
protected:
    InlinedReturnNode( int byteCodeIndex, PseudoRegister *src, PseudoRegister *dest ) :
            AbstractReturnNode( byteCodeIndex, src, dest ) {
    }


public:
    bool_t isInlinedReturnNode() const {
        return true;
    }


    bool_t endsBasicBlock() const {
        return NonTrivialNode::endsBasicBlock();
    }


    bool_t isTrivial() const {
        return true;
    }


    bool_t canBeEliminated() const {
        return true;
    }


    bool_t shouldCopyWhenSplitting() const {
        return true;
    }


    int cost() const {
        return 0;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->anInlinedReturnNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


// The NonLocalReturnSetupNode starts an NonLocalReturn by setting up two of the three special NonLocalReturn regs
// (home fp and home scope ID); it assumes the NonLocalReturn result already is in NonLocalReturnResultReg
// (so that the setup code can be shared).
//
// next() is the NonLocalReturnContinuationNode that actually does the NonLocalReturn.
// (Functionality is split up because the NonLocalReturnContinuationNode may be shared with NonLocalReturnTestNode)

class NonLocalReturnSetupNode : public AbstractReturnNode {
    Usage *_resultUse;
    Usage *_contextUse;            // needs context to load home FP
protected:
    NonLocalReturnSetupNode( PseudoRegister *result, int byteCodeIndex );

public:
    bool_t isExitNode() const {
        return true;
    }


    bool_t isNonLocalReturnSetupNode() const {
        return true;
    }


    // uses hardwired regs, has no src or dest
    bool_t canCopyPropagate() const {
        return false;
    }


    bool_t canCopyPropagateOop() const {
        return false;
    }


    bool_t hasSrc() const {
        return true;
    }  // otherwise breaks nonlocal_return
    bool_t isAssignmentLike() const {
        return false;
    }


    bool_t hasDest() const {
        return false;
    }


    bool_t canChangeDest() const {
        return false;
    }    // has no dest
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aNonLocalReturnSetupNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class NonLocalReturnContinuationNode : public AbstractReturnNode {
protected:
    NonLocalReturnContinuationNode( int byteCodeIndex, PseudoRegister *src, PseudoRegister *dest ) :
            AbstractReturnNode( byteCodeIndex, src, dest ) {
    }


public:
    bool_t isExitNode() const {
        return true;
    }


    bool_t isNonLocalReturnContinuationNode() const {
        return true;
    }


    // uses hardwired regs, has no src or dest
    bool_t canCopyPropagate() const {
        return false;
    }


    bool_t canCopyPropagateOop() const {
        return false;
    }


    bool_t hasSrc() const {
        return false;
    }


    bool_t isAssignmentLike() const {
        return false;
    }


    bool_t hasDest() const {
        return false;
    }


    bool_t canChangeDest() const {
        return false;
    }    // has no dest
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aNonLocalReturnContinuationNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


// normal method return
class ReturnNode : public AbstractReturnNode {
private:
    Usage *_resultUse;

protected:
    ReturnNode( PseudoRegister *res, int byteCodeIndex );

public:
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    bool_t isExitNode() const {
        return true;
    }


    bool_t hasSrc() const {
        return false;
    }


    bool_t isAssignmentLike() const {
        return false;
    }


    bool_t hasDest() const {
        return false;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aReturnNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


// Linking code for MergeNodes
// originally: MergeNodeClassTemplate(AbstractMergeNode, Node, GrowableArray<Node*>, TrivialNode, 3)

class AbstractMergeNode : public TrivialNode {
private:
    enum {
        N = 3         /* n-way merge */
    };

protected:
    GrowableArray<Node *> *_prevs;

public:
    AbstractMergeNode() {
        _prevs = new GrowableArray<Node *>( N );
    }


    AbstractMergeNode( Node *prev1, Node *prev2 ) {
        _prevs = new GrowableArray<Node *>( N );
        _prevs->append( prev1 );
        prev1->setNext( this );
        _prevs->append( prev2 );
        prev2->setNext( this );
    }


    bool_t hasSinglePredecessor() const {
        return _prevs->length() <= 1;
    }


    int nPredecessors() const {
        return _prevs->length();
    }


    Node *firstPrev() const {
        return _prevs->isEmpty() ? nullptr : _prevs->at( 0 );
    }


    void setPrev( Node *p ) {
        st_assert( p, "should be something" );
        st_assert( not _prevs->contains( p ), "shouldn't already be there" );
        _prevs->append( p );
    }


    Node *prev( int n ) const {
        return _prevs->at( n );
    }


protected:
    void removeMe();


    virtual void removePrev( Node *n ) {
        /* cut the _prev link between this and n	*/
        _prevs->remove( n );
    }


public:
    void movePrev( Node *from, Node *to );

    bool_t isPredecessor( const Node *n ) const;
};


class MergeNode : public AbstractMergeNode {

protected:
    MergeNode( int byteCodeIndex );

    MergeNode( Node *prev1, Node *prev2 );

public:
    bool_t _isLoopStart;        // does this node start a loop?
    bool_t _isLoopEnd;          // does this node end a loop? (i.e., first node after loop)
    bool_t _didStartBasicBlock; // used for debugging / assertion checks

    int cost() const {
        return 0;
    }


    bool_t isTrivial() const {
        return _prevs->length() <= 1;
    }


    bool_t startsBasicBlock() const {
        return _prevs->length() > 1 or _isLoopStart;
    }


    bool_t isMergeNode() const {
        return true;
    }


    BasicBlock *newBasicBlock();

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aMergeNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class ArithNode : public NonTrivialNode {    // abstract
    // NB: ArithNodes are not used for tagged int arithmetic -- see TArithNode
protected:
    ArithOpCode _op;
    ConstPseudoRegister *_constResult;    // non-nullptr if constant-folded

    ArithNode( ArithOpCode op, PseudoRegister *src, PseudoRegister *dst ) {
        _op          = op;
        _src         = src;
        _dest        = dst;
        _constResult = nullptr;
    }


public:
    bool_t canCopyPropagate() const {
        return true;
    }


    bool_t canCopyPropagateOop() const {
        return true;
    }


    bool_t hasSrc() const {
        return true;
    }


    bool_t hasDest() const {
        return true;
    }


    bool_t isAssignmentLike() const {
        return _constResult not_eq nullptr;
    }


    bool_t isArithNode() const {
        return true;
    }


    bool_t isCmpNode() const {
        return _op == ArithOpCode::tCmpArithOp;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );


    ArithOpCode op() const {
        return _op;
    }


    virtual bool_t operIsConst() const = 0;

    virtual int operConst() const = 0;

    virtual bool_t doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t repl );

    const char *opName() const;

    friend class NodeFactory;
};


class ArithRRNode : public ArithNode {  // reg op reg => reg

protected:
    PseudoRegister *_oper;
    Usage          *_operUse;

    ArithRRNode( ArithOpCode o, PseudoRegister *s, PseudoRegister *o2, PseudoRegister *d );

public:
    PseudoRegister *operand() const {
        return _oper;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    bool_t operIsConst() const;

    int operConst() const;

    bool_t doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->anArithRRNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class FloatArithRRNode : public ArithRRNode {  // for untagged float operations

    FloatArithRRNode( ArithOpCode o, PseudoRegister *s, PseudoRegister *o2, PseudoRegister *d ) :
            ArithRRNode( o, s, o2, d ) {
    }


public:
    bool_t isAccessingFloats() const {
        return true;
    }


    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aFloatArithRRNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class FloatUnaryArithNode : public ArithNode {
    // unary untagged float operation; src is an untagged float, dest is either another
    // untagged float or a floatOop
    FloatUnaryArithNode( ArithOpCode op, PseudoRegister *src, PseudoRegister *dst ) :
            ArithNode( op, src, dst ) {
    }


public:
    bool_t isAccessingFloats() const {
        return true;
    }


    bool_t isCmpNode() const {
        return false;
    }


    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );


    bool_t operIsConst() const {
        return false;
    }


    int operConst() const {
        ShouldNotCallThis();
        return 0;
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->aFloatUnaryArithNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class ArithRCNode : public ArithNode {  // reg op const => reg
    // used to compare against non-Oop constants (e.g. for markOop test)
    // DO NOT USE to add a reg and an Oop constant -- use ArithRR + ConstPseudoRegisters for that
protected:
    int _oper;


    ArithRCNode( ArithOpCode o, PseudoRegister *s, int o2, PseudoRegister *d ) :
            ArithNode( o, s, d ) {
        _oper = o2;
    }


public:
    int operand() const {
        return _oper;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->anArithRCNode( this );
    }


    bool_t operIsConst() const {
        return true;
    }


    int operConst() const {
        return _oper;
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};

// node with >1 successor; supplies linking code (next(i) et al.) and some default behavior
class AbstractBranchNode : public NonTrivialNode {
    // next() is the fall-through case, next1() the taken branch
public:
    virtual bool_t canFail() const = 0;        // does node have a failure branch?
    virtual Node *failureBranch() const {
        return next1();
    }


    bool_t endsBasicBlock() const {
        return true;
    }


protected:
    AbstractBranchNode() {
        _nxt = new GrowableArray<Node *>( EstimatedSuccessors );
    }


    void removeFailureIfPossible();

    void verify( bool_t verifySuccessors ) const;

    // ---------- node linking code --------------
private:
    enum {
        EstimatedSuccessors = 2  // most nodes have only 2 successors
    };
protected:
    GrowableArray<Node *> *_nxt;            /* elem 0 is next1 */

public:

    Node *next1() const {
        return _nxt->length() ? _nxt->at( 0 ) : nullptr;
    }


    bool_t hasSingleSuccessor() const {
        return next1() == nullptr;
    }


    int nSuccessors() const {
        return _nxt->length() + ( _next ? 1 : 0 );
    }


    Node *next() const {
        return _next;
    }


    Node *next( int i ) const {
        return i == 0 ? _next : _nxt->at( i - 1 );
    }


    void removeMe();

    void removeNext( Node *n );


    void setNext1( Node *n ) {
        st_assert( _nxt->length() == 0, "already set" );
        _nxt->append( n );
    }


    void setNext( Node *n ) {
        NonTrivialNode::setNext( n );
    }


    void setNext( int i, Node *n );

    void moveNext( Node *from, Node *to );

    bool_t isSuccessor( const Node *n ) const;
};

class TArithRRNode : public AbstractBranchNode {
    // tagged arithmetic operation; next() is success case, next1()
    // is failure (leaving ORed operands in Temp1 for tag test)
protected:
    ArithOpCode _op;
    PseudoRegister *_oper;
    Usage          *_operUse;
    bool_t _arg1IsInt;            // is _src a smi_t?
    bool_t _arg2IsInt;            // is _oper a smi_t?
    ConstPseudoRegister *_constResult;            // non-nullptr if constant-folded

    TArithRRNode( ArithOpCode o, PseudoRegister *s, PseudoRegister *o2, PseudoRegister *d, bool_t a1, bool_t a2 );

public:
    ArithOpCode op() const {
        return _op;
    }


    PseudoRegister *operand() const {
        return _oper;
    }


    bool_t arg1IsInt() const {
        return _arg1IsInt;
    }


    bool_t arg2IsInt() const {
        return _arg2IsInt;
    }


    bool_t canFail() const {
        return not( _arg1IsInt and _arg2IsInt );
    }


    bool_t isTArithNode() const {
        return true;
    }


    bool_t isAssignmentLike() const {
        return _constResult not_eq nullptr;
    }


    bool_t canCopyPropagate() const {
        return true;
    }


    bool_t canCopyPropagateOop() const {
        return true;
    }


    bool_t hasSrc() const {
        return true;
    }


    bool_t hasDest() const {
        return true;
    }


    bool_t doesTypeTests() const {
        return true;
    }


    bool_t hasUnknownCode() const;

    void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const;

    void assert_preg_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aTArithRRNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

protected:
    bool_t doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    friend class NodeFactory;
};


class CallNode : public AbstractBranchNode {
    // next1 is the NonLocalReturn branch (if there is one)
    // dest() is the return value
protected:
    CallNode( MergeNode *n, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *exprs );

public:
    GrowableArray<PseudoRegister *> *exprStack;   // current expr. stack for debugging info (nullptr if not needed)
    GrowableArray<Usage *>          *argUses;     // uses for args and receiver
    GrowableArray<PseudoRegister *> *uplevelUsed; // PseudoRegisters potentially uplevel-read during this call (nullptr if not needed)
    GrowableArray<PseudoRegister *> *uplevelDefd; // PseudoRegisters potentially uplevel-written during this call (nullptr if not needed)
    GrowableArray<Usage *>          *uplevelUses; // uses for uplevel-read names
    GrowableArray<Definition *>     *uplevelDefs; // definitions for uplevel-written names
    GrowableArray<PseudoRegister *> *args;        // args including receiver (at index 0, followed by first arg), or nullptr
    int                             nblocks;       // number of possibly live blocks at this point (for uplevel access computation)

    bool_t hasDest() const {
        return true;
    }


    bool_t isCallNode() const {
        return true;
    }


    bool_t canChangeDest() const {
        return false;
    }


    bool_t canBeEliminated() const {
        return false;
    }


    virtual bool_t canInvokeDelta() const = 0;    // can call invoke Delta code?
    MergeNode *nlrTestPoint() const;

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    SimpleBitVector trashedMask();

    void nlrCode();            // generate NonLocalReturn code sequence
    void verify() const;

    friend class NodeFactory;
};


class SendNode : public CallNode {

protected:
    LookupKey *_key;      // lookup key (for selector)
    bool_t _superSend;  // is it a super send?
    SendInfo *_info;     // to set CompiledInlineCache flags (counting, uninlinable, etc.)

    SendNode( LookupKey *key, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *exprStk, bool_t superSend, SendInfo *info );

public:
    bool_t isSendNode() const {
        return true;
    }


    bool_t isSuperSend() const {
        return _superSend;
    }


    bool_t isCounting() const;

    bool_t isUninlinable() const;

    bool_t staticReceiver() const;


    bool_t canInvokeDelta() const {
        return true;
    }


    bool_t canFail() const {
        return false;
    }


    int cost() const {
        return oopSize * 5;
    }      // include InlineCache + some param pushing
    PseudoRegister *recv() const;

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aSendNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class PrimitiveNode : public CallNode {

protected:
    PrimitiveDescriptor *_pdesc;

    PrimitiveNode( PrimitiveDescriptor *pdesc, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack );

public:
    bool_t canBeEliminated() const;

    bool_t canInvokeDelta() const;

    bool_t canFail() const;


    PrimitiveDescriptor *pdesc() const {
        return _pdesc;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aPrimitiveNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class DLLNode : public CallNode {
protected:
    SymbolOop _dll_name;
    SymbolOop _function_name;
    dll_func_ptr_t  _function;
    bool_t    _async;

    DLLNode( SymbolOop dll_name, SymbolOop function_name, dll_func_ptr_t function, bool_t async, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack );

public:
    bool_t canInvokeDelta() const;


    bool_t canFail() const {
        return true;
    }


    int nofArguments() const {
        return args == nullptr ? 0 : args->length();
    }


    SymbolOop dll_name() const {
        return _dll_name;
    }


    SymbolOop function_name() const {
        return _function_name;
    }


    dll_func_ptr_t function() const {
        return _function;
    }


    bool_t async() const {
        return _async;
    }


    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aDLLNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class InterruptCheckNode : public PrimitiveNode {
protected:
    static PrimitiveDescriptor *_intrCheck;


    InterruptCheckNode( GrowableArray<PseudoRegister *> *exprs ) :
            PrimitiveNode( _intrCheck, nullptr, nullptr, exprs ) {
    }


public:
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        Unimplemented();
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend void node_init();

    friend class NodeFactory;
};


class HoistedTypeTest;

class LoopRegCandidate;


//
// a LoopHeaderNode is inserted before every loop; usually it is a no-op for optimized integer loops, it does the pre-loop type tests
//
class LoopHeaderNode : public TrivialNode {

protected:
    // info for integer loops
    bool_t _integerLoop;                    // integer loop? (if no: inst. vars below are not set)
    PseudoRegister *_loopVar;              // loop variable
    PseudoRegister *_lowerBound;           // lower bound
    PseudoRegister *_upperBound;           // upper bound (or nullptr; mutually exclusive with boundArray)
    LoadOffsetNode *_upperLoad;            // loads array size that is the upper bound
    GrowableArray<AbstractArrayAtNode *> *_arrayAccesses;     // arrays indexed by loopVar

    LoopHeaderNode *_enclosingLoop;      // enclosing loop or nullptr
    // info for generic loops; all instance variables below this line are valid only after the loop optimization pass!
    GrowableArray<HoistedTypeTest *>  *_tests;              // type tests hoisted out of loop
    GrowableArray<LoopHeaderNode *>   *_nestedLoops;        // nested loops (nullptr if none)
    GrowableArray<LoopRegCandidate *> *_registerCandidates; // candidates for reg. allocation within loop (best comes first); nullptr if none
    bool_t                            _activated;            // gen() does nothing until activated
    int                               _nofCalls;             // number of non-inlined calls in loop (excluding unlikely code)

    LoopHeaderNode();

public:
    bool_t isLoopHeaderNode() const {
        return true;
    }


    bool_t isActivated() const {
        return _activated;
    }


    bool_t isInnerLoop() const {
        return _nestedLoops == nullptr;
    }


    bool_t isIntegerLoop() const {
        return _integerLoop;
    }


    PseudoRegister *loopVar() const {
        return _loopVar;
    }


    PseudoRegister *lowerBound() const {
        return _lowerBound;
    }


    PseudoRegister *upperBound() const {
        return _upperBound;
    }


    LoadOffsetNode *upperLoad() const {
        return _upperLoad;
    }


    GrowableArray<AbstractArrayAtNode *> *arrayAccesses() const {
        return _arrayAccesses;
    }


    GrowableArray<HoistedTypeTest *> *tests() const {
        return _tests;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const {
        ShouldNotCallThis();
        return nullptr;
    }


    int nofCallsInLoop() const {
        return _nofCalls;
    }


    void set_nofCallsInLoop( int n ) {
        _nofCalls = n;
    }


    void activate( PseudoRegister *loopVar, PseudoRegister *lowerBound, PseudoRegister *upperBound, LoadOffsetNode *upperLoad );

    void activate();                        // for non-integer loops
    void addArray( AbstractArrayAtNode *n );


    LoopHeaderNode *enclosingLoop() const {
        return _enclosingLoop;
    }


    void set_enclosingLoop( LoopHeaderNode *l );


    GrowableArray<LoopHeaderNode *> *nestedLoops() const {
        return _nestedLoops;
    }


    void addNestedLoop( LoopHeaderNode *l );


    GrowableArray<LoopRegCandidate *> *registerCandidates() const {
        return _registerCandidates;
    }


    void addRegisterCandidate( LoopRegCandidate *c );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aLoopHeaderNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;

    friend class CompiledLoop;

protected:
    void generateTypeTests( Label &cont, Label &failure );

    void generateIntegerLoopTests( Label &cont, Label &failure );

    void generateArrayLoopTests( Label &cont, Label &failure );

    void generateIntegerLoopTest( PseudoRegister *p, Label &cont, Label &failure );

    void handleConstantTypeTest( ConstPseudoRegister *r, GrowableArray<KlassOop> *klasses );
};


class BlockCreateNode : public PrimitiveNode {
    // Initializes block (closure) location with closure if it's
    // not a memoized block; and with 0 otherwise
    // src is nullptr (but non-nullptr for subclass instances)

protected:
    PseudoRegister *_context;    // context or parameter/self that's copied into the block (or nullptr)
    Usage          *_contextUse;    // use of _context

    void materialize();                // generates code to materialize the block
    void copyIntoContexts( Register val, Register t1, Register t2 );    // helper for above

    BlockCreateNode( BlockPseudoRegister *b, GrowableArray<PseudoRegister *> *expr_stack );

public:

    bool_t isBlockCreateNode() const {
        return true;
    }


    // block creation nodes are like assignments if memoized, so don't end BBs here
    bool_t endsBasicBlock() const {
        return not isMemoized() or NonTrivialNode::endsBasicBlock();
    }


    BlockPseudoRegister *block() const {
        st_assert( _dest->isBlockPseudoRegister(), "must be BlockPseudoRegister" );
        return (BlockPseudoRegister *) _dest;
    }


    bool_t isMemoized() const {
        return block()->isMemoized();
    }


    bool_t hasConstantSrc() const {
        return false;
    }


    bool_t hasSrc() const {
        return false;
    }


    int cost() const {
        return 2 * oopSize;
    }    // hope it's memoized

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    bool_t canBeEliminated() const {
        return not _dontEliminate;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aBlockCreateNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend void node_init();

    friend class NodeFactory;
};


class BlockMaterializeNode : public BlockCreateNode {
    // Materializes (creates) a block if it has not been materialized yet (no-op if not a memoized block)
    // src and dest are the BlockPseudoRegister
protected:
    BlockMaterializeNode( BlockPseudoRegister *b, GrowableArray<PseudoRegister *> *expr_stack ) :
            BlockCreateNode( b, expr_stack ) {
        _src = _dest;
    }


public:
    bool_t endsBasicBlock() const {
        return true;
    }


    bool_t hasSrc() const {
        return true;
    }


    int cost() const {
        return 5 * oopSize;
    } // assume blk is memoized

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aBlockMaterializeNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class ContextCreateNode : public PrimitiveNode {
    // src is parent context, dest is register holding created context
protected:
    int                             _nofTemps;             // no. of temps in context
    int                             _contextSize;          // size of compiled context
    int                             _contextNo;            // context number (index into compiler's contextList)
    GrowableArray<PseudoRegister *> *_parentContexts;     // context chain above parent context (if any)
    GrowableArray<Usage *>          *_parentContextUses;  // for _parentContexts

    ContextCreateNode( PseudoRegister *parent, PseudoRegister *context, int nofTemps, GrowableArray<PseudoRegister *> *expr_stack );

    ContextCreateNode( PseudoRegister *b, const ContextCreateNode *n, GrowableArray<PseudoRegister *> *expr_stack ); // for cloning

public:
    bool_t hasSrc() const {
        return _src not_eq nullptr;
    }


    bool_t canChangeDest() const {
        return false;
    }


    bool_t canBeEliminated() const {
        return false;
    }


    bool_t isContextCreateNode() const {
        return true;
    }


    bool_t canFail() const {
        return false;
    }


    PseudoRegister *context() const {
        return _dest;
    }


    int nofTemps() const {
        return _nofTemps;
    }


    int sizeOfContext() const {
        return _contextSize;
    }


    void set_sizeOfContext( int s ) {
        _contextSize = s;
    }


    int contextNo() const {
        return _contextNo;
    }


    void set_contextNo( int s ) {
        _contextNo = s;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void markAllocated( int *use_count, int *def_count );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aContextCreateNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend void node_init();

    friend class NodeFactory;
};


class ContextInitNode : public NonTrivialNode {
    // initializes contents of context; src is context (if _src == nullptr context was eliminated)
protected:
    GrowableArray<Expression *>           *_initializers;         // arguments/nil to be copied into context
    GrowableArray<Definition *>           *_contentDefs;          //
    GrowableArray<Usage *>                *_initializerUses;      //
    GrowableArray<BlockMaterializeNode *> *_materializedBlocks;   // blocks that must be materialized if this context is created

    ContextInitNode( ContextCreateNode *creator );

    ContextInitNode( PseudoRegister *b, const ContextInitNode *node );    // for cloning

public:
    bool_t hasSrc() const {
        return _src not_eq nullptr;
    }


    bool_t hasDest() const {
        return false;
    }


    bool_t isContextInitNode() const {
        return true;
    }


    bool_t canCopyPropagate() const {
        return true;
    }


    bool_t canBeEliminated() const {
        return false;
    }


    bool_t wasEliminated() const {
        return _src == nullptr;
    }


    GrowableArray<Expression *> *contents() const {
        return scope()->contextTemporaries();
    }


    int nofTemps() const {
        return _initializers->length();
    }


    Expression *contextTemp( int i ) const {
        return contents()->at( i );
    }


    Expression *initialValue( int i ) const {
        return _initializers->at( i );
    }


    void addBlockMaterializer( BlockMaterializeNode *n );

    ContextCreateNode *creator() const;

    void notifyNoContext();


    bool_t hasNoContext() const {
        return _src == nullptr;
    }


    void initialize( int no, Expression *expr );        // to copy something into the context right after creation
    int positionOfContextTemp( int i ) const;    // position of ith context temp in compiled context
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void markAllocated( int *use_count, int *def_count );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aContextInitNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend void node_init();

    friend class NodeFactory;
};


class ContextZapNode : public NonTrivialNode {

    // placeholder for context zap code; zapping is only needed if the node isActive().
private:
    ContextZapNode( PseudoRegister *context ) {
        _src = context;
        st_assert( _src, "src is nullptr" );
    }


public:
    bool_t isActive() const {
        return scope()->needsContextZapping();
    }


    bool_t isContextZapNode() const {
        return true;
    }


    bool_t hasSrc() const {
        return true;
    }


    bool_t shouldCopyWhenSplitting() const {
        return true;
    }


    PseudoRegister *context() const {
        return _src;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aContextZapNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class NonLocalReturnTestNode : public AbstractBranchNode {
    // placeholder for NonLocalReturn test code: tests if NonLocalReturn reached home scope, zaps context (if needed), then
    // returns from method (if home scope found) or continues the NonLocalReturn
    // next() is the "continue NonLocalReturn" branch, next1() the "found home" branch
protected:
    NonLocalReturnTestNode( int byteCodeIndex );

public:
    bool_t isNonLocalReturnTestNode() const {
        return true;
    }


    bool_t canChangeDest() const {
        return false;
    }


    bool_t canBeEliminated() const {
        return false;
    }


    bool_t canFail() const {
        return false;
    }


    void fixup();                // connect next() and next1() to sender scopes

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );


    // void eliminate(BasicBlock* bb, PseudoRegister* r, bool_t removing = false, bool_t cp = false);
    void markAllocated( int *use_count, int *def_count ) {
    };

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aNonLocalReturnTestNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class BranchNode : public AbstractBranchNode {
    // next() is the target of an untaken branch, next1() the taken
    // conditional branches expect CCs to be set by previous node (backend specific?)
    // usually after a node that sets CCs
protected:
    BranchOpCode _op;                       // branch untaken is likely
    bool_t       _taken_is_uncommon;        // true if branch taken is uncommon

    BranchNode( BranchOpCode op, bool_t taken_is_uncommon ) {
        _op                = op;
        _taken_is_uncommon = taken_is_uncommon;
    }


public:
    BranchOpCode op() const {
        return _op;
    }


    bool_t isBranchNode() const {
        return true;
    }


    bool_t canFail() const {
        return false;
    }


    void eliminateBranch( int op1, int op2, int res );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );


    void markAllocated( int *use_count, int *def_count ) {
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aBranchNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;


    void verify() {
        AbstractBranchNode::verify( false );
    }


    friend class NodeFactory;
};


class TypeTestNode : public AbstractBranchNode {
    // n-way map test; next(i) is the ith class [i=1..n], next() is the "otherwise" branch
    // _src is the register containing the receiver
protected:
    GrowableArray<KlassOop> *_classes;    // classes to test for
    bool_t _hasUnknown;                // can recv be anything? (if false, recv class
    // guaranteed to be in classes list)

    bool_t needsKlassLoad() const;        // does test need object's klass?
    TypeTestNode( PseudoRegister *r, GrowableArray<KlassOop> *classes, bool_t hasUnknown );

public:
    GrowableArray<KlassOop> *classes() const {
        return _classes;
    }


    bool_t hasUnknown() const {
        return _hasUnknown;
    }


    bool_t canFail() const {
        return _hasUnknown;
    }


    Node *failureBranch() const {
        return next();
    }


    void setUnknown( bool_t u ) {
        _hasUnknown = u;
    }


    bool_t isTypeTestNode() const {
        return true;
    }


    bool_t hasSrc() const {
        return true;
    }


    bool_t canCopyPropagate() const {
        return true;
    }


    bool_t canCopyPropagateOop() const {
        return true;
    }


    int cost() const {
        return 2 * oopSize * ( _classes->length() + needsKlassLoad() ? 1 : 0 );
    }


    Node *smiCase() const;            // the continuation for the smi_t case, nullptr if there is none

    bool_t doesTypeTests() const {
        return true;
    }


    bool_t hasUnknownCode() const;            // is there code (send) for unknown case?
    void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const;

    void assert_preg_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    void eliminate( BasicBlock *bb, PseudoRegister *r, ConstPseudoRegister *c, KlassOop m );

    void eliminateUnnecessary( KlassOop m );

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aTypeTestNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class AbstractArrayAtNode : public AbstractBranchNode {
    // array access: test index type & range, load element next() is the success case, next1() the failure
protected:
    // _src is the array, _dest the result
    PseudoRegister *_arg;          // index value
    Usage          *_argUse;       //
    PseudoRegister *_error;        // where to move the error string
    Definition     *_errorDef;     //
    bool_t _needBoundsCheck;        // need array bounds check?
    bool_t _intArg;                 // need not test for int if true
    int    _dataOffset;             // where start of array is (Oop offset)
    int    _sizeOffset;             // where size of array is (Oop offset)

    AbstractArrayAtNode( PseudoRegister *r, PseudoRegister *idx, bool_t ia, PseudoRegister *res, PseudoRegister *_err, int dataOffset, int sizeOffset ) {
        _src             = r;
        _arg             = idx;
        _intArg          = ia;
        _dest            = res;
        _error           = _err;
        _dataOffset      = dataOffset;
        _sizeOffset      = sizeOffset;
        _dontEliminate   = true;
        _needBoundsCheck = true;
    }


public:
    bool_t hasSrc() const {
        return true;
    }


    bool_t hasDest() const {
        return _dest not_eq nullptr;
    }


    bool_t canFail() const = 0;


    int cost() const {
        return 20 + ( _intArg ? 0 : 12 );
    } // fix this
    bool_t canCopyPropagate() const {
        return true;
    }


    int sizeOffset() const {
        return _sizeOffset;
    }


    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );


    bool_t doesTypeTests() const {
        return true;
    }


    bool_t needsBoundsCheck() const {
        return _needBoundsCheck;
    }


    bool_t hasUnknownCode() const;

    void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const;

    void assert_preg_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    void assert_in_bounds( PseudoRegister *r, LoopHeaderNode *n );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    friend class NodeFactory;
};


class ArrayAtNode : public AbstractArrayAtNode {

public:
    enum AccessType {
        byte_at,        // corresponds to primitiveIndexedByteAt:ifFail:
        double_byte_at, // corresponds to primitiveIndexedDoubleByteAt:ifFail:
        character_at,   // corresponds to primitiveIndexedDoubleByteCharacterAt:ifFail:
        object_at       // corresponds to primitiveIndexedObjectAt:ifFail:
    };

protected:
    AccessType _access_type;

    ArrayAtNode( AccessType access_type,    // specifies the operation
                 PseudoRegister *array,    // holds the array
                 PseudoRegister *index,    // holds the index
                 bool_t smiIndex,           // true if index is known to be a smi_t, false otherwise
                 PseudoRegister *result,   // where the result is stored
                 PseudoRegister *error,    // where the error symbol is stored if the operation fails
                 int data_offset,           // data offset in oops relative to array
                 int length_offset          // array length offset in oops relative to array
    );

public:
    AccessType access_type() const {
        return _access_type;
    }


    PseudoRegister *array() const {
        return _src;
    }


    PseudoRegister *index() const {
        return _arg;
    }


    PseudoRegister *error() const {
        return _error;
    }


    bool_t index_is_smi() const {
        return _intArg;
    }


    bool_t index_needs_bounds_check() const {
        return _needBoundsCheck;
    }


    bool_t canFail() const {
        return not index_is_smi() or index_needs_bounds_check();
    }


    int data_word_offset() const {
        return _dataOffset;
    }


    int size_word_offset() const {
        return _sizeOffset;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->anArrayAtNode( this );
    }


    friend class NodeFactory;
};


class AbstractArrayAtPutNode : public AbstractArrayAtNode {

protected:
    // array store: test index type & range, store element next() is the success case, next1() the failure
    PseudoRegister *elem;
    Usage          *elemUse;


    AbstractArrayAtPutNode( PseudoRegister *arr, PseudoRegister *idx, bool_t ia, PseudoRegister *el, PseudoRegister *res, PseudoRegister *_err, int doff, int soff ) :
            AbstractArrayAtNode( arr, idx, ia, res, _err, doff, soff ) {
        elem = el;
    }


public:
    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace = false );


    bool_t canCopyPropagateOop() const {
        return true;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    friend class NodeFactory;
};


class ArrayAtPutNode : public AbstractArrayAtPutNode {

public:
    enum AccessType {
        byte_at_put,            // corresponds to primitiveIndexedByteAt:put:ifFail:
        double_byte_at_put,     // corresponds to primitiveIndexedDoubleByteAt:put:ifFail:
        object_at_put           // corresponds to primitiveIndexedObjectAt:put:ifFail:
    };


    static bool_t stores_smi_elements( AccessType t ) {
        return t not_eq object_at_put;
    }


protected:
    AccessType _access_type;
    bool_t     _needs_store_check;
    bool_t     _smi_element;
    bool_t     _needs_element_range_check;

    ArrayAtPutNode( AccessType access_type,     // specifies the operation
                    PseudoRegister *array,     // holds the array
                    PseudoRegister *index,     // holds the index
                    bool_t smi_index,           // true if index is known to be a smi_t, false otherwise
                    PseudoRegister *element,   // holds the element
                    bool_t smi_element,         // true if element is known to be a smi_t, false otherwise
                    PseudoRegister *result,    // where the result is stored
                    PseudoRegister *error,     // where the error symbol is stored if the operation fails
                    int data_offset,            // data offset in oops relative to array
                    int length_offset,          // array length offset in oops relative to array
                    bool_t needs_store_check    // indicates whether a store check is necessary or not
    );

public:
    AccessType access_type() const {
        return _access_type;
    }


    PseudoRegister *array() const {
        return _src;
    }


    PseudoRegister *index() const {
        return _arg;
    }


    PseudoRegister *element() const {
        return elem;
    }


    PseudoRegister *error() const {
        return _error;
    }


    bool_t index_is_smi() const {
        return _intArg;
    }


    bool_t element_is_smi() const {
        return _smi_element;
    }


    bool_t index_needs_bounds_check() const {
        return _needBoundsCheck;
    }


    bool_t element_needs_range_check() const {
        return _needs_element_range_check;
    }


    bool_t canFail() const {
        return not index_is_smi() or index_needs_bounds_check() or ( access_type() not_eq object_at_put and ( not element_is_smi() or element_needs_range_check() ) );
    }


    bool_t needs_store_check() const {
        return _needs_store_check;
    }


    int data_word_offset() const {
        return _dataOffset;
    }


    int size_word_offset() const {
        return _sizeOffset;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const;

    void assert_preg_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->anArrayAtPutNode( this );
    }


    friend class NodeFactory;
};


class InlinedPrimitiveNode : public AbstractBranchNode {

public:
    enum class Operation {
        obj_klass,                  // corresponds to primitiveClass
        obj_hash,                   // corresponds to primitiveHash
        proxy_byte_at,              // corresponds to primitiveProxyByteAt:ifFail:
        proxy_byte_at_put,          // corresponds to primitiveProxyByteAt:put:ifFail:
    };

private:
    PseudoRegister *_arg1;         // 1st argument or nullptr
    PseudoRegister *_arg2;         // 2nd argument or nullptr
    PseudoRegister *_error;        // primitive error or nullptr if primitive can't fail
    Usage          *_arg1_use;     //
    Usage          *_arg2_use;     //
    Definition     *_error_def;    //
    bool_t    _arg1_is_smi;    // true if 1st argument is known to be a smi_t
    bool_t    _arg2_is_smi;    // true if 2nd argument is known to be a smi_t
    Operation _operation;      //
    // _src is	_recv;			    // receiver or nullptr

    InlinedPrimitiveNode( Operation op, PseudoRegister *result, PseudoRegister *error, PseudoRegister *recv, PseudoRegister *arg1, bool_t arg1_is_smi, PseudoRegister *arg2, bool_t arg2_is_smi );

public:
    Operation op() const {
        return _operation;
    }


    PseudoRegister *arg1() const {
        return _arg1;
    }


    PseudoRegister *arg2() const {
        return _arg2;
    }


    PseudoRegister *error() const {
        return _error;
    }


    bool_t arg1_is_smi() const {
        return _arg1_is_smi;
    }


    bool_t arg2_is_smi() const {
        return _arg2_is_smi;
    }


    bool_t hasSrc() const {
        return _src not_eq nullptr;
    }


    bool_t hasDest() const {
        return _dest not_eq nullptr;
    }


    bool_t canFail() const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( int *use_count, int *def_count );

    bool_t canBeEliminated() const;

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool_t removing = false, bool_t cp = false );


    bool_t canCopyPropagate() const {
        return false;
    }


    bool_t canCopyPropagateOop() const {
        return false;
    }


    bool_t copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool_t replace );

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->anInlinedPrimitiveNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class UncommonNode : public NonTrivialNode {
    GrowableArray<PseudoRegister *> *exprStack;

protected:
    UncommonNode( GrowableArray<PseudoRegister *> *e, int byteCodeIndex );

public:
    bool_t isUncommonNode() const {
        return true;
    }


    int cost() const {
        return 4;
    } // fix this

    bool_t isExitNode() const {
        return true;
    }


    bool_t isendsBasicBlock() const {
        return true;
    }


    virtual Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    void markAllocated( int *use_count, int *def_count ) {
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->anUncommonNode( this );
    }


    void verify() const;

    const char *print_string( const char *buf, bool_t printAddr = true ) const;


    GrowableArray<PseudoRegister *> *expressionStack() const {
        return exprStack;
    }


    friend class NodeFactory;
};


class UncommonSendNode : public UncommonNode {

private:
    int _argCount;

protected:
    UncommonSendNode( GrowableArray<PseudoRegister *> *e, int byteCodeIndex, int argCount = 0 );

public:
    int args() const {
        return _argCount;
    }


    bool_t isUncommonSendNode() const {
        return true;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void verify() const;

    void makeUses( BasicBlock *bb );

    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    friend class NodeFactory;
};


class FixedCodeNode : public TrivialNode {

public:
    enum class FixedCodeKind {
        dead_end,           // dead-end marker (for compiler debugging)
        inc_counter         // increment invocation counter
    };

protected:
    const FixedCodeKind _kind;


    FixedCodeNode( FixedCodeKind k ) :
            _kind( k ) {
    }


public:
    bool_t isExitNode() const {
        return _kind == FixedCodeKind::dead_end;
    }


    bool_t isDeadEndNode() const {
        return _kind == FixedCodeKind::dead_end;
    }


    FixedCodeKind kind() const {
        return _kind;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aFixedCodeNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class NopNode : public TrivialNode {
protected:
    NopNode() {
    }


public:
    bool_t isNopNode() const {
        return true;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    void apply( NodeVisitor *v ) {
        v->aNopNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};


class CommentNode : public TrivialNode {
protected:
    CommentNode( const char *s );

public:
    const char *comment;


    bool_t isCommentNode() const {
        return true;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    void apply( NodeVisitor *v ) {
        v->aCommentNode( this );
    }


    const char *print_string( const char *buf, bool_t printAddr = true ) const;

    friend class NodeFactory;
};
