
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/klass/Klass.hpp"
#include "vm/oop/SmallIntegerOopDescriptor.hpp"


//
// A klassOop is the C++ equivalent of a Delta class.
// Part of a klassOopDescriptor is a Klass which handle the dispatching for the C++ method calls.
//

//
//  klassOop object layout:
//    [header     ]
//    [klass_field]
//      [vtbl                 ] <-- the Klass object starts here
//      [non_indexable_size   ]
//      [has_untagged_contents]  can be avoided if prototype is stored.
//      [instvar              ]
//      [superKlass           ]
//      [methodDict           ]
//

class KlassOopDescriptor : public MemOopDescriptor {

private:
    Klass _klass_part;

public:
    KlassOop addr() const {
        return (KlassOop) MemOopDescriptor::addr();
    }


    Klass *klass_part() const {
        return &addr()->_klass_part;
    }


    bool is_invalid() const {
        return mark()->is_klass_invalid();
    }


    void set_invalid( bool value ) {
        set_mark( value ? mark()->set_klass_invalid() : mark()->clear_klass_invalid() );
    }


    // sizing
    static std::int32_t header_size() {
        return sizeof( KlassOopDescriptor ) / OOP_SIZE;
    }


    // debugging
    void print_superclasses();

    void bootstrap_object( Bootstrap *stream );


    static std::int32_t nonIndexableSizeOffset() {
        return (std::int32_t) ( &KlassOop( nullptr )->klass_part()->_non_indexable_size );
    }
};
