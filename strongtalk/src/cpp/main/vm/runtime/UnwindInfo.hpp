//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"

class CompiledVirtualFrame;

// UnwindInfo is a wrapper calls allowing a primitive like unwindprotect to call delta even though a non local return is in progress.

class UnwindInfo : public StackAllocatedObject {

private:
    // NonLocalReturn state
    std::int32_t _nlr_home;
    std::int32_t _nlr_home_id;
    ContextOop   _nlr_home_context;

public:
    Oop _nlr_result;    //

private:

    bool _is_compiled;  //

    UnwindInfo *_next;  // Link to next UnwindInfo

    // Return address patch state
    char *saved_C_frame_return_addr;                //
    char **saved_C_frame_return_addr_location;      //
    char *saved_patch_return_address;               //

public:
    UnwindInfo();

    ~UnwindInfo();

    UnwindInfo *next() const;

    void set_next( UnwindInfo *next );

    std::int32_t nlr_home() const;

    std::int32_t nlr_home_id() const;

    ContextOop nlr_home_context() const;

    void update_nlr_targets( CompiledVirtualFrame *f, ContextOop con );
};
