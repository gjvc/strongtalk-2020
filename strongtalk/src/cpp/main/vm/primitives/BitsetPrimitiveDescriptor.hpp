//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/assembler/MacroAssembler.hpp"

#include <bitset>


class BitsetPrimitiveDescriptor {
private:
    std::bitset<32> _bits;

public:
    BitsetPrimitiveDescriptor( const std::bitset<32> &bits );


    std::int32_t get_unsigned_bitfield( std::int32_t start_bit, std::int32_t field_length ) const {

        std::uint32_t result{ 0 };
        std::uint32_t mask = 1;

        for ( std::size_t i = 0; i < field_length; i++ ) {
            if ( _bits[ start_bit + i ] )
                result |= mask;
            mask <<= 1;
        }

        return result;
    }


    // flags
    std::int32_t number_of_parameters() const {
        return get_unsigned_bitfield( 0, 8 );
    }


    // # of parameters (self or arguments), excluding failure block (if any);
    // i.e., # of args that are actually passed to C
    PrimitiveGroup group() const {
        return (PrimitiveGroup) get_unsigned_bitfield( 8, 8 );
    }


    bool is_special_prim() const {
        return group() not_eq PrimitiveGroup::NormalPrimitive;
    }


    bool can_invoke_delta() const {
        return can_perform_NonLocalReturn();
    }   // can it call other Delta code?

    bool can_scavenge() const {
        return _bits[ 16 ];
    }   // can it trigger a scavenge/GC?

    bool can_perform_NonLocalReturn() const {
        return _bits[ 17 ];
    }   // can it do an NonLocalReturn or process abort?

    bool can_fail() const {
        return _bits[ 18 ];
    }   // can_fail: can primitive fail with arguments of correct type?  (NB: even if not can_fail(), primitive will fail if argument types are wrong)

    bool can_be_constant_folded() const {
        return _bits[ 19 ];
    }   // is it side-effect free? (so it can be const-folded if args are const)

    bool has_receiver() const {
        return _bits[ 20 ];
    }   // does it require a receiver? ({{self prim...}})

    bool is_internal() const {
        return _bits[ 21 ];
    }   // true for VM-internal primitives

    bool needs_delta_fp_code() const {
        return not _bits[ 22 ];
    }   // must caller set up lastDeltaFP?

};
