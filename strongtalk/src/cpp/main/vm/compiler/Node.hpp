
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
class TypeTestNode;             // type tests (compare klasses, incl std::int32_t/float)
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
    std::int16_t          _id;                      // unique node id for debugging
    std::int16_t          _num;                     // node number within basic block
    std::int16_t          _byteCodeIndex;           // byteCodeIndex within the sc
    BasicBlock            *_basicBlock;            // basic block to which this instance belongs
    InlinedScope          *_scope;                 // scope to which this instance belongs
    PseudoRegisterMapping *_pseudoRegisterMapping; // the mapping at that node, if any (will be modified during code generation)

public:
    Label _label;         // for jumps to this node -- SHOULD BE MOVED TO BasicBlock -- fix this
    bool  _dontEliminate; // for special cases: must not eliminate this node
    bool  _deleted;       // node has been deleted

    std::int32_t id() const {
//        return this == nullptr ? -1 : _id;
        return _id;
    }


    BasicBlock *bb() const {
        return _basicBlock;
    }


    std::int32_t num() const {
        return _num;
    }


    InlinedScope *scope() const {
        return _scope;
    }


    std::int32_t byteCodeIndex() const {
        return _byteCodeIndex;
    }


    void setBasicBlock( BasicBlock *b ) {
        _basicBlock = b;
    }


    void setNum( std::int32_t n ) {
        _num = n;
    }


    void setScope( InlinedScope *s );

    static std::int32_t currentID;              // current node ID
    static std::int32_t currentCommentID;       // current ID for comment nodes
    static ScopeInfo    lastScopeInfo;          // for programCounterDescriptor generation
    static std::int32_t lastByteCodeIndex;      //

    BasicNode();


    virtual bool isPrologueNode() const {
        return false;
    }


    virtual bool isAssignNode() const {
        return false;
    }


    virtual bool isTArithNode() const {
        return false;
    }


    virtual bool isArithNode() const {
        return false;
    }


    virtual bool isNonLocalReturnSetupNode() const {
        return false;
    }


    virtual bool isNonLocalReturnTestNode() const {
        return false;
    }


    virtual bool isNonLocalReturnContinuationNode() const {
        return false;
    }


    virtual bool isReturnNode() const {
        return false;
    }


    virtual bool isInlinedReturnNode() const {
        return false;
    }


    virtual bool isLoopHeaderNode() const {
        return false;
    }


    virtual bool isExitNode() const {
        return false;
    }


    virtual bool isMergeNode() const {
        return false;
    }


    virtual bool isBranchNode() const {
        return false;
    }


    virtual bool isBlockCreateNode() const {
        return false;
    }


    virtual bool isContextCreateNode() const {
        return false;
    }


    virtual bool isContextInitNode() const {
        return false;
    }


    virtual bool isContextZapNode() const {
        return false;
    }


    virtual bool isCommentNode() const {
        return false;
    }


    virtual bool isSendNode() const {
        return false;
    }


    virtual bool isCallNode() const {
        return false;
    }


    virtual bool isStoreNode() const {
        return false;
    }


    virtual bool isDeadEndNode() const {
        return false;
    }


    virtual bool isTypeTestNode() const {
        return false;
    }


    virtual bool isUncommonNode() const {
        return false;
    }


    virtual bool isUncommonSendNode() const {
        return false;
    }


    virtual bool isNopNode() const {
        return false;
    }


    virtual bool isCmpNode() const {
        return false;
    }


    virtual bool isArraySizeLoad() const {
        return false;
    }


    virtual bool isAccessingFloats() const {
        return false;
    }


    virtual bool isTrivial() const = 0;

protected:
    virtual Node *clone( PseudoRegister *from, PseudoRegister *to ) const {
        static_cast<void>(from); // unused
        static_cast<void>(to); // unused
        SubclassResponsibility();
        return nullptr;
    }


    void genProgramCounterDescriptor();

public:
    // for splitting: rough estimate of Space cost of node (in bytes)
    virtual std::int32_t cost() const {
        return OOP_SIZE;
    }


    virtual bool hasDest() const {
        return false;
    }


    virtual bool canCopyPropagate() const {
        return false;
    }


    // canCopyPropagate: can node replace a use with copy-propagated PseudoRegister?
    // if true, must implement copyPropagate below
    bool canCopyPropagate( Node *fromNode ) const; // can copy-propagate from fromNode to receiver?
    virtual bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false ) = 0;


    virtual bool canCopyPropagateOop() const {
        return false;
    }


    // canCopyPropagateOop: can node replace a use with a copy-propagated Oop?
    // if true, must handle ConstPseudoRegisters; implies canCopyPropagate
    virtual bool isAssignmentLike() const {
        return false;
    }


    // isAssignmentLike: node copies src to dest (implies hasSrc/Dest)
    virtual bool shouldCopyWhenSplitting() const {
        return false;
    }


    virtual bool hasSrc() const {
        return false;
    }


    virtual bool hasConstantSrc() const {
        return false;
    }


    virtual Oop constantSrc() const {
        ShouldNotCallThis();
        return 0;
    }


    virtual bool canChangeDest() const {
        st_assert( hasDest(), "shouldn't call" );
        return true;
    }


    virtual bool endsBasicBlock() const = 0;


    virtual bool startsBasicBlock() const {
        return false;
    }


    std::int32_t loopDepth() const {
        return _basicBlock->loopDepth();
    }


    virtual BasicBlock *newBasicBlock();


    virtual void makeUses( BasicBlock *bb ) {
        static_cast<void>(bb); // unused
    }


    virtual void removeUses( BasicBlock *bb ) {
        static_cast<void>(bb); // unused
    }


    virtual void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false ) = 0;


    virtual bool canBeEliminated() const {
        return not _dontEliminate;
    }


    virtual void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *lst ) {
        static_cast<void>(lst); // unused
    }


    virtual void markAllocated( std::int32_t *use_count, std::int32_t *def_count ) = 0;

    virtual SimpleBitVector trashedMask();

    virtual void gen();


    virtual void apply( NodeVisitor *v ) {
        static_cast<void>(v); // unused
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
    virtual bool doesTypeTests() const {
        return false;
    }          // does node perform any type test?

    virtual bool hasUnknownCode() const {
        return false;
    }          // does handle unknown cases? (with real code, not uncommon branch)

    virtual void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const {
        static_cast<void>(regs); // unused
        static_cast<void>(klasses); // unused
        ShouldNotCallThis();
    }


    // return a list of pseudoRegisters tested and, for each pseudoRegister, a list of its types
    virtual void assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n ) {
        (void) r; // unused
        (void) klasses; // unused
        (void) n; // unused
    } // assert that the klass of r (used by the receiver) is oneof(klasses)

    virtual void assert_in_bounds( PseudoRegister *r, LoopHeaderNode *n ) {
        (void) r; // unused
        (void) n; // unused
    }// assert that r (used by the reciver) is within array bounds

    virtual void print_short();


    virtual void print() {
        print_short();
    }


    virtual const char *toString( char *buf, bool printAddress = true ) const = 0;

    void printID() const;


    virtual void verify() const {
    }

    // Mappings
    //
    // Note: make sure, mapping at node is always unchanged, once it is set.

    PseudoRegisterMapping *mapping() const;


    bool hasMapping() const {
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
    virtual bool endsBasicBlock() const;

    virtual Node *likelySuccessor() const;

    virtual Node *uncommonSuccessor() const;

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    virtual void verify() const;

    friend class NodeFactory;


    virtual bool hasSingleSuccessor() const {
        return true;
    }


    virtual bool hasSinglePredecessor() const {
        return true;
    }


    virtual std::int32_t nPredecessors() const {
        return _prev ? 1 : 0;
    }


    virtual std::int32_t nSuccessors() const {
        return _next ? 1 : 0;
    }


    virtual bool isPredecessor( const Node *n ) const {
        return _prev == n;
    }


    virtual bool isSuccessor( const Node *n ) const {
        return _next == n;
    }


    Node *next() const {
        return _next;
    }


    virtual Node *next1() const {
        return nullptr;
    }


    virtual Node *next( std::int32_t i ) const {
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


    virtual Node *prev( std::int32_t n ) const {
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
        static_cast<void>(n); // unused
        ShouldNotCallThis();
    }


    virtual void setNext( std::int32_t i, Node *n ) {
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


    Node *append( std::int32_t i, Node *p ) {
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
    bool isTrivial() const {
        return true;
    }


    std::int32_t cost() const {
        return 0;
    }


    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false ) {
        static_cast<void>(bb); // unused
        static_cast<void>(u); // unused
        static_cast<void>(d); // unused
        static_cast<void>(replace); // unused
        return false;
    }


    void markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
        static_cast<void>(use_count); // unused
        static_cast<void>(def_count); // unused
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
    bool isTrivial() const {
        return false;
    }


    PseudoRegister *src() const;

    PseudoRegister *dest() const;


    PseudoRegister *dst() const {
        return dest();
    }


    void setDest( BasicBlock *bb, PseudoRegister *d );

    virtual bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void verify() const;

    friend class NodeFactory;
};


class PrologueNode : public NonTrivialNode {

protected:

    LookupKey          *_key;
    const std::int32_t _nofArgs;
    const std::int32_t _nofTemps;


    PrologueNode( LookupKey *key, std::int32_t nofArgs, std::int32_t nofTemps ) :
        _nofArgs( nofArgs ), _nofTemps( nofTemps ) {
        _key = key;
    }


public:
    bool isPrologueNode() const {
        return true;
    }


    virtual bool canChangeDest() const {
        return false;
    }    // has no dest

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );


    void removeUses( BasicBlock *bb ) {
        static_cast<void>(bb); // unused
        ShouldNotCallThis();
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->aPrologueNode( this );
    }


    bool canBeEliminated() const {
        return false;
    }


    void markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
        static_cast<void>(use_count); // unused
        static_cast<void>(def_count); // unused
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class LoadNode : public NonTrivialNode {

protected:
    LoadNode( PseudoRegister *d ) {
        _dest = d;
        st_assert( d, "dest is nullptr" );
    }


public:
    bool hasDest() const {
        return true;
    }


    bool canCopyPropagate() const {
        return true;
    }


    bool canCopyPropagateOop() const {
        return true;
    }


    void makeUses( BasicBlock *basicBlock );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    friend class NodeFactory;
};


class LoadIntNode : public LoadNode {

protected:
    std::int32_t _value;        // constant (vm-level, not an Oop) to be loaded

    LoadIntNode( PseudoRegister *dst, std::int32_t value ) :
        LoadNode( dst ) {
        _value = value;
    }


public:
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    std::int32_t value() {
        return _value;
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->aLoadIntNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class LoadOffsetNode : public LoadNode {

public:
    // _src is base address (e.g. object containing a slot)
    std::int32_t _offset;          // offset in words
    bool         _isArraySize;     // is this load implementing an array size primitive?

protected:
    LoadOffsetNode( PseudoRegister *dst, PseudoRegister *src, std::int32_t offset, bool isArraySize ) :
        LoadNode( dst ) {
        _src         = src;
        _offset      = offset;
        _isArraySize = isArraySize;
    }


public:
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    PseudoRegister *base() const {
        return _src;
    }


    bool hasSrc() const {
        return true;
    }


    bool isArraySizeLoad() const {
        return _isArraySize;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aLoadOffsetNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class LoadUplevelNode : public LoadNode {

private:
    Usage          *_context0Use;   //
    PseudoRegister *_context0;      // starting context
    std::int32_t   _nofLevels;      // no. of indirections to follow via context home field
    std::int32_t   _offset;         // offset of temporary in final context
    SymbolOop      _name;           // temporary name (for printing)

protected:
    LoadUplevelNode( PseudoRegister *dst, PseudoRegister *context0, std::int32_t nofLevels, std::int32_t offset, SymbolOop name );

public:
    PseudoRegister *context0() const {
        return _context0;
    }


    std::int32_t nofLevels() const {
        return _nofLevels;
    }


    std::int32_t offset() const {
        return _offset;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aLoadUplevelNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class StoreNode : public NonTrivialNode {

protected:
    StoreNode( PseudoRegister *s ) {
        _src = s;
        st_assert( _src, "src is nullptr" );
    }


public:
    bool isStoreNode() const {
        return true;
    }


    bool canCopyPropagate() const {
        return true;
    }


    bool canCopyPropagateOop() const {
        return true;
    }


    bool hasSrc() const {
        return true;
    }


    virtual bool needsStoreCheck() const {
        return false;
    }


    virtual const char *action() const = 0;        // for debugging messages

    virtual void setStoreCheck( bool ncs ) {
        static_cast<void>(ncs); // unused

    }


    void assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );

    friend class NodeFactory;
};


class StoreOffsetNode : public StoreNode {
    // store into data slot, do check-store if necessary
    // _src is value being stored, may be a ConstPseudoRegister*
private:
    PseudoRegister *_base;              // base address (object containing the slot)
    Usage          *_baseUse;           //
    std::int32_t   _offset;             // offset in words
    bool           _needsStoreCheck;    // does store need a GC store check?

protected:
    StoreOffsetNode( PseudoRegister *s, PseudoRegister *b, std::int32_t o, bool nsc ) :
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


    std::int32_t offset() const {
        return _offset;
    }


    bool needsStoreCheck() const {
        return _needsStoreCheck;
    }


    bool hasSrc() const {
        return true;
    }


    void setStoreCheck( bool ncs ) {
        _needsStoreCheck = ncs;
    }


    const char *action() const {
        return "stored into an object";
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );


    bool canBeEliminated() const {
        return false;
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->aStoreOffsetNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class StoreUplevelNode : public StoreNode {
    // when node is created, it is not known whether store will be to stack/reg or context
    // _src may be a ConstPseudoRegister*
private:
    Usage          *_context0Use;       //
    PseudoRegister *_context0;          // starting context
    std::int32_t   _nofLevels;          // no. of indirections to follow via context home field
    std::int32_t   _offset;             // offset of temporary in final context
    bool           _needsStoreCheck;    // generate a store check if true
    SymbolOop      _name;               // temporary name (for printing)

protected:
    StoreUplevelNode( PseudoRegister *src, PseudoRegister *context0, std::int32_t nofLevels, std::int32_t offset, SymbolOop name, bool needsStoreCheck );

public:
    PseudoRegister *context0() const {
        return _context0;
    }


    std::int32_t nofLevels() const {
        return _nofLevels;
    }


    std::int32_t offset() const {
        return _offset;
    }


    bool needsStoreCheck() const {
        return _needsStoreCheck;
    }


    void setStoreCheck( bool ncs ) {
        _needsStoreCheck = ncs;
    }


    const char *action() const {
        return "stored into a context temporary";
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aStoreUplevelNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class AssignNode : public StoreNode {
    // _src may be a ConstPseudoRegister*
protected:
    AssignNode( PseudoRegister *s, PseudoRegister *d );

public:
    std::int32_t cost() const {
        return OOP_SIZE / 2;
    }  // assume 50% eliminated
    bool isAccessingFloats() const;


    bool isAssignNode() const {
        return true;
    }


    bool hasDest() const {
        return true;
    }


    bool hasSrc() const {
        return true;
    }


    bool hasConstantSrc() const {
        return _src->isConstPseudoRegister();
    }


    bool isAssignmentLike() const {
        return true;
    }


    bool shouldCopyWhenSplitting() const {
        return true;
    }


    bool canBeEliminated() const;

    Oop constantSrc() const;


    const char *action() const {
        return _dest->isSinglyAssignedPseudoRegister() ? "passed as an argument" : "assigned to a local";
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->anAssignNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

protected:
    void genOop();

    friend class NodeFactory;
};


class AbstractReturnNode : public NonTrivialNode {

protected:
    AbstractReturnNode( std::int32_t byteCodeIndex, PseudoRegister *src, PseudoRegister *dest ) {
        _byteCodeIndex = byteCodeIndex;
        _src           = src;
        _dest          = dest;
    }


public:
    bool canBeEliminated() const {
        return false;
    }


    bool isReturnNode() const {
        return true;
    }


    bool endsBasicBlock() const {
        return true;
    }


    bool canCopyPropagate() const {
        return true;
    }


    bool canCopyPropagateOop() const {
        return true;
    }


    bool hasSrc() const {
        return true;
    }


    bool isAssignmentLike() const {
        return true;
    }


    bool hasDest() const {
        return true;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );


    void markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
        static_cast<void>(use_count); // unused
        static_cast<void>(def_count); // unused
    }


    friend class NodeFactory;
};


class InlinedReturnNode : public AbstractReturnNode {

    // should replace with AssignNode + ContextZap (if needed)
protected:
    InlinedReturnNode( std::int32_t byteCodeIndex, PseudoRegister *src, PseudoRegister *dest ) :
        AbstractReturnNode( byteCodeIndex, src, dest ) {
    }


public:
    bool isInlinedReturnNode() const {
        return true;
    }


    bool endsBasicBlock() const {
        return NonTrivialNode::endsBasicBlock();
    }


    bool isTrivial() const {
        return true;
    }


    bool canBeEliminated() const {
        return true;
    }


    bool shouldCopyWhenSplitting() const {
        return true;
    }


    std::int32_t cost() const {
        return 0;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->anInlinedReturnNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

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
    NonLocalReturnSetupNode( PseudoRegister *result, std::int32_t byteCodeIndex );

public:
    bool isExitNode() const {
        return true;
    }


    bool isNonLocalReturnSetupNode() const {
        return true;
    }


    // uses hardwired regs, has no src or dest
    bool canCopyPropagate() const {
        return false;
    }


    bool canCopyPropagateOop() const {
        return false;
    }


    bool hasSrc() const {
        return true;
    }  // otherwise breaks nonlocal_return
    bool isAssignmentLike() const {
        return false;
    }


    bool hasDest() const {
        return false;
    }


    bool canChangeDest() const {
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

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class NonLocalReturnContinuationNode : public AbstractReturnNode {
protected:
    NonLocalReturnContinuationNode( std::int32_t byteCodeIndex, PseudoRegister *src, PseudoRegister *dest ) :
        AbstractReturnNode( byteCodeIndex, src, dest ) {
    }


public:
    bool isExitNode() const {
        return true;
    }


    bool isNonLocalReturnContinuationNode() const {
        return true;
    }


    // uses hardwired regs, has no src or dest
    bool canCopyPropagate() const {
        return false;
    }


    bool canCopyPropagateOop() const {
        return false;
    }


    bool hasSrc() const {
        return false;
    }


    bool isAssignmentLike() const {
        return false;
    }


    bool hasDest() const {
        return false;
    }


    bool canChangeDest() const {
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

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


// normal method return
class ReturnNode : public AbstractReturnNode {
private:
    Usage *_resultUse;

protected:
    ReturnNode( PseudoRegister *res, std::int32_t byteCodeIndex );

public:
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    bool isExitNode() const {
        return true;
    }


    bool hasSrc() const {
        return false;
    }


    bool isAssignmentLike() const {
        return false;
    }


    bool hasDest() const {
        return false;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aReturnNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

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


    bool hasSinglePredecessor() const {
        return _prevs->length() <= 1;
    }


    std::int32_t nPredecessors() const {
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


    Node *prev( std::int32_t n ) const {
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

    bool isPredecessor( const Node *n ) const;
};


class MergeNode : public AbstractMergeNode {

protected:
    MergeNode( std::int32_t byteCodeIndex );

    MergeNode( Node *prev1, Node *prev2 );

public:
    bool _isLoopStart;        // does this node start a loop?
    bool _isLoopEnd;          // does this node end a loop? (i.e., first node after loop)
    bool _didStartBasicBlock; // used for debugging / assertion checks

    std::int32_t cost() const {
        return 0;
    }


    bool isTrivial() const {
        return _prevs->length() <= 1;
    }


    bool startsBasicBlock() const {
        return _prevs->length() > 1 or _isLoopStart;
    }


    bool isMergeNode() const {
        return true;
    }


    BasicBlock *newBasicBlock();

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aMergeNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class ArithNode : public NonTrivialNode {    // abstract
    // NB: ArithNodes are not used for tagged std::int32_t arithmetic -- see TArithNode
protected:
    ArithOpCode         _op;
    ConstPseudoRegister *_constResult;    // non-nullptr if constant-folded

    ArithNode( ArithOpCode op, PseudoRegister *src, PseudoRegister *dst ) {
        _op          = op;
        _src         = src;
        _dest        = dst;
        _constResult = nullptr;
    }


public:
    bool canCopyPropagate() const {
        return true;
    }


    bool canCopyPropagateOop() const {
        return true;
    }


    bool hasSrc() const {
        return true;
    }


    bool hasDest() const {
        return true;
    }


    bool isAssignmentLike() const {
        return _constResult not_eq nullptr;
    }


    bool isArithNode() const {
        return true;
    }


    bool isCmpNode() const {
        return _op == ArithOpCode::tCmpArithOp;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );


    ArithOpCode op() const {
        return _op;
    }


    virtual bool operIsConst() const = 0;

    virtual std::int32_t operConst() const = 0;

    virtual bool doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool repl );

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

    bool operIsConst() const;

    std::int32_t operConst() const;

    bool doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->anArithRRNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class FloatArithRRNode : public ArithRRNode {  // for untagged float operations

    FloatArithRRNode( ArithOpCode o, PseudoRegister *s, PseudoRegister *o2, PseudoRegister *d ) :
        ArithRRNode( o, s, o2, d ) {
    }


public:
    bool isAccessingFloats() const {
        return true;
    }


    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aFloatArithRRNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class FloatUnaryArithNode : public ArithNode {
    // unary untagged float operation; src is an untagged float, dest is either another
    // untagged float or a floatOop
    FloatUnaryArithNode( ArithOpCode op, PseudoRegister *src, PseudoRegister *dst ) :
        ArithNode( op, src, dst ) {
    }


public:
    bool isAccessingFloats() const {
        return true;
    }


    bool isCmpNode() const {
        return false;
    }


    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );


    bool operIsConst() const {
        return false;
    }


    std::int32_t operConst() const {
        ShouldNotCallThis();
        return 0;
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->aFloatUnaryArithNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class ArithRCNode : public ArithNode {  // reg op const => reg
    // used to compare against non-Oop constants (e.g. for markOop test)
    // DO NOT USE to add a reg and an Oop constant -- use ArithRR + ConstPseudoRegisters for that
protected:
    std::int32_t _oper;


    ArithRCNode( ArithOpCode o, PseudoRegister *s, std::int32_t o2, PseudoRegister *d ) :
        ArithNode( o, s, d ) {
        _oper = o2;
    }


public:
    std::int32_t operand() const {
        return _oper;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->anArithRCNode( this );
    }


    bool operIsConst() const {
        return true;
    }


    std::int32_t operConst() const {
        return _oper;
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};

// node with >1 successor; supplies linking code (next(i) et al.) and some default behavior
class AbstractBranchNode : public NonTrivialNode {
    // next() is the fall-through case, next1() the taken branch
public:
    virtual bool canFail() const = 0;        // does node have a failure branch?
    virtual Node *failureBranch() const {
        return next1();
    }


    bool endsBasicBlock() const {
        return true;
    }


protected:
    AbstractBranchNode() {
        _nxt = new GrowableArray<Node *>( EstimatedSuccessors );
    }


    void removeFailureIfPossible();

    void verify( bool verifySuccessors ) const;

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


    bool hasSingleSuccessor() const {
        return next1() == nullptr;
    }


    std::int32_t nSuccessors() const {
        return _nxt->length() + ( _next ? 1 : 0 );
    }


    Node *next() const {
        return _next;
    }


    Node *next( std::int32_t i ) const {
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


    void setNext( std::int32_t i, Node *n );

    void moveNext( Node *from, Node *to );

    bool isSuccessor( const Node *n ) const;
};

class TArithRRNode : public AbstractBranchNode {
    // tagged arithmetic operation; next() is success case, next1()
    // is failure (leaving ORed operands in Temp1 for tag test)
protected:
    ArithOpCode         _op;
    PseudoRegister      *_oper;
    Usage               *_operUse;
    bool                _arg1IsInt;            // is _src a smi_t?
    bool                _arg2IsInt;            // is _oper a smi_t?
    ConstPseudoRegister *_constResult;            // non-nullptr if constant-folded

    TArithRRNode( ArithOpCode o, PseudoRegister *s, PseudoRegister *o2, PseudoRegister *d, bool a1, bool a2 );

public:
    ArithOpCode op() const {
        return _op;
    }


    PseudoRegister *operand() const {
        return _oper;
    }


    bool arg1IsInt() const {
        return _arg1IsInt;
    }


    bool arg2IsInt() const {
        return _arg2IsInt;
    }


    bool canFail() const {
        return not( _arg1IsInt and _arg2IsInt );
    }


    bool isTArithNode() const {
        return true;
    }


    bool isAssignmentLike() const {
        return _constResult not_eq nullptr;
    }


    bool canCopyPropagate() const {
        return true;
    }


    bool canCopyPropagateOop() const {
        return true;
    }


    bool hasSrc() const {
        return true;
    }


    bool hasDest() const {
        return true;
    }


    bool doesTypeTests() const {
        return true;
    }


    bool hasUnknownCode() const;

    void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const;

    void assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aTArithRRNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

protected:
    bool doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

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
    std::int32_t                    nblocks;       // number of possibly live blocks at this point (for uplevel access computation)

    bool hasDest() const {
        return true;
    }


    bool isCallNode() const {
        return true;
    }


    bool canChangeDest() const {
        return false;
    }


    bool canBeEliminated() const {
        return false;
    }


    virtual bool canInvokeDelta() const = 0;    // can call invoke Delta code?
    MergeNode *nlrTestPoint() const;

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    SimpleBitVector trashedMask();

    void nlrCode();            // generate NonLocalReturn code sequence
    void verify() const;

    friend class NodeFactory;
};


class SendNode : public CallNode {

protected:
    LookupKey *_key;      // lookup key (for selector)
    bool      _superSend;  // is it a super send?
    SendInfo  *_info;     // to set CompiledInlineCache flags (counting, uninlinable, etc.)

    SendNode( LookupKey *key, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *exprStk, bool superSend, SendInfo *info );

public:
    bool isSendNode() const {
        return true;
    }


    bool isSuperSend() const {
        return _superSend;
    }


    bool isCounting() const;

    bool isUninlinable() const;

    bool staticReceiver() const;


    bool canInvokeDelta() const {
        return true;
    }


    bool canFail() const {
        return false;
    }


    std::int32_t cost() const {
        return OOP_SIZE * 5;
    }      // include InlineCache + some param pushing
    PseudoRegister *recv() const;

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aSendNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class PrimitiveNode : public CallNode {

protected:
    PrimitiveDescriptor *_pdesc;

    PrimitiveNode( PrimitiveDescriptor *pdesc, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack );

public:
    bool canBeEliminated() const;

    bool canInvokeDelta() const;

    bool canFail() const;


    PrimitiveDescriptor *pdesc() const {
        return _pdesc;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aPrimitiveNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class DLLNode : public CallNode {
protected:
    SymbolOop      _dll_name;
    SymbolOop      _function_name;
    dll_func_ptr_t _function;
    bool           _async;

    DLLNode( SymbolOop dll_name, SymbolOop function_name, dll_func_ptr_t function, bool async, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack );

public:
    bool canInvokeDelta() const;


    bool canFail() const {
        return true;
    }


    std::int32_t nofArguments() const {
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


    bool async() const {
        return _async;
    }


    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll );

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aDLLNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

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
        static_cast<void>(v); // unused
        Unimplemented();
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend void node_init();

    friend class NodeFactory;
};


class HoistedTypeTest;

class LoopseudoRegisterCandidate;


//
// a LoopHeaderNode is inserted before every loop; usually it is a no-op for optimized integer loops, it does the pre-loop type tests
//
class LoopHeaderNode : public TrivialNode {

protected:
    // info for integer loops
    bool                                 _integerLoop;                    // integer loop? (if no: inst. vars below are not set)
    PseudoRegister                       *_loopVar;              // loop variable
    PseudoRegister                       *_lowerBound;           // lower bound
    PseudoRegister                       *_upperBound;           // upper bound (or nullptr; mutually exclusive with boundArray)
    LoadOffsetNode                       *_upperLoad;            // loads array size that is the upper bound
    GrowableArray<AbstractArrayAtNode *> *_arrayAccesses;     // arrays indexed by loopVar

    LoopHeaderNode                              *_enclosingLoop;      // enclosing loop or nullptr
    // info for generic loops; all instance variables below this line are valid only after the loop optimization pass!
    GrowableArray<HoistedTypeTest *>            *_tests;              // type tests hoisted out of loop
    GrowableArray<LoopHeaderNode *>             *_nestedLoops;        // nested loops (nullptr if none)
    GrowableArray<LoopseudoRegisterCandidate *> *_registerCandidates; // candidates for reg. allocation within loop (best comes first); nullptr if none
    bool                                        _activated;            // gen() does nothing until activated
    std::int32_t                                _nofCalls;             // number of non-inlined calls in loop (excluding unlikely code)

    LoopHeaderNode();

public:
    bool isLoopHeaderNode() const {
        return true;
    }


    bool isActivated() const {
        return _activated;
    }


    bool isInnerLoop() const {
        return _nestedLoops == nullptr;
    }


    bool isIntegerLoop() const {
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
        static_cast<void>(from); // unused
        static_cast<void>(to); // unused
        ShouldNotCallThis();
        return nullptr;
    }


    std::int32_t nofCallsInLoop() const {
        return _nofCalls;
    }


    void set_nofCallsInLoop( std::int32_t n ) {
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


    GrowableArray<LoopseudoRegisterCandidate *> *registerCandidates() const {
        return _registerCandidates;
    }


    void addRegisterCandidate( LoopseudoRegisterCandidate *c );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aLoopHeaderNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

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

    bool isBlockCreateNode() const {
        return true;
    }


    // block creation nodes are like assignments if memoized, so don't end BBs here
    bool endsBasicBlock() const {
        return not isMemoized() or NonTrivialNode::endsBasicBlock();
    }


    BlockPseudoRegister *block() const {
        st_assert( _dest->isBlockPseudoRegister(), "must be BlockPseudoRegister" );
        return (BlockPseudoRegister *) _dest;
    }


    bool isMemoized() const {
        return block()->isMemoized();
    }


    bool hasConstantSrc() const {
        return false;
    }


    bool hasSrc() const {
        return false;
    }


    std::int32_t cost() const {
        return 2 * OOP_SIZE;
    }    // hope it's memoized

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    bool canBeEliminated() const {
        return not _dontEliminate;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aBlockCreateNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

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
    bool endsBasicBlock() const {
        return true;
    }


    bool hasSrc() const {
        return true;
    }


    std::int32_t cost() const {
        return 5 * OOP_SIZE;
    } // assume blk is memoized

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aBlockMaterializeNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class ContextCreateNode : public PrimitiveNode {
    // src is parent context, dest is register holding created context
protected:
    std::int32_t                    _nofTemps;             // no. of temps in context
    std::int32_t                    _contextSize;          // size of compiled context
    std::int32_t                    _contextNo;            // context number (index into compiler's contextList)
    GrowableArray<PseudoRegister *> *_parentContexts;     // context chain above parent context (if any)
    GrowableArray<Usage *>          *_parentContextUses;  // for _parentContexts

    ContextCreateNode( PseudoRegister *parent, PseudoRegister *context, std::int32_t nofTemps, GrowableArray<PseudoRegister *> *expr_stack );

    ContextCreateNode( PseudoRegister *b, const ContextCreateNode *n, GrowableArray<PseudoRegister *> *expr_stack ); // for cloning

public:
    bool hasSrc() const {
        return _src not_eq nullptr;
    }


    bool canChangeDest() const {
        return false;
    }


    bool canBeEliminated() const {
        return false;
    }


    bool isContextCreateNode() const {
        return true;
    }


    bool canFail() const {
        return false;
    }


    PseudoRegister *context() const {
        return _dest;
    }


    std::int32_t nofTemps() const {
        return _nofTemps;
    }


    std::int32_t sizeOfContext() const {
        return _contextSize;
    }


    void set_sizeOfContext( std::int32_t s ) {
        _contextSize = s;
    }


    std::int32_t contextNo() const {
        return _contextNo;
    }


    void set_contextNo( std::int32_t s ) {
        _contextNo = s;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aContextCreateNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

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
    bool hasSrc() const {
        return _src not_eq nullptr;
    }


    bool hasDest() const {
        return false;
    }


    bool isContextInitNode() const {
        return true;
    }


    bool canCopyPropagate() const {
        return true;
    }


    bool canBeEliminated() const {
        return false;
    }


    bool wasEliminated() const {
        return _src == nullptr;
    }


    GrowableArray<Expression *> *contents() const {
        return scope()->contextTemporaries();
    }


    std::int32_t nofTemps() const {
        return _initializers->length();
    }


    Expression *contextTemp( std::int32_t i ) const {
        return contents()->at( i );
    }


    Expression *initialValue( std::int32_t i ) const {
        return _initializers->at( i );
    }


    void addBlockMaterializer( BlockMaterializeNode *n );

    ContextCreateNode *creator() const;

    void notifyNoContext();


    bool hasNoContext() const {
        return _src == nullptr;
    }


    void initialize( std::int32_t no, Expression *expr );        // to copy something into the context right after creation
    std::int32_t positionOfContextTemp( std::int32_t i ) const;    // position of ith context temp in compiled context
    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aContextInitNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

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
    bool isActive() const {
        return scope()->needsContextZapping();
    }


    bool isContextZapNode() const {
        return true;
    }


    bool hasSrc() const {
        return true;
    }


    bool shouldCopyWhenSplitting() const {
        return true;
    }


    PseudoRegister *context() const {
        return _src;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void gen();


    void apply( NodeVisitor *v ) {
        v->aContextZapNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class NonLocalReturnTestNode : public AbstractBranchNode {
    // placeholder for NonLocalReturn test code: tests if NonLocalReturn reached home scope, zaps context (if needed), then
    // returns from method (if home scope found) or continues the NonLocalReturn
    // next() is the "continue NonLocalReturn" branch, next1() the "found home" branch
protected:
    NonLocalReturnTestNode( std::int32_t byteCodeIndex );

public:
    bool isNonLocalReturnTestNode() const {
        return true;
    }


    bool canChangeDest() const {
        return false;
    }


    bool canBeEliminated() const {
        return false;
    }


    bool canFail() const {
        return false;
    }


    void fixup();                // connect next() and next1() to sender scopes

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );


    // void eliminate(BasicBlock* bb, PseudoRegister* r, bool removing = false, bool cp = false);
    void markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
        (void) use_count; // unused
        (void) def_count; // unused
    };

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aNonLocalReturnTestNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class BranchNode : public AbstractBranchNode {
    // next() is the target of an untaken branch, next1() the taken
    // conditional branches expect CCs to be set by previous node (backend specific?)
    // usually after a node that sets CCs
protected:
    BranchOpCode _op;                       // branch untaken is likely
    bool         _taken_is_uncommon;        // true if branch taken is uncommon

    BranchNode( BranchOpCode op, bool taken_is_uncommon ) {
        _op                = op;
        _taken_is_uncommon = taken_is_uncommon;
    }


public:
    BranchOpCode op() const {
        return _op;
    }


    bool isBranchNode() const {
        return true;
    }


    bool canFail() const {
        return false;
    }


    void eliminateBranch( std::int32_t op1, std::int32_t op2, std::int32_t res );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );


    void markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
        static_cast<void>(use_count); // unused
        static_cast<void>(def_count); // unused
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aBranchNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;


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
    bool                    _hasUnknown;                // can recv be anything? (if false, recv class
    // guaranteed to be in classes list)

    bool needsKlassLoad() const;        // does test need object's klass?
    TypeTestNode( PseudoRegister *r, GrowableArray<KlassOop> *classes, bool hasUnknown );

public:
    GrowableArray<KlassOop> *classes() const {
        return _classes;
    }


    bool hasUnknown() const {
        return _hasUnknown;
    }


    bool canFail() const {
        return _hasUnknown;
    }


    Node *failureBranch() const {
        return next();
    }


    void setUnknown( bool u ) {
        _hasUnknown = u;
    }


    bool isTypeTestNode() const {
        return true;
    }


    bool hasSrc() const {
        return true;
    }


    bool canCopyPropagate() const {
        return true;
    }


    bool canCopyPropagateOop() const {
        return true;
    }


    std::int32_t cost() const {
        return 2 * OOP_SIZE * ( _classes->length() + needsKlassLoad() ? 1 : 0 );
    }


    Node *smiCase() const;            // the continuation for the smi_t case, nullptr if there is none

    bool doesTypeTests() const {
        return true;
    }


    bool hasUnknownCode() const;            // is there code (send) for unknown case?
    void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const;

    void assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

    void eliminate( BasicBlock *bb, PseudoRegister *r, ConstPseudoRegister *c, KlassOop m );

    void eliminateUnnecessary( KlassOop m );

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->aTypeTestNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

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
    bool           _needBoundsCheck;        // need array bounds check?
    bool           _intArg;                 // need not test for std::int32_t if true
    std::int32_t   _dataOffset;             // where start of array is (Oop offset)
    std::int32_t   _sizeOffset;             // where size of array is (Oop offset)

    AbstractArrayAtNode( PseudoRegister *r, PseudoRegister *idx, bool ia, PseudoRegister *res, PseudoRegister *_err, std::int32_t dataOffset, std::int32_t sizeOffset ) {
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
    bool hasSrc() const {
        return true;
    }


    bool hasDest() const {
        return _dest not_eq nullptr;
    }


    bool canFail() const = 0;


    std::int32_t cost() const {
        return 20 + ( _intArg ? 0 : 12 );
    } // fix this
    bool canCopyPropagate() const {
        return true;
    }


    std::int32_t sizeOffset() const {
        return _sizeOffset;
    }


    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );


    bool doesTypeTests() const {
        return true;
    }


    bool needsBoundsCheck() const {
        return _needBoundsCheck;
    }


    bool hasUnknownCode() const;

    void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const;

    void assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    void assert_in_bounds( PseudoRegister *r, LoopHeaderNode *n );

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );

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
                 bool smiIndex,           // true if index is known to be a smi_t, false otherwise
                 PseudoRegister *result,   // where the result is stored
                 PseudoRegister *error,    // where the error symbol is stored if the operation fails
                 std::int32_t data_offset,           // data offset in oops relative to array
                 std::int32_t length_offset          // array length offset in oops relative to array
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


    bool index_is_smi() const {
        return _intArg;
    }


    bool index_needs_bounds_check() const {
        return _needBoundsCheck;
    }


    bool canFail() const {
        return not index_is_smi() or index_needs_bounds_check();
    }


    std::int32_t data_word_offset() const {
        return _dataOffset;
    }


    std::int32_t size_word_offset() const {
        return _sizeOffset;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    const char *toString( char *buf, bool printAddress = true ) const;

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


    AbstractArrayAtPutNode( PseudoRegister *arr, PseudoRegister *idx, bool ia, PseudoRegister *el, PseudoRegister *res, PseudoRegister *_err, std::int32_t doff, std::int32_t soff ) :
        AbstractArrayAtNode( arr, idx, ia, res, _err, doff, soff ) {
        elem = el;
    }


public:
    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace = false );


    bool canCopyPropagateOop() const {
        return true;
    }


    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    friend class NodeFactory;
};


class ArrayAtPutNode : public AbstractArrayAtPutNode {

public:
    enum AccessType {
        byte_at_put,            // corresponds to primitiveIndexedByteAt:put:ifFail:
        double_byte_at_put,     // corresponds to primitiveIndexedDoubleByteAt:put:ifFail:
        object_at_put           // corresponds to primitiveIndexedObjectAt:put:ifFail:
    };


    static bool stores_smi_elements( AccessType t ) {
        return t not_eq object_at_put;
    }


protected:
    AccessType _access_type;
    bool       _needs_store_check;
    bool       _smi_element;
    bool       _needs_element_range_check;

    ArrayAtPutNode( AccessType access_type,     // specifies the operation
                    PseudoRegister *array,     // holds the array
                    PseudoRegister *index,     // holds the index
                    bool smi_index,           // true if index is known to be a smi_t, false otherwise
                    PseudoRegister *element,   // holds the element
                    bool smi_element,         // true if element is known to be a smi_t, false otherwise
                    PseudoRegister *result,    // where the result is stored
                    PseudoRegister *error,     // where the error symbol is stored if the operation fails
                    std::int32_t data_offset,            // data offset in oops relative to array
                    std::int32_t length_offset,          // array length offset in oops relative to array
                    bool needs_store_check    // indicates whether a store check is necessary or not
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


    bool index_is_smi() const {
        return _intArg;
    }


    bool element_is_smi() const {
        return _smi_element;
    }


    bool index_needs_bounds_check() const {
        return _needBoundsCheck;
    }


    bool element_needs_range_check() const {
        return _needs_element_range_check;
    }


    bool canFail() const {
        return not index_is_smi() or index_needs_bounds_check() or ( access_type() not_eq object_at_put and ( not element_is_smi() or element_needs_range_check() ) );
    }


    bool needs_store_check() const {
        return _needs_store_check;
    }


    std::int32_t data_word_offset() const {
        return _dataOffset;
    }


    std::int32_t size_word_offset() const {
        return _sizeOffset;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *> &klasses ) const;

    void assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n );

    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );

    const char *toString( char *buf, bool printAddress = true ) const;

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
    bool           _arg1_is_smi;    // true if 1st argument is known to be a smi_t
    bool           _arg2_is_smi;    // true if 2nd argument is known to be a smi_t
    Operation      _operation;      //
    // _src is	_recv;			    // receiver or nullptr

    InlinedPrimitiveNode( Operation op, PseudoRegister *result, PseudoRegister *error, PseudoRegister *recv, PseudoRegister *arg1, bool arg1_is_smi, PseudoRegister *arg2, bool arg2_is_smi );

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


    bool arg1_is_smi() const {
        return _arg1_is_smi;
    }


    bool arg2_is_smi() const {
        return _arg2_is_smi;
    }


    bool hasSrc() const {
        return _src not_eq nullptr;
    }


    bool hasDest() const {
        return _dest not_eq nullptr;
    }


    bool canFail() const;

    void makeUses( BasicBlock *bb );

    void removeUses( BasicBlock *bb );

    void markAllocated( std::int32_t *use_count, std::int32_t *def_count );

    bool canBeEliminated() const;

    void eliminate( BasicBlock *bb, PseudoRegister *r, bool removing = false, bool cp = false );


    bool canCopyPropagate() const {
        return false;
    }


    bool canCopyPropagateOop() const {
        return false;
    }


    bool copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace );

    Node *likelySuccessor() const;

    Node *uncommonSuccessor() const;

    void gen();


    void apply( NodeVisitor *v ) {
        v->anInlinedPrimitiveNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class UncommonNode : public NonTrivialNode {
    GrowableArray<PseudoRegister *> *exprStack;

protected:
    UncommonNode( GrowableArray<PseudoRegister *> *e, std::int32_t byteCodeIndex );

public:
    bool isUncommonNode() const {
        return true;
    }


    std::int32_t cost() const {
        return 4;
    } // fix this

    bool isExitNode() const {
        return true;
    }


    bool isendsBasicBlock() const {
        return true;
    }


    virtual Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    void markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
        static_cast<void>(use_count); // unused
        static_cast<void>(def_count); // unused
    }


    void gen();


    void apply( NodeVisitor *v ) {
        v->anUncommonNode( this );
    }


    void verify() const;

    const char *toString( char *buf, bool printAddress = true ) const;


    GrowableArray<PseudoRegister *> *expressionStack() const {
        return exprStack;
    }


    friend class NodeFactory;
};


class UncommonSendNode : public UncommonNode {

private:
    std::int32_t _argCount;

protected:
    UncommonSendNode( GrowableArray<PseudoRegister *> *e, std::int32_t byteCodeIndex, std::int32_t argCount = 0 );

public:
    std::int32_t args() const {
        return _argCount;
    }


    bool isUncommonSendNode() const {
        return true;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;

    void verify() const;

    void makeUses( BasicBlock *bb );

    const char *toString( char *buf, bool printAddress = true ) const;

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
    bool isExitNode() const {
        return _kind == FixedCodeKind::dead_end;
    }


    bool isDeadEndNode() const {
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


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class NopNode : public TrivialNode {
protected:
    NopNode() {
    }


public:
    bool isNopNode() const {
        return true;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    void apply( NodeVisitor *v ) {
        v->aNopNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};


class CommentNode : public TrivialNode {
protected:
    CommentNode( const char *s );

public:
    const char *_comment;


    const char *comment() {
        return _comment;
    }


    bool isCommentNode() const {
        return true;
    }


    Node *clone( PseudoRegister *from, PseudoRegister *to ) const;


    void apply( NodeVisitor *v ) {
        v->aCommentNode( this );
    }


    const char *toString( char *buf, bool printAddress = true ) const;

    friend class NodeFactory;
};
