//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/Klass.hpp"

// superclass for all heap objects

class MemOopKlass : public Klass {

protected:
    Oop *basicAllocate( std::size_t size, KlassOop *klass, bool_t permit_scavenge, bool_t tenured ) {
        return tenured ? Universe::allocate_tenured( size, permit_scavenge ) : Universe::allocate( size, (MemOop *) klass, permit_scavenge );
    }


public:
    // allocation properties
    bool_t can_inline_allocation() const {
        return true;
    }


    // reflective properties
    bool_t can_have_instance_variables() const {
        return true;
    }


    bool_t can_be_subclassed() const {
        return true;
    }


    // allocation operations
    Oop allocateObject( bool_t permit_scavenge = true, bool_t tenured = false );

    Oop allocateObjectSize( std::size_t size, bool_t permit_scavenge = true, bool_t tenured = false );

    // invocation creation
    KlassOop create_subclass( MixinOop mixin, Format format );

    KlassOop create_subclass( MixinOop mixin, KlassOop instSuper, KlassOop metaClass, Format format );

    static KlassOop create_class( KlassOop super_class, MixinOop mixin );


    // Format
    Format format() {
        return Format::mem_klass;
    }


    // copy operation
    Oop oop_shallow_copy( Oop obj, bool_t tenured );


    // printing support
    const char *name() const {
        return "MemOop";
    }


    // class creation
    friend void setKlassVirtualTableFromMemOopKlass( Klass *k );

    // ALL FUNCTIONS BELOW THIS POINT ARE DISPATCHED FROM AN OOP
public:
    // memory operations
    int oop_scavenge_contents( Oop obj );

    int oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );

    // printing operations
    void oop_print_on( Oop obj, ConsoleOutputStream *stream );

    void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );

    // iterators
    void oop_oop_iterate( Oop obj, OopClosure *blk );

    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );

    // verification
    bool_t oop_verify( Oop obj );


    // sizing
    int oop_header_size() const {
        return MemOopDescriptor::header_size();
    }
};

void setKlassVirtualTableFromMemOopKlass( Klass *k );
