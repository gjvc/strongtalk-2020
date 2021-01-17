
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/memory/util.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/oops/DoubleByteArrayOopDescriptor.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"


bool_t ByteArrayOopDescriptor::verify() {
    bool_t flag = MemOopDescriptor::verify();
    if ( flag ) {
        int l = length();
        if ( l < 0 ) {
            error( "ByteArrayOop %#lx has negative length", this );
            flag = false;
        }
    }
    return flag;
}


char * ByteArrayOopDescriptor::copy_null_terminated( int & Clength ) {
    // Copy the bytes() part. Always add trailing '\0'. If byte array
    // contains '\0', these will be escaped in the copy, i.e. "....\0...".
    // Clength is set to length of the copy (may be longer due to escaping).
    // Presence of null chars can be detected by comparing Clength to length().

    st_assert_byteArray( this, "should be a byte array" );
    Clength = length();
    char * res = copy_string( ( const char * ) bytes(), Clength );
    if ( strlen( res ) == ( uint32_t ) Clength )
        return res;                   // Simple case, no '\0' in byte array.

    // Simple case failed ...
    smi_t     t = length();               // Copy and 'escape' null chars.
    smi_t     i;
    for ( int i = length() - 1; i >= 0; i-- )
        if ( byte_at( i ) == '\0' )
            t++;
    // t is total length of result string.
    res = new_resource_array <char>( t + 1 );
    res[ t-- ] = '\0';
    Clength = t;
    for ( int i = length() - 1; i >= 0; i-- ) {
        if ( byte_at( i ) not_eq '\0' ) {
            res[ t-- ] = byte_at( i );
        } else {
            res[ t-- ] = '0';
            res[ t-- ] = '\\';
        }
    }
    st_assert( t == -1, "sanity check" );
    return res;
}


char * ByteArrayOopDescriptor::copy_c_heap_null_terminated() {
    // Copy the bytes() part. Always add trailing '\0'. If byte array
    // contains '\0', these will be escaped in the copy, i.e. "....\0...".
    // NOTE: The resulting string is allocated in Cheap

    st_assert_byteArray( this, "should be a byte array" );
    smi_t     t = length();               // Copy and 'escape' null chars.
    smi_t     i;
    for ( int i = length() - 1; i >= 0; i-- )
        if ( byte_at( i ) == '\0' )
            t++;
    // t is total length of result string.
    char * res = new_c_heap_array <char>( t + 1 );
    res[ t-- ] = '\0';
    for ( int i = length() - 1; i >= 0; i-- ) {
        if ( byte_at( i ) not_eq '\0' ) {
            res[ t-- ] = byte_at( i );
        } else {
            res[ t-- ] = '0';
            res[ t-- ] = '\\';
        }
    }
    st_assert( t == -1, "sanity check" );
    return res;
}


bool_t ByteArrayOopDescriptor::copy_null_terminated( char * buffer, int max_length ) {
    // %not optimized

    int len = length();

    bool_t is_truncated = false;
    if ( len >= max_length ) {

        len          = max_length - 1;
        is_truncated = true;
    }

    for ( int i = 0; i < len; i++ )
        buffer[ i ] = byte_at( i + 1 );

    buffer[ len ] = '\0';

    return is_truncated;
}


void ByteArrayOopDescriptor::bootstrap_object( Bootstrap * stream ) {
    MemOopDescriptor::bootstrap_object( stream );

    stream->read_oop( length_addr() );
    for ( int i = 1; i <= length(); i++ ) {
        byte_at_put( i, stream->read_byte() );
    }
}


static int sub_sign( int a, int b ) {
    if ( a < b )
        return -1;
    if ( a > b )
        return 1;
    return 0;
}


int compare_as_bytes( const uint8_t * a, const uint8_t * b ) {
    // machine dependent code; little endian code
    if ( a[ 0 ] - b[ 0 ] )
        return sub_sign( a[ 0 ], b[ 0 ] );
    if ( a[ 1 ] - b[ 1 ] )
        return sub_sign( a[ 1 ], b[ 1 ] );
    if ( a[ 2 ] - b[ 2 ] )
        return sub_sign( a[ 2 ], b[ 2 ] );
    return sub_sign( a[ 3 ], b[ 3 ] );
}


int ByteArrayOopDescriptor::compare( ByteArrayOop arg ) {
    // Get the addresses of the length fields
    const uint32_t * a = ( uint32_t * ) length_addr();
    const uint32_t * b = ( uint32_t * ) arg->length_addr();

    // Get the word sizes of the arays
    int a_size = roundTo( SMIOop( *a++ )->value() * sizeof( char ), sizeof( int ) ) / sizeof( int );
    int b_size = roundTo( SMIOop( *b++ )->value() * sizeof( char ), sizeof( int ) ) / sizeof( int );

    const uint32_t * a_end = a + min( a_size, b_size );
    while ( a < a_end ) {
        if ( *b++ not_eq *a++ )
            return compare_as_bytes( ( const uint8_t * ) ( a - 1 ), ( const uint8_t * ) ( b - 1 ) );
    }
    return sub_sign( a_size, b_size );
}


int ByteArrayOopDescriptor::compare_doubleBytes( DoubleByteArrayOop arg ) {
    // %not optimized
    int s1 = length();
    int s2 = arg->length();
    int n  = s1 < s2 ? s1 : s2;

    for ( int i = 1; i <= n; i++ ) {
        int result = sub_sign( byte_at( i ), arg->doubleByte_at( i ) );
        if ( result not_eq 0 )
            return result;
    }
    return sub_sign( s1, s2 );
}


int ByteArrayOopDescriptor::hash_value() {
    int len = length();
    int result;

    if ( len == 0 ) {
        result = 1;
    } else if ( len == 1 ) {
        result = byte_at( 1 );
    } else {
        uint32_t val;
        val    = byte_at( 1 );
        val    = ( val << 3 ) ^ ( byte_at( 2 ) ^ val );
        val    = ( val << 3 ) ^ ( byte_at( len ) ^ val );
        val    = ( val << 3 ) ^ ( byte_at( len - 1 ) ^ val );
        val    = ( val << 3 ) ^ ( byte_at( len / 2 + 1 ) ^ val );
        val    = ( val << 3 ) ^ ( len ^ val );
        result = MarkOopDescriptor::masked_hash( val );
    }
    return result == 0 ? 1 : result;
}


const char * ByteArrayOopDescriptor::as_string() {
    int len = length();
    char * str = new_resource_array <char>( len + 1 );
    int index    = 0;
    for ( ; index < len; index++ ) {
        str[ index ] = byte_at( index + 1 );
    }
    str[ index ] = '\0';
    return str;
}

//
//const char *ByteArrayOopDescriptor::as_string() {
//    return as_std_string().c_str();
//}
//

const std::string & ByteArrayOopDescriptor::as_std_string() {

    std::string s{};

    for ( int index = 0; index < length(); index++ ) {
        s += byte_at( index + 1 );
    }
    s += '\0';

    return s;
}


int ByteArrayOopDescriptor::number_of_arguments() const {
    int result = 0;
    st_assert( length() > 0, "selector should have a positive length" );

    // Return 1 if binary selector
    if ( is_binary() )
        return 1;

    // Return number of colons
    for ( int i = 1; i <= length(); i++ )
        if ( byte_at( i ) == ':' )
            result++;

    return result;
}


bool_t ByteArrayOopDescriptor::is_unary() const {
    if ( is_binary() )
        return false;
    for ( int i = 1; i <= length(); i++ )
        if ( byte_at( i ) == ':' )
            return false;
    return true;
}


bool_t ByteArrayOopDescriptor::is_binary() const {
    uint8_t first = byte_at( 1 );
    // special case _, as compiler treats as a letter
    return first not_eq '_' and ispunct( first ) ? true : false;
}


bool_t ByteArrayOopDescriptor::is_keyword() const {
    if ( is_binary() )
        return false;
    for ( int i = 1; i <= length(); i++ )
        if ( byte_at( i ) == ':' )
            return true;
    return false;
}
