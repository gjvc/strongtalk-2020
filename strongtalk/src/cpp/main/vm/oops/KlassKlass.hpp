//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"


class KlassKlass : public MemOopKlass {

    public:
        friend void setKlassVirtualTableFromKlassKlass( Klass * k );


        // testers
        bool_t oop_is_klass() const {
            return true;
        }


        // allocation properties
        bool_t can_inline_allocation() const {
            return false;
        }


        // cloning operations
        Oop allocateObject( bool_t permit_scavenge = true, bool_t tenured = false );

        // creates invocation
        KlassOop create_subclass( MixinOop mixin, Format format );


        // Format
        Format format() {
            return Format::klass_klass;
        }


        // memory operations
        int oop_scavenge_contents( Oop obj );

        int oop_scavenge_tenured_contents( Oop obj );

        void oop_follow_contents( Oop obj );

        Oop oop_primitive_allocate( Oop obj, bool_t allow_scavenge = true, bool_t tenured = false );

        Oop oop_primitive_allocate_size( Oop obj, int size );

        Oop oop_shallow_copy( Oop obj, bool_t tenured );

        bool_t oop_verify( Oop obj );


        const char * name() const {
            return "klass";
        }


        // printing operations
        void oop_print_on( Oop obj, ConsoleOutputStream * stream );

        void oop_print_value_on( Oop obj, ConsoleOutputStream * stream );

        // iterators
        void oop_oop_iterate( Oop obj, OopClosure * blk );

        void oop_layout_iterate( Oop obj, ObjectLayoutClosure * blk );


        // Sizing
        int oop_header_size() const {
            return KlassOopDescriptor::header_size();
        }
};

void setKlassVirtualTableFromKlassKlass( Klass * k );

