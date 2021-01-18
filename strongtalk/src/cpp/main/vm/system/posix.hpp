//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#ifdef __linux__

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"

#include "vm/system/posix.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/LongInteger64.hpp"
#include "os.hpp"
#include "vm/utilities/GrowableArray.hpp"

#include <ctime>
#include <cstdio>
#include <csignal>
#include <cerrno>

#include <ucontext.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/times.h>
#include <sys/mman.h>


class Lock {

    private:
        pthread_mutex_t * _mutex;

    public:
        Lock( pthread_mutex_t * mutex );
        ~Lock();

};


class Event : public CHeapAllocatedObject {

    private:
        bool_t          _signalled;
        pthread_mutex_t _mutex{};
        pthread_cond_t  _notifier{};

    public:
        void signal();


        void reset();


        bool_t waitFor();


        Event( bool_t state );


        ~Event();
};


class Thread : CHeapAllocatedObject {

    public:
        static Thread * find( pthread_t threadId ) {
            for ( std::int32_t index = 0; index < _threads->length(); index++ ) {
                Thread * candidate = _threads->at( index );
                if ( candidate == nullptr )
                    continue;
                if ( pthread_equal( threadId, candidate->_threadId ) )
                    return candidate;
            }
            return nullptr;
        }


        void suspend() {
            _suspendEvent.waitFor();
        }


        void resume() {
            _suspendEvent.signal();
        }


    private:
        Event                           _suspendEvent;
        static GrowableArray <Thread *> * _threads;
        pthread_t                       _threadId;
        clockid_t                       _clockId;
        std::int32_t                         _thread_index;
        void                            * _stackLimit;


        static void init() {
            ThreadCritical lock;
            _threads = new( true ) GrowableArray <Thread *>( 10, true );

        }


        Thread( pthread_t threadId, void * stackLimit ) :
            _threadId( threadId ), _suspendEvent( false ), _stackLimit( stackLimit ) {
            ThreadCritical lock;
            pthread_getcpuclockid( _threadId, &_clockId );
            _thread_index = _threads->length();
            _threads->push( this );
        };


        ~Thread() {
            ThreadCritical lock;
            _threads->at_put( _thread_index, nullptr );
        }


        double get_cpu_time() {
            struct timespec cpu;
            clock_gettime( _clockId, &cpu );
            return ( ( double ) cpu.tv_sec ) + ( ( double ) cpu.tv_nsec ) / 1e9;
        }


        friend class os;
};

class DLLLoadError {
};

class DLL : CHeapAllocatedObject {

    private:
        char * _name;
        void * _handle;


        DLL( const char * name ) {
            char * errbuf = new char[1024];
            _handle = dlopen( name, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE );
            checkHandle( _handle, "could not find library: %s" );
            _name = ( char * ) malloc( strlen( name ) + 1 );
            strcpy( _name, name );
        }


        void checkHandle( void * handle, const char * format ) {
            if ( handle == nullptr ) {
                char * message = ( char * ) malloc( 200 );
                sprintf( message, format, dlerror() );
                st_assert( handle != nullptr, message );
                free( message );
            }
        }


        ~DLL() {
            char * errbuf = new char[1024];
            if ( _handle )
                dlclose( _handle );
            if ( _name )
                free( _name );
        }


        bool_t isValid() {
            return ( _handle != nullptr ) and ( _name != nullptr );
        }


    public:
        dll_func lookup( const char * funcname ) {
            char     * errbuf = new char[1024];
            dll_func function = dll_func( dlsym( _handle, funcname ) );

            checkHandle( ( void * ) function, "could not find function: %s" );
            return function;
        }


        friend class os;
};


#endif
