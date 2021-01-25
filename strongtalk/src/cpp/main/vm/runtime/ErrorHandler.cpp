//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ErrorHandler.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/lookup/LookupCache.hpp"


// The following variables are used to do NonLocalReturns through C code
//extern "C" bool_t     have_nlr_through_C;
//extern "C" Oop        nlr_result;
//extern "C" std::int32_t        nlr_home;
//extern "C" std::int32_t        nlr_home_id;
//extern "C" ContextOop nlr_home_context;

extern "C" {
bool_t     have_nlr_through_C = false;
Oop        nlr_result;
std::int32_t        nlr_home;
std::int32_t        nlr_home_id;
ContextOop nlr_home_context;
}


void ErrorHandler::abort_compilation() {
    Unimplemented();
}


typedef Oop (xxx_nlr_at_func)( std::int32_t *frame_pointer, Oop *stack_pointer );


void ErrorHandler::abort_current_process() {
    xxx_nlr_at_func *provoke_nlr_at = (xxx_nlr_at_func *) StubRoutines::provoke_nlr_at();
    nlr_home    = 0;
    nlr_home_id = aborting_nlr_home_id();
    nlr_result  = smiOop_zero;
    provoke_nlr_at( DeltaProcess::active()->last_Delta_fp(), DeltaProcess::active()->last_Delta_sp() );
    ShouldNotReachHere();
}

//extern "C" void continue_nlr_in_delta(std::int32_t* frame_pointer, Oop* stack_pointer);

void ErrorHandler::continue_nlr_in_delta() {
    xxx_nlr_at_func *_continue_nlr_in_delta = (xxx_nlr_at_func *) StubRoutines::continue_nlr_in_delta();
    _continue_nlr_in_delta( DeltaProcess::active()->last_Delta_fp(), DeltaProcess::active()->last_Delta_sp() );
    ShouldNotReachHere();
}


void ErrorHandler::genesis() {
    Processes::kill_all();
    LookupCache::flush();
    Universe::flush_inline_caches_in_methods();
    Processes::start( new VMProcess );
}
