
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#if defined( __MINGW32__ ) || defined( _MSC_VER )

#include "vm/system/platform.hpp"
#include "vm/system/os.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/GrowableArray.hpp"

#include <vector>
#include <windows.h>

LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

const auto STACK_SIZE = ThreadStackSize * 1024;

typedef struct {

    std::int32_t (*main)( void * );
    void *parameter;
    void *stackLimit;

} thread_start_t;

std::int32_t WINAPI startThread( void *params );

class Thread : public CHeapAllocatedObject {

private:
    static std::vector<Thread *>   _threads;
    static GrowableArray<Thread *> *threads;
    static Event                   *thread_created;
    HANDLE                         thread_handle;
    std::int32_t                    thread_id;
    void                           *stack_limit;


    static void initialize() {
        threads        = new( true ) GrowableArray<Thread *>( 10, true );
        thread_created = os::create_event( false );
    }


    static void release() {
    }


    static bool equals( void *token, Thread *element ) {
        return token == (void *) element;
    }


    Thread( HANDLE handle, std::int32_t id, void *stackLimit ) :
            thread_handle( handle ), thread_id( id ), stack_limit( stackLimit ) {

        std::int32_t index = threads->find( nullptr, equals );
        if ( index < 0 ) {
            threads->append( this );
        } else {
            threads->at_put( index, this );
        }
    }


    virtual ~Thread() {
        std::int32_t index = threads->find( this );
        threads->at_put( index, nullptr );
    }


    static Thread *createThread( std::int32_t main( void *parameter ), void *parameter, std::int32_t *id_addr ) {
        ThreadCritical tc;
        thread_start_t params;
        params.main      = main;
        params.parameter = parameter;

        os::reset_event( thread_created );
        HANDLE result = CreateThread( nullptr, STACK_SIZE, (LPTHREAD_START_ROUTINE) startThread, &params, 0, reinterpret_cast<LPDWORD>( id_addr ) );
        if ( result == nullptr ) {
            return nullptr;
        }

        os::wait_for_event( thread_created );

        return new Thread( result, *id_addr, params.stackLimit );
    }


    static Thread *findThread( std::int32_t thread_id ) {
        for ( std::int32_t i = 0; i < threads->length(); i++ ) {
            Thread *thread = threads->at( i );
            if ( thread == nullptr )
                continue;
            if ( thread->thread_id == thread_id )
                return thread;
        }
        return nullptr;
    }


    friend class os;

    friend std::int32_t WINAPI startThread( void * );
    friend void os_init();
    friend void os_exit();
};


#endif
