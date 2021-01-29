//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include <cstdarg>

#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/DebugNotifier.hpp"

Notifier *Notifier::current = nullptr;


void DebugNotifier::error( const char *m, va_list ap ) {
    spdlog::info( "VM Error:" );
    _console->vprint_cr( m, ap );
    error_breakpoint();
}


void DebugNotifier::warning( const char *m, va_list ap ) {
    spdlog::info( "VM Warning:" );
    _console->vprint_cr( m, ap );
    if ( BreakAtWarning )
        error_breakpoint();
}


void DebugNotifier::compiler_warning( const char *m, va_list ap ) {
    spdlog::info( "Compiler Warning:" );
    _console->vprint_cr( m, ap );
    if ( BreakAtWarning )
        error_breakpoint();
}


extern "C" void error( const char *format, ... ) {
    va_list ap;
    va_start( ap, format );
    if ( Notifier::current == nullptr )
        Notifier::current = new DebugNotifier();
    Notifier::current->error( format, ap );
    va_end( ap );
}

extern "C" void warn( const char *format, ... ) {
    va_list ap;
    va_start( ap, format );
    if ( Notifier::current == nullptr )
        Notifier::current = new DebugNotifier();
    Notifier::current->warning( format, ap );
    va_end( ap );
}

extern "C" void compiler_warning( const char *format, ... ) {
    if ( PrintCompilerWarnings ) {
        va_list ap;
        va_start( ap, format );
        if ( Notifier::current == nullptr )
            Notifier::current = new DebugNotifier();
        Notifier::current->compiler_warning( format, ap );
        va_end( ap );
    }
}
