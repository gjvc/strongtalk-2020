//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/compiler/PseudoRegister.hpp"



// DefinitionUsageInfo represents PseudoRegister's definitions/uses within BasicBlock
class DefinitionUsageInfo : public PrintableResourceObject {

public:
    PseudoRegister *_pseudoRegister;
    SList<Usage *>      _usages;    // uses (in order of nodes within BasicBlock)
    SList<Definition *> _definitions;


    DefinitionUsageInfo( PseudoRegister *r ) {
        _pseudoRegister = r;
    }


    void getLiveRange( int &firstNodeID, int &lastNodeId );

    void propagateTo( BasicBlock *bb, const PseudoRegister *r, const Definition *def, Usage *use, const bool_t global );

    void propagateTo( BasicBlock *useBB, Usage *use, const NonTrivialNode *fromNode, PseudoRegister *src, NonTrivialNode *toNode, const bool_t global );

    void print_short();

    void print();
};

typedef void (*BBDoFn)( BasicBlock *b );
