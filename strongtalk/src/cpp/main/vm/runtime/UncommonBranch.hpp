//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/ResourceObject.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/oop/ContextOopDescriptor.hpp"
#include "vm/runtime/StackChunkBuilder.hpp"


// Run-time system code to handle uncommon traps
//
// An uncommon trap can be in two states:
//  - unused; when it has never been executed.
//  - used;   when the trap has been executed before.
//
// Initially a uncommon trap is in the unused state
// but the first time it is executed the state will be changed to used.
// The state is reflected in the call destination. See StubRoutines.hpp.

// This routine is called by either used_uncommon_trap & unused_uncommon_trap
// (see StubRoutines.hpp).

extern void uncommon_trap();



// -----------------------------------------------------------------------------

class FrameAndContextElement : public ResourceObject {
public:
    Frame      _frame;
    ContextOop _context;


    FrameAndContextElement( Frame *f, ContextOop c ) :
        _frame{ *f },
        _context{ c } {
    }

    FrameAndContextElement() = default;
    virtual ~FrameAndContextElement() = default;
    FrameAndContextElement( const FrameAndContextElement & ) = default;
    FrameAndContextElement &operator=( const FrameAndContextElement & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


};


// -----------------------------------------------------------------------------

class EnableDeoptimization : StackAllocatedObject {
public:
    EnableDeoptimization() {
        StackChunkBuilder::begin_deoptimization();
    }


    ~EnableDeoptimization() {
        StackChunkBuilder::end_deoptimization();
    }
};
