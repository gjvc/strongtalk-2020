
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


class ContextKlass : public MemOopKlass {

public:
    // testers
    bool oop_is_context() const {
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


    // allocation
    Oop allocateObjectSize( std::int32_t num_of_temps, bool permit_scavenge = true, bool tenured = false );

    static ContextOop allocate_context( std::int32_t num_of_temps );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );


    // Format
    Format format() {
        return Format::context_klass;
    }


    // scavenge
    std::int32_t oop_scavenge_contents( Oop obj );

    std::int32_t oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );

    // iterators
    void oop_oop_iterate( Oop obj, OopClosure *blk );

    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );


    // sizing
    std::int32_t oop_header_size() const {
        return ContextOopDescriptor::header_size();
    }


    std::int32_t oop_size( Oop obj ) const;

    // printing support
    void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );

    void oop_print_on( Oop obj, ConsoleOutputStream *stream );


    const char *name() const {
        return "context";
    }


    // class creation
    friend void setKlassVirtualTableFromContextKlass( Klass *k );
};
