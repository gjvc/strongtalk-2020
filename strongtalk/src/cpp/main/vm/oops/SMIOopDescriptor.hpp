
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/bits.hpp"
#include "vm/oops/OopDescriptor.hpp"


// -----------------------------------------------------------------------------

typedef class SMIOopDescriptor * SMIOop;

// 0, 1 in SMIOop format
//#define smiOop_zero  SMIOop( (0L << TAG_SIZE) + INTEGER_TAG )
//#define smiOop_one   SMIOop( (1L << TAG_SIZE) + INTEGER_TAG )

inline auto smiOop_zero = reinterpret_cast<SMIOop>( ( 0L << TAG_SIZE ) + INTEGER_TAG );
inline auto smiOop_one  = reinterpret_cast<SMIOop>( ( 1L << TAG_SIZE ) + INTEGER_TAG );

// minimum and maximum smiOops
//#define smi_min  (-(1 << (BitsPerWord - 3)))      // -2^29
//#define smi_max  ( (1 << (BitsPerWord - 3)) - 1)  // +2^29 - 1

constexpr auto smi_min = ( -( 1 << ( BitsPerWord - 3 ) ) );      // -2^2
constexpr auto smi_max = ( ( 1 << ( BitsPerWord - 3 ) ) - 1 );  // +2^29 - 1

//#define smiOop_min   SMIOop((smi_min << TAG_SIZE) + INTEGER_TAG)
//#define smiOop_max   SMIOop((smi_max << TAG_SIZE) + INTEGER_TAG)

inline auto smiOop_min = SMIOop( ( smi_min << TAG_SIZE ) + INTEGER_TAG );
inline auto smiOop_max = SMIOop( ( smi_max << TAG_SIZE ) + INTEGER_TAG );


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


        void print_on( ConsoleOutputStream * stream );


        friend SMIOop as_byte_count_smiOop( smi_t value ) {
            st_assert( lowerBits( value, TAG_SIZE ) == 0, "not a legal byte count" );
            return SMIOop( value + INTEGER_TAG );
        }


        smi_t byte_count() const {
            return smi_t( this ) - INTEGER_TAG;
        }
};
