//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"


class BlockClosureKlass : public MemOopKlass {

public:
    // testers
    bool oop_is_block() const {
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


    // arity
    std::int32_t number_of_arguments() const;

    // allocation
    Oop allocateObject( bool permit_scavenge = true, bool tenured = false );

    static KlassOop blockKlassFor( std::int32_t numberOfArguments );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );


    // Format
    Format format() {
        return Format::blockClosure_klass;
    }


    // scavenge
    std::int32_t oop_scavenge_contents( Oop obj );

    std::int32_t oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );

    // iterators
    void oop_oop_iterate( Oop obj, OopClosure *blk );

    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );


    // Sizing
    std::int32_t oop_header_size() const {
        return BlockClosureOopDescriptor::header_size();
    }


    // memory operations
    bool oop_verify( Oop obj );

    // printing operations
    void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );


    const char *name() const {
        return "block";
    }


    // class creation
    friend void setKlassVirtualTableFromBlockClosureKlass( Klass *k );
};

void setKlassVirtualTableFromBlockClosureKlass( Klass *k );

void setKlassVirtualTableFromContextKlass( Klass *k );
