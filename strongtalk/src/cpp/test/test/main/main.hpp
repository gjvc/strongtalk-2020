//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/Delta.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "test/main/TestDeltaProcess.hpp"
#include "vm/memory/oopFactory.hpp"


void initializeSmalltalkEnvironment();
void setProcessRefs( DeltaProcess * process, ProcessOop processObj );
void start_vm_process( TestDeltaProcess * testProcess );
void stop_vm_process();


Oop newProcess() {
    return Delta::call( Universe::find_global( "Process" ), oopFactory::new_symbol( "new" ) );
}


extern TestDeltaProcess * testProcess;
extern VMProcess        * vmProcess;
extern Thread           * vmThread;
extern Event            * done;
