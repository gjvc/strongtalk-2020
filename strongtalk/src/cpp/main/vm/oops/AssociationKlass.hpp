//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/memory/Closure.hpp"

//
void setKlassVirtualTableFromAssociationKlass( Klass *k );


// associations are cons cells used in the Delta system dictionary.
class AssociationKlass : public MemOopKlass {
public:
    friend void setKlassVirtualTableFromAssociationKlass( Klass *k );


    bool_t oop_is_association() const {
        return true;
    }


    // allocation properties
    bool_t can_inline_allocation() const {
        return false;
    }


    // allocation operations
    Oop allocateObject( bool_t permit_scavenge = true, bool_t tenured = false );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );

    static KlassOop create_class( KlassOop super_class, MixinOop mixin );


    // Format
    Format format() {
        return Format::association_klass;
    }


    // memory operations
    bool_t oop_verify( Oop obj );


    const char *name() const {
        return "association";
    }


    // Reflective properties
    bool_t can_have_instance_variables() const {
        return false;
    }


    bool_t can_be_subclassed() const {
        return false;
    }


    // printing operations
    void oop_short_print_on( Oop obj, ConsoleOutputStream *stream );

    void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );

    // memory operations
    std::int32_t oop_scavenge_contents( Oop obj );

    std::int32_t oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );

    // iterators
    void oop_oop_iterate( Oop obj, OopClosure *blk );

    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );


    // sizing
    std::int32_t oop_header_size() const {
        return AssociationOopDescriptor::header_size();
    }
};
