
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/memory/Universe.hpp"


class BlockScavenge {

private:
    static std::int32_t counter;

public:
    static std::int32_t *counter_addr() {
        return &counter;
    }


    static bool is_blocked() {
        return counter > 0;
    }


    BlockScavenge() {
        counter++;
    }


    ~BlockScavenge() {
        counter--;
    }


    BlockScavenge( const BlockScavenge & ) = default;
    BlockScavenge &operator=( const BlockScavenge & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

};


// Lars, please complete this at some point
class VerifyNoScavenge : public StackAllocatedObject {
private:
    std::int32_t _scavengeCount;

public:
    VerifyNoScavenge() :
        _scavengeCount{ Universe::scavengeCount } {
    }

    VerifyNoScavenge( const VerifyNoScavenge & ) = default;
    VerifyNoScavenge &operator=( const VerifyNoScavenge & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    virtual ~VerifyNoScavenge() {
        if ( _scavengeCount not_eq Universe::scavengeCount ) {
            st_fatal( "scavenge should not have happened" );
        }
    }
};


class VerifyNoAllocation : public VerifyNoScavenge {
private:
    Oop *_top_of_eden;

public:
    VerifyNoAllocation() :
        _top_of_eden{ Universe::new_gen.eden()->top() } {
    }

    VerifyNoAllocation( const VerifyNoAllocation & ) = default;
    VerifyNoAllocation &operator=( const VerifyNoAllocation & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    virtual ~VerifyNoAllocation() {
        if ( _top_of_eden not_eq Universe::new_gen.eden()->top() ) {
            st_fatal( "allocation should not have happened" );
        }
    }
};
