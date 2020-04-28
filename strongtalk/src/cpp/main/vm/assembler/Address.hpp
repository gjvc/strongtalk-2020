//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/assembler/Register.hpp"
#include "vm/code/RelocationInformation.hpp"


// Address operands for assembler

class Address : public ValueObject {

    public:
        enum ScaleFactor {
            no_scale = -1, //
            times_1  = 0, //
            times_2  = 1, //
            times_4  = 2, //
            times_8  = 3, //
        };

    private:
        Register                              _base;              //
        Register                              _index;             //
        ScaleFactor                           _scale;             //
        address_t                             _displacement;      //
        RelocationInformation::RelocationType _relocationType;    //

    public:
        Address();

        Address( address_t displacement, RelocationInformation::RelocationType rtype );

        Address( Register base, address_t displacement = 0, RelocationInformation::RelocationType rtype = RelocationInformation::RelocationType::none );

        Address( Register base, Register index, ScaleFactor scale, address_t displacement = 0, RelocationInformation::RelocationType rtype = RelocationInformation::RelocationType::none );

        friend class Assembler;

};

