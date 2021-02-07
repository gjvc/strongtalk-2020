
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/flags.hpp"
#include "vm/system/platform.hpp"


// The EventLogEvent log is used for debugging; it is a circular buffer containing the last N events.
// An EventLogEvent is represented by an identifying string and up to EVENT_PARAMS parameters.

const std::int32_t EVENT_PARAMS = 3;       // number of params per EventLogEvent

// helper macros


enum class EventLogEventStatus {
    starting,   //
    ending,     //
    atomic      //
};


struct EventLogEvent /* no superclass - never allocated individually */ {
    const char          *_name;                 // in printf format
    EventLogEventStatus _status;        // for nested events
    const void          *args[EVENT_PARAMS];    //
};


struct EventLog : public CHeapAllocatedObject {

    EventLogEvent *_eventBuffer;    // event buffer
    EventLogEvent *_end;            //
    EventLogEvent *_next;           // where the next entry will go
    std::int32_t  _nestingDepth;               // current nesting depth

    EventLog();

    void init();

    EventLogEvent *nextEvent( EventLogEvent *e, EventLogEvent *start, EventLogEvent *end );

    EventLogEvent *prevEvent( EventLogEvent *e, EventLogEvent *start, EventLogEvent *end );


    void inc() {
        _next = nextEvent( _next, _eventBuffer, _end );
    }


    void log( EventLogEvent *e ) {
        *_next = *e;
        inc();
    }


    void log( const char *name ) {
        _next->_name   = name;
        _next->_status = EventLogEventStatus::atomic;
        inc();
    }


    void log( const char *name, const void *p1 ) {
        _next->_name   = name;
        _next->_status = EventLogEventStatus::atomic;
        _next->args[ 0 ] = p1;
        inc();
    }


    void log( const char *name, const void *p1, const void *p2 ) {
        _next->_name   = name;
        _next->_status = EventLogEventStatus::atomic;
        _next->args[ 0 ] = p1;
        _next->args[ 1 ] = p2;
        inc();
    }


    void log( const char *name, const void *p1, const void *p2, const void *p3 ) {
        _next->_name   = name;
        _next->_status = EventLogEventStatus::atomic;
        _next->args[ 0 ] = p1;
        _next->args[ 1 ] = p2;
        _next->args[ 2 ] = p3;
        inc();
    }


    void resize();                // resize buffer

    void print() {
        printPartial( _end - _eventBuffer );
    }


    void printPartial( std::int32_t n );
};

extern EventLog *eventLog;


class EventMarker : StackAllocatedObject {    // for events which have a duration
public:
    EventLogEvent event;
    EventLogEvent *here;


    EventMarker( const char *n ) :
        event(),
        here{ nullptr } {
        init( n );
    }


    EventMarker( const char *n, const void *p1 ) :
        event(),
        here{ nullptr } {
        init( n, p1 );
    }


    EventMarker( const char *n, const void *p1, const void *p2 ) :
        event(),
        here{ nullptr } {
        init( n, p1, p2 );
    }


    EventMarker( const char *n, const void *p1, const void *p2, const void *p3 ) :
        event(),
        here{ nullptr } {
        init( n, p1, p2, p3 );
    }


    void init( const char *n, const void *p1 = nullptr, const void *p2 = nullptr, const void *p3 = nullptr ) {
        here = eventLog->_next;
        eventLog->log( n, p1, p2, p3 );
        here->_status = EventLogEventStatus::starting;
        event = *here;
        eventLog->_nestingDepth++;
    }



    EventMarker() = default;
    EventMarker( const EventMarker & ) = default;
    EventMarker &operator=( const EventMarker & ) = default;
    void operator delete( void *ptr ) { (void)ptr; }


    virtual ~EventMarker() {
        eventLog->_nestingDepth--;
        // optimization to make log less verbose; this isn't totally failproof but that's ok
        if ( here == eventLog->_next - 1 ) {
            *here = event;
            here->_status = EventLogEventStatus::atomic;       // nothing happened inbetween
        } else {
            event._status = EventLogEventStatus::ending;
            eventLog->log( &event );
        }
    }
};
