//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"


class ProfiledNode;

class FlatProfilerTask;

class NativeMethod;


enum TickPosition {
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
        static ProfiledNode ** _table;
        static int          _tableSize;

        static DeltaProcess     * _deltaProcess;
        static FlatProfilerTask * _flatProfilerTask;
        static Timer            _timer;

        static int _gc_ticks;           // total ticks in GC/scavenge
        static int _semaphore_ticks;    //
        static int _stub_ticks;         //
        static int _compiler_ticks;     // total ticks in compilation
        static int _unknown_ticks;      //


        static int total_ticks() {
            return _gc_ticks + _semaphore_ticks + _stub_ticks + _unknown_ticks;
        }


        friend class FlatProfilerTask;

        static void interpreted_update( MethodOop method, KlassOop klass, TickPosition where );

        static void compiled_update( NativeMethod * nm, TickPosition where );

        static int entry( int value );

        static void record_tick_for_running_frame( Frame fr );

        static void record_tick_for_calling_frame( Frame fr );

    public:
        static void allocate_table();

        static void reset();

        static void engage( DeltaProcess * p );

        static DeltaProcess * disengage();

        static bool_t is_active();

        static void print( int cutoff );

        static void record_tick();


        static DeltaProcess * process() {
            return _deltaProcess;
        }
};

