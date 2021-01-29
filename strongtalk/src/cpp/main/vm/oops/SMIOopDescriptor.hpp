
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/OopDescriptor.hpp"


// -----------------------------------------------------------------------------

typedef class SMIOopDescriptor *SMIOop;

// 0, 1 in SMIOop format
inline auto smiOop_zero = reinterpret_cast<SMIOop>( ( 0L << TAG_SIZE ) + INTEGER_TAG );
inline auto smiOop_one  = reinterpret_cast<SMIOop>( ( 1L << TAG_SIZE ) + INTEGER_TAG );

// minimum and maximum smiOops
constexpr auto SMI_MIN_VALUE = ( -( 1 << ( BITS_PER_WORD - 3 ) ) );      // -2^2
constexpr auto SMI_MAX_VALUE = ( +( 1 << ( BITS_PER_WORD - 3 ) ) - 1 );  // +2^29 - 1

inline auto smiOop_min = SMIOop( ( SMI_MIN_VALUE << TAG_SIZE ) + INTEGER_TAG );
inline auto smiOop_max = SMIOop( ( SMI_MAX_VALUE << TAG_SIZE ) + INTEGER_TAG );


// -----------------------------------------------------------------------------

inline SMIOop smiOopFromValue( smi_t value ) {
    return SMIOop( ( value << TAG_SIZE ) + INTEGER_TAG );
}


class SMIOopDescriptor : public OopDescriptor {

public:
    friend SMIOop smiOopFromValue( smi_t value );


    smi_t value() const {
        return smi_t( this ) >> TAG_SIZE; // arithmetic shift to preserve sign
    }


    smi_t identity_hash() const {
        return value();
    }


    SMIOop increment() {
        return SMIOop( smi_t( this ) + smi_t( smiOop_one ) );
    }


    SMIOop decrement() {
        return SMIOop( smi_t( this ) - smi_t( smiOop_one ) );
    }


    void print_on( ConsoleOutputStream *stream );


    friend SMIOop as_byte_count_smiOop( smi_t value ) {
        st_assert( lowerBits( value, TAG_SIZE ) == 0, "not a legal byte count" );
        return SMIOop( value + INTEGER_TAG );
    }


    smi_t byte_count() const {
        return smi_t( this ) - INTEGER_TAG;
    }
};
