
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

// lprintf replaces printf -- never use printf in the VM!
// output written by lprintf can easily be redirected, duplicated into a log file, etc.

extern "C" void lprintf( const char *m, ... );
extern "C" void lputc( const char c );
extern "C" void lputs( const char *str );

// like sprintf, but updates the buf pointer so that subsequent sprintf invocations append to the string
extern "C" void my_sprintf( char *&buf, const char *format, ... );
extern "C" void my_sprintf_len( char *&buf, const std::int32_t len, const char *format, ... );    // make output len chars std::int32_t

void flush_logFile();
