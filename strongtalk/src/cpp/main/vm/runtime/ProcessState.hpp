//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


enum class ProcessState {
    initialized,                // State right after creation.
    running,                    // The process is running.
    yielded,                    // Gave up control by calling yield.
    stopped,                    // Gave up control by calling stop.
    preempted,                  // Was preempted by system.
    uncommon,                   // currently handling an uncommon branch
    in_async_dll,               // currently executing an asynchronous dll call
    yielded_after_async_dll,    // completed execution of asynchronous dll call
    completed,                  // Ran to completion.
    boolean_error,              // A boolean was expected at hardcoded control structure.
    lookup_error,               // The receiver does not understand doesNotUnderstand:.
    primitive_lookup_error,     // Binding a primitive failed.
    DLL_lookup_error,           // Binding a DLL function failed.
    float_error,                // A float was expected at hardcoded float operation.
    NonLocalReturn_error,       // Context for NonLocalReturn did not exist.
    stack_overflow,             // Stack exhausted.
    aborted                     // Process has been aborted
};
