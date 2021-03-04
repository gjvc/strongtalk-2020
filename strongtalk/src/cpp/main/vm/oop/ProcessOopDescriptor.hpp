//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/runtime/DeltaProcess.hpp"


// A processOop is the Delta level process.
// It is a proxy for a vm c-heap allocate Process (see process.hpp).

class ProcessOopDescriptor : public MemOopDescriptor {

protected:
    DeltaProcess *_process;


    ProcessOopDescriptor *addr() {
        return (ProcessOopDescriptor *) MemOopDescriptor::addr();
    }


public:
    friend ProcessOop as_processOop( void *p );


    DeltaProcess *process() {
        return addr()->_process;
    }


    void set_process( DeltaProcess *p ) {
        st_assert( Oop(p)->isSmallIntegerOop(), "not a small_int_t" );
        addr()->_process = p;
    }


    // Returns whether the process is alive.
    bool is_live() {
        return process() not_eq nullptr;
    }


    // Yields to the scheduling process.
    void yield();

    // Returns the status of the process as symbol
    SymbolOop status_symbol();

    // Timing
    double user_time();

    double system_time();


    // sizing
    static std::int32_t header_size() {
        return sizeof( ProcessOopDescriptor ) / OOP_SIZE;
    }


    // bootstrappingInProgress
    void bootstrap_object( Bootstrap *stream );

    friend class ProcessKlass;
};


inline ProcessOop as_processOop( void *p ) {
    return ProcessOop( as_memOop( p ) );
}
