//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/oops/DoubleByteArrayKlass.hpp"
#include "vm/oops/DoubleByteArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceArea.hpp"


bool_t DoubleByteArrayOopDescriptor::verify() {
    bool_t flag = MemOopDescriptor::verify();
    if ( flag ) {
        int l = length();
        if ( l < 0 ) {
            error( "doubleByteArrayOop %#lx has negative length", this );
            flag = false;
        }
    }
    return flag;
}


void DoubleByteArrayOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    MemOopDescriptor::bootstrap_object( stream );
    stream->read_oop( length_addr() );

    for ( std::size_t i = 1; i <= length(); i++ )
        doubleByte_at_put( i, stream->read_doubleByte() );
}


static int sub_sign( int a, int b ) {
    if ( a < b )
        return -1;

    if ( a > b )
        return 1;

    return 0;
}


int compare_as_doubleBytes( const std::uint16_t *a, const std::uint16_t *b ) {
    // machine dependent code; little endian code
    if ( a[ 0 ] - b[ 0 ] )
        return sub_sign( a[ 0 ], b[ 0 ] );

    return sub_sign( a[ 1 ], b[ 1 ] );
}


int DoubleByteArrayOopDescriptor::compare( DoubleByteArrayOop arg ) {
    // Get the addresses of the length fields
    const std::uint32_t *a = (const std::uint32_t *) length_addr();
    const std::uint32_t *b = (const std::uint32_t *) arg->length_addr();

    // Get the word sizes of the arays
    int a_size = roundTo( SMIOop( *a++ )->value() * sizeof( std::uint16_t ), sizeof( int ) ) / sizeof( int );
    int b_size = roundTo( SMIOop( *b++ )->value() * sizeof( std::uint16_t ), sizeof( int ) ) / sizeof( int );

    const std::uint32_t *a_end = a + min( a_size, b_size );
    while ( a < a_end ) {
        if ( *b++ not_eq *a++ )
            return compare_as_doubleBytes( (const std::uint16_t *) ( a - 1 ), (const std::uint16_t *) ( b - 1 ) );
    }
    return sub_sign( a_size, b_size );
}


/*
int doubleByteArrayOopDescriptor::compare(doubleByteArrayOop arg) {
  // Get the addresses of the length fields
  int         len_a = length();
  std::uint16_t* a     = doubleBytes();

  int         len_b = arg->length();
  std::uint16_t* b     = arg->doubleBytes();

  std::uint16_t* end = len_a <= len_b ? a + len_a : b + len_b;

  while(a < end) {
    int result = *a++ - *b++;
    if (result not_eq 0) {
      if (result < 0) return -1;
      if (result > 0) return  1;
    }
  }
  return sub_sign(len_a, len_b);
}
*/

int DoubleByteArrayOopDescriptor::hash_value() {
    int len = length();
    int result;

    if ( len == 0 ) {
        result = 1;
    } else if ( len == 1 ) {
        result = doubleByte_at( 1 );
    } else {
        std::uint32_t val;
        val    = doubleByte_at( 1 );
        val    = ( val << 3 ) ^ ( doubleByte_at( 2 ) ^ val );
        val    = ( val << 3 ) ^ ( doubleByte_at( len ) ^ val );
        val    = ( val << 3 ) ^ ( doubleByte_at( len - 1 ) ^ val );
        val    = ( val << 3 ) ^ ( doubleByte_at( len / 2 + 1 ) ^ val );
        val    = ( val << 3 ) ^ ( len ^ val );
        result = MarkOopDescriptor::masked_hash( val );
    }
    return result == 0 ? 1 : result;
}


bool_t DoubleByteArrayOopDescriptor::copy_null_terminated( char *buffer, int max_length ) {

    int    len          = length();
    bool_t is_truncated = false;
    if ( len >= max_length ) {
        len          = max_length - 1;
        is_truncated = true;
    }

    for ( std::size_t i = 0; i < len; i++ )
        buffer[ i ] = (char) doubleByte_at( i + 1 );

    buffer[ len ] = '\0';

    return is_truncated;
}


char *DoubleByteArrayOopDescriptor::as_string() {
    int len = length();
    char *str = new_resource_array<char>( len + 1 );
    int index    = 0;
    for ( ; index < len; index++ ) {
        str[ index ] = (char) doubleByte_at( index + 1 );
    }
    str[ index ] = '\0';
    return str;
}
