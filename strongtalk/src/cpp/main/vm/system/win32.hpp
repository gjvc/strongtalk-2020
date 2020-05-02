
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#if defined( __MINGW32__ )

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/memory/util.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/utilities/DebugNotifier.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/system/bits.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/LongInteger64.hpp"
#include "vm/system/os.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/vmOperations.hpp"

#include <vector>
#include <windows.h>


const auto STACK_SIZE = ThreadStackSize * 1024;

typedef struct _thread_start {
    int (* main)( void * );
    void * parameter;
    void * stackLimit;
} thread_start;

int WINAPI startThread( void * params );



class Thread : public CHeapAllocatedObject {

    private:
//        static std::vector <Thread *>   _threads;
        static GrowableArray <Thread *> * threads;
        static Event * thread_created;
        HANDLE thread_handle;
        int    thread_id;
        void * stack_limit;


        static void initialize() {
            threads        = new( true ) GrowableArray <Thread *>( 10, true );
            thread_created = os::create_event( false );
        }


        static void release() {
        }


        static bool_t equals( void * token, Thread * element ) {
            return token == ( void * ) element;
        }


        Thread( HANDLE handle, int id, void * stackLimit ) :
            thread_handle( handle ), thread_id( id ), stack_limit( stackLimit ) {
            _console->print_cr( "Thread::Thread()" );

            int index = threads->find( nullptr, equals );
            if ( index < 0 )
                threads->append( this );
            else
                threads->at_put( index, this );
        }


        virtual ~Thread() {
            _console->print_cr( "Thread::~Thread()" );
            int index = threads->find( this );
            threads->at_put( index, nullptr );
        }


        static Thread * createThread( int main( void * parameter ), void * parameter, int * id_addr ) {
            _console->print_cr( "Thread::createThread()" );

            ThreadCritical tc;
            thread_start   params;
            params.main      = main;
            params.parameter = parameter;

            os::reset_event( thread_created );
            HANDLE result = CreateThread( nullptr, STACK_SIZE, ( LPTHREAD_START_ROUTINE ) startThread, &params, 0, reinterpret_cast<LPDWORD>( id_addr ) );
            if ( result == nullptr )
                return nullptr;

            os::wait_for_event( thread_created );

            return new Thread( result, *id_addr, params.stackLimit );
        }


        static Thread * findThread( int thread_id ) {
            for ( int i = 0; i < threads->length(); i++ ) {
                Thread * thread = threads->at( i );
                if ( thread == nullptr )
                    continue;
                if ( thread->thread_id == thread_id )
                    return thread;
            }
            return nullptr;
        }


        friend class os;

        friend int WINAPI startThread( void * );
        friend void os_init();
        friend void os_exit();
};


#endif
