
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/platform/os.hpp"

#include <cstring>
#include <fstream>
#include <iostream>


std::ofstream theLogFileOutputStream;
static char   fname[256];

const char *CURRENT_LOG_FILE  = "strongtalk.log";
const char *PREVIOUS_LOG_FILE = "strongtalk.log.old";


// don't use #include files for the things below because this would include the conflicting definitions of lprintf et al.
extern "C" bool PrintVMMessages;
extern "C" bool LogVMMessages;
extern "C" bool AlwaysFlushVMMessages;
extern "C" {
void breakpoint();
void error_breakpoint();
}


void lprintf_exit() {

    if ( theLogFileOutputStream ) {
        theLogFileOutputStream.close();
        remove( fname );
    }

}


static void check_log_file() {
    if ( LogVMMessages and not theLogFileOutputStream ) {
        os::move_file( CURRENT_LOG_FILE, PREVIOUS_LOG_FILE );
        theLogFileOutputStream.open( CURRENT_LOG_FILE );
    }
}


extern "C" void lprintf( const char *m, ... ) {

    char    buf[1024];
    va_list ap;
    va_start( ap, m );
    vsprintf( buf, m, ap );
    va_end( ap );

    check_log_file();
    if ( LogVMMessages ) {
        theLogFileOutputStream << buf;
        if ( AlwaysFlushVMMessages ) {
            theLogFileOutputStream.flush();
        }
    }

    if ( PrintVMMessages ) {
        std::cout << buf;
        if ( AlwaysFlushVMMessages ) {
            std::cout.flush();
        }
    }
}


extern "C" void lputc( const char c ) {

    check_log_file();
    if ( LogVMMessages ) {
//        theLogFileOutputStream << c;
        if ( AlwaysFlushVMMessages ) {
            theLogFileOutputStream.flush();
        }
    }

    if ( PrintVMMessages ) {
        std::cout << c;
        if ( AlwaysFlushVMMessages ) {
            std::cout.flush();
        }
    }

}


extern "C" void lputs( const char *str ) {
    check_log_file();
    if ( LogVMMessages ) {
        theLogFileOutputStream << str;
        if ( AlwaysFlushVMMessages ) {
            theLogFileOutputStream.flush();
        }
    }

    if ( PrintVMMessages ) {
        std::cout << str;
        if ( AlwaysFlushVMMessages ) {
            std::cout.flush();
        }
    }
}


void flush_logFile() {
    if ( theLogFileOutputStream.good() ) {
        theLogFileOutputStream.flush();
    }
}


extern "C" void my_sprintf( char *&buf, const char *format, ... ) {

    // like sprintf, but updates the buf pointer so that subsequent sprintfs append to the string
    va_list ap;
    va_start( ap, format );
    vsprintf( const_cast<char *>(buf), format, ap );
    va_end( ap );

    buf += strlen( buf );
}


extern "C" void my_sprintf_len( char *&buf, const std::int32_t len, const char *format, ... ) {
    char *oldbuf = buf;

    va_list ap;
    va_start( ap, format );
    vsprintf( static_cast<char *>(buf), format, ap );
    va_end( ap );
    buf += strlen( buf );
    for ( ; buf < oldbuf + len; *buf++ = ' ' );

    *buf = '\0';
}
