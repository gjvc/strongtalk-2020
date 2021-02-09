
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/utilities/ConsoleOutputStream.hpp"


ConsoleOutputStream *_console;


void ConsoleOutputStream::basic_print( const char *str ) {

    SPDLOG_INFO( "{}", str );
//    for ( std::size_t i = 0; i < strlen( str ); i++ ) {
//        put( str[ i ] );
//    }

}


void ConsoleOutputStream::print( const char *format, ... ) {
    char buffer[BUFLEN];

    va_list ap;
    va_start( ap, format );
    if ( vsnprintf( buffer, BUFLEN, format, ap ) < 0 ) {
        SPDLOG_WARN( "increase BUFLEN in ConsoleOutputStream.cpp -- output truncated" );
        buffer[ BUFLEN - 1 ] = 0;
    }
    va_end( ap );
    basic_print( buffer );
}


void ConsoleOutputStream::print_cr( const char *format, ... ) {
    char    buffer[BUFLEN];
    va_list ap;
    va_start( ap, format );
    if ( vsnprintf( buffer, BUFLEN, format, ap ) < 0 ) {
        SPDLOG_WARN( "increase BUFLEN in ConsoleOutputStream.cpp -- output truncated" );
        buffer[ BUFLEN - 1 ] = 0;
    }
    va_end( ap );
    basic_print( buffer );
    cr();
}


void ConsoleOutputStream::vprint( const char *format, va_list argptr ) {
    char buffer[BUFLEN];
    if ( vsnprintf( buffer, BUFLEN, format, argptr ) < 0 ) {
        SPDLOG_WARN( "increase BUFLEN in ConsoleOutputStream.cpp -- output truncated" );
        buffer[ BUFLEN - 1 ] = 0;
    }
    basic_print( buffer );
}


ConsoleOutputStream::ConsoleOutputStream( std::int32_t width ) :
    _width{ width },
    _position{ 0 },
    _indentation{ 0 } {
}


void ConsoleOutputStream::indent() {

    while ( _position < _indentation ) {
        sp();
    }

}


void ConsoleOutputStream::vprint_cr( const char *format, va_list argptr ) {
    vprint( format, argptr );
    cr();
}


void ConsoleOutputStream::fill_to( std::int32_t col ) {
    while ( position() < col ) {
        sp();
    }
}


void ConsoleOutputStream::put( char c ) {
    lputc( c );
    _position++;
}


void ConsoleOutputStream::sp() {
    put( ' ' );
}


void ConsoleOutputStream::cr() {
    put( '\n' );
    _position = 0;
}
