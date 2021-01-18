
//
//
//
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/posix.hpp"
#include "vm/runtime/Process.hpp"


typedef int (*osfn)( void * );
typedef int (*fn)( DeltaProcess * );



// This is a fake DeltaProcess used to run the tests.
// It is there to allow VM operations to be executed on the VMProcess thread.

class TestDeltaProcess : public DeltaProcess {

private:
    static int launch_tests( DeltaProcess *process );

public:
    TestDeltaProcess();
    TestDeltaProcess( fn launchfn );
    ~TestDeltaProcess();
    void addToProcesses();
    void removeFromProcesses();


    void deoptimized_wrt_marked_nativeMethods() {
    }


    bool_t has_stack() const {
        return false;
    }


    static int launch_scheduler( DeltaProcess *process );
};


extern Event            *done;
extern TestDeltaProcess *testProcess;
extern VMProcess        *vmProcess;
extern Thread           *vmThread;
