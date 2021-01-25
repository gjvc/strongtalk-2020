//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"


class ProfiledNode;

class FlatProfilerTask;

class NativeMethod;


enum class TickPosition {
    in_code,        //
    in_primitive,   //
    in_compiler,    //
    in_pic,         //
    other           //
};

class Frame;

class DeltaProcess;

class FlatProfiler : AllStatic {

private:
    static ProfiledNode **_table;
    static std::int32_t _tableSize;

    static DeltaProcess     *_deltaProcess;
    static FlatProfilerTask *_flatProfilerTask;
    static Timer _timer;

    static std::int32_t _gc_ticks;           // total ticks in GC/scavenge
    static std::int32_t _semaphore_ticks;    //
    static std::int32_t _stub_ticks;         //
    static std::int32_t _compiler_ticks;     // total ticks in compilation
    static std::int32_t _unknown_ticks;      //


    static std::int32_t total_ticks() {
        return _gc_ticks + _semaphore_ticks + _stub_ticks + _unknown_ticks;
    }


    friend class FlatProfilerTask;

    static void interpreted_update( MethodOop method, KlassOop klass, TickPosition where );

    static void compiled_update( NativeMethod *nm, TickPosition where );

    static std::int32_t entry( std::int32_t value );

    static void record_tick_for_running_frame( Frame fr );

    static void record_tick_for_calling_frame( Frame fr );

public:
    static void allocate_table();

    static void reset();

    static void engage( DeltaProcess *p );

    static DeltaProcess *disengage();

    static bool_t is_active();

    static void print( std::int32_t cutoff );

    static void record_tick();


    static DeltaProcess *process() {
        return _deltaProcess;
    }
};
