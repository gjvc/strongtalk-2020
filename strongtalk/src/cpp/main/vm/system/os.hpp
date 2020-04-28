//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/utilities/LongInteger64.hpp"


// os defines the interface to operating system

//typedef void ( __CALLING_CONVENTION *dll_func )( ... );
typedef void (  * dll_func )( ... );

class Thread;

class Event;

class DLL;

class os {

    private:
        static int _vm_page_size;

        static void initialize_system_info();

        friend void os_init();

    public:
        static int argc();

        static char ** argv();

        static void set_args( int argc, char * argv[] );

        static int getenv( const char * name, char * buffer, int len );

        static void add_exception_handler( void handler( void * fp, void * sp, void * pc ) );

        // We must call updateTimes before calling userTime or currentTime.
        static int updateTimes();

        static double userTime();

        static double systemTime();

        static double currentTime();

        // Timing operations for threads
        static double user_time_for( Thread * thread );

        static double system_time_for( Thread * thread );

        // Returns the elapsed time in seconds since the vm started.
        static double elapsedTime();

        // Interface to the win32 performance counter
        static LongInteger64 elapsed_counter();

        static LongInteger64 elapsed_frequency();

        static void timerStart();

        static void timerStop();

        static void timerPrintBuffer();

        static void * get_hInstance();

        static void * get_prevInstance();

        static int get_nCmdShow();


        // OS interface to Virtual Memory - used for object memory allocations
        static int vm_page_size() {
            return _vm_page_size;
        }


        static char * reserve_memory( int size );

        static bool_t commit_memory( const char * addr, int size );

        static bool_t uncommit_memory( const char * addr, int size );

        static bool_t release_memory( const char * addr, int size );

        static bool_t guard_memory( const char * addr, int size );

        static const char * exec_memory( int size );

        // OS interface to C memory routines - used for small allocations
        static void * malloc( int size );

        static void * calloc( int size, char filler );

        static void free( void * p );

        // threads
        static Thread * starting_thread( int * id_addr );

        static Thread * create_thread( int main( void * parameter ), void * parameter, int * id_addr );

        static Event * create_event( bool_t initial_state );

        static void * stack_limit( Thread * thread );

        static int current_thread_id();

        static void wait_for_event( Event * event );

        static void transfer( Thread * from_thread, Event * from_event, Thread * to_thread, Event * to_event );

        static void transfer_and_continue( Thread * from_thread, Event * from_event, Thread * to_thread, Event * to_event );

        static void terminate_thread( Thread * thread );

        static void delete_event( Event * event );

        static void reset_event( Event * event );

        static void signal_event( Event * event );

        static bool_t wait_for_event_or_timer( Event * event, int timeout_in_ms );

        static void sleep( int ms );

        // thread support for profiling
        static void suspend_thread( Thread * thread );

        static void resume_thread( Thread * thread );

        static void fetch_top_frame( Thread * thread, int ** sp, int ** fp, char ** pc );

        static void breakpoint();

        static int message_box( const char * title, const char * message );

        static void fatalExit( int num );

        static bool_t move_file( const char * from, const char * to );

        static bool_t check_directory( const char * dir_name );

        // DLL support
        static dll_func dll_lookup( const char * name, DLL * library );

        static DLL * dll_load( const char * name );

        static bool_t dll_unload( DLL * library );

        static const char * dll_extension();

        // Platform
        static const char * platform_class_name();

        static int error_code();
};


// A critical region for controlling thread transfer at interrupts
class ThreadCritical {
    private:
        static bool_t _initialized;

        friend void os_init();

        friend void os_exit();

        static void intialize();

        static void release();

    public:
        static bool_t initialized() {
            return _initialized;
        }


        ThreadCritical();

        ~ThreadCritical();
};

