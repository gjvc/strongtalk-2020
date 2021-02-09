//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


class KlassKlass : public MemOopKlass {

public:
    friend void setKlassVirtualTableFromKlassKlass( Klass *k );


    // testers
    bool oopIsKlass() const {
        return true;
    }


    // allocation properties
    bool can_inline_allocation() const {
        return false;
    }


    // cloning operations
    Oop allocateObject( bool permit_scavenge = true, bool tenured = false );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );


    // Format
    Format format() {
        return Format::klass_klass;
    }


    // memory operations
    std::int32_t oop_scavenge_contents( Oop obj );

    std::int32_t oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );

    Oop oop_primitive_allocate( Oop obj, bool allow_scavenge = true, bool tenured = false );

    Oop oop_primitive_allocate_size( Oop obj, std::int32_t size );

    Oop oop_shallow_copy( Oop obj, bool tenured );

    bool oop_verify( Oop obj );


    const char *name() const {
        return "klass";
    }


    // printing operations
    void oop_print_on( Oop obj, ConsoleOutputStream *stream );

    void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );

    // iterators
    void oop_oop_iterate( Oop obj, OopClosure *blk );

    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );


    // Sizing
    std::int32_t oop_header_size() const {
        return KlassOopDescriptor::header_size();
    }
};

void setKlassVirtualTableFromKlassKlass( Klass *k );
