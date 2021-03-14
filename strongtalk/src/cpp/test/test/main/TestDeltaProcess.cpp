
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Delta.hpp"
#include "vm/platform/os.hpp"

#include "vm/runtime/Processes.hpp"
#include "test/main/TestDeltaProcess.hpp"
#include "vm/runtime/DeltaProcess.hpp"
#include "vm/runtime/VMProcess.hpp"

#include "test/main/main.hpp"

#include <gtest/gtest.h>


Event            *done;
TestDeltaProcess *testProcess;
VMProcess        *vmProcess;
Thread           *vmThread;


Oop newProcess() {
    return Delta::call( Universe::find_global( "Process" ), OopFactory::new_symbol( "new" ) );
}


std::int32_t TestDeltaProcess::launch_tests( DeltaProcess *process ) {

    process->suspend_at_creation();
    DeltaProcess::set_active( process );
    initializeSmalltalkEnvironment();

    std::int32_t status = RUN_ALL_TESTS();

    os::signal_event( done );
    return status;
}


// mock scheduler loop to allow test process->scheduler transfers and returns
std::int32_t TestDeltaProcess::launch_scheduler( DeltaProcess *process ) {
    process->suspend_at_creation();
    DeltaProcess::set_active( process );
    while ( true ) {
        if ( DeltaProcess::wait_for_async_dll( 10 ) )
            process->transfer_to( testProcess );
    }
    return 0;
}


void TestDeltaProcess::addToProcesses() {
    Oop process = newProcess();
    st_assert( process->is_process(), "Should be process" );
    set_processObject( ProcessOop( process ) );
    ProcessOop( process )->set_process( this );
    Processes::add( this );
}


TestDeltaProcess::TestDeltaProcess() :
    DeltaProcess( nullptr, nullptr ) {
    std::int32_t ignore;
    Processes::remove( this );
    os::terminate_thread( _thread ); // don't want to launch delta!
    _thread      = os::create_thread( (std::int32_t ( * )( void * )) &launch_tests, this, &ignore );
    _stack_limit = (char *) os::stack_limit( _thread );

    Oop process = newProcess();
    st_assert( process->is_process(), "Should be process" );
    set_processObject( ProcessOop( process ) );
    ProcessOop( process )->set_process( this );
}


TestDeltaProcess::TestDeltaProcess( fn launchfn ) :
    DeltaProcess( nullptr, nullptr ) {

    std::int32_t ignore;
    Processes::remove( this );
    os::terminate_thread( _thread ); // don't want to launch delta!
    _thread      = os::create_thread( (osfn) launchfn, this, &ignore );
    _stack_limit = (char *) os::stack_limit( _thread );

    Oop process = newProcess();
    st_assert( process->is_process(), "Should be process" );
    set_processObject( ProcessOop( process ) );
    ProcessOop( process )->set_process( this );
}


TestDeltaProcess::~TestDeltaProcess() {
    set_processObject( ProcessOop( newProcess() ) );
}
