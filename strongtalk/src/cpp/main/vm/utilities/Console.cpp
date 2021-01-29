
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/system/platform.hpp"
#include "vm/utilities/Console.hpp"
#include "vm/utilities/OutputStream.hpp"


template<typename... Args>
void Console::info( const char *fmt, Args &... args ) {

}


void console_init() {
    if ( _console )
        return;
    _console = new( true ) ConsoleOutputStream;
    spdlog::info( "%system-init:  ConsoleOutputStream-open" );
}


void logging_init() {

    auto console = spdlog::stdout_color_mt( "console" );
    spdlog::set_level( spdlog::level::debug );

    spdlog::set_pattern( "%Y-%m-%d %H:%M:%S [%l] [%t] - <%s>|<%#>|<%!>,%v" );

    spdlog::set_pattern( "[source %s] [function %!] [line %#] %v" );
    spdlog::set_default_logger( console );

    spdlog::info( "%logging-init: hello-world" );

}
