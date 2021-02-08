
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Process.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/system/os.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/runtime/StackChunkBuilder.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/ErrorHandler.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/primitives/primitives.hpp"


// The tricky part is to restore the original return address of the primitive before the delta call.
// This is necessary for a consistent stack during the delta call.

//
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wmissing-noreturn"
//

// Interpreter entry point for redoing a send.
//extern "C" void redo_bytecode_after_deoptimization();
//extern "C" char redo_bytecode_after_deoptimization;

extern "C" {
bool         nlr_through_unpacking                 = false;
Oop          result_through_unpacking              = nullptr;
std::int32_t number_of_arguments_through_unpacking = 0;
char         *C_frame_return_addr                  = nullptr;

extern ContextOop   nlr_home_context;
extern bool         have_nlr_through_C;
extern std::int32_t nlr_home;
extern std::int32_t nlr_home_id;
extern Oop          nlr_result;
}


DeltaProcess  *DeltaProcess::_active_delta_process = nullptr;
DeltaProcess  *DeltaProcess::_main_process         = nullptr;
volatile char *DeltaProcess::_active_stack_limit   = nullptr;
DeltaProcess  *DeltaProcess::_scheduler_process    = nullptr;
bool          DeltaProcess::_is_idle               = false;
volatile bool DeltaProcess::_interrupt             = false;

volatile bool   DeltaProcess::_process_has_terminated      = false;
ProcessState    DeltaProcess::_state_of_terminated_process = ProcessState::initialized;

Event *DeltaProcess::_async_dll_completion_event = nullptr;


bool processSemaphore = false;

// For current Delta process, the last FP/Sp is stored in these global vars, not the instance vars of the process
std::int32_t *last_Delta_fp = nullptr;
Oop          *last_Delta_sp = nullptr;


// last_Delta_fp
std::int32_t *DeltaProcess::last_Delta_fp() const {
    return this == _active_delta_process ? ::last_Delta_fp : _last_Delta_fp;
}


void DeltaProcess::set_last_Delta_fp( std::int32_t *fp ) {
    if ( this == _active_delta_process ) {
        ::last_Delta_fp = fp;
    } else {
        _last_Delta_fp = fp;
    }
}


Oop *DeltaProcess::last_Delta_sp() const {
    return this == _active_delta_process ? ::last_Delta_sp : _last_Delta_sp;
}


void DeltaProcess::set_last_Delta_sp( Oop *sp ) {
    if ( this == _active_delta_process ) {
        ::last_Delta_sp = sp;
    } else {
        _last_Delta_sp = sp;
    }
}


const char *DeltaProcess::last_Delta_pc() const {
//    if ( this == nullptr )
//        return nullptr;
    return _last_Delta_pc;
}


void DeltaProcess::set_last_Delta_pc( const char *pc ) {
    _last_Delta_pc = pc;
}


void DeltaProcess::transfer( ProcessState reason, DeltaProcess *target ) {
    // change time_stamp for target
    target->inc_time_stamp();

    {
        ThreadCritical tc;

        st_assert( this == active(), "receiver must be the active process" );

        // save state
        _last_Delta_fp = ::last_Delta_fp;    // *don't* use accessors! (check their implementation to see why)
        _last_Delta_sp = ::last_Delta_sp;
        set_state( reason );

        // restore state
        ::last_Delta_fp = target->_last_Delta_fp;    // *don't* use accessors!
        ::last_Delta_sp = target->_last_Delta_sp;
        set_current( target );
        set_active( target );
        resetStepping();
    }

    // transfer
    basic_transfer( target );
}


void DeltaProcess::suspend( ProcessState reason ) {

    st_assert( not is_scheduler(), "active must be other than scheduler" );
    st_assert( not in_vm_operation(), "must not be in VM operation" );

    transfer( reason, scheduler() );
    if ( is_terminating() ) {
        ErrorHandler::abort_current_process();
    }

}


ProcessState DeltaProcess::transfer_to( DeltaProcess *destination ) {

    st_assert( is_scheduler(), "active must be scheduler" );
    st_assert( not in_vm_operation(), "must not be in VM operation" );

    // Do not transfer if destination process is execution DLL.
    if ( destination->state() == ProcessState::in_async_dll )
        return destination->state();

    transfer( ProcessState::yielded, destination );
    if ( process_has_terminated() ) {
        return state_of_terminated_process();
    }

    return destination->state();
}


void DeltaProcess::transfer_to_vm() {

    {
        ThreadCritical tc;

        st_assert( this == active(), "receiver must be the active process" );

        // save state
        _last_Delta_fp = ::last_Delta_fp;    // *don't* use accessors! (check their implementation to see why)
        _last_Delta_sp = ::last_Delta_sp;
        set_current( VMProcess::vm_process() );
        resetStepping();
    }
    basic_transfer( VMProcess::vm_process() );
}


void DeltaProcess::suspend_at_creation() {
    // This is called as soon a DeltaProcess is created
    // Let's wait until we're given the torch.
    SPDLOG_INFO( "status-delta-process-suspend-at-creation: thread_id [{}] waiting for event", this->thread_id() );
    os::wait_for_event( _event );
}


void DeltaProcess::transfer_and_continue() {

    {
        ThreadCritical tc;

        st_assert( not is_scheduler(), "active must be other than scheduler" );
        st_assert( not in_vm_operation(), "must not be in VM operation" );
        st_assert( this == active(), "receiver must be the active process" );

        // save state
        _last_Delta_fp = ::last_Delta_fp;    // *don't* use accessors! (check their implementation to see why)
        _last_Delta_sp = ::last_Delta_sp;
        set_state( ProcessState::in_async_dll );

        // restore state
        ::last_Delta_fp = scheduler()->_last_Delta_fp;    // *don't* use accessors!
        ::last_Delta_sp = scheduler()->_last_Delta_sp;
        set_current( scheduler() );
        set_active( scheduler() );

        if ( TraceProcessEvents ) {
            _console->print( "Async call: " );
            print();
            _console->print( "        to: " );
            scheduler()->print();
            _console->cr();
        }
    }

    os::transfer_and_continue( _thread, _event, scheduler()->_thread, scheduler()->_event );
}


bool DeltaProcess::wait_for_async_dll( std::int32_t timeout_in_ms ) {

    if ( not os::wait_for_event_or_timer( _async_dll_completion_event, 0 ) ) {
        os::reset_event( _async_dll_completion_event );
        return false;
    }

    if ( Processes::has_completed_async_call() ) {
        return true;
    }

    if ( TraceProcessEvents ) {
        SPDLOG_INFO( "Waiting for async {:d} ms", timeout_in_ms );
    }

    _is_idle = true;
    bool result = os::wait_for_event_or_timer( _async_dll_completion_event, timeout_in_ms );
    _is_idle = false;

    if ( not result ) {
        os::reset_event( _async_dll_completion_event );
    }

    if ( TraceProcessEvents ) {
        SPDLOG_INFO( result ? " {timeout}" : " {async}" );
    }

    return result;
}


void DeltaProcess::initialize_async_dll_event() {
    _async_dll_completion_event = os::create_event( false );
}


void DeltaProcess::async_dll_call_completed() {
    os::signal_event( _async_dll_completion_event );
}


void DeltaProcess::wait_for_control() {

    if ( TraceProcessEvents ) {
        _console->print( "*" );
    }

    set_state( ProcessState::yielded_after_async_dll );
    async_dll_call_completed();
    os::wait_for_event( _event );
    if ( is_terminating() ) {
        ErrorHandler::abort_current_process();
    }

}


extern "C" bool have_nlr_through_C;


void DeltaProcess::createMainProcess() {
    Oop          mainProcess  = Universe::find_global( "MainProcess" );
    SymbolOop    start        = oopFactory::new_symbol( "start" );
    DeltaProcess *thisProcess = new DeltaProcess( mainProcess, start, false );
    DeltaProcess::set_main( thisProcess );
    DeltaProcess::initialize_async_dll_event();
}


void DeltaProcess::runMainProcess() {
    DeltaProcess *mainProcess = DeltaProcess::main();
    st_assert( mainProcess, "main process has not been assigned yet" );
    launch_delta( mainProcess );
}


// Code entry point for at Delta process
std::int32_t DeltaProcess::launch_delta( DeltaProcess *process ) {

    SPDLOG_INFO( "delta-process-launch-delta-process:  thread_id [{:d}]", process->thread_id() );

    // Wait until we get the torch
    process->suspend_at_creation();

    // We have the torch
    st_assert( process == DeltaProcess::active(), "process consistency check" );
    st_assert( process->is_deltaProcess(), "this should be a deltaProcess" );

    DeltaProcess *p     = static_cast<DeltaProcess *>(process);
    Oop          result = Delta::call( p->receiver(), p->selector() );
    static_cast<void>( result );

    if ( have_nlr_through_C ) {
        if ( nlr_home_id == ErrorHandler::aborting_nlr_home_id() ) {
            p->set_state( ProcessState::aborted );
        } else {
            p->set_state( ProcessState::NonLocalReturn_error );
        }
    } else {
        p->set_state( ProcessState::completed );
    }
    st_assert( process == DeltaProcess::active(), "process consistency check" );

    VM_TerminateProcess op( process );
    VMProcess::execute( &op );

    return 0;
}


DeltaProcess::DeltaProcess( Oop receiver, SymbolOop selector, bool createThread ) :
    _receiver{ receiver },
    _selector{ selector },
    _state{ ProcessState::initialized },
    stopping{ false },
    _next{ nullptr },
    _last_Delta_fp{ nullptr },
    _last_Delta_pc{ nullptr },
    _last_Delta_sp{ nullptr },
    _is_terminating{ false },
    _unwind_head{ nullptr },
    _firstHandle{ nullptr },
    _time_stamp{ 0 },
    _processObject{},
    _debugInfo{},
    _isCallback{ false } {

    _event  = os::create_event( false );
    _thread = createThread
              ? os::create_thread( (std::int32_t ( * )( void * )) &launch_delta, (void *) this, &_thread_id )
              : os::starting_thread( &_thread_id );

    _stack_limit = (char *) os::stack_limit( _thread );

    SPDLOG_INFO( "creating process 0x{0:x}", static_cast<const void *>( this ) );

    set_last_Delta_fp( nullptr );
    set_last_Delta_sp( nullptr );
    set_last_Delta_pc( nullptr );

    Processes::add( this );
}


extern "C" void popStackHandles( const char *nextFrame ) {

    DeltaProcess *active = DeltaProcess::active();
    if ( active->thread_id() not_eq os::current_thread_id() ) {
        active = Processes::find_from_thread_id( os::current_thread_id() );
    }

    BaseHandle *current = active->firstHandle();
    while ( current and (const char *) current < nextFrame ) {
        current->pop();
        current = active->firstHandle();
    }

}


Frame DeltaProcess::profile_top_frame() {
    std::int32_t *sp;
    std::int32_t *fp;
    char         *pc;
    os::fetch_top_frame( _thread, &sp, &fp, &pc );
    Frame result( (Oop *) sp, fp, pc );
    return result;
}


static std::int32_t interruptions = 0;


void DeltaProcess::check_stack_overflow() {

    bool isInterrupted = false;
    if ( EnableProcessPreemption ) {
        ThreadCritical tc;
        isInterrupted = _interrupt;
        _interrupt    = false;
    }

    if ( isInterrupted ) {
        st_assert( EnableProcessPreemption, "Should not be interrupted unless preemption enabled" );
        interruptions++;
        _active_stack_limit = active()->_stack_limit;
        if ( interruptions % 1000 == 0 )
            spdlog::warn( "Interruptions: %d", interruptions );
        if ( DeltaProcess::active()->is_scheduler() )
            return;
        active()->suspend( ProcessState::yielded );
    } else if ( not active()->is_scheduler() ) {
        active()->suspend( ProcessState::stack_overflow );
    } else {
        st_fatal( "Stack overflow in scheduler" );
    }
}


extern "C" void check_stack_overflow() {
    DeltaProcess::check_stack_overflow();
}


DeltaProcess::~DeltaProcess() {
    processObject()->set_process( nullptr );
    if ( Processes::includes( this ) ) {
        Processes::remove( this );
    }
}


void DeltaProcess::preempt_active() {
    st_assert( EnableProcessPreemption, "Should not preempt active process when preemption not enabled" );
    _interrupt          = true;
    _active_stack_limit = (char *) 0x7fffffff;
}


void DeltaProcess::print() {

    switch ( state() ) {
        case ProcessState::initialized:
            SPDLOG_INFO( "{} initialized", processObject()->print_value_string() );
            break;
        case ProcessState::running:
            SPDLOG_INFO( "{} running", processObject()->print_value_string() );
            break;
        case ProcessState::yielded:
            SPDLOG_INFO( "{} yielded", processObject()->print_value_string() );
            break;
        case ProcessState::in_async_dll:
            SPDLOG_INFO( "{} in asynchronous dll call", processObject()->print_value_string() );
            break;
        case ProcessState::yielded_after_async_dll:
            SPDLOG_INFO( "{} yielded after asynchronous dll", processObject()->print_value_string() );
            break;
        case ProcessState::preempted:
            SPDLOG_INFO( "{} preempted", processObject()->print_value_string() );
            break;
        case ProcessState::completed:
            SPDLOG_INFO( "{} completed", processObject()->print_value_string() );
            break;
        case ProcessState::boolean_error:
            SPDLOG_INFO( "{} boolean error", processObject()->print_value_string() );
            break;
        case ProcessState::lookup_error:
            SPDLOG_INFO( "{} lookup error", processObject()->print_value_string() );
            break;
        case ProcessState::primitive_lookup_error:
            SPDLOG_INFO( "{} primitive lookup error", processObject()->print_value_string() );
            break;
        case ProcessState::DLL_lookup_error:
            SPDLOG_INFO( "{} DLL lookup error", processObject()->print_value_string() );
            break;
        case ProcessState::NonLocalReturn_error:
            SPDLOG_INFO( "{} NonLocalReturn error", processObject()->print_value_string() );
            break;
        case ProcessState::stack_overflow:
            SPDLOG_INFO( "{} stack overflow", processObject()->print_value_string() );
            break;
        default:
            void(0);
    }

}


void DeltaProcess::frame_iterate( FrameClosure *blk ) {

    //
    blk->begin_process( this );

    //
    if ( has_stack() ) {
        Frame v = last_frame();
        do {
            blk->do_frame( &v );
            v = v.sender();
        } while ( not v.is_first_frame() );
    }

    //
    blk->end_process( this );
}


void DeltaProcess::oop_iterate( OopClosure *blk ) {

    //
    blk->do_oop( (Oop *) &_receiver );
    blk->do_oop( (Oop *) &_selector );
    blk->do_oop( (Oop *) &_processObject );


    //
    for ( UnwindInfo *p = _unwind_head; p; p = p->next() ) {
        blk->do_oop( (Oop *) &p->_nlr_result );
    }

    //
    if ( has_stack() ) {
        Frame v = last_frame();
        do {
            v.oop_iterate( blk );
            v = v.sender();
        } while ( not v.is_first_frame() );
    }

}


SymbolOop DeltaProcess::symbol_from_state( ProcessState state ) {
    switch ( state ) {
        case ProcessState::initialized:
            return vmSymbols::initialized();
        case ProcessState::yielded:
            return vmSymbols::yielded();
        case ProcessState::running:
            return vmSymbols::running();
        case ProcessState::stopped:
            return vmSymbols::stopped();
        case ProcessState::preempted:
            return vmSymbols::preempted();
        case ProcessState::aborted:
            return vmSymbols::aborted();
        case ProcessState::in_async_dll:
            return vmSymbols::in_async_dll();
        case ProcessState::yielded_after_async_dll:
            return vmSymbols::yielded();
        case ProcessState::completed:
            return vmSymbols::completed();
        case ProcessState::boolean_error:
            return vmSymbols::boolean_error();
        case ProcessState::lookup_error:
            return vmSymbols::lookup_error();
        case ProcessState::primitive_lookup_error:
            return vmSymbols::primitive_lookup_error();
        case ProcessState::DLL_lookup_error:
            return vmSymbols::DLL_lookup_error();
        case ProcessState::float_error:
            return vmSymbols::float_error();
        case ProcessState::NonLocalReturn_error:
            return vmSymbols::NonLocalReturn_error();
        case ProcessState::stack_overflow:
            return vmSymbols::stack_overflow();
        default:
            return vmSymbols::not_found();
    }

    return vmSymbols::not_found();
}


bool DeltaProcess::has_stack() const {
    if ( state() == ProcessState::initialized ) {
        return false;
    }
    if ( state() == ProcessState::completed ) {
        return false;
    }
    return true;
}


void DeltaProcess::follow_roots() {

    MarkSweep::follow_root( (Oop *) &_receiver );
    MarkSweep::follow_root( (Oop *) &_selector );
    MarkSweep::follow_root( (Oop *) &_processObject );

    if ( has_stack() ) {
        Frame v = last_frame();
        do {
            v.follow_roots();
            v = v.sender();
        } while ( not v.is_first_frame() );
    }
}


void DeltaProcess::verify() {
    ResourceMark  rm;
    BlockScavenge bs;

    VirtualFrame *f = last_delta_vframe();
    if ( f == nullptr )
        return;

    // Do not verify the first VirtualFrame
    // It will fail if scavenging in allocateContext (Lars)
    for ( f = f->sender(); f; f = f->sender() ) {
        f->verify();
    }
}


void DeltaProcess::enter_uncommon() {
    st_assert( _state == ProcessState::running, "must be running" );
    _state = ProcessState::uncommon;
}


void DeltaProcess::exit_uncommon() {
    st_assert( _state == ProcessState::uncommon, "must be uncommon" );
    _state = ProcessState::running;
}

//  [-1            ] <-- link to next frame
//  [-1            ] <-- return address
//
//  [              ] <-- old_sp
//  ...
//  [              ] <-- old_fp

static Oop            *old_sp;
static Oop            *new_sp;
static std::int32_t   *old_fp;
static std::int32_t   *cur_fp;
static ObjectArrayOop frame_array;

extern "C" Oop *setup_deoptimization_and_return_new_sp( Oop *old_sp, std::int32_t *old_fp, ObjectArrayOop frame_array, std::int32_t *current_frame ) {
    ResourceMark resourceMark;

    // Save all parameters for later use (check unpack_frame_array)
    ::old_sp      = old_sp;
    ::old_fp      = old_fp;
    ::frame_array = frame_array;
    ::cur_fp      = current_frame;

    SMIOop number_of_vframes = SMIOop( frame_array->obj_at( StackChunkBuilder::number_of_vframes_index ) );
    SMIOop number_of_locals  = SMIOop( frame_array->obj_at( StackChunkBuilder::number_of_locals_index ) );

    st_assert( number_of_vframes->is_smi(), "must be smi_t" );
    st_assert( number_of_locals->is_smi(), "must be smi_t" );

    new_sp = old_sp - Frame::interpreter_stack_size( number_of_vframes->value(), number_of_locals->value() );
    return new_sp;
}


// Used to transfer information from deoptimize_stretch to unpack_frame_array.
static bool redo_the_send;

extern "C" {
std::int32_t redo_send_offset = 0;
}


void DeltaProcess::deoptimize_redo_last_send() {
    redo_the_send = true;
}


void trace_deoptimization_start() {

    if ( TraceDeoptimization ) {
        if ( nlr_through_unpacking ) {
            SPDLOG_INFO( " NonLocalReturn {}", ( nlr_home == (std::int32_t) cur_fp ) ? "inside" : "outside" );
        } else {
            SPDLOG_INFO( " Unpacking NonLocalReturn {}", ( nlr_home == (std::int32_t) cur_fp ) ? "inside" : "outside" );
        }
        _console->print( " - array " );
        frame_array->print_value();

        //
        SPDLOG_INFO( " @ 0x%lx", static_cast<const void *>(old_fp) );
    }

}


void trace_deoptimization_frame( Frame &current, Oop *current_sp, const char *current_pc ) {
    if ( TraceDeoptimization ) {
        Frame v( current_sp, current.fp(), current_pc );
        v.print_for_deoptimization( _console );
    }
}


void unpack_first_frame( const char *&current_pc, Frame &current, CodeIterator &c ) {

    // first VirtualFrame in the array
    if ( nlr_through_unpacking ) {
        // NonLocalReturn is coming through unpacked vfraes
        current_pc = c.interpreter_return_point();
        // current_pc points to a normal return point in the interpreter.
        // To find the nlr return point we first compute the nlr offset.
        current_pc = ic_info_at( current_pc )->NonLocalReturn_target();
        current.set_hp( c.next_hp() );

    } else if ( redo_the_send ) {
        // Deoptimizing uncommon trap
        current_pc = Interpreter::redo_bytecode_after_deoptimization();
        current.set_hp( c.next_hp() );
        redo_send_offset = c.next_hp() - c.hp();
        redo_the_send    = false;

    } else {
        // Normal case
        current_pc = c.interpreter_return_point( true );
        current.set_hp( c.next_hp() );

        if ( c.is_message_send() ) {
            number_of_arguments_through_unpacking = c.ic()->nof_arguments();

        } else if ( c.is_primitive_call() ) {
            number_of_arguments_through_unpacking = c.primitive_cache()->number_of_parameters();

        } else if ( c.is_dll_call() ) {
            // The callee should not pop the argument since a DLL call is like a c function call.
            // The continuation code for the DLL call will pop the arguments!
            number_of_arguments_through_unpacking = 0;
        }
    }
}


// Called from assembler in unpack_unoptimized_frames.
// Based on the statics (old_sp, old_fp, and frame_array) this function unpacks
// the array into interpreter frames.
// Returning from this function should activate the most recent deoptimized frame.
extern "C" void unpack_frame_array() {
    BlockScavenge bs;
    ResourceMark  rm;

    std::int32_t *pc_addr = (std::int32_t *) new_sp - 1;
    st_assert( *pc_addr = -1, "just checking" );

    trace_deoptimization_start();

    bool must_find_nlr_target = nlr_through_unpacking and nlr_home == (std::int32_t) cur_fp;
    bool nlr_target_found     = false; // For verification

    // link for the current frame
//    std::int32_t *link_addr = (std::int32_t *) new_sp - 2;

    Oop          *current_sp = new_sp;
    std::int32_t pos         = 3;
    std::int32_t length      = frame_array->length();
    bool         first       = true;
    Frame        current;
    // unpack one frame at at time from most recent to least recent
    do {
        Oop       receiver = frame_array->obj_at( pos++ );
        MethodOop method   = MethodOop( frame_array->obj_at( pos++ ) );
        st_assert( method->is_method(), "expecting method" );

        SMIOop byteCodeIndex_obj = SMIOop( frame_array->obj_at( pos++ ) );
        st_assert( byteCodeIndex_obj->is_smi(), "expecting smi_t" );
        std::int32_t byteCodeIndex = byteCodeIndex_obj->value();

        SMIOop locals_obj = SMIOop( frame_array->obj_at( pos++ ) );
        st_assert( locals_obj->is_smi(), "expecting smi_t" );
        std::int32_t locals = locals_obj->value();

        current = Frame( current_sp, (std::int32_t *) current_sp + locals + 2 );

        // fill in the locals
        for ( std::int32_t i = 0; i < locals; i++ ) {
            current.set_temp( i, frame_array->obj_at( pos++ ) );
        }

        CodeIterator c( method, byteCodeIndex );

        const char *current_pc;

        if ( first ) {
            unpack_first_frame( current_pc, current, c );
        } else {
            current_pc = c.interpreter_return_point();
            current.set_hp( c.next_hp() );
        }
        current.set_receiver( receiver );

        current.patch_pc( current_pc );
        current.patch_fp( current.fp() );

        // Revive the contexts
        if ( not method->is_blockMethod() and method->activation_has_context() ) {
            ContextOop con = ContextOop( current.temp( 0 ) );
            st_assert( con->is_context(), "must be context" );
            Oop frame_oop = Oop( current.fp() );
            con->set_parent( frame_oop );

            if ( nlr_through_unpacking and nlr_home == (std::int32_t) cur_fp ) {
                if ( nlr_home_context == con ) {
                    // This frame is the target of the NonLocalReturn
                    // set nlr_home to frame pointer of current frame
                    nlr_home         = (std::int32_t) current.fp();
                    // compute number of arguments to pop
                    nlr_home_id      = ~method->number_of_arguments();
                    nlr_target_found = true;
                    // _console->print("target frame for NonLocalReturn (%d, 0x%lx):",method->number_of_arguments(), nlr_home_id);
                }
            }
        }

        trace_deoptimization_frame( current, current_sp, current_pc );

        first = false;
        // Next pc
        current_sp += Frame::interpreter_frame_size( locals );

    } while ( pos <= length );

    if ( must_find_nlr_target and not nlr_target_found ) {
        st_fatal( "Target for NonLocalReturn not found when unpacking frame" );
    }

    st_assert ( current_sp == old_sp, "We have not reached the end" );
    current.set_link( old_fp );
}

extern "C" void verify_at_end_of_deoptimization() {
    if ( PrintStackAfterUnpacking ) {
        BlockScavenge bs;
        ResourceMark  rm;
        DeltaProcess::active()->verify();
        SPDLOG_INFO( "[Stack after unpacking]" );
        DeltaProcess::active()->trace_stack_for_deoptimization();
    }
}


void DeltaProcess::deoptimize_stretch( Frame *first_frame, Frame *last_frame ) {

    if ( TraceDeoptimization ) {
        SPDLOG_INFO( "[Deoptimizing]" );
        Frame c = *first_frame;
        c.print_for_deoptimization( _console );
        while ( c.fp() not_eq last_frame->fp() ) {
            c = c.sender();
            c.print_for_deoptimization( _console );
        }
    }

    StackChunkBuilder packer( first_frame->fp() );

    VirtualFrame *vf = VirtualFrame::new_vframe( first_frame );
    st_assert( vf->is_compiled_frame(), "must be Delta frame" );

    for ( DeltaVirtualFrame *current = (DeltaVirtualFrame *) vf; current and ( current->fr().fp() <= last_frame->fp() ); current = (DeltaVirtualFrame *) current->sender() ) {
        packer.append( current );
    }

    // Patch frame
    // - patch the pc first to convert the frame into a deoptimized_frame
    first_frame->patch_pc( StubRoutines::unpack_unoptimized_frames() );
    first_frame->set_return_addr( last_frame->return_addr() );
    first_frame->set_real_sender_sp( last_frame->sender_sp() );
    first_frame->set_frame_array( packer.as_objArray() );
    first_frame->set_link( last_frame->link() );
}


void DeltaProcess::deoptimized_wrt_marked_native_methods() {
    // chop the stack into stretches of frames in need for deoptimization
    if ( not has_stack() )
        return;

    Frame v = last_frame();
    do {
        if ( v.should_be_deoptimized() )
            deoptimize_stretch( &v, &v );
        v = v.sender();
    } while ( not v.is_first_frame() );
}


Frame DeltaProcess::last_frame() {
    st_assert( last_Delta_fp(), "must have last_Delta_fp() when suspended" );
    if ( last_Delta_fp() == nullptr ) {
        trace_stack();
    }
    if ( last_Delta_pc() == nullptr ) {
        Frame c( last_Delta_sp(), last_Delta_fp() );
        return c;
    } else {
        Frame c( last_Delta_sp(), last_Delta_fp(), last_Delta_pc() );
        return c;
    }
}


DeltaVirtualFrame *DeltaProcess::last_delta_vframe() {
    // If no stack is present return nullptr
    if ( not has_stack() )
        return nullptr;

    Frame              f   = last_frame();
    for ( VirtualFrame *vf = VirtualFrame::new_vframe( &f ); vf; vf = vf->sender() ) {
        if ( vf->is_delta_frame() )
            return (DeltaVirtualFrame *) vf;
    }
    return nullptr;
}


std::int32_t DeltaProcess::depth() {
    std::int32_t d = 0;
    for ( Frame  v = last_frame(); v.link(); v = v.sender() )
        d++;
    return d;
}


std::int32_t DeltaProcess::vdepth( Frame *f ) {
    static_cast<void>(f); // unused
    Unimplemented();
    return 0;
}


void DeltaProcess::trace_stack() {
    trace_stack_from( last_delta_vframe() );
}


void DeltaProcess::trace_stack_from( VirtualFrame *start_frame ) {
    SPDLOG_INFO( "- Stack trace" );
    std::int32_t vframe_no = 1;

    for ( VirtualFrame *f = start_frame; f; f = f->sender() ) {
        if ( f->is_delta_frame() ) {
            ( (DeltaVirtualFrame *) f )->print_activation( vframe_no++ );
        } else {
            f->print();
        }
        if ( vframe_no == StackPrintLimit ) {
            SPDLOG_INFO( "...<more frames>..." );
            return;
        }
    }
}


void DeltaProcess::trace_stack_for_deoptimization( Frame *f ) {
    if ( has_stack() ) {
        std::int32_t vframe_no = 1;
        Frame        v         = f ? *f : last_frame();
        do {
            v.print_for_deoptimization( _console );
            v = v.sender();
            if ( vframe_no == StackPrintLimit ) {
                SPDLOG_INFO( "...<more frames>..." );
                return;
            }
            vframe_no++;
        } while ( not v.is_first_frame() );
    }
}


void DeltaProcess::trace_top( std::int32_t start_frame, std::int32_t number_of_frames ) {
    FlagSetting fs( ActivationShowCode, true );

    SPDLOG_INFO( "- Stack trace ({}, {})", start_frame, number_of_frames );
    std::int32_t vframe_no = 1;

    for ( VirtualFrame *f = last_delta_vframe(); f; f = f->sender() ) {
        if ( vframe_no >= start_frame ) {
            if ( f->is_delta_frame() ) {
                ( (DeltaVirtualFrame *) f )->print_activation( vframe_no );
            } else
                f->print();
            if ( vframe_no - start_frame + 1 >= number_of_frames )
                return;
        }
        vframe_no++;
    }
}


void DeltaProcess::update_nlr_targets( CompiledVirtualFrame *f, ContextOop con ) {
    for ( UnwindInfo *p = _unwind_head; p; p = p->next() ) {
        p->update_nlr_targets( f, con );
    }
}


double DeltaProcess::user_time() {
    return _thread ? os::user_time_for( _thread ) : 0.0;
}


double DeltaProcess::system_time() {
    return _thread ? os::system_time_for( _thread ) : 0.0;
}


void DeltaProcess::setIsCallback( bool isCallback ) {
    _isCallback = isCallback;
}


void DeltaProcess::applyStepping() {
    _debugInfo.apply();
}


void DeltaProcess::resetStepping() {
    _debugInfo.reset();
}


void DeltaProcess::setupSingleStep() {
    _debugInfo.interceptForStep();
}


void DeltaProcess::setupStepNext( std::int32_t *fr ) {
    _debugInfo.interceptForNext( fr );
}


void DeltaProcess::setupStepReturn( std::int32_t *fr ) {
    _debugInfo.interceptForReturn( fr );
}


void DeltaProcess::resetStep() {
    _debugInfo.resetInterceptor();
}


bool DeltaProcess::is_deltaProcess() const {
    return true;
}


bool DeltaProcess::isUncommon() const {
    return _state == ProcessState::uncommon;
}


Oop DeltaProcess::receiver() const {
    return _receiver;
}


SymbolOop DeltaProcess::selector() const {
    return _selector;
}


DeltaProcess *DeltaProcess::next() const {
    return _next;
}


void DeltaProcess::set_next( DeltaProcess *p ) {
    _next = p;
}


ProcessOop DeltaProcess::processObject() const {
    return _processObject;
}


void DeltaProcess::set_processObject( ProcessOop p ) {
    _processObject = p;
}


bool DeltaProcess::is_terminating() {
    return _is_terminating;
}


void DeltaProcess::set_terminating() {
    _is_terminating = true;
}


std::int32_t DeltaProcess::time_stamp() const {
    return _time_stamp;
}


void DeltaProcess::inc_time_stamp() {
    _time_stamp++;
}


ProcessState DeltaProcess::state() const {
    return _state;
}


SymbolOop DeltaProcess::status_symbol() const {
    return symbol_from_state( state() );
}


void DeltaProcess::push_unwind( UnwindInfo *info ) {
    info->set_next( _unwind_head );
    _unwind_head = info;
}


void DeltaProcess::pop_unwind() {
    _unwind_head = _unwind_head->next();
}


void DeltaProcess::unwinds_do( void (*f)( UnwindInfo * ) ) {
    for ( UnwindInfo *p = _unwind_head; p; p = p->next() )
        f( p );
}


BaseHandle *DeltaProcess::firstHandle() {
    return _firstHandle;
}


void DeltaProcess::setFirstHandle( BaseHandle *handle ) {
    _firstHandle = handle;
}


bool DeltaProcess::is_ready() const {
    return state() == ProcessState::initialized or state() == ProcessState::yielded;
}


bool DeltaProcess::is_active() const {
    return this == active();
}


bool DeltaProcess::is_scheduler() const {
    return this == scheduler();
}


bool DeltaProcess::in_vm_operation() const {
    return is_active() and VMProcess::vm_operation() not_eq nullptr;
}


void DeltaProcess::set_active( DeltaProcess *p ) {
    _active_delta_process = p;
    _active_stack_limit   = p->_stack_limit;

    if ( _active_delta_process->state() not_eq ProcessState::uncommon ) {
        _active_delta_process->set_state( ProcessState::running );
    }
}


void DeltaProcess::set_scheduler( DeltaProcess *p ) {
    _scheduler_process = p;
}


DeltaProcess *DeltaProcess::active() {
    return _active_delta_process;
}


void DeltaProcess::set_main( DeltaProcess *p ) {
    _main_process = p;
}


DeltaProcess *DeltaProcess::main() {
    return _main_process;
}


bool DeltaProcess::is_idle() {
    return _is_idle;
}


DeltaProcess *DeltaProcess::scheduler() {
    return _scheduler_process;
}


void DeltaProcess::set_terminating_process( ProcessState state ) {
    _state_of_terminated_process = state;
    _process_has_terminated      = true;
}


bool DeltaProcess::process_has_terminated() {
    bool result = _process_has_terminated;
    _process_has_terminated = false;
    return result;
}


ProcessState DeltaProcess::state_of_terminated_process() {
    return _state_of_terminated_process;
}


void DeltaProcess::returnToDebugger() {
    resetStepping(); // reset dispatch table
    resetStep();     // disable stepping
    suspend( ProcessState::stopped );// stop!
}


bool DeltaProcess::stepping = false;
