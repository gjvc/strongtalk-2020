//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "allocation.hpp"
#include "vm/utilities/GrowableArray.hpp"


// MarkSweep takes care of garbage collection
class OopRelocations;

class MarkSweep : AllStatic {
    public:
        static Oop collect( Oop p = nullptr );

        // Call backs
        static void follow_root( Oop * p );

        static void reverse_and_push( Oop * p );

        static void reverse_and_follow( Oop * p );

        static void add_heap_code_offset( int offset );

        static int next_heap_code_offset();

    private:
        // the traversal stack used during phase1.
        static GrowableArray <MemOop> * _stack;
        // the hcode pointer offsets saved before and
        // and retrieved after the garbage collection.
        static GrowableArray <int>    * hcode_offsets;
        static int                    hcode_pos;
        // resource area for non-aligned oops requiring relocation (eg. in nativeMethods)
        static OopRelocations         * _oopRelocations;

    private:
        static void mark_sweep_phase1( Oop * p );

        static void mark_sweep_phase2();

        static void mark_sweep_phase3();

        static inline MemOop reverse( Oop * p );

        static void allocate();

        static void deallocate();

        static void trace( const char * msg );
};

