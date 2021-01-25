//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"

#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/memory/util.hpp"


class DebugInfo : public ValueObject {

private:
    void (*_interceptorEntryPoint)( std::int32_t * );     // entry point of the dispatch table entry point
    std::int32_t *_frameBreakpoint;                       // pointer to the target frame for stepping, if any

public:
    DebugInfo() :
            _interceptorEntryPoint( nullptr ), _frameBreakpoint( nullptr ) {
    }


    void interceptForStep();

    void interceptForNext( std::int32_t *fr );

    void interceptForReturn( std::int32_t *fr );

    void apply();

    void reset();


    void resetInterceptor() {
        _interceptorEntryPoint = nullptr; // TODO: should we use breakpoint() or similar?
        _frameBreakpoint       = nullptr;
    }

};
