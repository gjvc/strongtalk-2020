//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/DoubleValueArrayOopDescriptor.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/oops/Klass.hpp"


// doubleByteArrays are arrays containing double bytes

// memory layout:
//    [header      ]
//    [klass_field ]
//    [instVars    ]*
//    [length      ]      offset

class DoubleByteArrayOopDescriptor : public MemOopDescriptor {
    public:
        friend DoubleByteArrayOop as_doubleByteArrayOop( void * p );

        void bootstrap_object( Bootstrap * stream );


        // accessors
        DoubleByteArrayOopDescriptor * addr() const {
            return ( DoubleByteArrayOopDescriptor * ) MemOopDescriptor::addr();
        }


        bool_t is_within_bounds( int index ) const {
            return 1 <= index and index <= length();
        }


        Oop * addr_as_oops() const {
            return ( Oop * ) addr();
        }


        // returns the location of the length field
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


        // returns the location where the double bytes start
        std::uint16_t * doubleBytes() const {
            return ( std::uint16_t * ) &length_addr()[ 1 ];
        }


        std::uint16_t * doubleByte_at_addr( int which ) const {
            st_assert( which > 0 and which <= length(), "index out of bounds" );
            return &doubleBytes()[ which - 1 ];
        }


        std::uint16_t doubleByte_at( int which ) const {
            return *doubleByte_at_addr( which );
        }


        void doubleByte_at_put( int which, std::uint16_t contents ) {
            *doubleByte_at_addr( which ) = contents;
        }


        // three way compare
        int compare( DoubleByteArrayOop arg );

        // Returns the hash value for the string
        int hash_value();

        // copy string to buffer as null terminated ascii string.
        bool_t copy_null_terminated( char * buffer, int max_length );

        char * as_string();

        // memory operations
        bool_t verify();

        friend class doubleByteArrayKlass;
};


inline DoubleByteArrayOop as_doubleByteArrayOop( void * p ) {
    return DoubleByteArrayOop( as_memOop( p ) );
}
