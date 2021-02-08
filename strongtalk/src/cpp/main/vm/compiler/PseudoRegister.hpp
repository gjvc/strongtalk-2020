//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/code/LogicalAddress.hpp"
#include "vm/compiler/slist.hpp"
#include "vm/compiler/CompileTimeClosure.hpp"
#include "vm/compiler/DefinitionUsage.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/runtime/ResourceObject.hpp"


// All intermediate language (IL) operations work on pseudo registers (PseudoRegisters).
// In contrast to real registers, the number of available pseudo registers is unlimited.

// PseudoRegister are standard pseudo regs; the different kinds of PseudoRegisters differ mainly in their purpose and in their live ranges
// PseudoRegister: locals etc., multiply assigned, live range is the entire scope
// SinglyAssignedPseudoRegister: single source-level assignment, live range = arbitrary subrange
// SplitPseudoRegister: used for splitting
// BlockPseudoRegisters: for blocks, live range = creating byteCodeIndex .. end
// TempPseudoRegisters: local to one BasicBlock, used for certain hard-coded idioms (e.g. loading an uplevel-accessed value)

class RegisterEqClass;

class DefinitionUsageInfo;

class CopyPropagationInfo;

class BlockPseudoRegister;


class PseudoRegister : public PrintableResourceObject {

protected:
    std::int16_t              _id;               // unique id
    std::int16_t              _usageCount;       // number of uses (including soft uses) (negative means incorrect/unknown values, e.g. hardwired regs)
    std::int16_t              _definitionCount;  // number of definitions  (including soft uses) (negative means incorrect/unknown values, e.g. hardwired regs)
    std::int16_t              _softUsageCount;   // number of "soft" uses
    LogicalAddress            *_logicalAddress; // for new backend only: logical address if created or nullptr
    static const std::int32_t AvgBBIndexLen;     // estimated # of BasicBlock instances in which this appears

public:
    static std::int32_t                            currentNo;               // id of next PseudoRegister created
    GrowableArray<PseudoRegisterBasicBlockIndex *> _dus;                    // definitions and uses
    InlinedScope                                   *_scope;                 // scope to which this belongs
    Location                                       _location;               // real location assigned to this pseudoRegister
    CopyPropagationInfo                            *_copyPropagationInfo;   // to follow effects of copy propagation
    GrowableArray<PseudoRegister *>                *cpseudoRegisters;       // registers copy-propagated away by this
    std::int32_t                                   regClass;                // register equivalence class number
    PseudoRegister                                 *regClassLink;           // next element in class
    std::int32_t                                   _weight;                 // weight (importance) for reg. allocation (-1 if targeted register)
    GrowableArray<BlockPseudoRegister *>           *_uplevelReaders;        // list of blocks that uplevel-read this (or nullptr)
    GrowableArray<BlockPseudoRegister *>           *_uplevelWriters;        // list of blocks that uplevel-write this (or nullptr)
    bool                                           _debug;                  // value/loc needed for debugging info?
    std::int32_t                                   _map_index_cache;        // caches old map index - used to improve PseudoRegisterMapping access speed - must be >= 0

protected:
    static const std::int32_t VeryNegative;

public:
    PseudoRegister( InlinedScope *s ) :
        _id{ static_cast<int16_t>(currentNo++) },
        _usageCount{},
        _definitionCount{},
        _softUsageCount{},
        _logicalAddress{ nullptr },
        _scope{ nullptr },
        _location{ Location::UNALLOCATED_LOCATION },
        _copyPropagationInfo{ nullptr },
        cpseudoRegisters{ nullptr },
        regClass{},
        regClassLink{ nullptr },
        _weight{},
        _uplevelReaders{ nullptr },
        _uplevelWriters{ nullptr },
        _debug{ false },
        _map_index_cache{},
        _dus( AvgBBIndexLen ) {
        _scope = s;
        st_assert( s, "must have a scope" );
    }


    PseudoRegister( InlinedScope *s, Location l, bool incU, bool incD ) :
        _id{ static_cast<int16_t>(currentNo++) },
        _usageCount{},
        _definitionCount{},
        _softUsageCount{},
        _logicalAddress{ nullptr },
        _scope{ nullptr },
        _location{ Location::UNALLOCATED_LOCATION },
        _copyPropagationInfo{ nullptr },
        cpseudoRegisters{ nullptr },
        regClass{},
        regClassLink{ nullptr },
        _weight{},
        _uplevelReaders{ nullptr },
        _uplevelWriters{ nullptr },
        _debug{ false },
        _map_index_cache{},
        _dus( AvgBBIndexLen ) {

        st_assert( s, "must have a scope" );
        st_assert( not l.equals( Location::ILLEGAL_LOCATION ), "illegal location" );

        _scope    = s;
        _location = l;

        if ( incU ) {
            _usageCount = VeryNegative;
        }

        if ( incD ) {
            _definitionCount = VeryNegative;
        }

    }


    PseudoRegister() = default;
    virtual ~PseudoRegister() = default;
    PseudoRegister( const PseudoRegister & ) = default;
    PseudoRegister &operator=( const PseudoRegister & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    std::int32_t id() const {
        return _id;
    }


    InlinedScope *scope() const {
        return _scope;
    }


    virtual bool isSinglyAssignedPseudoRegister() const {
        return false;
    }


    virtual bool isArgSinglyAssignedPseudoRegister() const {
        return false;
    }


    virtual bool isSplitPseudoRegister() const {
        return false;
    }


    virtual bool isTempPseudoRegister() const {
        return false;
    }


    virtual bool isNoPseudoRegister() const {
        return false;
    }


    virtual bool isBlockPseudoRegister() const {
        return false;
    }


    virtual bool isConstPseudoRegister() const {
        return false;
    }


    virtual bool isMemoized() const {
        return false;
    }


    bool uplevelR() const {
        return _uplevelReaders not_eq nullptr;
    }


    bool uplevelW() const {
        return _uplevelWriters not_eq nullptr;
    }


    void addUplevelAccessor( BlockPseudoRegister *blk, bool read, bool write );

    void removeUplevelAccessor( BlockPseudoRegister *blk );

    void removeAllUplevelAccessors();


    virtual std::int32_t begByteCodeIndex() const {
        return PrologueByteCodeIndex;
    }


    virtual std::int32_t endByteCodeIndex() const {
        return EpilogueByteCodeIndex;
    }


    virtual bool canCopyPropagate() const;


    bool incorrectDU() const {
        return _usageCount < 0 or _definitionCount < 0;
    }


    bool incorrectD() const {
        return _definitionCount < 0;
    }


    bool incorrectU() const {
        return _usageCount < 0;
    }


    void makeIncorrectDU( bool incU, bool incD );

protected:
    // don't use nuses() et al, use isUsed() instead
    // they are protected to avoid bugs; it's only safe to call them if uses are correct
    // (otherwise, nuses() et al can return negative values)
    std::int32_t nuses() const {
        return _usageCount;
    }


    std::int32_t hardUses() const {
        return _usageCount - _softUsageCount;
    }


    std::int32_t ndefs() const {
        return _definitionCount;
    }


    friend class RegisterAllocator;

public:
    std::int32_t nsoftUses() const {
        return _softUsageCount;
    }


    bool isUsed() const {
        return _definitionCount or _usageCount;
    }


    bool isUnused() const {
        return not isUsed();
    }


    bool hasNoUses() const {
        return _usageCount == 0;
    }


    // NB: be careful with isUnused() vs. hasNoUses() -- they're different if a PseudoRegister has
    // no (hard) uses but has definitions
    virtual bool isSinglyAssigned() const {
        return _definitionCount == 1;
    }


    bool isSinglyUsed() const {
        return _usageCount == 1;
    }


    bool isOnlySoftUsed() const {
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

    virtual bool extendLiveRange( Node *n );

    virtual bool extendLiveRange( InlinedScope *s, std::int32_t byteCodeIndex );

    void forAllDefsDo( Closure<Definition *> *c );

    void forAllUsesDo( Closure<Usage *> *c );

    bool isLocalTo( BasicBlock *bb ) const;

    void makeSameRegClass( PseudoRegister *other, GrowableArray<RegisterEqClass *> *classes );

    virtual void allocateTo( Location r );

    virtual bool canBeEliminated( bool withUses = false ) const;

    virtual void eliminate( bool withUses = false );

    bool isCPEquivalent( PseudoRegister *r ) const;

    bool checkEquivalentDefs() const;

    bool slow_isLiveAt( Node *n ) const;

    virtual bool isLiveAt( Node *n ) const;

protected:
    void addDUHelper( Node *n, SList<DefinitionUsage *> *l, DefinitionUsage *el );

    virtual NameNode *locNameNode( bool mustBeLegal ) const;

    void eliminateUses( DefinitionUsageInfo *info, BasicBlock *bb );

    void eliminateDefs( DefinitionUsageInfo *info, BasicBlock *bb, bool removing );

    void updateCPInfo( NonTrivialNode *n );

public:
    virtual void print();


    virtual void print_short() {
        SPDLOG_INFO( "%s", name() );
    }


    virtual const char *name() const;            // string representing the pseudoRegister name
    const char *safeName() const;            // same as name() but handles nullptr receiver
    virtual const char *prefix() const {
        return "P";
    }


    virtual bool verify() const;

    virtual NameNode *nameNode( bool mustBeLegal = true ) const; // for debugging info
    LogicalAddress *createLogicalAddress();


    LogicalAddress *logicalAddress() const {
        return _logicalAddress;
    }


    PseudoRegister *cpseudoRegister() const;                // return "copy-propagation-equivalent" PseudoRegister

    friend InlinedScope *findAncestor( InlinedScope *s1, std::int32_t &byteCodeIndex1, InlinedScope *s2, std::int32_t &byteCodeIndex2 );
    // find closest common ancestor of s1 and s2, and the
    // respective sender byteCodeIndexs in that scope

    // Initialization (before every compile)
    static void initPseudoRegisters();
};


// A temp pseudoRegister is exactly like a PseudoRegister except that it is live for only
// a very std::int16_t (hard-wired) code sequence such as loading a frame ptr etc.
class TemporaryPseudoRegister : public PseudoRegister {
public:
    TemporaryPseudoRegister( InlinedScope *s ) :
        PseudoRegister( s ) {
    }


    TemporaryPseudoRegister( InlinedScope *s, Location l, bool incU, bool incD ) :
        PseudoRegister( s, l, incU, incD ) {
    }


    bool isTempPseudoRegister() const {
        return true;
    }


    bool isLiveAt( Node *n ) const {
        static_cast<void>(n); // unused
        return false;
    }


    bool canCopyPropagate() const {
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
    InlinedScope *_creationScope;                       // source scope to which receiver belongs
    std::int32_t _creationStartByteCodeIndex;           // startByteCodeIndex in creationScope
    std::int32_t _begByteCodeIndex, _endByteCodeIndex;  // live range = [_begByteCodeIndex, _endByteCodeIndex] in scope (for reg. alloc. purposes)
    const bool   _isInContext;                          // is this SinglyAssignedPseudoRegister a context location?

public:
    SinglyAssignedPseudoRegister( InlinedScope *s, std::int32_t stream = IllegalByteCodeIndex, std::int32_t en = IllegalByteCodeIndex, bool inContext = false );


    SinglyAssignedPseudoRegister( InlinedScope *s, Location l, bool incU, bool incD, std::int32_t stream, std::int32_t en ) :
        PseudoRegister( (InlinedScope *) s, l, incU, incD ), _isInContext( false ),
        _begByteCodeIndex{ stream },
        _creationStartByteCodeIndex{ stream },
        _endByteCodeIndex{ en },
        _creationScope{ s } {
    }


    SinglyAssignedPseudoRegister() = default;
    virtual ~SinglyAssignedPseudoRegister() = default;
    SinglyAssignedPseudoRegister( const SinglyAssignedPseudoRegister & ) = default;
    SinglyAssignedPseudoRegister &operator=( const SinglyAssignedPseudoRegister & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    std::int32_t begByteCodeIndex() const {
        return _begByteCodeIndex;
    }


    std::int32_t endByteCodeIndex() const {
        return _endByteCodeIndex;
    }


    bool isInContext() const {
        return _isInContext;
    }


    InlinedScope *creationScope() const {
        return _creationScope;
    }


    bool isSinglyAssigned() const {
        return true;
    }


    bool extendLiveRange( Node *n );

    bool extendLiveRange( InlinedScope *s, std::int32_t byteCodeIndex );

    bool isLiveAt( Node *n ) const;


    bool isSinglyAssignedPseudoRegister() const {
        return true;
    }


    const char *prefix() const {
        return "SAP";
    }


    bool verify() const;

protected:
    bool basic_isLiveAt( InlinedScope *s, std::int32_t byteCodeIndex ) const;

    friend class ExpressionStack;
};


class BlockPseudoRegister : public SinglyAssignedPseudoRegister {
protected:
    CompileTimeClosure              *_closure;            // the compile-time closure representation
    bool                            _memoized;                // is this a memoized block?
    bool                            _escapes;            // does the block escape?
    GrowableArray<Node *>           *_escapeNodes;            // list of all nodes where the block escapes (or nullptr)
    GrowableArray<PseudoRegister *> *_uplevelRead;            // list of PseudoRegisters uplevel-read by block method (or nullptr)
    GrowableArray<PseudoRegister *> *_uplevelWritten;        // list of PseudoRegisters uplevel-written by block method (or nullptr)
    GrowableArray<Location *>       *_contextCopies;        // list of context location containing a copy of the receiver (or nullptr)
    static std::int32_t             _numBlocks;

public:
    BlockPseudoRegister( InlinedScope *scope, CompileTimeClosure *closure, std::int32_t beg, std::int32_t end );

    BlockPseudoRegister() = default;
    virtual ~BlockPseudoRegister() = default;
    BlockPseudoRegister( const BlockPseudoRegister & ) = default;
    BlockPseudoRegister &operator=( const BlockPseudoRegister & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    bool isBlockPseudoRegister() const {
        return true;
    }


    virtual NameNode *locNameNode( bool mustBeLegal ) const;


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


    static std::int32_t numBlocks() {
        return _numBlocks;
    }


    void memoize();                // memoize this block if possible/desirable
    void markEscaped( Node *n );            // mark this block as escaping at node n
    void markEscaped();                // ditto; receiver escapes because of uplevel access from escaping block
    bool isMemoized() const {
        return _memoized;
    }


    bool escapes() const {
        return _escapes;
    }


    bool canBeEliminated( bool withUses = false ) const;

    void eliminate( bool withUses = false );

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

    bool verify() const;

    void print();

    friend InlinedScope *scopeFromBlockKlass( KlassOop blockKlass );

    friend class PseudoRegister;
};


class NoResultPseudoRegister : public PseudoRegister {    // "no result" register (should have no uses)
public:
    NoResultPseudoRegister( InlinedScope *scope ) :
        PseudoRegister( scope ) {
        _location = Location::NO_REGISTER;
    }


    virtual bool isNoPseudoRegister() const {
        return true;
    }


    bool canCopyPropagate() const {
        return false;
    }


    NameNode *nameNode( bool mustBeLegal ) const;


    const char *name() const {
        return "nil";
    }


    bool verify() const;
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
        PseudoRegister( s ),
        constant{ c } {
        st_assert( not c->is_mem() or c->is_old(), "constant must be tenured" );
    }


    ConstPseudoRegister() = default;
    virtual ~ConstPseudoRegister() = default;
    ConstPseudoRegister( const ConstPseudoRegister & ) = default;
    ConstPseudoRegister &operator=( const ConstPseudoRegister & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



public:
    friend ConstPseudoRegister *new_ConstPseudoRegister( InlinedScope *s, Oop c );

    friend ConstPseudoRegister *findConstPseudoRegister( Node *n, Oop c );


    bool isConstPseudoRegister() const {
        return true;
    }


    void allocateTo( Location r );

    void extendLiveRange( InlinedScope *s );

    bool extendLiveRange( Node *n );

    bool covers( Node *n ) const;

    bool needsRegister() const;

    NameNode *nameNode( bool mustBeLegal = true ) const;


    const char *prefix() const {
        return "ConstP";
    }


    const char *name() const;

    bool verify() const;
};

ConstPseudoRegister *new_ConstPseudoRegister( InlinedScope *s, Oop c );


#if 0

// SplitPseudoRegisters hold the receiver of a split message send; their main
// purpose is to make register allocation/live range computation simple
// and efficient without implementing general live range analysis

class SplitPseudoRegister : public SinglyAssignedPseudoRegister {
 public:
  SplitSig* sig;

  SplitPseudoRegister(InlinedScope* s, std::int32_t stream, std::int32_t en, SplitSig* signature) : SinglyAssignedPseudoRegister(s, stream, en) {
    sig = signature;
  }

  bool	isSinglyAssigned() const		{ return true; }
  bool	extendLiveRange(Node* n);
  bool	isLiveAt(Node* n) const;
  bool	isSplitPseudoRegister() const 	    		{ return true; }
  char*	prefix() const				{ return "SplitP"; }
  char*	name() const;
};

#endif
