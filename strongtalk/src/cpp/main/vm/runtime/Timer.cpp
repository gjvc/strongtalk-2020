//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Timer.hpp"
#include "vm/system/os.hpp"
#include "vm/utilities/OutputStream.hpp"


void Timer::start() {
    os::updateTimes();
    _userTime   = os::userTime();
    _systemTime = os::systemTime();
}


void Timer::stop() {
    os::updateTimes();
    _userTime   = os::userTime() - _userTime;
    _systemTime = os::systemTime() - _systemTime;
}


double Timer::seconds() {
    return _userTime;
}


void Timer::print() {
    spdlog::info( "%3.3f", _userTime );
}


void ElapsedTimer::start() {
    _counter = os::elapsed_counter();
}


void ElapsedTimer::stop() {
    _counter = os::elapsed_counter() - _counter;
}


double ElapsedTimer::seconds() {
    double count = _counter.as_double();
    double freq  = os::elapsed_frequency().as_double();
    return count / freq;
}


void ElapsedTimer::print() {
    spdlog::info( "%3.3f", seconds() );
}


TimeStamp::TimeStamp() :
        counter( 0, 0 ) {
}


void TimeStamp::update() {
    counter = os::elapsed_counter();
}


double TimeStamp::seconds() {
    LongInteger64 new_count = os::elapsed_counter();
    double        count     = ( new_count - counter ).as_double();
    double        freq      = os::elapsed_frequency().as_double();
    return count / freq;
}


TraceTime::TraceTime( const char *title, bool doit ) {
    active = doit;
    if ( active ) {
        _console->print( "[%s", title );
        t.start();
    }
}


TraceTime::~TraceTime() {
    if ( active ) {
        t.stop();
        spdlog::info( ", %3.3f secs]", t.seconds() );
    }
}
