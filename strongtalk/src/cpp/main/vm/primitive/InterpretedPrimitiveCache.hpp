
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/primitive/PrimitiveDescriptor.hpp"
#include "vm/oop/SymbolOopDescriptor.hpp"
#include "vm/memory/allocation.hpp"


class InterpretedPrimitiveCache : public ValueObject {

private:
    std::uint8_t *hp() const {
        return (std::uint8_t *) this;
    }


public:
    SymbolOop name() const;

    std::int32_t number_of_parameters() const;

    PrimitiveDescriptor *pdesc() const;

    bool has_receiver() const;

    bool has_failure_code() const;
};
