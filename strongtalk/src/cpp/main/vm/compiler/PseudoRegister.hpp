//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/LogicalAddress.hpp"
#include "vm/compiler/slist.hpp"
#include "vm/compiler/CompileTimeClosure.hpp"
#include "vm/compiler/defUse.hpp"
#include "vm/utilities/lprintf.hpp"
#include "BasicBlock.hpp"
#include "vm/runtime/ResourceObject.hpp"


// All intermediate language (IL) operations work on pseudo registers (PseudoRegisters).
// In contrast to real registers, the number of available pseudo registers is unlimited.

// PseudoRegister are standard pseudo regs; the different kinds of PseudoRegisters differ mainly in their purpose and in their live ranges
// PseudoRegister: locals etc., multiply assigned, live range is the entire scope
// SinglyAssignedPseudoRegister: single source-level assignment, live range = arbitrary subrange
// SplitPReg: used for splitting
// BlockPseudoRegisters: for blocks, live range = creating byteCodeIndex .. end
// TempPseudoRegisters: local to one BasicBlock, used for certain hard-coded idioms (e.g. loading an uplevel-accessed value)

class RegisterEqClass;

class DefinitionUsageInfo;

class CopyPropagationInfo;

class BlockPseudoRegister;


class PseudoRegister : public PrintableResourceObject {

protected:
    std::int16_t _id;               // unique id
    std::int16_t _usageCount;       // number of uses (including soft uses) (negative means incorrect/unknown values, e.g. hardwired regs)
    std::int16_t _definitionCount;  // number of definitions  (including soft uses) (negative means incorrect/unknown values, e.g. hardwired regs)
    std::int16_t _softUsageCount;   // number of "soft" uses
    LogicalAddress *_logicalAddress; // for new backend only: logical address if created or nullptr
    static const int AvgBBIndexLen;     // estimated # of BasicBlock instances in which this appears

public:
    static std::size_t                                     currentNo;              // id of next PseudoRegister created
    GrowableArray<PseudoRegisterBasicBlockIndex *> _dus;                   // definitions and uses
    InlinedScope *_scope;               // scope to which this belongs
    Location _location;              // real location assigned to this preg
    CopyPropagationInfo *_copyPropagationInfo; // to follow effects of copy propagation
    GrowableArray<PseudoRegister *> *cpRegs;               // registers copy-propagated away by this
    int                             regClass;               // register equivalence class number
    PseudoRegister *regClassLink;         // next element in class
    int                                  _weight;                // weight (importance) for reg. allocation (-1 if targeted register)
    GrowableArray<BlockPseudoRegister *> *_uplevelReaders;      // list of blocks that uplevel-read this (or nullptr)
    GrowableArray<BlockPseudoRegister *> *_uplevelWriters;      // list of blocks that uplevel-write this (or nullptr)
    bool_t                               _debug;                 // value/loc needed for debugging info?
    int                                  _map_index_cache;       // caches old map index - used to improve PregMapping access speed - must be >= 0

protected:
    void initialize() {
        _id                  = currentNo++;
        _uplevelReaders      = nullptr;
        _uplevelWriters      = nullptr;
        _debug               = false;
        _usageCount          = 0;
        _definitionCount     = 0;
        _softUsageCount      = 0;
        _weight              = 0;
        _logicalAddress      = nullptr;
        _copyPropagationInfo = nullptr;
        regClass             = 0;
        regClassLink         = 0;
        cpRegs               = nullptr;
        _map_index_cache     = 0;
    }


    static const int VeryNegative;

public:
    PseudoRegister( InlinedScope *s ) :
            _dus( AvgBBIndexLen ) {
        st_assert( s, "must have a scope" );
        initialize();
        _scope    = s;
        _location = unAllocated;
    }


    PseudoRegister( InlinedScope *s, Location l, bool_t incU, bool_t incD ) :
            _dus( AvgBBIndexLen ) {
        st_assert( s, "must have a scope" );
        st_assert( not l.equals( illegalLocation ), "illegal location" );
        initialize();
        _scope               = s;
        _location            = l;
        if ( incU )
            _usageCount      = VeryNegative;
        if ( incD )
            _definitionCount = VeryNegative;
    }


    int id() const {
        return _id;
    }


    InlinedScope *scope() const {
        return _scope;
    }


    virtual bool_t isSinglyAssignedPseudoRegister() const {
        return false;
    }


    virtual bool_t isArgSinglyAssignedPseudoRegister() const {
        return false;
    }


    virtual bool_t isSplitPseudoRegister() const {
        return false;
    }


    virtual bool_t isTempPseudoRegister() const {
        return false;
    }


    virtual bool_t isNoPseudoRegister() const {
        return false;
    }


    virtual bool_t isBlockPseudoRegister() const {
        return false;
    }


    virtual bool_t isConstPseudoRegister() const {
        return false;
    }


    virtual bool_t isMemoized() const {
        return false;
    }


    bool_t uplevelR() const {
        return _uplevelReaders not_eq nullptr;
    }


    bool_t uplevelW() const {
        return _uplevelWriters not_eq nullptr;
    }


    void addUplevelAccessor( BlockPseudoRegister *blk, bool_t read, bool_t write );

    void removeUplevelAccessor( BlockPseudoRegister *blk );

    void removeAllUplevelAccessors();


    virtual int begByteCodeIndex() const {
        return PrologueByteCodeIndex;
    }


    virtual int endByteCodeIndex() const {
        return EpilogueByteCodeIndex;
    }


    virtual bool_t canCopyPropagate() const;


    bool_t incorrectDU() const {
        return _usageCount < 0 or _definitionCount < 0;
    }


    bool_t incorrectD() const {
        return _definitionCount < 0;
    }


    bool_t incorrectU() const {
        return _usageCount < 0;
    }


    void makeIncorrectDU( bool_t incU, bool_t incD );

protected:
    // don't use nuses() et al, use isUsed() instead
    // they are protected to avoid bugs; it's only safe to call them if uses are correct
    // (otherwise, nuses() et al can return negative values)
    int nuses() const {
        return _usageCount;
    }


    int hardUses() const {
        return _usageCount - _softUsageCount;
    }


    int ndefs() const {
        return _definitionCount;
    }


    friend class RegisterAllocator;

public:
    int nsoftUses() const {
        return _softUsageCount;
    }


    bool_t isUsed() const {
        return _definitionCount or _usageCount;
    }


    bool_t isUnused() const {
        return not isUsed();
    }


    bool_t hasNoUses() const {
        return _usageCount == 0;
    }


    // NB: be careful with isUnused() vs. hasNoUses() -- they're different if a PseudoRegister has
    // no (hard) uses but has definitions
    virtual bool_t isSinglyAssigned() const {
        return _definitionCount == 1;
    }


    bool_t isSinglyUsed() const {
        return _usageCount == 1;
    }


    bool_t isOnlySoftUsed() const {
        return hardUses() == 0;
    }


    void incUses( Usage *use );

    void decUses( Usage *use );

    void incDefs( Definition *def );

    void decDefs( Definition *def );

    // incremental update of def-use info
    Usage *addUse( DefinitionUsageInfo *info, NonTrivialNode *n );

    Usage *addUse( BasicBlock *bb, NonTrivialNode *n );

    void removeUse( DefinitionUsageInfo *info, Usage *u );

    void removeUse( BasicBlock *bb, Usage *u );

    Definition *addDef( DefinitionUsageInfo *info, NonTrivialNode *n );

    Definition *addDef( BasicBlock *bb, NonTrivialNode *n );

    void removeDef( DefinitionUsageInfo *info, Definition *d );

    void removeDef( BasicBlock *bb, Definition *d );

    virtual bool_t extendLiveRange( Node *n );

    virtual bool_t extendLiveRange( InlinedScope *s, int byteCodeIndex );

    void forAllDefsDo( Closure<Definition *> *c );

    void forAllUsesDo( Closure<Usage *> *c );

    bool_t isLocalTo( BasicBlock *bb ) const;

    void makeSameRegClass( PseudoRegister *other, GrowableArray<RegisterEqClass *> *classes );

    virtual void allocateTo( Location r );

    virtual bool_t canBeEliminated( bool_t withUses = false ) const;

    virtual void eliminate( bool_t withUses = false );

    bool_t isCPEquivalent( PseudoRegister *r ) const;

    bool_t checkEquivalentDefs() const;

    bool_t slow_isLiveAt( Node *n ) const;

    virtual bool_t isLiveAt( Node *n ) const;

protected:
    void addDUHelper( Node *n, SList<DefinitionUsage *> *l, DefinitionUsage *el );

    virtual NameNode *locNameNode( bool_t mustBeLegal ) const;

    void eliminateUses( DefinitionUsageInfo *info, BasicBlock *bb );

    void eliminateDefs( DefinitionUsageInfo *info, BasicBlock *bb, bool_t removing );

    void updateCPInfo( NonTrivialNode *n );

public:
    virtual void print();


    virtual void print_short() {
        lprintf( "%s\n", name() );
    }


    virtual const char *name() const;            // string representing the preg name
    const char *safeName() const;            // same as name() but handles nullptr receiver
    virtual const char *prefix() const {
        return "P";
    }


    virtual bool_t verify() const;

    virtual NameNode *nameNode( bool_t mustBeLegal = true ) const; // for debugging info
    LogicalAddress *createLogicalAddress();


    LogicalAddress *logicalAddress() const {
        return _logicalAddress;
    }


    PseudoRegister *cpReg() const;                // return "copy-propagation-equivalent" PseudoRegister

    friend InlinedScope *findAncestor( InlinedScope *s1, int &byteCodeIndex1, InlinedScope *s2, int &byteCodeIndex2 );
    // find closest common ancestor of s1 and s2, and the
    // respective sender byteCodeIndexs in that scope

    // Initialization (before every compile)
    static void initPRegs();
};


// A temp preg is exactly like a PseudoRegister except that it is live for only
// a very std::int16_t (hard-wired) code sequence such as loading a frame ptr etc.
class TemporaryPseudoRegister : public PseudoRegister {
public:
    TemporaryPseudoRegister( InlinedScope *s ) :
            PseudoRegister( s ) {
    }


    TemporaryPseudoRegister( InlinedScope *s, Location l, bool_t incU, bool_t incD ) :
            PseudoRegister( s, l, incU, incD ) {
    }


    bool_t isTempPseudoRegister() const {
        return true;
    }


    bool_t isLiveAt( Node *n ) const {
        return false;
    }


    bool_t canCopyPropagate() const {
        return false;
    }


    const char *prefix() const {
        return "TempP";
    }
};

// singly-assigned PseudoRegister (in source-level terms, e.g. expr. stack entry, arg;
// may have several definitions because of splitting etc.
// makes copy propagation simpler
class SinglyAssignedPseudoRegister : public PseudoRegister {
protected:
    InlinedScope *_creationScope;        // source scope to which receiver belongs
    int          creationStartByteCodeIndex;        // startByteCodeIndex in creationScope
    int          _begByteCodeIndex, _endByteCodeIndex;        // live range = [_begByteCodeIndex, _endByteCodeIndex] in scope
    // (for reg. alloc. purposes)
    const bool_t _isInContext;        // is this SinglyAssignedPseudoRegister a context location?
public:
    SinglyAssignedPseudoRegister( InlinedScope *s, int stream = IllegalByteCodeIndex, int en = IllegalByteCodeIndex, bool_t inContext = false );


    SinglyAssignedPseudoRegister( InlinedScope *s, Location l, bool_t incU, bool_t incD, int stream, int en ) :
            PseudoRegister( (InlinedScope *) s, l, incU, incD ), _isInContext( false ) {
        _begByteCodeIndex = creationStartByteCodeIndex = stream;
        _endByteCodeIndex = en;
        _creationScope    = s;
    }


    int begByteCodeIndex() const {
        return _begByteCodeIndex;
    }


    int endByteCodeIndex() const {
        return _endByteCodeIndex;
    }


    bool_t isInContext() const {
        return _isInContext;
    }


    InlinedScope *creationScope() const {
        return _creationScope;
    }


    bool_t isSinglyAssigned() const {
        return true;
    }


    bool_t extendLiveRange( Node *n );

    bool_t extendLiveRange( InlinedScope *s, int byteCodeIndex );

    bool_t isLiveAt( Node *n ) const;


    bool_t isSinglyAssignedPseudoRegister() const {
        return true;
    }


    const char *prefix() const {
        return "SAP";
    }


    bool_t verify() const;

protected:
    bool_t basic_isLiveAt( InlinedScope *s, int byteCodeIndex ) const;

    friend class ExpressionStack;
};


class BlockPseudoRegister : public SinglyAssignedPseudoRegister {
protected:
    CompileTimeClosure *_closure;            // the compile-time closure representation
    bool_t                          _memoized;                // is this a memoized block?
    bool_t                          _escapes;            // does the block escape?
    GrowableArray<Node *>           *_escapeNodes;            // list of all nodes where the block escapes (or nullptr)
    GrowableArray<PseudoRegister *> *_uplevelRead;            // list of PseudoRegisters uplevel-read by block method (or nullptr)
    GrowableArray<PseudoRegister *> *_uplevelWritten;        // list of PseudoRegisters uplevel-written by block method (or nullptr)
    GrowableArray<Location *>       *_contextCopies;        // list of context location containing a copy of the receiver (or nullptr)
    static std::size_t                      _numBlocks;

public:
    BlockPseudoRegister( InlinedScope *scope, CompileTimeClosure *closure, int beg, int end );


    bool_t isBlockPseudoRegister() const {
        return true;
    }


    virtual NameNode *locNameNode( bool_t mustBeLegal ) const;


    InlinedScope *scope() const {
        return (InlinedScope *) _scope;
    }


    InlinedScope *parent() const;


    CompileTimeClosure *closure() const {
        return _closure;
    }


    MethodOop method() const {
        return _closure->method();
    }


    static std::size_t numBlocks() {
        return _numBlocks;
    }


    void memoize();                // memoize this block if possible/desirable
    void markEscaped( Node *n );            // mark this block as escaping at node n
    void markEscaped();                // ditto; receiver escapes because of uplevel access from escaping block
    bool_t isMemoized() const {
        return _memoized;
    }


    bool_t escapes() const {
        return _escapes;
    }


    bool_t canBeEliminated( bool_t withUses = false ) const;

    void eliminate( bool_t withUses = false );

    void computeUplevelAccesses();


    GrowableArray<PseudoRegister *> *uplevelRead() const {
        return _uplevelRead;
    }


    GrowableArray<PseudoRegister *> *uplevelWritten() const {
        return _uplevelWritten;
    }


    GrowableArray<Location *> *contextCopies() const {
        return _contextCopies;
    }


    void addContextCopy( Location *l );


    const char *prefix() const {
        return "BlkP";
    }


    const char *name() const;

    bool_t verify() const;

    void print();

    friend InlinedScope *scopeFromBlockKlass( KlassOop blockKlass );

    friend class PseudoRegister;
};


class NoResultPseudoRegister : public PseudoRegister {    // "no result" register (should have no uses)
public:
    NoResultPseudoRegister( InlinedScope *scope ) :
            PseudoRegister( scope ) {
        _location = noRegister;
        initialize();
    }


    virtual bool_t isNoPseudoRegister() const {
        return true;
    }


    bool_t canCopyPropagate() const {
        return false;
    }


    NameNode *nameNode( bool_t mustBeLegal ) const;


    const char *name() const {
        return "nil";
    }


    bool_t verify() const;
};


class ConstPseudoRegister : public PseudoRegister {
    // ConstPseudoRegisters are used to CSE constants; at register allocation time,
    // they can either get a register (if one is available), or they're
    // treated as literals by code generation, i.e. loaded into a temp reg
    // before each use.
public:
    Oop constant;
protected:
    ConstPseudoRegister( InlinedScope *s, Oop c ) :
            PseudoRegister( s ) {
        constant = c;
        st_assert( not c->is_mem() or c->is_old(), "constant must be tenured" );
    }


public:
    friend ConstPseudoRegister *new_ConstPReg( InlinedScope *s, Oop c );

    friend ConstPseudoRegister *findConstPReg( Node *n, Oop c );


    bool_t isConstPseudoRegister() const {
        return true;
    }


    void allocateTo( Location r );

    void extendLiveRange( InlinedScope *s );

    bool_t extendLiveRange( Node *n );

    bool_t covers( Node *n ) const;

    bool_t needsRegister() const;

    NameNode *nameNode( bool_t mustBeLegal = true ) const;


    const char *prefix() const {
        return "ConstP";
    }


    const char *name() const;

    bool_t verify() const;
};

ConstPseudoRegister *new_ConstPReg( InlinedScope *s, Oop c );


#if 0

// SplitPseudoRegisters hold the receiver of a split message send; their main
// purpose is to make register allocation/live range computation simple
// and efficient without implementing general live range analysis

class SplitPReg : public SinglyAssignedPseudoRegister {
 public:
  SplitSig* sig;

  SplitPReg(InlinedScope* s, int stream, int en, SplitSig* signature) : SinglyAssignedPseudoRegister(s, stream, en) {
    sig = signature;
  }

  bool_t	isSinglyAssigned() const		{ return true; }
  bool_t	extendLiveRange(Node* n);
  bool_t	isLiveAt(Node* n) const;
  bool_t	isSplitPseudoRegister() const 	    		{ return true; }
  char*	prefix() const				{ return "SplitP"; }
  char*	name() const;
};

#endif
