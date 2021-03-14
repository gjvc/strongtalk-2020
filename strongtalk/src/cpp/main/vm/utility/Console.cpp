
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/platform/platform.hpp"
#include "vm/utility/Console.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"
#include "vm/memory/Universe.hpp"


extern bool bootstrappingInProgress;


template<typename... Args>
void Console::info( Args &... args ) {
    SPDLOG_INFO( args... );
}



void console_init() {
    if ( _console )
        return;
    _console = new( true ) ConsoleOutputStream;
}


void logging_init() {

    auto console = spdlog::stdout_color_mt( "console" );
    spdlog::set_level( spdlog::level::debug );

    spdlog::set_pattern( "%Y-%m-%d  %H:%M:%S.%f  %s:%#  %!  %v" );
    spdlog::set_default_logger( console );

    // announce
    SPDLOG_INFO( "-----------------------------------------------------------------------------" );
    SPDLOG_INFO( ">>> Strongtalk Delta Virtual Machine, {}.{}{} ({}, {})", Universe::major_version(), Universe::minor_version(), Universe::beta_version(), __DATE__, __TIME__ );
    SPDLOG_INFO( ">>> (C) 1994 - 2021, The Strongtalk authors and contributors" );
    SPDLOG_INFO( "-----------------------------------------------------------------------------" );

}
