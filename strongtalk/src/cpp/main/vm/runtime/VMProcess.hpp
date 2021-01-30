
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/Process.hpp"

// A single VMProcess (the default thread) is used for heavy vm operations like scavenge, garbage_collect etc.
class VMProcess : public Process {

public:
    VMProcess();


    // tester
    bool is_vmProcess() const {
        return true;
    }


    // activates the virtual machine
    void activate_system();

    // the ever running loop for the VMProcess
    void loop();

    // transfer
    void transfer_to( DeltaProcess *target_process );

    // misc.
    void print();

    // Terminate
    static void terminate( DeltaProcess *proc );

    // execution of vm operation
    static void execute( VM_Operation *op );


    // returns the current vm operation if any.
    static VM_Operation *vm_operation() {
        return _vm_operation;
    }


    // returns the single instance of VMProcess.
    static VMProcess *vm_process() {
        return _vm_process;
    }


private:
    static VM_Operation *_vm_operation;
    static VMProcess    *_vm_process;
};
