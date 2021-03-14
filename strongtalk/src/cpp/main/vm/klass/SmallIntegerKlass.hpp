//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/klass/Klass.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


class SMIKlass : public Klass {

public:
//    SMIKlass() = default;
//    ~SMIKlass() = default;


    static void operator delete( void *p ) { (void) p; }


    friend void setKlassVirtualTableFromSmiKlass( Klass *k );


    bool oopIsSmallInteger() const {
        return true;
    }


    const char *name() const {
        return "small_int_t";
    }


    // Reflective properties
    bool can_have_instance_variables() const {
        return false;
    }


    bool can_be_subclassed() const {
        return false;
    }


    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );


    // Format
    Format format() {
        return Format::smi_klass;
    }


    // Copy operations
    Oop oop_shallow_copy( Oop obj, bool tenured );

    // printing operations
    void oop_print_value( Oop obj, ConsoleOutputStream *stream );
};

void setKlassVirtualTableFromSmiKlass( Klass *k );
