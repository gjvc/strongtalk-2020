//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/UnwindInfo.hpp"
#include "vm/system/asserts.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/code/ScopeDescriptor.hpp"


extern "C" bool         have_nlr_through_C;
extern "C" char         *C_frame_return_addr;
extern "C" std::int32_t nlr_home;
extern "C" std::int32_t nlr_home_id;
extern "C" Oop          nlr_result;


UnwindInfo::UnwindInfo() {
    st_assert( have_nlr_through_C, "you must have have_nlr_through_C before using unwindInfo" );

    // Save NonLocalReturn state
    _nlr_home    = ::nlr_home;
    _nlr_home_id = ::nlr_home_id;
    _nlr_result  = ::nlr_result;

    // Save patch information
    st_assert( last_Delta_fp, "last_Delta_fp must be set" );
    saved_C_frame_return_addr          = C_frame_return_addr;
    saved_C_frame_return_addr_location = (char **) ( last_Delta_sp - 1 );
    saved_patch_return_address         = *saved_C_frame_return_addr_location;

    // Restore original return address
    *saved_C_frame_return_addr_location = saved_C_frame_return_addr;

    _is_compiled = _nlr_home_id >= 0;
    DeltaProcess::active()->push_unwind( this );
}


UnwindInfo::~UnwindInfo() {
    // If we get an aborting NonLocalReturn in the protect part we should continue the aborting NonLocalReturn
    // and not the original NonLocalReturn.
    if ( ::nlr_home not_eq 0 ) {
        // Restore NonLocalReturn state
        ::nlr_home    = _nlr_home;
        ::nlr_home_id = _nlr_home_id;
        ::nlr_result  = _nlr_result;
    }
    // Restore patch information
    *saved_C_frame_return_addr_location = saved_patch_return_address;
    C_frame_return_addr = saved_C_frame_return_addr;

    DeltaProcess::active()->pop_unwind();
}


void UnwindInfo::update_nlr_targets( CompiledVirtualFrame *f, ContextOop con ) {
    // Convert the nlr information if:
    //    nlr_home     is the frame pointer of f
    // and nlr_home_id  is the offset of f's scope
    if ( f->fr().fp() == (std::int32_t *) nlr_home() and f->scope()->offset() == nlr_home_id() ) {
        _nlr_home_context = con;
    }
}


UnwindInfo *UnwindInfo::next() const {
    return _next;
}


void UnwindInfo::set_next( UnwindInfo *next ) {
    _next = next;
}


std::int32_t UnwindInfo::nlr_home() const {
    return _nlr_home;
}


std::int32_t UnwindInfo::nlr_home_id() const {
    return _nlr_home_id;
}


ContextOop UnwindInfo::nlr_home_context() const {
    return _nlr_home_context;
}
