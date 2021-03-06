//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/Delta.hpp"
#include "vm/oop/ProcessOopDescriptor.hpp"
#include "test/main/TestDeltaProcess.hpp"
#include "vm/memory/OopFactory.hpp"


void initializeSmalltalkEnvironment();
void setProcessRefs( DeltaProcess *process, ProcessOop processObject );
void start_vm_process( TestDeltaProcess *testProcess );
void stop_vm_process();

extern TestDeltaProcess *testProcess;
extern VMProcess        *vmProcess;
extern Thread           *vmThread;
extern Event            *done;
