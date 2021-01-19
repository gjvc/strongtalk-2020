//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"

// A PeriodicTask has the sole purpose of executing its task
// function with regular intervals.
// Usage:
//   PeriodicTask pf(10);
//   pf.enroll();
//   ...
//   pf.deroll();

class PeriodicTask : public CHeapAllocatedObject {

private:
    int _counter;
    int _interval;

    friend void real_time_tick( int delay_time );

public:
    PeriodicTask( int interval_time ); // interval is in milliseconds of elapsed time
    ~PeriodicTask();

    bool_t is_enrolled() const;

    void enroll();

    void deroll();


    bool_t is_pending( int delay_time ) {
        _counter += delay_time;
        return _counter >= _interval;
    }


    virtual void task() = 0;
};
