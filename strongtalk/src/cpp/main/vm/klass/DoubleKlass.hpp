//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/klass/MemOopKlass.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


class DoubleKlass : public MemOopKlass {
public:
    // testers
    bool oopIsDouble() const {
        return true;
    }


    // allocation properties
    bool can_inline_allocation() const {
        return false;
    }


    // reflective properties
    bool can_have_instance_variables() const {
        return false;
    }


    bool can_be_subclassed() const {
        return false;
    }


    // allocates a double
    Oop allocateObject( bool permit_scavenge = true, bool tenured = false );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );


    // Format
    Format format() {
        return Format::double_klass;
    }


    // memory operations
    std::int32_t oop_scavenge_contents( Oop obj );

    std::int32_t oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );

    // printing operations
    void oop_short_print_on( Oop obj, ConsoleOutputStream *stream );

    void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );


    const char *name() const {
        return "double";
    }


    // iterators
    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );

    void oop_oop_iterate( Oop obj, OopClosure *blk );


    // sizing
    std::int32_t oop_header_size() const {
        return DoubleOopDescriptor::header_size();
    }


    // class creation
    friend void setKlassVirtualTableFromDoubleKlass( Klass *k );
};

void setKlassVirtualTableFromDoubleKlass( Klass *k );
