
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/runtime/arguments.hpp"
#include "vm/runtime/init.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oop/ProcessOopDescriptor.hpp"
#include "vm/oop/AssociationOopDescriptor.hpp"
#include "vm/system/os.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/vmOperations.hpp"

#include "test/runtime/testProcess.hpp"
#include "vm/runtime/Processes.hpp"

#include "vm/runtime/DeltaProcess.hpp"
#include "vm/runtime/VMProcess.hpp"

#include "test/main/TestDeltaProcess.hpp"

#include <gtest/gtest.h>


void addTestToProcesses() {
    testProcess->addToProcesses();
}


void removeTestFromProcesses() {
    testProcess->removeFromProcesses();
}


void TestDeltaProcess::removeFromProcesses() {
    Processes::remove( this );
}


Oop newProcess();


void setProcessRefs( DeltaProcess *process, ProcessOop processObject ) {
    processObject->set_process( process );
    process->set_processObject( processObject );
}


void initializeSmalltalkEnvironment() {

    AddTestProcess ap;

    PersistentHandle _new( OopFactory::new_symbol( "new" ) );
    PersistentHandle initialize( OopFactory::new_symbol( "initialize" ) );
    PersistentHandle runBaseClassInitializers( OopFactory::new_symbol( "runBaseClassInitializers" ) );
    PersistentHandle processorScheduler( Universe::find_global( "ProcessorScheduler" ) );
    PersistentHandle smalltalk( Universe::find_global( "Smalltalk" ) );
    PersistentHandle systemInitializer( Universe::find_global( "SystemInitializer" ) );
    PersistentHandle forSeconds( OopFactory::new_symbol( "forSeconds:" ) );

    PersistentHandle processor( Delta::call( processorScheduler.as_oop(), _new.as_oop() ) );

    AssociationOop processorAssoc = Universe::find_global_association( "Processor" );
    processorAssoc->set_value( processor.as_oop() );

    DeltaProcess *scheduler = new TestDeltaProcess( &TestDeltaProcess::launch_scheduler );
    DeltaProcess::set_scheduler( scheduler );

    Delta::call( processor.as_oop(), initialize.as_oop() );
    Delta::call( systemInitializer.as_oop(), runBaseClassInitializers.as_oop() );
    Delta::call( smalltalk.as_oop(), initialize.as_oop() );

}


static std::int32_t vmLoopLauncher( DeltaProcess *testProcess ) {
    vmProcess->transfer_to( testProcess );
    vmProcess->loop();
    return 0;
}


void start_vm_process( TestDeltaProcess *testProcess ) {

    std::int32_t threadId;
    vmProcess = new VMProcess();
    DeltaProcess::initialize_async_dll_event();

    ::testProcess = testProcess; // set it before the next line starts the system
    vmThread      = os::create_thread( (std::int32_t ( * )( void * )) &vmLoopLauncher, testProcess, &threadId );
}


void stop_vm_process() {
    os::terminate_thread( vmThread );
}


int main( int argc, char *argv[] ) {

    SPDLOG_INFO( "-----------------------------------------------------------------------------" );
    SPDLOG_INFO( ">>> STRONGTALK TEST HARNESS" );
    SPDLOG_INFO( "-----------------------------------------------------------------------------" );

    ::testing::InitGoogleTest( &argc, argv );

    parse_arguments( argc, argv );
    init_globals();
    load_image();
    ResourceMark resourceMark;

    vmSymbols::initialize();

    done = os::create_event( false );

    TestDeltaProcess testProcess;
    start_vm_process( &testProcess );
    os::wait_for_event( done );
    stop_vm_process();

    return EXIT_SUCCESS;
}
