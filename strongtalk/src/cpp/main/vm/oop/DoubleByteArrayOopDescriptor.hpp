//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/oop/DoubleValueArrayOopDescriptor.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/klass/Klass.hpp"


// doubleByteArrays are arrays containing double bytes

// memory layout:
//    [header      ]
//    [klass_field ]
//    [instVars    ]*
//    [length      ]      offset

class DoubleByteArrayOopDescriptor : public MemOopDescriptor {
public:
    friend DoubleByteArrayOop as_doubleByteArrayOop( void *p );

    void bootstrap_object( Bootstrap *stream );


    // accessors
    DoubleByteArrayOopDescriptor *addr() const {
        return (DoubleByteArrayOopDescriptor *) MemOopDescriptor::addr();
    }


    bool is_within_bounds( std::int32_t index ) const {
        return 1 <= index and index <= length();
    }


    Oop *addr_as_oops() const {
        return (Oop *) addr();
    }


    // returns the location of the length field
    Oop *length_addr() const {
        return &addr_as_oops()[ blueprint()->non_indexable_size() ];
    }


    small_int_t length() const {
        Oop len = *length_addr();
        st_assert( len->isSmallIntegerOop(), "length of indexable should be small_int_t" );
        return SmallIntegerOop( len )->value();
    }


    void set_length( small_int_t len ) {
        *length_addr() = (Oop) smiOopFromValue( len );
    }


    // returns the location where the double bytes start
    std::uint16_t *doubleBytes() const {
        return (std::uint16_t *) &length_addr()[ 1 ];
    }


    std::uint16_t *doubleByte_at_addr( std::int32_t which ) const {
        st_assert( which > 0 and which <= length(), "index out of bounds" );
        return &doubleBytes()[ which - 1 ];
    }


    std::uint16_t doubleByte_at( std::int32_t which ) const {
        return *doubleByte_at_addr( which );
    }


    void doubleByte_at_put( std::int32_t which, std::uint16_t contents ) {
        *doubleByte_at_addr( which ) = contents;
    }


    // three way compare
    std::int32_t compare( DoubleByteArrayOop arg );

    // Returns the hash value for the string
    std::int32_t hash_value();

    // copy string to buffer as null terminated ascii string.
    bool copy_null_terminated( char *buffer, std::int32_t max_length );

    char *as_string();

    // memory operations
    bool verify();

    friend class doubleByteArrayKlass;
};


inline DoubleByteArrayOop as_doubleByteArrayOop( void *p ) {
    return DoubleByteArrayOop( as_memOop( p ) );
}
