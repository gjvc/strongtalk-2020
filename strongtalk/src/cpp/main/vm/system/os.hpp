//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/utilities/LongInteger64.hpp"


//typedef void ( __CALLING_CONVENTION *dll_func_ptr_t )( ... );
typedef void (  *dll_func_ptr_t )( ... );

class Thread;

class Event;

class DLL;




// os defines the interface to operating system

class os {

private:
    static std::int32_t _vm_page_size;

    static void initialize_system_info();

    friend void os_init();

public:
    static std::int32_t argc();

    static char **argv();

    static void set_args( std::int32_t argc, char *argv[] );

    static std::int32_t getenv( const char *name, char *buffer, std::int32_t len );

    static void add_exception_handler( void handler( void *fp, void *sp, void *pc ) );

    // We must call updateTimes before calling userTime or currentTime.
    static std::int32_t updateTimes();

    static double userTime();

    static double systemTime();

    static double currentTime();

    // Timing operations for threads
    static double user_time_for( Thread *thread );

    static double system_time_for( Thread *thread );

    // Returns the elapsed time in seconds since the vm started.
    static double elapsedTime();

    // Interface to the win32 performance counter
    static LongInteger64 elapsed_counter();

    static LongInteger64 elapsed_frequency();

    static void timerStart();

    static void timerStop();

    static void timerPrintBuffer();

    static void *get_hInstance();

    static void *get_prevInstance();

    static std::int32_t get_nCmdShow();


    // OS interface to Virtual Memory - used for object memory allocations
    static std::int32_t vm_page_size() {
        return _vm_page_size;
    }


    static char *reserve_memory( std::int32_t size );

    static bool commit_memory( const char *addr, std::int32_t size );

    static bool uncommit_memory( const char *addr, std::int32_t size );

    static bool release_memory( const char *addr, std::int32_t size );

    static bool guard_memory( const char *addr, std::int32_t size );

    static const char *exec_memory( std::int32_t size );

    // OS interface to C memory routines - used for small allocations
    static void *malloc( std::int32_t size );

    static void *calloc( std::int32_t size, char filler );

    static void free( void *p );

    // threads
    static Thread *starting_thread( std::int32_t *id_addr );

    static Thread *create_thread( std::int32_t main( void *parameter ), void *parameter, std::int32_t *id_addr );

    static Event *create_event( bool initial_state );

    static void *stack_limit( Thread *thread );

    static std::int32_t current_thread_id();

    static void wait_for_event( Event *event );

    static void transfer( Thread *from_thread, Event *from_event, Thread *to_thread, Event *to_event );

    static void transfer_and_continue( Thread *from_thread, Event *from_event, Thread *to_thread, Event *to_event );

    static void terminate_thread( Thread *thread );

    static void delete_event( Event *event );

    static void reset_event( Event *event );

    static void signal_event( Event *event );

    static bool wait_for_event_or_timer( Event *event, std::int32_t timeout_in_ms );

    static void sleep( std::int32_t ms );

    // thread support for profiling
    static void suspend_thread( Thread *thread );

    static void resume_thread( Thread *thread );

    static void fetch_top_frame( Thread *thread, std::int32_t **sp, std::int32_t **fp, char **pc );

    static void breakpoint();

    static std::int32_t message_box( const char *title, const char *message );

    static void fatalExit( std::int32_t num );

    static bool move_file( const char *from, const char *to );

    static bool check_directory( const char *dir_name );

    // DLL support
    static dll_func_ptr_t dll_lookup( const char *name, DLL *library );

    static DLL *dll_load( const char *name );

    static bool dll_unload( DLL *library );

    static const char *dll_extension();

    // Platform
    static const char *platform_class_name();

    static std::int32_t error_code();
};


// A critical region for controlling thread transfer at interrupts
class ThreadCritical {

private:
    static bool _initialized;

    friend void os_init();

    friend void os_exit();

    static void intialize();

    static void release();

public:
    static bool initialized() {
        return _initialized;
    }


    ThreadCritical();

    ~ThreadCritical();
};
