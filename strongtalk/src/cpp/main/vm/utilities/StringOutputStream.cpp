
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/utilities/StringOutputStream.hpp"


StringOutputStream::StringOutputStream( const int32_t initial_size ) :
    ConsoleOutputStream() {
    buffer_length = initial_size;
    buffer        = new_resource_array<char>( buffer_length );
    buffer_pos    = 0;
}


void StringOutputStream::put( char c ) {
    if ( buffer_pos >= buffer_length ) {
        // grow string buffer
        char *oldbuf = buffer;
        buffer = new_resource_array<char>( buffer_length * 2 );
        strncpy( buffer, oldbuf, buffer_length );
        buffer_length *= 2;
    }
    buffer[ buffer_pos++ ] = c;
    _position++;
}


char *StringOutputStream::as_string() {
    char *copy = new_resource_array<char>( buffer_pos + 1 );
    memcpy( copy, buffer, buffer_pos );
    copy[ buffer_pos ] = '\0';
    return copy;
}


struct ByteArrayOopDescriptor *StringOutputStream::as_byteArray() {
    ByteArrayOopDescriptor *a = oopFactory::new_byteArray( buffer_pos );
    for ( int32_t          i  = 0; i < buffer_pos; i++ ) {
        a->byte_at_put( i + 1, buffer[ i ] );
    }
    return a;
}
