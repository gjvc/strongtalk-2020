
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include <cstdarg>
#include <string>
#include <sstream>

#include "vm/system/asserts.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/DebugNotifier.hpp"

#include "spdlog/sinks/base_sink.h"


template<typename Mutex>
class NotifierSink : public spdlog::sinks::base_sink<Mutex> {
protected:
    void sink_it_( const spdlog::details::log_msg &msg ) override {

        // log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
        // msg.raw contains pre formatted log

        // If needed (very likely but not mandatory), the sink formats the message before sending it to its final destination:
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format( msg, formatted );
        std::cout << fmt::to_string( formatted );

        switch ( msg.level ) {
            case spdlog::level::info:
                break;

            case spdlog::level::warn:
                if ( BreakAtWarning ) {
                    error_breakpoint();
                }
                break;

            case spdlog::level::err:
                error_breakpoint();
                break;

        }

        if ( Notifier::current == nullptr ) {
            Notifier::current = new DebugNotifier();
            // Notifier::current->warning( const_cast<char*( fmt::to_string( formatted ).c_str() ) );
        }

    }


    void flush_() override {
        std::cout << std::flush;
    }

};


Notifier *Notifier::current = nullptr;


void DebugNotifier::error( const char *m, va_list ap ) {
    SPDLOG_INFO( "VM Error:" );
    _console->vprint_cr( m, ap );
    error_breakpoint();
}


void DebugNotifier::warning( const char *m, va_list ap ) {
    SPDLOG_INFO( "VM Warning:" );
    _console->vprint_cr( m, ap );
    if ( BreakAtWarning )
        error_breakpoint();
}


void DebugNotifier::compiler_warning( const char *m, va_list ap ) {
    SPDLOG_INFO( "Compiler Warning:" );
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
