//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"


class DoubleKlass : public MemOopKlass {
public:
    // testers
    bool_t oop_is_double() const {
        return true;
    }


    // allocation properties
    bool_t can_inline_allocation() const {
        return false;
    }


    // reflective properties
    bool_t can_have_instance_variables() const {
        return false;
    }


    bool_t can_be_subclassed() const {
        return false;
    }


    // allocates a double
    Oop allocateObject( bool_t permit_scavenge = true, bool_t tenured = false );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );


    // Format
    Format format() {
        return Format::double_klass;
    }


    // memory operations
    int oop_scavenge_contents( Oop obj );

    int oop_scavenge_tenured_contents( Oop obj );

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
    int oop_header_size() const {
        return DoubleOopDescriptor::header_size();
    }


    // class creation
    friend void setKlassVirtualTableFromDoubleKlass( Klass *k );
};

void setKlassVirtualTableFromDoubleKlass( Klass *k );
