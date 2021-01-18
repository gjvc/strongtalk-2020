//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"


extern void addTestToProcesses();
extern void removeTestFromProcesses();

class AddTestProcess : public ValueObject {

public:
    AddTestProcess() {
        addTestToProcesses();
    }


    ~AddTestProcess() {
        removeTestFromProcesses();
    }
};
