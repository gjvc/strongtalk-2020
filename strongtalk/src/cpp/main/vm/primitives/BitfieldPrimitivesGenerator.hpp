//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/assembler/MacroAssembler.hpp"

#include <bitset>


class BitsetPrimitiveDescriptor {
private:
    std::bitset<32> _bits;

public:
    BitsetPrimitiveDescriptor( const std::bitset<32> &bits );


    int get_unsigned_bitfield( std::size_t start_bit, std::size_t field_length ) const {

        uint32_t result{ 0 };
        uint32_t mask = 1;

        for ( std::size_t i = 0; i < field_length; i++ ) {
            if ( _bits[ start_bit + i ] )
                result |= mask;
            mask <<= 1;
        }

        return result;
    }


    // flags
    int number_of_parameters() const {
        return get_unsigned_bitfield( 0, 8 );
    }


    // # of parameters (self or arguments), excluding failure block (if any);
    // i.e., # of args that are actually passed to C
    PrimitiveGroup group() const {
        return (PrimitiveGroup) get_unsigned_bitfield( 8, 8 );
    }


    bool_t is_special_prim() const {
        return group() not_eq PrimitiveGroup::NormalPrimitive;
    }


    bool_t can_invoke_delta() const {
        return can_perform_NonLocalReturn();
    }   // can it call other Delta code?

    bool_t can_scavenge() const {
        return _bits[ 16 ];
    }   // can it trigger a scavenge/GC?

    bool_t can_perform_NonLocalReturn() const {
        return _bits[ 17 ];
    }   // can it do an NonLocalReturn or process abort?

    bool_t can_fail() const {
        return _bits[ 18 ];
    }   // can_fail: can primitive fail with arguments of correct type?  (NB: even if not can_fail(), primitive will fail if argument types are wrong)

    bool_t can_be_constant_folded() const {
        return _bits[ 19 ];
    }   // is it side-effect free? (so it can be const-folded if args are const)

    bool_t has_receiver() const {
        return _bits[ 20 ];
    }   // does it require a receiver? ({{self prim...}})

    bool_t is_internal() const {
        return _bits[ 21 ];
    }   // true for VM-internal primitives

    bool_t needs_delta_fp_code() const {
        return not _bits[ 22 ];
    }   // must caller set up lastDeltaFP?

};
