
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/runtime/DebugInfo.hpp"
#include "vm/runtime/UnwindInfo.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/runtime/VirtualFrame.hpp"


//
void trace_stack_at_exception( std::int32_t *sp, std::int32_t *fp, const char *pc );
void suspend_process_at_stack_overflow( std::int32_t *sp, std::int32_t *fp, const char *pc );

//
// Class hierarchy
// - Process
//   - VMProcess
//   - DeltaProcess
//


class Thread;

class Event;

class VM_Operation;


class Process : public PrintableCHeapAllocatedObject {

public:

    Process() :
        _thread{ nullptr },
        _thread_id{ 0 },
        _event{ nullptr },
        _stack_limit{ nullptr } {
    }

    virtual ~Process() = default;
    Process( const Process & ) = default;
    Process &operator=( const Process & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    virtual bool is_vmProcess() const {
        return false;
    }


    virtual bool is_deltaProcess() const {
        return false;
    }


    virtual void resetStepping() {
    };


    virtual void applyStepping() {
    };

    void abort();


    std::int32_t thread_id() const {
        return _thread_id;
    }


    static bool external_suspend_current();

    static void external_resume_current();


    // sets the current process
    static void set_current( Process *p ) {
        _current_process = p;
    }


    // returns the running process DeltaProcess or VMProcess
    static Process *current() {
        return _current_process;
    }




protected:
    // transfer to control
    void basic_transfer( Process *target );

    // OS data associated with the process
    Thread         *_thread;            // Native thread
    std::int32_t   _thread_id;          // Native thread id (set by OS when created)
    Event          *_event;             // Thread lock
    char           *_stack_limit;       // lower limit of stack
    static Process *_current_process;   // active Delta process or vm process

};


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



extern "C" const char *active_stack_limit();
extern "C" void check_stack_overflow();

// "semaphore" to protect some vm critical sections (process transfer etc.)
extern "C" bool processSemaphore;

extern "C" std::int32_t *last_delta_fp;
extern "C" Oop          *last_delta_sp;

extern std::int32_t CurrentHash;

extern "C" void set_stack_overflow_for( DeltaProcess *currentProcess );

// error handling routines called from compiled code

extern "C" void suspend_on_NonLocalReturn_error();


enum class InterpreterErrorConstants {

    start_of_runtime_system_errors = 512,                                   //
    primitive_lookup_failed        = 1 + start_of_runtime_system_errors,    //
    boolean_expected               = 2 + start_of_runtime_system_errors,    //
    nonlocal_return_error          = 3 + start_of_runtime_system_errors,    //
    halted                         = 4 + start_of_runtime_system_errors,    //
    illegal_code                   = 5 + start_of_runtime_system_errors,    //
    not_implemented                = 6 + start_of_runtime_system_errors,    //
    stack_misaligned               = 7 + start_of_runtime_system_errors,    //
    ebx_wrong                      = 8 + start_of_runtime_system_errors,    //
    obj_wrong                      = 9 + start_of_runtime_system_errors,    //
    nlr_offset_wrong               = 10 + start_of_runtime_system_errors,   //
    last_delta_fp_wrong            = 11 + start_of_runtime_system_errors,   //
    primitive_result_wrong         = 12 + start_of_runtime_system_errors,   //
    float_expected                 = 13 + start_of_runtime_system_errors,   //

};

void trace_stack( std::int32_t thread_id );
