//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/assembler/Location.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"


// register allocator for the Compiler -- currently a simple usage count thing

class RegisterAllocator : public ResourceObject {

    private:
        IntFreeList * _stackLocs;

    public:
        RegisterAllocator();

        void preAllocate( PseudoRegister * r );

        void allocate( GrowableArray <PseudoRegister *> * globals );

        bool_t allocateConst( ConstPseudoRegister * r, Location preferred = unAllocated );


        int nofStackTemps() {
            return _stackLocs->length();
        }
};


extern RegisterAllocator * theAllocator;


// helper structure for local register allocation
class RegCandidate : public ResourceObject {

    public:
        PseudoRegister * _pseudoRegister;    // PseudoRegister to be allocated
        Location       _location;           // possible location for it
        int            _ndefs;              // required # definitions of loc

    public:
        RegCandidate( PseudoRegister * reg, Location l, int n ) {
            _pseudoRegister = reg;
            _location       = l;
            _ndefs          = n;
        }
};

class RegisterEqClass : public ResourceObject {

    public:
        // holds PseudoRegisters belonging to equivalence class
        // PseudoRegisters are linked through regClassLink
        PseudoRegister * first;
        PseudoRegister * last;

        RegisterEqClass( PseudoRegister * f );

        void append( PseudoRegister * other );
};

