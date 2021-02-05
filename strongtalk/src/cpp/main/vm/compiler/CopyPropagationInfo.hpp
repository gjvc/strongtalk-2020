
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/runtime/ResourceObject.hpp"


// keeps track of effects of copy propagation (for debugging info)

class CopyPropagationInfo : public PrintableResourceObject {

public:
    NonTrivialNode *_definition;   // eliminated definition
    PseudoRegister *_register;     // equivalent PseudoRegister

    CopyPropagationInfo( NonTrivialNode *d, PseudoRegister *r1 ) :
        _definition{ d },
        _register{ r1 } {
    }


    bool isConstant() const;

    Oop constant() const;

    void print();

protected:
    CopyPropagationInfo( NonTrivialNode *def );

    friend CopyPropagationInfo *new_CPInfo( NonTrivialNode *def );
};


CopyPropagationInfo *new_CPInfo( NonTrivialNode *def ); // may return nullptr if def isn't suitable
