//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/primitives/process_primitives.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/runtime/ErrorHandler.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/interpreter/DispatchTable.hpp"
#include "vm/memory/util.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/VirtualFrameOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"


TRACE_FUNC( TraceProcessPrims, "process" )


int processOopPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->is_process(), "receiver must be process")


Oop __CALLING_CONVENTION processOopPrimitives::create( Oop receiver, Oop block ) {
    PROLOGUE_2( "create", receiver, block )
    st_assert( receiver->is_klass() and KlassOop( receiver )->klass_part()->oop_is_process(), "must be process class" );
    if ( block->klass() not_eq Universe::zeroArgumentBlockKlassObj() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    ProcessOop process = ProcessOop( receiver->primitive_allocate() );
    st_assert( process->is_process(), "must be process" );

    DeltaProcess * p = new DeltaProcess( block, oopFactory::new_symbol( "value" ) );
    process->set_process( p );
    p->set_processObj( process );
    return process;
}


Oop __CALLING_CONVENTION processOopPrimitives::yield() {
    PROLOGUE_0( "yield" );
    if ( not DeltaProcess::active()->is_scheduler() ) {
        DeltaProcess::active()->suspend( ProcessState::yielded );
    }
    return DeltaProcess::active()->processObj();
}


class PrintFrameClosure : public FrameClosure {
        void do_frame( Frame * f ) {
            f->print();
            if ( f->is_compiled_frame() ) {
                f->code()->print();
            } else if ( f->is_interpreted_frame() ) {
                f->method()->print();
            }
        }
};


void print_nativeMethod( NativeMethod * nm ) {
    nm->print();
}


Oop __CALLING_CONVENTION processOopPrimitives::stop() {
    PROLOGUE_0( "stop" );
    //PrintFrameClosure pfc;
    //DeltaProcess::active()->frame_iterate(&pfc);
    //Universe::code->nativeMethods_do(print_nativeMethod);
    if ( not DeltaProcess::active()->is_scheduler() ) {
        DeltaProcess::active()->suspend( ProcessState::stopped );
    }
    return DeltaProcess::active()->processObj();
}


Oop __CALLING_CONVENTION processOopPrimitives::setMainProcess( Oop receiver ) {
    PROLOGUE_1( "setMainProcess", receiver );

    if ( not receiver->is_process() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );

    ProcessOop process = ProcessOop( receiver );
    DeltaProcess::main()->set_processObj( process );
    process->set_process( DeltaProcess::main() );

    return receiver;
}


Oop __CALLING_CONVENTION processOopPrimitives::transferTo( Oop process ) {
    PROLOGUE_1( "transferTo", process );

    // Check if argument is a processOop
    if ( not process->is_process() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Check if we are in the scheduler
    if ( not DeltaProcess::active()->is_scheduler() )
        return markSymbol( vmSymbols::not_in_scheduler() );

    // Make sure process is not dead
    if ( not ProcessOop( process )->is_live() )
        return markSymbol( vmSymbols::dead() );

    DeltaProcess * proc = ProcessOop( process )->process();

    // Do the transfer (remember transfer_to is a no-op if process is in async DLL)
    ProcessState state = DeltaProcess::scheduler()->transfer_to( proc );

    // Please be careful here since a scavenge/garbage collect can happen!

    return DeltaProcess::symbol_from_state( state );
}


Oop __CALLING_CONVENTION processOopPrimitives::set_mode( Oop process, Oop mode, Oop value ) {
    PROLOGUE_3( "set_mode", process, mode, value );

    // Check if argument is a processOop
    if ( not process->is_process() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Check if we are in the scheduler
    if ( not DeltaProcess::active()->is_scheduler() )
        return markSymbol( vmSymbols::not_in_scheduler() );

    // Make sure process is not dead
    if ( not ProcessOop( process )->is_live() )
        return markSymbol( vmSymbols::dead() );

    DeltaProcess * proc = ProcessOop( process )->process();

    // FIX THIS
    //  proc->set_single_step_mode();

    // Do the transfer (remember transfer_to is a no-op if process is in async DLL)
    ProcessState state = DeltaProcess::scheduler()->transfer_to( proc );

    // Please be careful here since a scavenge/garbage collect can happen!

    // FIX THIS
    //  proc->set_normal_mode();

    return DeltaProcess::symbol_from_state( state );
}


extern "C" void single_step_handler();


Oop __CALLING_CONVENTION processOopPrimitives::start_evaluator( Oop process ) {
    PROLOGUE_1( "start_evaluator", process );

    // Check if argument is a processOop
    if ( not process->is_process() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Check if we are in the scheduler
    if ( not DeltaProcess::active()->is_scheduler() )
        return markSymbol( vmSymbols::not_in_scheduler() );

    // Make sure process is not dead
    if ( not ProcessOop( process )->is_live() )
        return markSymbol( vmSymbols::dead() );

    DeltaProcess * proc = ProcessOop( process )->process();
    {
        ResourceMark resourceMark;
        StubRoutines::setSingleStepHandler( &single_step_handler );
        DispatchTable::intercept_for_step( nullptr );
    }
    ProcessState state  = DeltaProcess::scheduler()->transfer_to( proc );

    return DeltaProcess::symbol_from_state( state );
}


Oop __CALLING_CONVENTION processOopPrimitives::terminate( Oop receiver ) {
    PROLOGUE_1( "terminate", receiver );
    ASSERT_RECEIVER;

    // Make sure process is not dead
    if ( not ProcessOop( receiver )->is_live() )
        return markSymbol( vmSymbols::dead() );

    DeltaProcess * proc = ProcessOop( receiver )->process();
    if ( proc == DeltaProcess::active() ) {
        // Start the abort now
        ErrorHandler::abort_current_process();
    } else {
        proc->set_terminating();
    }
    return receiver;
}


Oop __CALLING_CONVENTION processOopPrimitives::activeProcess() {
    PROLOGUE_0( "activeProcess" );

    ProcessOop p = DeltaProcess::active()->processObj();
    st_assert( p->is_process(), "must be process" );

    return p;
}


Oop __CALLING_CONVENTION processOopPrimitives::status( Oop process ) {
    PROLOGUE_1( "status", process );

    // Check if argument is a processOop
    if ( not process->is_process() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return ProcessOop( process )->status_symbol();
}


Oop __CALLING_CONVENTION processOopPrimitives::scheduler_wait( Oop milliseconds ) {
    PROLOGUE_1( "scheduler_wait", milliseconds );

    // Check if argument is a smi_t
    if ( not milliseconds->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return DeltaProcess::wait_for_async_dll( SMIOop( milliseconds )->value() ) ? trueObj : falseObj;
}


Oop __CALLING_CONVENTION processOopPrimitives::trace_stack( Oop receiver, Oop size ) {
    PROLOGUE_2( "trace_stack", receiver, size );
    ASSERT_RECEIVER;

    // Check argument
    if ( not size->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Make sure process is not dead
    if ( not ProcessOop( receiver )->is_live() )
        return markSymbol( vmSymbols::dead() );

    // Print the stack
    {
        ResourceMark resourceMark;
        ProcessOop( receiver )->process()->trace_top( 1, SMIOop( size )->value() );
    }

    return receiver;
}


Oop __CALLING_CONVENTION processOopPrimitives::enter_critical() {
    PROLOGUE_0( "enter_critical" );
    // %fix this when implementing preemption
    return DeltaProcess::active()->processObj();
}


Oop __CALLING_CONVENTION processOopPrimitives::leave_critical() {
    PROLOGUE_0( "leave_critical" );
    // %fix this when implementing preemption
    return DeltaProcess::active()->processObj();
}


Oop __CALLING_CONVENTION processOopPrimitives::yield_in_critical() {
    PROLOGUE_0( "yield_in_critical" );
    // %fix this when implementing preemption
    if ( not DeltaProcess::active()->is_scheduler() ) {
        DeltaProcess::active()->suspend( ProcessState::yielded );
    }
    return DeltaProcess::active()->processObj();
}


Oop __CALLING_CONVENTION processOopPrimitives::user_time( Oop receiver ) {
    PROLOGUE_1( "enter_critical", receiver );
    ASSERT_RECEIVER;
    return oopFactory::new_double( ProcessOop( receiver )->user_time() );
}


Oop __CALLING_CONVENTION processOopPrimitives::system_time( Oop receiver ) {
    PROLOGUE_1( "enter_critical", receiver );
    ASSERT_RECEIVER;
    return oopFactory::new_double( ProcessOop( receiver )->system_time() );
}


Oop __CALLING_CONVENTION processOopPrimitives::stack( Oop receiver, Oop limit ) {
    PROLOGUE_2( "stack", receiver, limit );
    ASSERT_RECEIVER;

    // Check type of limit
    if ( not limit->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Make sure process is not dead
    if ( not ProcessOop( receiver )->is_live() )
        return markSymbol( vmSymbols::dead() );

    // Make sure we are not retrieving activation for the current process
    if ( ProcessOop( receiver )->process() == DeltaProcess::active() )
        return markSymbol( vmSymbols::running() );


    ResourceMark  rm;
    BlockScavenge bs;

    int        l       = SMIOop( limit )->value();
    ProcessOop process = ProcessOop( receiver );
    GrowableArray <Oop> * stack = new GrowableArray <Oop>( 100 );

    VirtualFrame * vf = ProcessOop( receiver )->process()->last_delta_vframe();

    for ( int i = 1; i <= l and vf; i++ ) {
        stack->push( oopFactory::new_vframe( process, i ) );
        vf = vf->sender();
    }

    return oopFactory::new_objArray( stack );
}
