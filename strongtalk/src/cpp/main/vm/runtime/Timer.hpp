//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/utilities/LongInteger64.hpp"


// Timers for simple measurements

class Timer {
    private:
        double _userTime;
        double _systemTime;
    public:
        void start();

        void stop();

        void print();

        double seconds();
};


class ElapsedTimer {

    private:
        LongInteger64 _counter;

    public:
        ElapsedTimer() :
            _counter( 0, 0 ) {
        }


        void start();

        void stop();

        void print();

        double seconds();
};


class TimeStamp {
    private:
        LongInteger64 counter;

    public:
        TimeStamp();

        // update to current elapsed time
        void update();

        // returns seconds since updated
        double seconds();
};

// TraceTime is used for tracing the execution time of a block
// Usage:
//  { TraceTime t("block time")
//    some_code();
//  }
//

class TraceTime {
    private:
        bool_t       active;
        ElapsedTimer t;
    public:
        TraceTime( const char * title, bool_t doit = true );

        ~TraceTime();
};
