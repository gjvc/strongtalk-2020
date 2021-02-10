
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/DoubleValueArrayOopDescriptor.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/utilities/Integer.hpp"

#include <cstring>


// byteArrays are arrays containing bytes
//
// memory layout:
//    [header      ]
//    [klass_field ]
//    [instVars    ]*
//    [length      ]
//    [bytes       ]* = bytes(1) .. bytes(length) + padding

class ByteArrayOopDescriptor : public MemOopDescriptor {

public:
    friend ByteArrayOop as_byteArrayOop( void *p );

    void bootstrap_object( Bootstrap *bootstrap );


    // accessors
    ByteArrayOopDescriptor *addr() const {
        return (ByteArrayOopDescriptor *) MemOopDescriptor::addr();
    }


    bool is_within_bounds( std::int32_t index ) const {
        return 1 <= index and index <= length();
    }


    Oop *addr_as_oops() const {
        return (Oop *) addr();
    }


    Oop *length_addr() const {
        return &addr_as_oops()[ blueprint()->non_indexable_size() ];
    }


    std::size_t length() const {
        Oop len = *length_addr();
        st_assert( len->isSmallIntegerOop(), "length of indexable should be small_int_t" );
        auto value = SmallIntegerOop( len )->value();
        return reinterpret_cast<std::int32_t>( value );
    }


    void set_length( small_int_t len ) {
        *length_addr() = (Oop) smiOopFromValue( len );
    }


    std::uint8_t *bytes() const {
        return (std::uint8_t *) &length_addr()[ 1 ];
    }


    char *chars() const {
        return (char *) bytes();
    }


    std::uint8_t *byte_at_addr( std::size_t which ) const {
        st_assert( which > 0 and which <= length(), "index out of bounds" );
        return &bytes()[ which - 1 ];
    }


    std::uint8_t byte_at( std::size_t which ) const {
        return *byte_at_addr( which );
    }


    void byte_at_put( std::int32_t which, std::uint8_t contents ) {
        *byte_at_addr( which ) = contents;
    }


    // support for large integers

    Integer &number() {
        return *( (Integer *) bytes() );
    }


    // memory operations
    bool verify();


    // C-string operations

    char *copy_null_terminated( std::int32_t &Clength );

    // Copy the bytes() part. Always add trailing '\0'.
    // If byte array contains '\0', these will be escaped in the copy, i.e. "....\0...".
    // Clength is set to length of the copy (may be longer due to escaping).
    // Presence of null chars can be detected by comparing Clength to length().

    bool copy_null_terminated( char *buffer, std::int32_t max_length );


    char *copy_null_terminated() {
        std::int32_t ignore;
        return copy_null_terminated( ignore );
    }


    char *copy_c_heap_null_terminated();
    // Identical to copy_null_terminated but allocates the resulting string in the C heap instead of in the resource area.

    bool equals( const char *name ) {
        return equals( name, strlen( name ) );
    }


    bool equals( const char *name, std::size_t len ) {
        return len == length() and strncmp( chars(), name, len ) == 0;
    }


    bool equals( ByteArrayOop s ) {
        return equals( s->chars(), s->length() );
    }


    // three way compare
    std::int32_t compare( ByteArrayOop arg );

    std::int32_t compare_doubleBytes( DoubleByteArrayOop arg );

    // Returns the hash value for the string.
    std::int32_t hash_value();

    // resource allocated print string
    const char *as_string();
    const std::string &as_std_string();

    // Selector specific operations.
    std::int32_t number_of_arguments() const;

    bool is_unary() const;

    bool is_binary() const;

    bool is_keyword() const;

    friend class ByteArrayKlass;

};


inline ByteArrayOop as_byteArrayOop( void *p ) {
    return ByteArrayOop( as_memOop( p ) );
}
