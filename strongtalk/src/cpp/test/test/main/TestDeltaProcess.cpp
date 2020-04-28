//
//
//
//

#include "vm/runtime/Delta.hpp"
#include "vm/system/os.hpp"

#include "test/main/TestDeltaProcess.hpp"
#include "test/main/main.hpp"

#include <gtest/gtest.h>


Event            * done;
TestDeltaProcess * testProcess;
VMProcess        * vmProcess;
Thread           * vmThread;


int TestDeltaProcess::launch_tests( DeltaProcess * process ) {

    process->suspend_at_creation();
    DeltaProcess::set_active( process );
    initializeSmalltalkEnvironment();

    int status = RUN_ALL_TESTS();

    os::signal_event( done );
    return status;
}


// mock scheduler loop to allow test process->scheduler transfers and returns
int TestDeltaProcess::launch_scheduler( DeltaProcess * process ) {
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
    set_processObj( ProcessOop( process ) );
    ProcessOop( process )->set_process( this );
    Processes::add( this );
}


TestDeltaProcess::TestDeltaProcess() :
    DeltaProcess( nullptr, nullptr ) {
    int ignore;
    Processes::remove( this );
    os::terminate_thread( _thread ); // don't want to launch delta!
    _thread      = os::create_thread( ( int ( * )( void * ) ) &launch_tests, this, &ignore );
    _stack_limit = ( char * ) os::stack_limit( _thread );

    Oop process = newProcess();
    st_assert( process->is_process(), "Should be process" );
    set_processObj( ProcessOop( process ) );
    ProcessOop( process )->set_process( this );
}


TestDeltaProcess::TestDeltaProcess( fn launchfn ) :
    DeltaProcess( nullptr, nullptr ) {

    int ignore;
    Processes::remove( this );
    os::terminate_thread( _thread ); // don't want to launch delta!
    _thread      = os::create_thread( ( osfn ) launchfn, this, &ignore );
    _stack_limit = ( char * ) os::stack_limit( _thread );

    Oop process = newProcess();
    st_assert( process->is_process(), "Should be process" );
    set_processObj( ProcessOop( process ) );
    ProcessOop( process )->set_process( this );
}


TestDeltaProcess::~TestDeltaProcess() {
    set_processObj( ProcessOop( newProcess() ) );
}
