//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/DebugInfo.hpp"
#include "vm/interpreter/DispatchTable.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/Process.hpp"


void DebugInfo::interceptForStep() {
    _interceptorEntryPoint = &DispatchTable::intercept_for_step;
    _frameBreakpoint       = nullptr;
}


void DebugInfo::interceptForNext( std::int32_t *fr ) {
    _interceptorEntryPoint = &DispatchTable::intercept_for_next;
    _frameBreakpoint       = fr;
}


void DebugInfo::interceptForReturn( std::int32_t *fr ) {
    _interceptorEntryPoint = &DispatchTable::intercept_for_return;
    _frameBreakpoint       = fr;
}


extern "C" void returnToDebugger() {
    if ( not DeltaProcess::active()->is_scheduler() ) {
        DeltaProcess::active()->returnToDebugger();
    }
}


void DebugInfo::apply() {
    if ( _interceptorEntryPoint ) {
        StubRoutines::setSingleStepHandler( &returnToDebugger );
        _interceptorEntryPoint( _frameBreakpoint );
        DeltaProcess::stepping = true;
    }
}


void DebugInfo::reset() {
    if ( _interceptorEntryPoint ) {
        DispatchTable::reset();
        StubRoutines::setSingleStepHandler( nullptr ); // TODO: replace with breakpoint() or similar?
        DeltaProcess::stepping = false;
    }
}
