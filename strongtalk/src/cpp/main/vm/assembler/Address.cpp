//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/code/RelocationInformation.hpp"
#include "vm/assembler/Register.hpp"
#include "vm/assembler/Address.hpp"
#include "vm/assembler/x86_registers.hpp"


Address::Address() {
    _base           = noreg;
    _index          = noreg;
    _scale          = Address::ScaleFactor::no_scale;
    _displacement   = 0;
    _relocationType = RelocationInformation::RelocationType::none;
}


Address::Address( address_t displacement, RelocationInformation::RelocationType rtype ) {
    _base           = noreg;
    _index          = noreg;
    _scale          = Address::ScaleFactor::no_scale;
    _displacement   = displacement;
    _relocationType = rtype;
}


Address::Address( Register base, address_t displacement, RelocationInformation::RelocationType rtype ) {
    _base           = base;
    _index          = noreg;
    _scale          = Address::ScaleFactor::no_scale;
    _displacement   = displacement;
    _relocationType = rtype;
}


Address::Address( Register base, Register index, ScaleFactor scale, address_t displacement, RelocationInformation::RelocationType rtype ) {
    st_assert( ( index == noreg ) == ( scale == Address::ScaleFactor::no_scale ), "inconsistent address" );
    _base           = base;
    _index          = index;
    _scale          = scale;
    _displacement   = displacement;
    _relocationType = rtype;
}
