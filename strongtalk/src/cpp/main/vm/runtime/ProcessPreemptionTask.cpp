//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/runtime/PeriodicTask.hpp"
#include "vm/runtime/ProcessPreemptionTask.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/Process.hpp"


void ProcessPreemptionTask::task() {
    if ( EnableProcessPreemption )
        DeltaProcess::preempt_active();
}


void preemption_init() {
    _console->print_cr( "%%system-init:  preemption_init" );

    ProcessPreemptionTask * task = new ProcessPreemptionTask;
    task->enroll();
}
