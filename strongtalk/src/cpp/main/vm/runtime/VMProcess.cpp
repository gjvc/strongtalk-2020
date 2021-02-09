
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/VMProcess.hpp"
#include "vm/runtime/DeltaProcess.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/runtime/Sweeper.hpp"
#include "vm/memory/oopFactory.hpp"


// ======= VMProcess ========

VMProcess::VMProcess() {
    st_assert( vm_process() == nullptr, "we may only allocate one VMProcess" );

    _vm_process   = this;
    _vm_operation = nullptr;

    _thread = os::starting_thread( &_thread_id );
    _event  = os::create_event( true );
}


void VMProcess::transfer_to( DeltaProcess *target ) {

    {
        ThreadCritical tc;

        // restore state
        ::last_delta_fp = target->_last_delta_fp;   // *don't* use accessors!
        ::last_delta_sp = target->_last_delta_sp;   // *don't* use accessors!
        DeltaProcess::set_active( target );
        DeltaProcess::set_current( target );
        resetStepping();
    }

    basic_transfer( target );
}


void VMProcess::terminate( DeltaProcess *proc ) {

    st_assert( Process::current()->is_vmProcess(), "can only be called from vm process" );
    st_assert( proc->is_deltaProcess(), "must be deltaProcess" );
    st_assert( proc->_thread, "thread must be present" );
    st_assert( proc->_event, "event must be present" );

    os::terminate_thread( proc->_thread );
    proc->_thread = nullptr;
    os::delete_event( proc->_event );
    proc->_event = nullptr;

    DeltaProcess::set_terminating_process( proc->state() );
}


void VMProcess::activate_system() {

    // Find the Delta level 'Processor'
    ProcessOop proc = ProcessOop( Universe::find_global( "Processor" ) );
    if ( not proc->is_process() ) {
        KlassOop scheduler_klass = KlassOop( Universe::find_global( "ProcessorScheduler" ) );
        proc = ProcessOop( scheduler_klass->klass_part()->allocateObject() );
        AssociationOop assoc = Universe::find_global_association( "Processor" );
        assoc->set_value( proc );
    }

    // Create the initial process
    DeltaProcess::set_scheduler( new DeltaProcess( proc, oopFactory::new_symbol( "start" ) ) );

    // Bind the scheduler to Processor
    proc->set_process( DeltaProcess::scheduler() );
    DeltaProcess::scheduler()->set_processObject( proc );

    // Transfer control to the scheduler
    transfer_to( DeltaProcess::scheduler() );

    // Call the ever running loop handling vm operations
    loop();
}


void VMProcess::loop() {

    while ( true ) {
        st_assert( vm_operation(), "A VM_Operation should be present" );
        vm_operation()->evaluate();

        // if the process's thread is dead then the stack may already be released
        // in which case the vm_operation is no longer valid, so check for a
        // terminated process first. Can't use accessor as it resets the flag!
        DeltaProcess *p = DeltaProcess::_process_has_terminated ? DeltaProcess::scheduler() : vm_operation()->calling_process();
        _vm_operation = nullptr;
        transfer_to( p );
    }
}


void VMProcess::print() {
    SPDLOG_INFO( "VMProcess" );
}


void VMProcess::execute( VM_Operation *op ) {

    if ( Sweeper::is_running() ) {
        // We cannot perform a vm operations when running the sweeper since the sweeper is run outside the active process.
        st_fatal( "VMProcess is called during sweeper run" );
    }

    if ( DeltaProcess::active()->in_vm_operation() ) {
        // already running in VM process, no need to switch
        op->evaluate();
    } else {
        op->set_calling_process( DeltaProcess::active() );
        _vm_operation = op;
        // Suspend currentProcess and resume vmProcess
        DeltaProcess::active()->transfer_to_vm();
    }
}
