//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"


class ObjectArrayKlass : public MemOopKlass {

    public:
        // allocation properties
        bool_t can_inline_allocation() const {
            return false;
        }


        // creation operation
        Oop allocateObjectSize( int size, bool_t permit_scavenge = true, bool_t tenured = false );

        // creates invocation
        KlassOop create_subclass( MixinOop mixin, Format format );

        static KlassOop create_class( KlassOop super_class, MixinOop mixin );


        // Format
        Format format() {
            return Format::objArray_klass;
        }


        // allocated an object array for an interpreter PolymorphicInlineCache.
        static ObjectArrayOop allocate_tenured_pic( int size );


        // Return the Oop size for a objArrayOop
        int object_size( int size ) const {
            return non_indexable_size() + 1 + size;
        }


        // Layout
        int length_offset() const {
            return non_indexable_size();
        }


        int array_offset() const {
            return non_indexable_size() + 1;
        }


        friend void setKlassVirtualTableFromObjArrayKlass( Klass * k );


        const char * name() const {
            return "objArray";
        }

        // ALL FUNCTIONS BELOW THIS POINT ARE DISPATCHED FROM AN OOP

        // size operation
        int oop_size( Oop obj ) const {
            return object_size( ObjectArrayOop( obj )->length() );
        }


        // Memory operations
        int oop_scavenge_contents( Oop obj );

        int oop_scavenge_tenured_contents( Oop obj );

        void oop_follow_contents( Oop obj );


        // testers
        bool_t oop_is_objArray() const {
            return true;
        }


        bool_t oop_is_indexable() const {
            return true;
        }


        void oop_short_print_on( Oop obj, ConsoleOutputStream * stream );

        // iterators
        void oop_oop_iterate( Oop obj, OopClosure * blk );

        void oop_layout_iterate( Oop obj, ObjectLayoutClosure * blk );


        // Sizing
        int oop_header_size() const {
            return ObjectArrayOopDescriptor::header_size();
        }
};

void setKlassVirtualTableFromObjArrayKlass( Klass * k );

