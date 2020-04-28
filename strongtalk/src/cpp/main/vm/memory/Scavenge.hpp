
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/memory/Universe.hpp"


class BlockScavenge {
    private:
        static int counter;
    public:
        static int * counter_addr() {
            return &counter;
        }


        static bool_t is_blocked() {
            return counter > 0;
        }


        BlockScavenge() {
            counter++;
        }


        ~BlockScavenge() {
            counter--;
        }
};


// Lars, please complete this at some point
class VerifyNoScavenge : public StackAllocatedObject {
    private:
        int _scavengeCount;

    public:
        VerifyNoScavenge() {
            _scavengeCount = Universe::scavengeCount;
        }


        virtual ~VerifyNoScavenge() {
            if ( _scavengeCount not_eq Universe::scavengeCount ) {
                fatal( "scavenge should not have happened" );
            }
        }
};

class VerifyNoAllocation : public VerifyNoScavenge {
    private:
        Oop * _top_of_eden;

    public:
        VerifyNoAllocation() {
            _top_of_eden = Universe::new_gen.eden()->top();
        }


        virtual ~VerifyNoAllocation() {
            if ( _top_of_eden not_eq Universe::new_gen.eden()->top() ) {
                fatal( "allocation should not have happened" );
            }
        }
};
