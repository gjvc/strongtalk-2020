//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/code/NodeVisitor.hpp"
#include "vm/compiler/defUse.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/runtime/ResourceObject.hpp"

class BasicBlockIterator : public PrintableResourceObject {

    private:
        Node * _first;                        // first node

    public:                                                         // was protected: originally
        GrowableArray <BasicBlock *>          * _basicBlockTable;     // BBs sorted in topological order
        int                                   _basicBlockCount;      // number of BBs
        GrowableArray <PseudoRegister *>      * pregTable;            // holds all PseudoRegisters; indexed by their id
        GrowableArray <PseudoRegister *>      * globals;              // holds globally allocated PseudoRegisters; indexed by their num()
        bool_t                                _usesBuilt;            // true after uses have been built
        bool_t                                _blocksBuilt;          // true after basic blocks have been built
        GrowableArray <BlockPseudoRegister *> * exposedBlks;          // list of escaping blocks

    public:

        BasicBlockIterator() {
            _basicBlockTable = nullptr;     //
            _basicBlockCount = 0;           //
            _usesBuilt       = false;       //
            _blocksBuilt     = false;       //
        }


        void build( Node * first );          // build bbTable

    protected:
        void buildBBs();

        void buildTable();

        static BasicBlock * bb_for( Node * n );

        void add_BBs_to_list( GrowableArray <BasicBlock *> & list, GrowableArray <BasicBlock *> & work );

    public:
        bool_t isSequential( int curr, int next ) const;        // are the two BasicBlock indices sequential in bbTable order?
        bool_t isSequentialCode( BasicBlock * curr, BasicBlock * next ) const;    // are the two BBs sequential in codeGen order?
        GrowableArray <BasicBlock *> * code_generation_order();        // list of BBs in code generation order

        void clear();                            // clear bbTable
        void apply( BBDoFn f );

        void makeUses();

        void eliminateUnneededResults();

        void computeEscapingBlocks();

        void computeUplevelAccesses();

        void localAlloc();

        void localCopyPropagate();

        void globalCopyPropagate();

        void bruteForceCopyPropagate();

        void gen( Node * first );

        void apply( NodeVisitor * v );

        bool_t verifyLabels();

        void print();

        void print_code( bool_t suppressTrivial );

        void verify();
};

extern BasicBlockIterator * bbIterator;
