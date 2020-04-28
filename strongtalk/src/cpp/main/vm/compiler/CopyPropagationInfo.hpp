//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/runtime/ResourceObject.hpp"

// keeps track of effects of copy propagation (for debugging info)

class CopyPropagationInfo : public PrintableResourceObject {

    public:
        NonTrivialNode * def;   // eliminated definition
        PseudoRegister * r;     // equivalent PseudoRegister

        CopyPropagationInfo( NonTrivialNode * d, PseudoRegister * r1 ) {
            def = d;
            r   = r1;
        }


        bool_t isConstant() const;

        Oop constant() const;

        void print();

    protected:
        CopyPropagationInfo( NonTrivialNode * def );

        friend CopyPropagationInfo * new_CPInfo( NonTrivialNode * def );
};


CopyPropagationInfo * new_CPInfo( NonTrivialNode * def ); // may return nullptr if def isn't suitable
