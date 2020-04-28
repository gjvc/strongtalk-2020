
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/utilities/OutputStream.hpp"


constexpr size_t BUFLEN{ 64 * 1024 };  // max size of output of individual print() methods

ConsoleOutputStream * _console;


// -----------------------------------------------------------------------------

void ConsoleOutputStream::basic_print( const char * str ) {

    for ( int i = 0; i < strlen( str ); i++ )
        put( str[ i ] );
}


void ConsoleOutputStream::print( const char * format, ... ) {
    char buffer[BUFLEN];

    va_list ap;
    va_start( ap, format );
    if ( vsnprintf( buffer, BUFLEN, format, ap ) < 0 ) {
        warning( "increase BUFLEN in ConsoleOutputStream.cpp -- output truncated" );
        buffer[ BUFLEN - 1 ] = 0;
    }
    va_end( ap );
    basic_print( buffer );
}


void ConsoleOutputStream::print_cr( const char * format, ... ) {
    char    buffer[BUFLEN];
    va_list ap;
    va_start( ap, format );
    if ( vsnprintf( buffer, BUFLEN, format, ap ) < 0 ) {
        warning( "increase BUFLEN in ConsoleOutputStream.cpp -- output truncated" );
        buffer[ BUFLEN - 1 ] = 0;
    }
    va_end( ap );
    basic_print( buffer );
    cr();
}


void ConsoleOutputStream::vprint( const char * format, va_list argptr ) {
    char buffer[BUFLEN];
    if ( vsnprintf( buffer, BUFLEN, format, argptr ) < 0 ) {
        warning( "increase BUFLEN in ConsoleOutputStream.cpp -- output truncated" );
        buffer[ BUFLEN - 1 ] = 0;
    }
    basic_print( buffer );
}


// -----------------------------------------------------------------------------

ConsoleOutputStream::ConsoleOutputStream( int width ) {
    _width       = width;
    _position    = 0;
    _indentation = 0;
}


void ConsoleOutputStream::indent() {
    while ( _position < _indentation )
        sp();
}


void ConsoleOutputStream::vprint_cr( const char * format, va_list argptr ) {
    vprint( format, argptr );
    cr();
}


void ConsoleOutputStream::fill_to( int col ) {
    while ( position() < col )
        sp();
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



// -----------------------------------------------------------------------------


StringOutputStream::StringOutputStream( const int initial_size ) :
    ConsoleOutputStream() {
    buffer_length = initial_size;
    buffer        = new_resource_array <char>( buffer_length );
    buffer_pos    = 0;
}


void StringOutputStream::put( char c ) {
    if ( buffer_pos >= buffer_length ) {
        // grow string buffer
        char * oldbuf = buffer;
        buffer = new_resource_array <char>( buffer_length * 2 );
        strncpy( buffer, oldbuf, buffer_length );
        buffer_length *= 2;
    }
    buffer[ buffer_pos++ ] = c;
    _position++;
}


// -----------------------------------------------------------------------------


char * StringOutputStream::as_string() {
    char * copy = new_resource_array <char>( buffer_pos + 1 );
    memcpy( copy, buffer, buffer_pos );
    copy[ buffer_pos ] = '\0';
    return copy;
}


ByteArrayOop StringOutputStream::as_byteArray() {
    ByteArrayOop a = oopFactory::new_byteArray( buffer_pos );
    for ( int    i = 0; i < buffer_pos; i++ ) {
        a->byte_at_put( i + 1, buffer[ i ] );
    }
    return a;
}


FileOutputStream::FileOutputStream( const char * file_name ) {
    _file.open( file_name );
}


void FileOutputStream::put( char c ) {
    _file.put( c );
    _position++;
}


FileOutputStream::~FileOutputStream() {
    _file.close();
}


void console_init() {
    if ( _console )
        return;
    _console = new( true ) ConsoleOutputStream;
    _console->print_cr( "%%system-init:  ConsoleOutputStream-open" );
}


ConsoleOutputStream * getStd() {
    console_init();
    return _console;
}
