
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


bool ByteArrayOopDescriptor::verify() {
    bool flag = MemOopDescriptor::verify();
    if ( flag ) {
        std::int32_t l = length();
        if ( l < 0 ) {
            error( "ByteArrayOop 0x{0:x} has negative length", this );
            flag = false;
        }
    }
    return flag;
}


char *ByteArrayOopDescriptor::copy_null_terminated( std::int32_t &Clength ) {
    // Copy the bytes() part. Always add trailing '\0'. If byte array
    // contains '\0', these will be escaped in the copy, i.e. "....\0...".
    // Clength is set to length of the copy (may be longer due to escaping).
    // Presence of null chars can be detected by comparing Clength to length().

    st_assert_byteArray( this, "should be a byte array" );
    Clength = length();
    char *res = copy_string( (const char *) bytes(), Clength );
    if ( strlen( res ) == (std::uint32_t) Clength )
        return res;                   // Simple case, no '\0' in byte array.

    // Simple case failed ...
    small_int_t t = length();               // Copy and 'escape' null chars.
//    small_int_t              i;

    for ( std::size_t i = length() - 1; i >= 0; i-- ) {
        if ( byte_at( i ) == '\0' ) {
            t++;
        }
    }

    // t is total length of result string.
    res = new_resource_array<char>( t + 1 );
    res[ t-- ] = '\0';
    Clength = t;
    for ( std::size_t i = length() - 1; i >= 0; i-- ) {
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


char *ByteArrayOopDescriptor::copy_c_heap_null_terminated() {
    // Copy the bytes() part. Always add trailing '\0'. If byte array
    // contains '\0', these will be escaped in the copy, i.e. "....\0...".
    // NOTE: The resulting string is allocated in Cheap

    st_assert_byteArray( this, "should be a byte array" );
    small_int_t t = length();               // Copy and 'escape' null chars.
//    small_int_t              i;

    for ( std::size_t i = length() - 1; i >= 0; i-- ) {
        if ( byte_at( i ) == '\0' ) {
            t++;
        }
    }

    // t is total length of result string.
    char *res = new_c_heap_array<char>( t + 1 );
    res[ t-- ] = '\0';
    for ( std::size_t i = length() - 1; i >= 0; i-- ) {
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


bool ByteArrayOopDescriptor::copy_null_terminated( char *buffer, std::int32_t max_length ) {
    // %not optimized

    std::int32_t len = length();

    bool is_truncated = false;
    if ( len >= max_length ) {

        len          = max_length - 1;
        is_truncated = true;
    }

    for ( std::size_t i = 0; i < len; i++ )
        buffer[ i ] = byte_at( i + 1 );

    buffer[ len ] = '\0';

    return is_truncated;
}


void ByteArrayOopDescriptor::bootstrap_object( Bootstrap *bootstrap ) {
    MemOopDescriptor::bootstrap_object( bootstrap );

    bootstrap->read_oop( length_addr() );
    for ( std::size_t i = 1; i <= length(); i++ ) {
        byte_at_put( i, bootstrap->read_uint8_t() );
    }
}


static std::int32_t sub_sign( std::int32_t a, std::int32_t b ) {
    if ( a < b )
        return -1;

    if ( a > b )
        return 1;

    return 0;
}


std::int32_t compare_as_bytes( const std::uint8_t *a, const std::uint8_t *b ) {
    // machine dependent code; little endian code
    if ( a[ 0 ] - b[ 0 ] )
        return sub_sign( a[ 0 ], b[ 0 ] );
    if ( a[ 1 ] - b[ 1 ] )
        return sub_sign( a[ 1 ], b[ 1 ] );
    if ( a[ 2 ] - b[ 2 ] )
        return sub_sign( a[ 2 ], b[ 2 ] );

    return sub_sign( a[ 3 ], b[ 3 ] );
}


std::int32_t ByteArrayOopDescriptor::compare( ByteArrayOop arg ) {
    // Get the addresses of the length fields
    const std::uint32_t *a = (std::uint32_t *) length_addr();
    const std::uint32_t *b = (std::uint32_t *) arg->length_addr();

    // Get the word sizes of the arays
    std::int32_t a_size = roundTo( SmallIntegerOop( *a++ )->value() * sizeof( char ), sizeof( std::int32_t ) ) / sizeof( std::int32_t );
    std::int32_t b_size = roundTo( SmallIntegerOop( *b++ )->value() * sizeof( char ), sizeof( std::int32_t ) ) / sizeof( std::int32_t );

    const std::uint32_t *a_end = a + min( a_size, b_size );
    while ( a < a_end ) {
        if ( *b++ not_eq *a++ )
            return compare_as_bytes( (const std::uint8_t *) ( a - 1 ), (const std::uint8_t *) ( b - 1 ) );
    }
    return sub_sign( a_size, b_size );
}


std::int32_t ByteArrayOopDescriptor::compare_doubleBytes( DoubleByteArrayOop arg ) {
    // %not optimized
    std::int32_t s1 = length();
    std::int32_t s2 = arg->length();
    std::int32_t n  = s1 < s2 ? s1 : s2;

    for ( std::size_t i = 1; i <= n; i++ ) {
        std::int32_t result = sub_sign( byte_at( i ), arg->doubleByte_at( i ) );
        if ( result not_eq 0 )
            return result;
    }
    return sub_sign( s1, s2 );
}


std::int32_t ByteArrayOopDescriptor::hash_value() {
    std::int32_t len = length();
    std::int32_t result;

    if ( len == 0 ) {
        result = 1;
    } else if ( len == 1 ) {
        result = byte_at( 1 );
    } else {
        std::uint32_t val;
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


const char *ByteArrayOopDescriptor::as_string() {

    std::int32_t len  = length();
    char         *str = new_resource_array<char>( len + 1 );

    for ( std::size_t index = 0; index < len; index++ ) {
        str[ index ] = byte_at( index + 1 );
    }

    str[ len ] = '\0';

    return str;
}


//const char *ByteArrayOopDescriptor::as_string() {
//    return as_std_string().c_str();
//}


//const std::string &ByteArrayOopDescriptor::as_std_string() {
//
//    std::string s{};
//
//    for ( std::size_t index = 0; index < length(); index++ ) {
//        s += byte_at( index + 1 );
//    }
//    s += '\0';
//
//    return s;
//}


std::int32_t ByteArrayOopDescriptor::number_of_arguments() const {
    std::int32_t result = 0;
    st_assert( length() > 0, "selector should have a positive length" );

    // Return 1 if binary selector
    if ( is_binary() )
        return 1;

    // Return number of colons
    for ( std::size_t i = 1; i <= length(); i++ )
        if ( byte_at( i ) == ':' )
            result++;

    return result;
}


bool ByteArrayOopDescriptor::is_unary() const {
    if ( is_binary() )
        return false;
    for ( std::size_t i = 1; i <= length(); i++ )
        if ( byte_at( i ) == ':' )
            return false;
    return true;
}


bool ByteArrayOopDescriptor::is_binary() const {
    std::uint8_t first = byte_at( 1 );
    // special case _, as compiler treats as a letter
    return first not_eq '_' and ispunct( first ) ? true : false;
}


bool ByteArrayOopDescriptor::is_keyword() const {
    if ( is_binary() )
        return false;

    for ( std::size_t i = 1; i <= length(); i++ )
        if ( byte_at( i ) == ':' )
            return true;

    return false;
}
