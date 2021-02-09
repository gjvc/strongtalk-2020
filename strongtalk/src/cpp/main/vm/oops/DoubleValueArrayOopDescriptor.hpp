//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/DoubleValueArrayOopDescriptor.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/oops/Klass.hpp"

// doubleValueArrays are arrays containing double value [8 bytes each]

// memory layout:
//    [header      ]
//    [klass_field ]
//    [instVars    ]*
//    [length      ]      offset

class DoubleValueArrayOopDescriptor : public MemOopDescriptor {
public:
    friend DoubleValueArrayOop as_doubleValueArrayOop( void *p );

    void bootstrap_object( Bootstrap *stream );


    // accessors
    DoubleValueArrayOopDescriptor *addr() const {
        return (DoubleValueArrayOopDescriptor *) MemOopDescriptor::addr();
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
    double *double_start() const {
        return (double *) &length_addr()[ 1 ];
    }


    double *double_at_addr( std::int32_t which ) const {
        st_assert( which > 0 and which <= length(), "index out of bounds" );
        return &double_start()[ which - 1 ];
    }


    double double_at( std::int32_t which ) const {
        return *double_at_addr( which );
    }


    void double_at_put( std::int32_t which, double value ) {
        *double_at_addr( which ) = value;
    }


    // memory operations
    bool verify();

    friend class doubleValueArrayKlass;
};


inline DoubleValueArrayOop as_doubleValueArrayOop( void *p ) {
    return DoubleValueArrayOop( as_memOop( p ) );
}
