//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/MemOopKlass.hpp"

class MethodKlass : public MemOopKlass {
public:
    bool_t oop_is_method() const {
        return true;
    }


    // allocation properties
    bool_t can_inline_allocation() const {
        return false;
    }


    int object_size( std::size_t size_of_codes ) const {
        return non_indexable_size() + size_of_codes + 1;
    }


    int oop_size( Oop obj ) const {
        return object_size( MethodOop( obj )->size_of_codes() );
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
        return Format::method_klass;
    }


    // memory operations
    int oop_scavenge_contents( Oop obj );

    int oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );

    friend void setKlassVirtualTableFromMethodKlass( Klass *k );

    // iterators
    void oop_oop_iterate( Oop obj, OopClosure *blk );

    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );


    // Sizing
    int oop_header_size() const {
        return MethodOopDescriptor::header_size();
    }


    void oop_print_layout( Oop obj );

    // printing support
    void oop_print_on( Oop obj, ConsoleOutputStream *stream );

    void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );


    const char *name() const {
        return "method";
    }


    // Construction (called from primitive)
    MethodOop constructMethod( Oop name, int flags, int nofArgs, ObjectArrayOop debugInfo, ByteArrayOop bytes, ObjectArrayOop oops );
};

void setKlassVirtualTableFromMethodKlass( Klass *k );
