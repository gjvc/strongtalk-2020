
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


HINSTANCE _hInstance     = nullptr;
HINSTANCE _hPrevInstance = nullptr;
int       _nCmdShow      = 0;


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


int WINAPI startThread( void * params ) {
    _console->print_cr( "::startThread()" );

    char * spptr;
    asm( "mov %%esp, %0;" :"=r"(spptr) );
    int stackHeadroom = 2 * os::vm_page_size();
    ( ( thread_start * ) params )->stackLimit = spptr - STACK_SIZE + stackHeadroom;

    int (* main)( void * ) = ( ( thread_start * ) params )->main;
    void * parameter = ( ( thread_start * ) params )->parameter;

    os::signal_event( Thread::thread_created );
    return main( parameter );
}


Event * Thread::thread_created = nullptr;
GrowableArray <Thread *> * Thread::threads = nullptr;

static HANDLE main_process;
static int    main_thread_id;
static HANDLE watcher_thread;
static Thread * main_thread;

static FILETIME process_creation_time;
static FILETIME process_exit_time;
static FILETIME process_user_time;
static FILETIME process_kernel_time;

extern void intercept_for_single_step();


static inline double fileTimeAsDouble( FILETIME * time ) {
    const double high   = ( double ) ( ( uint32_t ) ~0 );
    const double split  = 10000000.0;
    double       result = ( time->dwLowDateTime / split ) + time->dwHighDateTime * ( high / split );
    return result;
}


int os::getenv( const char * name, char * buffer, int len ) {
    int result = GetEnvironmentVariable( name, buffer, len );
    return result not_eq 0;
}


bool_t os::move_file( const char * from, const char * to ) {
    return MoveFileEx( from, to, MOVEFILE_REPLACE_EXISTING ) ? true : false;
}


bool_t os::check_directory( const char * dir_name ) {
    bool_t result = CreateDirectory( dir_name, nullptr ) ? true : false;
    if ( not result ) {
        int error = GetLastError();
        if ( error == ERROR_ALREADY_EXISTS )
            return true;
        return false;
    }
    return true;
}


void os::breakpoint() {
    DebugBreak();
}


Thread * os::starting_thread( int * id_addr ) {
    _console->print_cr( "os::starting_thread()" );

    *id_addr = main_thread_id;
    return main_thread;
}


Thread * os::create_thread( int main( void * parameter ), void * parameter, int * id_addr ) {
    return Thread::createThread( main, parameter, id_addr );
}


void * os::stack_limit( Thread * thread ) {
    return thread->stack_limit;
}


void os::terminate_thread( Thread * thread ) {
    _console->print_cr( "os::terminate_thread()" );

    HANDLE handle = thread->thread_handle;
    delete thread;
//    thread = nullptr;

    TerminateThread( handle, 0 );
    CloseHandle( handle );
}


void os::delete_event( Event * event ) {
    CloseHandle( ( HANDLE ) event );
}


Event * os::create_event( bool_t initial_state ) {
    HANDLE result = CreateEvent( nullptr, TRUE, initial_state, nullptr );
    if ( result == nullptr ) st_fatal( "CreateEvent failed" );
    return ( Event * ) result;
}


int os::updateTimes() {
    return GetProcessTimes( main_process, &process_creation_time, &process_exit_time, &process_kernel_time, &process_user_time );
}


double os::userTime() {
    return fileTimeAsDouble( &process_user_time );
}


double os::systemTime() {
    return fileTimeAsDouble( &process_kernel_time );
}


double os::user_time_for( Thread * thread ) {
    FILETIME creation_time;
    FILETIME exit_time;
    FILETIME user_time;
    FILETIME kernel_time;
    if ( GetThreadTimes( main_process, &creation_time, &exit_time, &kernel_time, &user_time ) ) {
        return fileTimeAsDouble( &user_time );
    }
    return 0.0;
}


double os::system_time_for( Thread * thread ) {
    FILETIME creation_time;
    FILETIME exit_time;
    FILETIME user_time;
    FILETIME kernel_time;
    if ( GetThreadTimes( main_process, &creation_time, &exit_time, &kernel_time, &user_time ) ) {
        return fileTimeAsDouble( &kernel_time );
    }
    return 0.0;
}


static int           has_performance_count = 0;
static LongInteger64 initial_performance_count( 0, 0 );
static LongInteger64 performance_frequency( 0, 0 );


LongInteger64 os::elapsed_counter() {
    LARGE_INTEGER count;
    QueryPerformanceCounter( &count );
    LongInteger64 current( count.LowPart, count.HighPart );
    return current;
}


LongInteger64 os::elapsed_frequency() {
    return performance_frequency;
}


static void initialze_performance_counter() {
    LARGE_INTEGER count;
    if ( QueryPerformanceFrequency( &count ) ) {
        has_performance_count = 1;
        performance_frequency = LongInteger64( count.LowPart, count.HighPart );
        QueryPerformanceCounter( &count );
        initial_performance_count = LongInteger64( count.LowPart, count.HighPart );
    } else {
        has_performance_count = 0;
    }
}


double os::elapsedTime() {
    if ( not has_performance_count )
        return 0.0;
    LARGE_INTEGER current_count;
    QueryPerformanceCounter( &current_count );

    LongInteger64 current( current_count.LowPart, current_count.HighPart );
    double        count = ( current - initial_performance_count ).as_double();
    double        freq  = performance_frequency.as_double();

    return count / freq;
}


double os::currentTime() {
    SYSTEMTIME s;
    FILETIME   f;
    GetSystemTime( &s );
    SystemTimeToFileTime( &s, &f );
    return fileTimeAsDouble( &f );
}


void os::fatalExit( int num ) {
    FatalExit( num );
}


dll_func os::dll_lookup( const char * name, DLL * library ) {
    dll_func result = ( dll_func ) GetProcAddress( ( HINSTANCE ) library, name );
    return result;
}


DLL * os::dll_load( const char * name ) {
    HINSTANCE lib = LoadLibrary( name );
    return ( DLL * ) lib;
}


bool_t os::dll_unload( DLL * library ) {
    return FreeLibrary( ( HINSTANCE ) library ) ? true : false;
}


const char * os::dll_extension() {
    return ".dll";
}


const char * exception_name( uint32_t code ) {
    switch ( code ) {
        case EXCEPTION_ACCESS_VIOLATION:
            return "Access violation";
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            return "Data misaligned";
        case EXCEPTION_BREAKPOINT:
            return "Breakpoint";
        case EXCEPTION_SINGLE_STEP:
            return "Single step";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "Array bounds exceeded";
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            return "Float denormal operand";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            return "Float divide by zero";
        case EXCEPTION_FLT_INEXACT_RESULT:
            return "Float inexact result";
        case EXCEPTION_FLT_INVALID_OPERATION:
            return "Float invalid operation";
        case EXCEPTION_FLT_OVERFLOW:
            return "Float overflow";
        case EXCEPTION_FLT_STACK_CHECK:
            return "Float stack check";
        case EXCEPTION_FLT_UNDERFLOW:
            return "Float underflow";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "Integer divide by zero";
        case EXCEPTION_INT_OVERFLOW:
            return "Integer overflow";
        case EXCEPTION_PRIV_INSTRUCTION:
            return "Privileged instruction";
        case EXCEPTION_IN_PAGE_ERROR:
            return "In page error";
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return "Illegal Instruction";
        case EXCEPTION_STACK_OVERFLOW:
            return "Stack overflow";
        case EXCEPTION_GUARD_PAGE:
            return "Guard page";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            return "Noncontinuable exception";
        case EXCEPTION_INVALID_DISPOSITION:
            return "Invalid disposition";
        default:
            return "Unknown exception";
    }
}


void trace_stack_at_exception( int * sp, int * fp, const char * pc );
void suspend_process_at_stack_overflow( int * sp, int * fp, const char * pc );


LONG WINAPI topLevelExceptionFilter( struct _EXCEPTION_POINTERS * exceptionInfo ) {

    uint32_t code = exceptionInfo->ExceptionRecord->ExceptionCode;

    if ( code == EXCEPTION_BREAKPOINT ) {
        // This exception is called when an assertion fails (__asm { int 3} is executed).
        // It is therefore imperative we continue the search hereby enabling spawning of a Just-in-time debugger.
        return EXCEPTION_CONTINUE_SEARCH;
    }

    lprintf( "Exception caught \"%s\".\n", exception_name( code ) );

    if ( code == EXCEPTION_STACK_OVERFLOW ) {
        lprintf( "  Oops, we encountered a stack overflow.\n" );
        lprintf( "  You should check your program for infinite recursion!\n" );
        suspend_process_at_stack_overflow( ( int * ) exceptionInfo->ContextRecord->Esp, ( int * ) exceptionInfo->ContextRecord->Ebp, ( const char * ) exceptionInfo->ContextRecord->Eip );
        lprintf( "  Continue execution ?\n" );
    } else {
        // Do not report vm state when getting stack overflow
        report_vm_state();
    }

    if ( os::message_box( "Exception caught", "Do you want a stack trace?" ) ) {
        trace_stack_at_exception( ( int * ) exceptionInfo->ContextRecord->Esp, ( int * ) exceptionInfo->ContextRecord->Ebp, ( const char * ) exceptionInfo->ContextRecord->Eip );
    }
    return EXCEPTION_CONTINUE_SEARCH;
}


void * os::get_hInstance() {
    return ( void * ) _hInstance;
}


void * os::get_prevInstance() {
    return ( void * ) _hPrevInstance;
}


int os::get_nCmdShow() {
    return _nCmdShow;
}


extern int     bootstrappingInProgress;
static CONTEXT context;


void os::timerStart() {
}


void os::timerStop() {
}


void os::timerPrintBuffer() {
}

// Virtual Memory

char * os::reserve_memory( int size ) {
    return ( char * ) VirtualAlloc( nullptr, size, MEM_RESERVE, PAGE_READWRITE );
}


bool_t os::commit_memory( const char * addr, int size ) {
    bool_t result = VirtualAlloc( const_cast<char *>( addr ), size, MEM_COMMIT, PAGE_READWRITE ) not_eq nullptr;
    if ( not result ) {
        int error = GetLastError();
        lprintf( "commit_memory error %d 0x%lx\n", error, error );
    }
    return result;
}


bool_t os::uncommit_memory( const char * addr, int size ) {
    return VirtualFree( const_cast<char *>( addr ), size, MEM_DECOMMIT ) ? true : false;
}


bool_t os::release_memory( const char * addr, int size ) {
    return VirtualFree( const_cast<char *>( addr ), 0, MEM_RELEASE ) ? true : false;
}


bool_t os::guard_memory( const char * addr, int size ) {
    DWORD old_status;
    return VirtualProtect( const_cast<char *>( addr ), size, PAGE_READWRITE | PAGE_GUARD, &old_status ) ? true : false;
}


const char * os::exec_memory( int size ) {
    return reinterpret_cast<char *>( VirtualAlloc( nullptr, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE ) );
}


void * os::malloc( int size ) {
    if ( not ThreadCritical::initialized() ) {
        return ::malloc( size );
    } else {
        ThreadCritical tc;
        return ::malloc( size );
    }
}


void * os::calloc( int size, char filler ) {
    if ( not ThreadCritical::initialized() ) {
        return ::calloc( size, filler );
    } else {
        ThreadCritical tc;
        return ::calloc( size, filler );
    }
}


void os::free( void * p ) {
    if ( not ThreadCritical::initialized() ) {
        ::free( p );
    } else {
        ThreadCritical tc;
        ::free( p );
    }
}


void os::transfer( Thread * from_thread, Event * from_event, Thread * to_thread, Event * to_event ) {
    _console->print_cr( "os::transfer()" );

    ResetEvent( ( HANDLE ) from_event );
    SetEvent( ( HANDLE ) to_event );
    WaitForSingleObject( ( HANDLE ) from_event, INFINITE );
}


void os::transfer_and_continue( Thread * from_thread, Event * from_event, Thread * to_thread, Event * to_event ) {
    _console->print_cr( "os::transfer_and_continue()" );

    ResetEvent( ( HANDLE ) from_event );
    SetEvent( ( HANDLE ) to_event );
}


void os::suspend_thread( Thread * thread ) {
    SuspendThread( thread->thread_handle );
}


void os::resume_thread( Thread * thread ) {
    ResumeThread( thread->thread_handle );
}


void os::sleep( int ms ) {
    Sleep( ms );
}


void os::fetch_top_frame( Thread * thread, int ** sp, int ** fp, char ** pc ) {
    CONTEXT context;
    context.ContextFlags = CONTEXT_CONTROL;
    if ( GetThreadContext( thread->thread_handle, &context ) ) {
        *sp = ( int * ) context.Esp;
        *fp = ( int * ) context.Ebp;
        *pc = reinterpret_cast<char *>( context.Eip );
    } else {
        *sp = nullptr;
        *fp = nullptr;
        *pc = nullptr;
    }
}


int os::current_thread_id() {
    return GetCurrentThreadId();
}


void os::wait_for_event( Event * event ) {
    WaitForSingleObject( ( HANDLE ) event, INFINITE );
}


void os::reset_event( Event * event ) {
    ResetEvent( ( HANDLE ) event );
}


void os::signal_event( Event * event ) {
    SetEvent( ( HANDLE ) event );
}


bool_t os::wait_for_event_or_timer( Event * event, int timeout_in_ms ) {
    return WAIT_TIMEOUT == WaitForSingleObject( ( HANDLE ) event, timeout_in_ms );
}


extern "C" bool_t WizardMode;

void process_settings_file( const char * file_name, bool_t quiet );

static int number_of_ctrl_c = 0;


bool_t WINAPI HandlerRoutine( DWORD dwCtrlType ) {

    if ( CTRL_BREAK_EVENT == dwCtrlType ) {
        _console->print_cr( "%%break" );
        intercept_for_single_step();

    } else {
        if ( number_of_ctrl_c < 4 ) {
            _console->print_cr( "%%break-loading-breakrc" );
            process_settings_file( ".breakrc", false );
        } else {
            lprintf( "\n{aborting}\n" );

#ifdef __GNUC__
            __asm__("int3;");
#else
            __asm int 3
#endif

            breakpoint();
        }
        number_of_ctrl_c++;
    }

    return TRUE;
}


void real_time_tick( int delay_time );

// The sole purpose of the watcher thread is simulating timer interrupts.

DWORD WINAPI WatcherMain( LPVOID lpvParam ) {
    const int delay_interval = 1; // Delay 1 ms
    while ( 1 ) {
        Sleep( delay_interval );
        real_time_tick( delay_interval );
    }
    return 0;
}


int os::_vm_page_size = 0;


void os::initialize_system_info() {
    _console->print_cr( "os::initialize_system_info()" );

    SYSTEM_INFO si;
    GetSystemInfo( &si );
    _vm_page_size = si.dwPageSize;
    initialze_performance_counter();
}


int os::message_box( const char * title, const char * message ) {
    int result = MessageBox( nullptr, message, title, MB_YESNO | MB_ICONERROR | MB_SYSTEMMODAL | MB_DEFAULT_DESKTOP_ONLY );
//    int result = IDYES; // ugly hack to reduce DLL depends
    return result == IDYES;
}


const char * os::platform_class_name() {
    return "Win32Platform";
}


extern "C" bool_t EnableTasks;

LARGE_INTEGER counter;

CRITICAL_SECTION ThreadSection;

bool_t ThreadCritical::_initialized = false;


void ThreadCritical::intialize() {
    InitializeCriticalSection( &ThreadSection );
    _initialized = true;
}


void ThreadCritical::release() {
    DeleteCriticalSection( &ThreadSection );
}


ThreadCritical::ThreadCritical() {
    EnterCriticalSection( &ThreadSection );
}


ThreadCritical::~ThreadCritical() {
    LeaveCriticalSection( &ThreadSection );
}


void (* handler)( void * fp, void * sp, void * pc ) = nullptr;
bool_t handling_exception;


LONG WINAPI testVectoredHandler( struct _EXCEPTION_POINTERS * exceptionInfo ) {

    lprintf( "Caught exception.\n" );
    if ( true and handler and not handling_exception ) {
        handling_exception = true;
        handler( ( void * ) exceptionInfo->ContextRecord->Ebp, ( void * ) exceptionInfo->ContextRecord->Esp, ( void * ) exceptionInfo->ContextRecord->Eip );
        handling_exception = false;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}


void os::add_exception_handler( void new_handler( void * fp, void * sp, void * pc ) ) {
    handler = new_handler;
    AddVectoredExceptionHandler( 0, testVectoredHandler );
}


int os::error_code() {
    return GetLastError();
}


void os_init_processor_affinity() {

    // todo: remove this processor affinity stuff
    ULONG systemMask;
    ULONG processMask;
    GetProcessAffinityMask( GetCurrentProcess(), &processMask, &systemMask );

    ULONG processorId = 1;
    while ( not( processMask & processorId ) and processorId < processMask )
        processorId >>= 1;

    _console->print_cr( "%%system-init:  os-init:  set-processor-affinity: processorId: [%ld]", processorId );
    if ( not SetProcessAffinityMask( GetCurrentProcess(), processorId ) )
        _console->print_cr( "error code: %d", GetLastError() );

}


void os_init() {
    _console->print_cr( "os_init" );

    ThreadCritical::intialize();
    Thread::initialize();

    if ( _hInstance == nullptr ) {
        _hInstance = GetModuleHandle( nullptr );
        _nCmdShow  = SW_SHOWNORMAL;
    }

    os::initialize_system_info();

    SetConsoleCtrlHandler( &HandlerRoutine, TRUE );

    HANDLE threadHandle;

    // Initialize main_process and main_thread
    main_process = GetCurrentProcess();  // Remember main_process is a pseudo handle
    if ( not DuplicateHandle( main_process, GetCurrentThread(), main_process, &threadHandle, THREAD_ALL_ACCESS, FALSE, 0 ) ) {
        st_fatal( "DuplicateHandle failed\n" );
    }
    main_thread_id = ( int ) GetCurrentThreadId();

    main_thread = new Thread( threadHandle, main_thread_id, nullptr );

    // Setup Windows Exceptions
    SetUnhandledExceptionFilter( topLevelExceptionFilter );

    // Create the watcher thread

    if ( EnableTasks ) {
        DWORD watcher_id;
        watcher_thread = CreateThread( nullptr, 0, &WatcherMain, 0, 0, &watcher_id );
        SetThreadPriority( watcher_thread, THREAD_PRIORITY_HIGHEST );
    }
}


void os_exit() {
    _console->print_cr( "os_exit" );
    Thread::release();
    ThreadCritical::release();
}


void os::set_args( int argc, char * argv[] ) {
}


int os::argc() {
    return __argc;
}


char ** os::argv() {
    return __argv;
}


#endif
