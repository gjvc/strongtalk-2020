//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"


// Label represents a target destination for jumps, calls and non-local returns.
// After declaration they can be freely used to denote known or (yet) unknown
// target destinations.
// Assembler::bind is used to bind a label to the current
// code position. A label can be bound only once.


// Labels refer to positions in the (to be) generated code. There are bound, unbound, and undefined labels.
//
// Bound labels refer to known positions in the already generated code. pos() is the position the label refers to.
//
// Unbound labels refer to unknown positions in the code to be generated; pos() is the position of the 32bit Displacement of the last instruction using the label.
//
// Undefined labels are labels that haven't been used yet. They refer to no position at all.


class Label : public ValueObject {

private:
    // _pos encodes both the binding state (via its sign)
    // and the binding position (via its value) of a label.
    //
    // _pos <  0	bound label, pos() returns the target (jump) position
    // _pos == 0	unused label
    // _pos >  0	unbound label, pos() returns the last displacement (see .cpp file) in the chain
    int _pos;


    int pos() const;


    void bind_to( int pos );

    void link_to( int pos );

    void unuse();


public:
    bool_t is_bound() const;

    bool_t is_unbound() const;

    bool_t is_unused() const;


    Label();

    ~Label();


    friend class Assembler;

    friend class MacroAssembler;

    friend class Displacement;
};
