//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include <vm/runtime/ResourceObject.hpp>
#include <vm/assembler/Location.hpp>
//#include <vm/compiler/BitVector.hpp>


// A RegisterMask tells the scavenger which registers / stack locs are
// live.  (One bit per location.)

class RegisterMask : ResourceObject {
    BitVector *bv;
public:
    LongRegisterMask();
    void allocate( Location l );
    void deallocate( Location l );
    bool isAllocated( Location l );
    RegisterMask regs();        // returns mask for registers
    void print();

}}

RegisterMask &allocate( RegisterMask &s1, RegisterMask s2 ) {
    setBits( s1, s2 );
    return s1;
}


bool isAllocated( RegisterMask s, Location l ) {
    Unimplemented();
    // return isBitSet(s, l);
    return false;
}


RegisterMask &deallocate( RegisterMask &s1, RegisterMask s2 ) {
    clearBits( s1, s2 );
    return s1;
}


RegisterMask &allocateRegister( RegisterMask &s, Location r ) {
    Unimplemented();
    // if (isRegister(r)) setNthBit(s, r);
    return s;
}


RegisterMask &deallocateRegister( RegisterMask &s1, Location r ) {
    Unimplemented();
    // assert(isRegister(r), "not a register");
    // clearNthBit(s1, r);
    return s1;
}


Location pick( RegisterMask &alloc, RegisterMask mask = ~0 );

void printRegister( Location r );
void printAllocated( RegisterMask r );

// like a RegisterMask, but has arbitrary length (i.e., bits for stack temps and regs)

class LongRegisterMask : ResourceObject {
    BitVector *bv;
public:
    LongRegisterMask();
    void allocate( Location l );
    void deallocate( Location l );
    bool isAllocated( Location l );
    RegisterMask regs();        // returns mask for registers
    void print();

private:
    void grow();
    friend std::int32_t findFirstUnused( LongRegisterMask **masks, std::int32_t len, std::int32_t start );
};

Location findFirstUnusedTemp( LongRegisterMask **masks, std::int32_t len );
