
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/code/NodeVisitor.hpp"
#include "vm/compiler/DefinitionUsage.hpp"
#include "vm/compiler/DefinitionUsageInfo.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/compiler/BitVector.hpp"

class Node;

class BasicBlockIterator;

class DefinitionUsageInfo;

class BlockPseudoRegister;


// a BasicBlockDefinitionAndUsageTable contains all definitions and uses of a BasicBlock
class BasicBlockDefinitionAndUsageTable : public PrintableResourceObject {

public:
    GrowableArray<DefinitionUsageInfo *> *info;        // one element per PseudoRegister used / defined
    BasicBlockDefinitionAndUsageTable() :
        info{ nullptr } {
    }


    virtual ~BasicBlockDefinitionAndUsageTable() = default;
    BasicBlockDefinitionAndUsageTable( const BasicBlockDefinitionAndUsageTable & ) = default;
    BasicBlockDefinitionAndUsageTable &operator=( const BasicBlockDefinitionAndUsageTable & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void print_short() {
        SPDLOG_INFO( "BasicBlockDefinitionAndUsageTable 0x{0:x}", static_cast<void *>( this ) );
    }


    void print();
};


// BasicBlock is used by the Compiler to perform local optimizations and code generation
class BasicBlock : public PrintableResourceObject {

protected:
    bool _visited;

public: // was "protected:" originally
    Node         *_first;            //
    Node         *_last;             //
    std::int16_t _nodeCount;         // number of nodes in this BasicBlock

protected:
    std::int16_t _id;                // unique BasicBlock id
    std::int16_t _loopDepth;         // the loop nesting level
    std::int16_t _genCount;          // code already generated?

public:
    BasicBlockDefinitionAndUsageTable duInfo;        // definitions/uses of PseudoRegisters
    static std::int32_t               genCounter;    // to enumerate BBs in code-generation order

public:
    BasicBlock( Node *f, Node *l, std::int16_t n ) :
        _first{ f },
        _last{ l },
        _nodeCount{ n },
        _id{ 0 },
        _genCount{ 0 },
        duInfo{},
        _loopDepth{ 0 },
        _visited{ false } {

        BasicBlock::genCounter = 0;

    }


    BasicBlock() = default;
    virtual ~BasicBlock() = default;
    BasicBlock( const BasicBlock & ) = default;
    BasicBlock &operator=( const BasicBlock & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    BasicBlock *after_visit() {
        _visited = true;
        return this;
    }


    BasicBlock *before_visit() {
        _visited = false;
        return this;
    }


    bool visited() const {
        return _visited;
    }


    // successor/predecessor functionality
    bool hasSingleSuccessor() const;

    bool hasSinglePredecessor() const;

    std::int32_t nPredecessors() const;

    std::int32_t nSuccessors() const;

    bool isPredecessor( const BasicBlock *n ) const;

    bool isSuccessor( const BasicBlock *n ) const;

    BasicBlock *next() const;

    BasicBlock *next1() const;

    BasicBlock *next( std::int32_t i ) const;

    BasicBlock *firstPrev() const;

    BasicBlock *prev( std::int32_t i ) const;


    std::int32_t id() const {
//        return this == nullptr ? -1 : _id;
        return _id;
    }


    std::int16_t loopDepth() const {
        return _loopDepth;
    }


    void setGenCount() {
        _genCount = genCounter++;
    }


    std::int16_t genCount() const {
        return _genCount;
    }


    void localCopyPropagate();

    void bruteForceCopyPropagate();

    void makeUses();

    Usage *addUse( NonTrivialNode *n, PseudoRegister *r, bool soft = false );

    Definition *addDef( NonTrivialNode *n, PseudoRegister *r );

    void remove( Node *n );                // remove node
    void addAfter( Node *prev, Node *newNode );    // add node after prev
    void localAlloc( GrowableArray<BitVector *> *hardwired, GrowableArray<PseudoRegister *> *localRegs, GrowableArray<BitVector *> *lives );

protected:
    void init( Node *first, Node *last, std::int32_t n );

    void slowLocalAlloc( GrowableArray<BitVector *> *hardwired, GrowableArray<PseudoRegister *> *localRegs, GrowableArray<BitVector *> *lives );

    void doAlloc( PseudoRegister *r, Location l );

public:
    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );

    void apply( NodeVisitor *v );

    bool verifyLabels();

    bool contains( const Node *n ) const;

    void verify();

    void dfs( GrowableArray<BasicBlock *> *list, std::int32_t depth );

    void print_short();

    void print_code( bool suppressTrivial );

    void print();

protected:
    std::int32_t addUDHelper( PseudoRegister *r );

    void renumber();

    friend class BasicBlockIterator;
};
