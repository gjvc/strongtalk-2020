//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"


// The ErrorHandler takes care of error recovery

class ErrorHandler : AllStatic {
public:
    static void abort_compilation();      // Aborts the current compilation and continues the process execution

    static void abort_current_process();      // Aborts the current process and terminates it

    static void continue_nlr_in_delta();      // Continues the NonLocalReturn at the next Delta frame

    static void genesis(); // Aborts all processes and restarts the scheduler


    // Returns the value used during an abort
    static std::int32_t aborting_nlr_home_id() {
        return 0xcafebabe;
    }

};
