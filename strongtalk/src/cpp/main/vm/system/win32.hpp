
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

typedef struct {
    int (* main)( void * );
    void * parameter;
    void * stackLimit;
} thread_start_t;

int WINAPI startThread( void * params );


class Thread : public CHeapAllocatedObject {

    private:
        static std::vector <Thread *>   _threads_;
        static GrowableArray <Thread *> * _threads;
        static Event * _thread_created;
        HANDLE _thread_handle;
        int    _thread_id;
        void * _stack_limit;


        static void initialize() {
            _threads        = new( true ) GrowableArray <Thread *>( 10, true );
            _thread_created = os::create_event( false );
        }


        static void release() {
        }


        static bool_t equals( void * token, Thread * element ) {
            return token == ( void * ) element;
        }


        Thread( HANDLE handle, int id, void * stackLimit ) :
            _thread_handle( handle ), _thread_id( id ), _stack_limit( stackLimit ) {
            _console->print_cr( "Thread::Thread()" );

            int index = _threads->find( nullptr, equals );
            if ( index < 0 )
                _threads->append( this );
            else
                _threads->at_put( index, this );
        }


        virtual ~Thread() {
            _console->print_cr( "Thread::~Thread()" );
            int index = _threads->find( this );
            _threads->at_put( index, nullptr );
        }


        static Thread * createThread( int main( void * parameter ), void * parameter, int * id_addr ) {
            _console->print_cr( "Thread::createThread()" );

            ThreadCritical tc;
            thread_start_t params;
            params.main      = main;
            params.parameter = parameter;

            os::reset_event( _thread_created );
            HANDLE result = CreateThread( nullptr, STACK_SIZE, ( LPTHREAD_START_ROUTINE ) startThread, &params, 0, reinterpret_cast<LPDWORD>( id_addr ) );
            if ( result == nullptr )
                return nullptr;

            os::wait_for_event( _thread_created );

            return new Thread( result, *id_addr, params.stackLimit );
        }


        static Thread * findThread( int thread_id ) {
            for ( int i = 0; i < _threads->length(); i++ ) {
                Thread * thread = _threads->at( i );
                if ( thread == nullptr )
                    continue;
                if ( thread->_thread_id == thread_id )
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
