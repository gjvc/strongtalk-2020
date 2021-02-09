//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Process.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/system/os.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/runtime/Sweeper.hpp"
#include "vm/runtime/ErrorHandler.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/runtime/StackChunkBuilder.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/runtime/evaluator.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


std::int32_t CurrentHash = 23;


bool Process::external_suspend_current() {
    if ( current() == nullptr ) {
        return false;
    }

    os::suspend_thread( current()->_thread );
    return true;
}


void Process::external_resume_current() {
    os::resume_thread( current()->_thread );
}


void Process::basic_transfer( Process *target ) {
    if ( TraceProcessEvents ) {
        _console->print( "Process: " );
        print();
        _console->print( " -> " );
        target->print();
        _console->cr();
    }
    os::transfer( _thread, _event, target->_thread, target->_event );
    applyStepping();
}


VMProcess    *VMProcess::_vm_process   = nullptr;
VM_Operation *VMProcess::_vm_operation = nullptr;


// ======= DeltaProcess ========

extern "C" const char *active_stack_limit() {
    return (const char *) &DeltaProcess::_active_stack_limit;
}

Process *Process::_current_process = nullptr;


void handle_error( ProcessState error ) {
    DeltaProcess *proc = DeltaProcess::active();
    if ( proc->is_scheduler() ) {
        SPDLOG_INFO( "Error happened in the scheduler" );
        _console->print( "Status: " );
        proc->status_symbol()->print_symbol_on( _console );
        _console->cr();
        evaluator::read_eval_loop();
    } else {
        proc->suspend( error );
    }
    ErrorHandler::abort_current_process();
    ShouldNotReachHere();
}


void handle_interpreter_error( const char *message ) {
    SPDLOG_WARN( "Interpreter error: %s", message );
    handle_error( ProcessState::stopped );
}


extern "C" void suspend_on_error( InterpreterErrorConstants error_code ) {
    // Called from the the interpreter

    // Real errors
    switch ( error_code ) {
        case InterpreterErrorConstants::primitive_lookup_failed:
            handle_error( ProcessState::primitive_lookup_error );
            break;
        case InterpreterErrorConstants::boolean_expected:
            handle_error( ProcessState::boolean_error );
            break;
        case InterpreterErrorConstants::nonlocal_return_error:
            handle_error( ProcessState::NonLocalReturn_error );
            break;
        case InterpreterErrorConstants::float_expected:
            handle_error( ProcessState::float_error );
            break;
        default:
            nullptr;
            break;

    }


    // Interpreter errors
    switch ( error_code ) {
        case InterpreterErrorConstants::halted:
            handle_interpreter_error( "executed halt bytecode" );
            break;
        case InterpreterErrorConstants::illegal_code:
            handle_interpreter_error( "illegal code" );
            break;
        case InterpreterErrorConstants::not_implemented:
            handle_interpreter_error( "not implemented" );
            break;
        case InterpreterErrorConstants::stack_misaligned:
            handle_interpreter_error( "stack misaligned" );
            break;
        case InterpreterErrorConstants::ebx_wrong:
            handle_interpreter_error( "ebx wrong" );
            break;
        case InterpreterErrorConstants::obj_wrong:
            handle_interpreter_error( "obj wrong" );
            break;
        case InterpreterErrorConstants::nlr_offset_wrong:
            handle_interpreter_error( "NonLocalReturn offset wrong" );
            break;
        case InterpreterErrorConstants::last_delta_fp_wrong:
            handle_interpreter_error( "last Delta frame wrong" );
            break;
        case InterpreterErrorConstants::primitive_result_wrong:
            handle_interpreter_error( "ilast C entry frame wrong" );
            break;
        default:
            nullptr;
            break;
    }
    ShouldNotReachHere();
}

extern "C" void suspend_on_NonLocalReturn_error() {
    // Called from compiled code
    DeltaProcess::active()->suspend( ProcessState::NonLocalReturn_error );
}


void trace_stack_at_exception( std::int32_t *sp, std::int32_t *fp, const char *pc ) {
    ResourceMark resourceMark;

    SPDLOG_INFO( "Trace at exception" );

    VirtualFrame *vf;
    if ( last_delta_fp ) {
        Frame c( last_delta_sp, last_delta_fp );
        vf = VirtualFrame::new_vframe( &c );
    } else {
        Frame c( (Oop *) sp, fp, pc );
        vf = VirtualFrame::new_vframe( &c );
    }

    DeltaProcess::trace_stack_from( vf );
}


void suspend_process_at_stack_overflow( std::int32_t *sp, std::int32_t *fp, const char *pc ) {
    DeltaProcess *proc = DeltaProcess::active();

    proc->set_last_delta_pc( pc );
    last_delta_fp = fp;
    last_delta_sp = (Oop *) sp;

    if ( proc->is_scheduler() ) {
        SPDLOG_INFO( "Stack overflow happened in scheduler" );
    } else {
        proc->suspend( ProcessState::stack_overflow );
        proc->set_terminating();
    }
}


void trace_stack( std::int32_t thread_id ) {

    ResourceMark resourceMark;
    Process      *process = Processes::find_from_thread_id( thread_id );

    if ( process->is_deltaProcess() ) {
        ( (DeltaProcess *) process )->trace_stack();
    }

}
