//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/Process.hpp"


class Processes : AllStatic {

private:
    static DeltaProcess *_processList;

public:
    // Process management
    static void add( DeltaProcess *p );

    static void remove( DeltaProcess *p );

    static bool includes( DeltaProcess *p );

    static DeltaProcess *last();

    static DeltaProcess *find_from_thread_id( std::int32_t id );

    // Start the vm process
    static void start( VMProcess *p );

    // State
    static bool has_completed_async_call();

    // Killing
    static void kill_all();

    // Iterating
    static void frame_iterate( FrameClosure *blk );

    static void oop_iterate( OopClosure *blk );

    static void process_iterate( ProcessClosure *blk );

    // Scavenge
    static void scavenge_contents();

    // Garbage collection
    static void follow_roots();

    static void convert_heap_code_pointers();

    static void restore_heap_code_pointers();

    // Verifycation
    static void verify();

    static void print();

    // Deoptimization

public:
    // deoptimizes frames dependent on a NativeMethod
    static void deoptimize_wrt( NativeMethod *nm );

    // deoptimizes frames dependent on at least one NativeMethod in the list
    static void deoptimize_wrt( GrowableArray<NativeMethod *> *list );

    // deoptimizes all frames
    static void deoptimize_all();

    // deoptimizes all frames tied to marked nativeMethods
    static void deoptimized_wrt_marked_nativeMethods();

    // deoptimization support for NonLocalReturn
    static void update_nlr_targets( CompiledVirtualFrame *f, ContextOop con );
};
