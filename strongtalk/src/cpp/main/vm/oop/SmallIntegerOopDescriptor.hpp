
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/oop/OopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


// -----------------------------------------------------------------------------

inline auto taggedIntegerFromInteger( small_int_t value ) {
    return ( value << TAG_SIZE ) + INTEGER_TAG;
}


inline SmallIntegerOop smiOopFromValue( small_int_t value ) {
    return SmallIntegerOop( taggedIntegerFromInteger( value ) );
}


// -----------------------------------------------------------------------------

// 0, 1 in SmallIntegerOop format
inline auto smiOop_zero = reinterpret_cast<SmallIntegerOop>( taggedIntegerFromInteger( 0L ) );
inline auto smiOop_one  = reinterpret_cast<SmallIntegerOop>( taggedIntegerFromInteger( 1L ) );

// minimum and maximum smiOops
constexpr auto SMI_MIN_VALUE = ( -( 1 << ( BITS_PER_WORD - 3 ) ) );      // -2^2
constexpr auto SMI_MAX_VALUE = ( +( 1 << ( BITS_PER_WORD - 3 ) ) - 1 );  // +2^29 - 1

//
inline auto smiOop_min = SmallIntegerOop( taggedIntegerFromInteger( SMI_MIN_VALUE ) );
inline auto smiOop_max = SmallIntegerOop( taggedIntegerFromInteger( SMI_MAX_VALUE ) );



// -----------------------------------------------------------------------------

class SmallIntegerOopDescriptor : public OopDescriptor {

public:
    friend SmallIntegerOop smiOopFromValue( small_int_t value );


    small_int_t value() const {
        return small_int_t( this ) >> TAG_SIZE; // arithmetic shift to preserve sign
    }


    small_int_t identity_hash() const {
        return value();
    }


    SmallIntegerOop increment() {
        return SmallIntegerOop( small_int_t( this ) + small_int_t( smiOop_one ) );
    }


    SmallIntegerOop decrement() {
        return SmallIntegerOop( small_int_t( this ) - small_int_t( smiOop_one ) );
    }


    void print_on( ConsoleOutputStream *stream );


    friend SmallIntegerOop as_byte_count_smiOop( small_int_t value ) {
        st_assert( lowerBits( value, TAG_SIZE ) == 0, "not a legal byte count" );
        return SmallIntegerOop( value + INTEGER_TAG );
    }


    small_int_t byte_count() const {
        return small_int_t( this ) - INTEGER_TAG;
    }
};


typedef class SmallIntegerOopDescriptor *SmallIntegerOop;
