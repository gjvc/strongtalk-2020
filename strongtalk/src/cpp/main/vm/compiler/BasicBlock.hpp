
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/code/NodeVisitor.hpp"
#include "vm/compiler/defUse.hpp"
#include "vm/utilities/lprintf.hpp"
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
    BasicBlockDefinitionAndUsageTable() {
        info = nullptr;
    }


    void print_short() {
        lprintf( "BasicBlockDefinitionAndUsageTable [%#lx[", this );
    }


    void print();
};


// BasicBlock is used by the Compiler to perform local optimizations and code generation
class BasicBlock : public PrintableResourceObject {

protected:
    bool_t _visited;

public: // was "protected:" originally
    Node *_first;            //
    Node *_last;             //
    std::int16_t _nodeCount;         // number of nodes in this BasicBlock

protected:
    std::int16_t _id;                // unique BasicBlock id
    std::int16_t _loopDepth;         // the loop nesting level
    std::int16_t _genCount;          // code already generated?

public:
    BasicBlockDefinitionAndUsageTable duInfo;        // definitions/uses of PseudoRegisters
    static int                        genCounter;    // to enumerate BBs in code-generation order

public:
    BasicBlock( Node *f, Node *l, int n ) {
        init( f, l, n );
        _visited = false;
    }


    BasicBlock *after_visit() {
        _visited = true;
        return this;
    }


    BasicBlock *before_visit() {
        _visited = false;
        return this;
    }


    bool_t visited() const {
        return _visited;
    }


    // successor/predecessor functionality
    bool_t hasSingleSuccessor() const;

    bool_t hasSinglePredecessor() const;

    int nPredecessors() const;

    int nSuccessors() const;

    bool_t isPredecessor( const BasicBlock *n ) const;

    bool_t isSuccessor( const BasicBlock *n ) const;

    BasicBlock *next() const;

    BasicBlock *next1() const;

    BasicBlock *next( int i ) const;

    BasicBlock *firstPrev() const;

    BasicBlock *prev( int i ) const;


    int id() const {
        return this == nullptr ? -1 : _id;
    }


    int loopDepth() const {
        return _loopDepth;
    }


    void setGenCount() {
        _genCount = genCounter++;
    }


    int genCount() const {
        return _genCount;
    }


    void localCopyPropagate();

    void bruteForceCopyPropagate();

    void makeUses();

    Usage *addUse( NonTrivialNode *n, PseudoRegister *r, bool_t soft = false );

    Definition *addDef( NonTrivialNode *n, PseudoRegister *r );

    void remove( Node *n );                // remove node
    void addAfter( Node *prev, Node *newNode );    // add node after prev
    void localAlloc( GrowableArray<BitVector *> *hardwired, GrowableArray<PseudoRegister *> *localRegs, GrowableArray<BitVector *> *lives );

protected:
    void init( Node *first, Node *last, int n );

    void slowLocalAlloc( GrowableArray<BitVector *> *hardwired, GrowableArray<PseudoRegister *> *localRegs, GrowableArray<BitVector *> *lives );

    void doAlloc( PseudoRegister *r, Location l );

public:
    void computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l );

    void apply( NodeVisitor *v );

    bool_t verifyLabels();

    bool_t contains( const Node *n ) const;

    void verify();

    void dfs( GrowableArray<BasicBlock *> *list, int depth );

    void print_short();

    void print_code( bool_t suppressTrivial );

    void print();

protected:
    int addUDHelper( PseudoRegister *r );

    void renumber();

    friend class BasicBlockIterator;
};
