//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/Klass.hpp"


class SMIKlass : public Klass {

public:
    friend void setKlassVirtualTableFromSmiKlass( Klass *k );


    bool_t oop_is_smi() const {
        return true;
    }


    const char *name() const {
        return "smi_t";
    }


    // Reflective properties
    bool_t can_have_instance_variables() const {
        return false;
    }


    bool_t can_be_subclassed() const {
        return false;
    }


    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );


    // Format
    Format format() {
        return Format::smi_klass;
    }


    // Copy operations
    Oop oop_shallow_copy( Oop obj, bool_t tenured );

    // printing operations
    void oop_print_value( Oop obj, ConsoleOutputStream *stream );
};

void setKlassVirtualTableFromSmiKlass( Klass *k );
