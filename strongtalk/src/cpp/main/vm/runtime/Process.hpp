//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/runtime/DebugInfo.hpp"
#include "vm/runtime/UnwindInfo.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/runtime/VirtualFrame.hpp"


// Class hierarchy
// - Process
//   - VMProcess
//   - DeltaProcess

class Thread;
class Event;
class VM_Operation;


class Process : public PrintableCHeapAllocatedObject {

    public:

        virtual bool_t is_vmProcess() const {
            return false;
        }


        virtual bool_t is_deltaProcess() const {
            return false;
        }


        virtual void resetStepping() {
        };


        virtual void applyStepping() {
        };

        void abort();


        int thread_id() const {
            return _thread_id;
        }


        static bool_t external_suspend_current();

        static void external_resume_current();


        // sets the current process
        static void set_current( Process * p ) {
            _current_process = p;
        }


        // returns the running process DeltaProcess or VMProcess
        static Process * current() {
            return _current_process;
        }


    protected:
        // transfer to control
        void basic_transfer( Process * target );

        // OS data associated with the process
        Thread         * _thread;            // Native thread
        int            _thread_id;          // Native thread id (set by OS when created)
        Event          * _event;             // Thread lock
        char           * _stack_limit;       // lower limit of stack
        static Process * _current_process;   //  active Delta process or vm process
};


// A single VMProcess (the default thread) is used for heavy vm operations like scavenge, garbage_collect etc.
class VMProcess : public Process {

    public:
        VMProcess();


        // tester
        bool_t is_vmProcess() const {
            return true;
        }


        // activates the virtual machine
        void activate_system();

        // the ever running loop for the VMProcess
        void loop();

        // transfer
        void transfer_to( DeltaProcess * target_process );

        // misc.
        void print();

        // Terminate
        static void terminate( DeltaProcess * proc );

        // execution of vm operation
        static void execute( VM_Operation * op );


        // returns the current vm operation if any.
        static VM_Operation * vm_operation() {
            return _vm_operation;
        }


        // returns the single instance of VMProcess.
        static VMProcess * vm_process() {
            return _vm_process;
        }


    private:
        static VM_Operation * _vm_operation;
        static VMProcess    * _vm_process;
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



extern "C" const char * active_stack_limit();
extern "C" void check_stack_overflow();

class DeltaProcess : public Process {

    private:
        Oop          _receiver;      // receiver of the initial message.
        SymbolOop    _selector;      // selector of the initial message.
        DeltaProcess * _next;          // the next process in the list (see Processes).
        ProcessOop   _processObj;    // the Delta level process object.
        ProcessState _state;         // process state.

        int        * _last_Delta_fp;
        Oop        * _last_Delta_sp;
        const char * _last_Delta_pc;      // For now only used for stack overflow

        volatile bool_t _is_terminating;

        int       _time_stamp;
        DebugInfo _debugInfo;               // debug info used while stepping
        bool_t    _isCallback;

        friend class VMProcess;

    public:
        static bool_t stepping;

        DeltaProcess( Oop receiver, SymbolOop selector, bool_t createThread = true );

        virtual ~DeltaProcess();

        void setIsCallback( bool_t isCallback );

        virtual void applyStepping();

        virtual void resetStepping();

        void setupSingleStep();

        void setupStepNext( int * fr );

        void setupStepReturn( int * fr );

        void resetStep();

        void returnToDebugger();

        // testers
        bool_t is_deltaProcess() const;

        bool_t isUncommon() const;

        // Accessors
        Oop receiver() const;

        SymbolOop selector() const;

        // process chain operations
        DeltaProcess * next() const;

        void set_next( DeltaProcess * p );

        // process Oop
        ProcessOop processObj() const;

        void set_processObj( ProcessOop p );

        bool_t is_terminating();

        void set_terminating();

        int time_stamp() const;

        void inc_time_stamp();

        // last_Delta_fp
        int * last_Delta_fp() const;

        void set_last_Delta_fp( int * fp );

        // last_Delta_sp
        Oop * last_Delta_sp() const;

        void set_last_Delta_sp( Oop * sp );

        // last_Delta_pc
        const char * last_Delta_pc() const;

        void set_last_Delta_pc( const char * pc );

        ProcessState state() const;

        SymbolOop status_symbol() const;

        static SymbolOop symbol_from_state( ProcessState state );

        // List of active unwind protect activations for this process.
        // We need this list for deoptimization.
        // If a compiled frame is replaced with a deoptimized frame and the unwind protect has nlr_target in the compiled frame, we have to convert the nlr information to match the deoptimized frame.

    public:
        void push_unwind( UnwindInfo * info );

        void pop_unwind();

        void unwinds_do( void f( UnwindInfo * info ) );

        void update_nlr_targets( CompiledVirtualFrame * f, ContextOop con );

    private:
        UnwindInfo * _unwind_head; // points to the most recent unwind protect activation.
        BaseHandle * _firstHandle;

    private:
        void set_state( ProcessState s ) {
            _state = s;
        }


    public:
        // State operations
        BaseHandle * firstHandle();

        void setFirstHandle( BaseHandle * handle );

        // returns whether this process has a stack.
        bool_t has_stack() const;

        // returns whether this process is ready for execution.
        bool_t is_ready() const;

        // Memory operations
        void follow_roots();

        // Iterator
        void frame_iterate( FrameClosure * blk );

        void oop_iterate( OopClosure * blk );

        // Deoptimization
        virtual void deoptimized_wrt_marked_native_methods();

        void deoptimize_stretch( Frame * first_frame, Frame * last_frame );

        static void deoptimize_redo_last_send();

        // Uncommon branches
        void enter_uncommon();

        void exit_uncommon();

        // Misc. operations
        void print();

        void verify();

        // Stack operations
        Frame last_frame();

        DeltaVirtualFrame * last_delta_vframe();

        // Print the stack trace
        void trace_stack();

        void trace_top( int start_frame, int number_of_frames );

        void trace_stack_for_deoptimization( Frame * f = nullptr );

        // Print the stack starting at top_frame
        static void trace_stack_from( VirtualFrame * top_frame );

        // Timing
        double user_time();

        double system_time();

        int depth();

        int vdepth( Frame * f = nullptr );

        // Debugging state
        bool_t stopping;    // just returned from "finish" operation; stop ASAP

        // Profiling operation (see fprofile.cpp)
        Frame profile_top_frame();

    private:
        void transfer( ProcessState reason, DeltaProcess * destination );

    public:

        // returns whether this process is the active delta process.
        bool_t is_active() const;

        // returns whether this process is the scheduler.
        bool_t is_scheduler() const;

        // returns whether this process currently is executing a vm_operation.
        bool_t in_vm_operation() const;

        // transfer control to the scheduler.
        void suspend( ProcessState reason );

        void suspend_at_creation();

        // asynchronous dll support
        void transfer_and_continue();

        void wait_for_control();

        // transfers control from the scheduler.
        ProcessState transfer_to( DeltaProcess * target );

        // transfers control to vm process.
        void transfer_to_vm();

        // Static operations
    private:
        static DeltaProcess    * _active_delta_process;
        static DeltaProcess    * _main_process;
        static DeltaProcess    * _scheduler_process;
        static bool_t          _is_idle;
        static volatile char   * _active_stack_limit;    //
        static volatile bool_t _interrupt;              //

        // The launch function for a new thread
        static int launch_delta( DeltaProcess * process );

    public:
        // sets the active process (note: public only to support testing)
        static void set_active( DeltaProcess * p );

        // sets the scheduler process
        static void set_scheduler( DeltaProcess * p );

        // returns the active delta process
        static DeltaProcess * active();

        static void set_main( DeltaProcess * p );

        // returns the process representing the main thread
        static DeltaProcess * main();

        // tells whether the system is idle (waiting in wait_for_async_dll).
        static bool_t is_idle();

        // returns the scheduler process
        static DeltaProcess * scheduler();

        static void set_terminating_process( ProcessState state );

        static bool_t process_has_terminated();

        static ProcessState state_of_terminated_process();

    private:
        static volatile bool_t _process_has_terminated;
        static ProcessState    _state_of_terminated_process;

        static void check_stack_overflow();

    public:
        // Called whenever a async dll call completes
        static void async_dll_call_completed();

        static void initialize_async_dll_event();

        // Waits for a completed async call or timeout.
        // Returns whether the timer expired.
        static bool_t wait_for_async_dll( int timeout_in_ms );

        static void preempt_active();

        // create and run the main process - ie. the process for the initial thread
        static void createMainProcess();

        static void runMainProcess();

    private:
        // Event for waking up the process scheduler when a async dll completes
        static Event * _async_dll_completion_event;

        friend class VMProcess; // to allow access to _process_has_terminated
        friend class InterpreterGenerator;

        friend const char * active_stack_limit();

        friend void check_stack_overflow();

        friend class StackHandle;
};


class Processes : AllStatic {

    private:
        static DeltaProcess * _processList;

    public:
        // Process management
        static void add( DeltaProcess * p );

        static void remove( DeltaProcess * p );

        static bool_t includes( DeltaProcess * p );

        static DeltaProcess * last();

        static DeltaProcess * find_from_thread_id( int id );

        // Start the vm process
        static void start( VMProcess * p );

        // State
        static bool_t has_completed_async_call();

        // Killing
        static void kill_all();

        // Iterating
        static void frame_iterate( FrameClosure * blk );

        static void oop_iterate( OopClosure * blk );

        static void process_iterate( ProcessClosure * blk );

        // Scavenge
        static void scavenge_contents();

        // Garbage collection
        static void follow_roots();

        static void convert_heap_code_pointers();

        static void restore_heap_code_pointers();

        // Verifycation
        static void verify();

        static void print();

        // Deoptimization

    public:
        // deoptimizes frames dependent on a NativeMethod
        static void deoptimize_wrt( NativeMethod * nm );

        // deoptimizes frames dependent on at least one NativeMethod in the list
        static void deoptimize_wrt( GrowableArray <NativeMethod *> * list );

        // deoptimizes all frames
        static void deoptimize_all();

        // deoptimizes all frames tied to marked nativeMethods
        static void deoptimized_wrt_marked_nativeMethods();

        // deoptimization support for NonLocalReturn
        static void update_nlr_targets( CompiledVirtualFrame * f, ContextOop con );
};


// "semaphore" to protect some vm critical sections (process transfer etc.)
extern "C" bool_t processSemaphore;

extern "C" int * last_Delta_fp;
extern "C" Oop * last_Delta_sp;

extern int CurrentHash;

extern "C" void set_stack_overflow_for( DeltaProcess * currentProcess );

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
    stack_missaligned              = 7 + start_of_runtime_system_errors,    //
    ebx_wrong                      = 8 + start_of_runtime_system_errors,    //
    obj_wrong                      = 9 + start_of_runtime_system_errors,    //
    nlr_offset_wrong               = 10 + start_of_runtime_system_errors,   //
    last_Delta_fp_wrong            = 11 + start_of_runtime_system_errors,   //
    primitive_result_wrong         = 12 + start_of_runtime_system_errors,   //
    float_expected                 = 13 + start_of_runtime_system_errors,   //
};

void trace_stack( int thread_id );
