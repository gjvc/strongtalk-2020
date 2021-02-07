
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/Process.hpp"


//
class DeltaProcess : public Process {

private:
    Oop          _receiver;      // receiver of the initial message.
    SymbolOop    _selector;      // selector of the initial message.
    DeltaProcess *_next;          // the next process in the list (see Processes).
    ProcessOop   _processObject;    // the Delta level process object.
    ProcessState _state;         // process state.

    std::int32_t *_last_Delta_fp;
    Oop          *_last_Delta_sp;
    const char   *_last_Delta_pc;      // For now only used for stack overflow

    volatile bool _is_terminating;

    std::int32_t _time_stamp;
    DebugInfo    _debugInfo;               // debug info used while stepping
    bool         _isCallback;

    friend class VMProcess;

public:
    static bool stepping;

    DeltaProcess( Oop receiver, SymbolOop selector, bool createThread = true );
    DeltaProcess() = default;
    virtual ~DeltaProcess();
    DeltaProcess( const DeltaProcess & ) = default;
    DeltaProcess &operator=( const DeltaProcess & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    void setIsCallback( bool isCallback );

    virtual void applyStepping();

    virtual void resetStepping();

    void setupSingleStep();

    void setupStepNext( std::int32_t *fr );

    void setupStepReturn( std::int32_t *fr );

    void resetStep();

    void returnToDebugger();

    // testers
    bool is_deltaProcess() const;

    bool isUncommon() const;

    // Accessors
    Oop receiver() const;

    SymbolOop selector() const;

    // process chain operations
    DeltaProcess *next() const;

    void set_next( DeltaProcess *p );

    // process Oop
    ProcessOop processObject() const;

    void set_processObject( ProcessOop p );

    bool is_terminating();

    void set_terminating();

    std::int32_t time_stamp() const;

    void inc_time_stamp();

    // last_Delta_fp
    std::int32_t *last_Delta_fp() const;

    void set_last_Delta_fp( std::int32_t *fp );

    // last_Delta_sp
    Oop *last_Delta_sp() const;

    void set_last_Delta_sp( Oop *sp );

    // last_Delta_pc
    const char *last_Delta_pc() const;

    void set_last_Delta_pc( const char *pc );

    ProcessState state() const;

    SymbolOop status_symbol() const;

    static SymbolOop symbol_from_state( ProcessState state );

    // List of active unwind protect activations for this process.
    // We need this list for deoptimization.
    // If a compiled frame is replaced with a deoptimized frame and the unwind protect has nlr_target in the compiled frame, we have to convert the nlr information to match the deoptimized frame.

public:
    void push_unwind( UnwindInfo *info );

    void pop_unwind();

    void unwinds_do( void f( UnwindInfo *info ) );

    void update_nlr_targets( CompiledVirtualFrame *f, ContextOop con );

private:
    UnwindInfo *_unwind_head; // points to the most recent unwind protect activation.
    BaseHandle *_firstHandle;

private:
    void set_state( ProcessState s ) {
        _state = s;
    }


public:
    // State operations
    BaseHandle *firstHandle();

    void setFirstHandle( BaseHandle *handle );

    // returns whether this process has a stack.
    bool has_stack() const;

    // returns whether this process is ready for execution.
    bool is_ready() const;

    // Memory operations
    void follow_roots();

    // Iterator
    void frame_iterate( FrameClosure *blk );

    void oop_iterate( OopClosure *blk );

    // Deoptimization
    virtual void deoptimized_wrt_marked_native_methods();

    void deoptimize_stretch( Frame *first_frame, Frame *last_frame );

    static void deoptimize_redo_last_send();

    // Uncommon branches
    void enter_uncommon();

    void exit_uncommon();

    // Misc. operations
    void print();

    void verify();

    // Stack operations
    Frame last_frame();

    DeltaVirtualFrame *last_delta_vframe();

    // Print the stack trace
    void trace_stack();

    void trace_top( std::int32_t start_frame, std::int32_t number_of_frames );

    void trace_stack_for_deoptimization( Frame *f = nullptr );

    // Print the stack starting at top_frame
    static void trace_stack_from( VirtualFrame *top_frame );

    // Timing
    double user_time();

    double system_time();

    std::int32_t depth();

    std::int32_t vdepth( Frame *f = nullptr );

    // Debugging state
    bool stopping;    // just returned from "finish" operation; stop ASAP

    // Profiling operation (see fprofile.cpp)
    Frame profile_top_frame();

private:
    void transfer( ProcessState reason, DeltaProcess *destination );

public:

    // returns whether this process is the active delta process.
    bool is_active() const;

    // returns whether this process is the scheduler.
    bool is_scheduler() const;

    // returns whether this process currently is executing a vm_operation.
    bool in_vm_operation() const;

    // transfer control to the scheduler.
    void suspend( ProcessState reason );

    void suspend_at_creation();

    // asynchronous dll support
    void transfer_and_continue();

    void wait_for_control();

    // transfers control from the scheduler.
    ProcessState transfer_to( DeltaProcess *target );

    // transfers control to vm process.
    void transfer_to_vm();

    // Static operations
private:
    static DeltaProcess  *_active_delta_process;
    static DeltaProcess  *_main_process;
    static DeltaProcess  *_scheduler_process;
    static bool          _is_idle;
    static volatile char *_active_stack_limit;    //
    static volatile bool _interrupt;              //

    // The launch function for a new thread
    static std::int32_t launch_delta( DeltaProcess *process );

public:
    // sets the active process (note: public only to support testing)
    static void set_active( DeltaProcess *p );

    // sets the scheduler process
    static void set_scheduler( DeltaProcess *p );

    // returns the active delta process
    static DeltaProcess *active();

    static void set_main( DeltaProcess *p );

    // returns the process representing the main thread
    static DeltaProcess *main();

    // tells whether the system is idle (waiting in wait_for_async_dll).
    static bool is_idle();

    // returns the scheduler process
    static DeltaProcess *scheduler();

    static void set_terminating_process( ProcessState state );

    static bool process_has_terminated();

    static ProcessState state_of_terminated_process();

private:
    static volatile bool _process_has_terminated;
    static ProcessState  _state_of_terminated_process;

    static void check_stack_overflow();

public:
    // Called whenever a async dll call completes
    static void async_dll_call_completed();

    static void initialize_async_dll_event();

    // Waits for a completed async call or timeout.
    // Returns whether the timer expired.
    static bool wait_for_async_dll( std::int32_t timeout_in_ms );

    static void preempt_active();

    // create and run the main process - ie. the process for the initial thread
    static void createMainProcess();

    static void runMainProcess();

private:
    // Event for waking up the process scheduler when a async dll completes
    static Event *_async_dll_completion_event;

    friend class VMProcess; // to allow access to _process_has_terminated
    friend class InterpreterGenerator;

    friend const char *active_stack_limit();

    friend void check_stack_overflow();

    friend class StackHandle;
};
