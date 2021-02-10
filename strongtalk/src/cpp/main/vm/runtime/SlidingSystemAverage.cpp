//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/runtime/SlidingSystemAverage.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/runtime/PeriodicTask.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/code/StubRoutines.hpp"


// -----------------------------------------------------------------------------

std::array<char, SlidingSystemAverage::buffer_size>             SlidingSystemAverage::_buffer{};
std::array<std::uint32_t, SlidingSystemAverage::number_of_cases>SlidingSystemAverage::_stat;
std::uint32_t                                                   SlidingSystemAverage::_position;



// -----------------------------------------------------------------------------

void SlidingSystemAverage::reset() {
    for ( std::size_t i = 0; i < buffer_size; i++ ) {
        _buffer[ i ] = nowhere;
    }
}


std::array<std::uint32_t, SlidingSystemAverage::number_of_cases> SlidingSystemAverage::update() {
    // clear the array;
    std::uint32_t index = 0;
    for ( ; index < number_of_cases; index++ ) {
        _stat[ index ] = 0;
    }

    index = _position;
    do {
        _stat[ _buffer[ index ] ]++;
        index = ( index + 1 ) % buffer_size;
    } while ( index not_eq _position );

    return _stat;
}


void SlidingSystemAverage::add( char type ) {
    _buffer[ _position ] = type;
    _position = ( _position + 1 ) % buffer_size;
}


// The sweeper task is activated every second (1000 milliseconds).
class SystemAverageTask : public PeriodicTask {

public:
    SystemAverageTask() :
        PeriodicTask( 10 ) {
    }


    void task() {
        char type = '\0';
        if ( last_delta_fp ) {
            if ( theCompiler ) {
                type = SlidingSystemAverage::in_compiler;
            } else if ( garbageCollectionInProgress ) {
                type = SlidingSystemAverage::in_garbage_collect;
            } else if ( DeltaProcess::is_idle() ) {
                type = SlidingSystemAverage::is_idle;
            } else {
                type = SlidingSystemAverage::in_vm;
            }
        } else {
            // interpreted code / compiled code / runtime routine
            Frame fr = DeltaProcess::active()->profile_top_frame();
            if ( fr.is_interpreted_frame() ) {
                type = SlidingSystemAverage::in_interpreted_code;
            } else if ( fr.is_compiled_frame() ) {
                type = SlidingSystemAverage::in_compiled_code;
            } else if ( PolymorphicInlineCache::in_heap( fr.pc() ) ) {
                type = SlidingSystemAverage::in_pic_code;
            } else if ( StubRoutines::contains( fr.pc() ) ) {
                type = SlidingSystemAverage::in_stub_code;
            }
        }
        SlidingSystemAverage::add( type );
    }
};


void systemAverage_init() {
    SPDLOG_INFO( "system-init:  systemAverage_init" );

    SlidingSystemAverage::reset();
    if ( UseSlidingSystemAverage ) {
        ( new SystemAverageTask )->enroll();
    }
}
