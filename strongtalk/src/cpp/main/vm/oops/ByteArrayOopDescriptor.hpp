
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
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
        friend ByteArrayOop as_byteArrayOop( void * p );

        void bootstrap_object( Bootstrap * stream );


        // accessors
        ByteArrayOopDescriptor * addr() const {
            return ( ByteArrayOopDescriptor * ) MemOopDescriptor::addr();
        }


        bool_t is_within_bounds( int index ) const {
            return 1 <= index and index <= length();
        }


        Oop * addr_as_oops() const {
            return ( Oop * ) addr();
        }


        Oop * length_addr() const {
            return &addr_as_oops()[ blueprint()->non_indexable_size() ];
        }


        smi_t length() const {
            Oop len = *length_addr();
            st_assert( len->is_smi(), "length of indexable should be smi_t" );
            return SMIOop( len )->value();
        }


        void set_length( smi_t len ) {
            *length_addr() = ( Oop ) smiOopFromValue( len );
        }


        uint8_t * bytes() const {
            return ( uint8_t * ) &length_addr()[ 1 ];
        }


        char * chars() const {
            return ( char * ) bytes();
        }


        uint8_t * byte_at_addr( int which ) const {
            st_assert( which > 0 and which <= length(), "index out of bounds" );
            return &bytes()[ which - 1 ];
        }


        uint8_t byte_at( int which ) const {
            return *byte_at_addr( which );
        }


        void byte_at_put( int which, uint8_t contents ) {
            *byte_at_addr( which ) = contents;
        }


        // support for large integers

        Integer & number() {
            return *( ( Integer * ) bytes() );
        }


        // memory operations
        bool_t verify();


        // C-string operations

        char * copy_null_terminated( int & Clength );

        // Copy the bytes() part. Always add trailing '\0'.
        // If byte array contains '\0', these will be escaped in the copy, i.e. "....\0...".
        // Clength is set to length of the copy (may be longer due to escaping).
        // Presence of null chars can be detected by comparing Clength to length().

        bool_t copy_null_terminated( char * buffer, int max_length );


        char * copy_null_terminated() {
            int ignore;
            return copy_null_terminated( ignore );
        }


        char * copy_c_heap_null_terminated();
        // Identical to copy_null_terminated but allocates the resulting string in the C heap instead of in the resource area.

        bool_t equals( const char * name ) {
            return equals( name, strlen( name ) );
        }


        bool_t equals( const char * name, int len ) {
            return len == length() and strncmp( chars(), name, len ) == 0;
        }


        bool_t equals( ByteArrayOop s ) {
            return equals( s->chars(), s->length() );
        }


        // three way compare
        int compare( ByteArrayOop arg );

        int compare_doubleBytes( DoubleByteArrayOop arg );

        // Returns the hash value for the string.
        int hash_value();

        // resource allocated print string
        const char * as_string();
        const std::string & as_std_string();

        // Selector specific operations.
        int number_of_arguments() const;

        bool_t is_unary() const;

        bool_t is_binary() const;

        bool_t is_keyword() const;

        friend class ByteArrayKlass;

};


inline ByteArrayOop as_byteArrayOop( void * p ) {
    return ByteArrayOop( as_memOop( p ) );
}


