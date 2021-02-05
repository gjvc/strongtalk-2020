//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/system/os.hpp"
#include "vm/runtime/PeriodicTask.hpp"

constexpr std::int32_t max_tasks = 10;
std::int32_t           num_tasks = 0;

std::array<PeriodicTask *, max_tasks> tasks;


bool pending_tasks( std::int32_t delay_time ) {
    bool result = false;

    for ( std::int32_t i = 0; i < num_tasks; i++ ) {
        result = tasks[ i ]->is_pending( delay_time ) or result;
    }
    return result;
}


void real_time_tick( std::int32_t delay_time ) {

    // Do not perform any tasks while bootstrappingInProgress
    if ( bootstrappingInProgress )
        return;

    if ( pending_tasks( delay_time ) ) {
        ThreadCritical tc;
        if ( not Process::external_suspend_current() )
            return;

        for ( std::int32_t i = 0; i < num_tasks; i++ ) {
            PeriodicTask *task = tasks[ i ];
            if ( task->_counter >= task->_interval ) {
                task->task();
                task->_counter = 0;
            }
        }

        Process::external_resume_current();
    }
}


PeriodicTask::PeriodicTask( std::int32_t interval_time ) :
    _counter{ 0 },
    _interval{ interval_time } {
}


PeriodicTask::~PeriodicTask() {
    if ( is_enrolled() ) {
        deroll();
    }
}


bool PeriodicTask::is_enrolled() const {
    for ( std::int32_t i = 0; i < num_tasks; i++ )
        if ( tasks[ i ] == this )
            return true;
    return false;
}


void PeriodicTask::enroll() {
    if ( num_tasks == max_tasks ) st_fatal( "Overflow in PeriodicTask table" );
    tasks[ num_tasks++ ] = this;
}


void PeriodicTask::deroll() {
    std::int32_t index = 0;
    for ( ; index < num_tasks and tasks[ index ] not_eq this; index++ ) {
        void( 1 );
    }

    if ( index == max_tasks ) {
        return;
    }

    num_tasks--;
    for ( ; index < num_tasks; index++ ) {
        tasks[ index ] = tasks[ index + 1 ];
    }
}
