//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/system/sizes.hpp"


class ByteArrayKlass : public MemOopKlass {

    public:
        // allocation properties
        bool_t can_inline_allocation() const {
            return false;
        }


        // Return the Oop size for a ByteArrayOop
        int object_size( int number_of_bytes ) const {
            return non_indexable_size() + 1 + roundTo( number_of_bytes, oopSize ) / oopSize;
        }


        // Layout
        int length_offset() const {
            return non_indexable_size();
        }


        int array_offset() const {
            return non_indexable_size() + 1;
        }


        // creation operations
        Oop allocateObject( bool_t permit_scavenge = true, bool_t tenured = false );

        Oop allocateObjectSize( int bytes, bool_t permit_scavenge = true, bool_t tenured = false );

        // creates invocation
        KlassOop create_subclass( MixinOop mixin, Format format );

        static KlassOop create_class( KlassOop super_class, MixinOop mixin );


        // Format
        Format format() {
            return Format::byteArray_klass;
        }


        // Initialize the object
        void initialize_object( ByteArrayOop obj, const char * value, int len );

        friend void setKlassVirtualTableFromByteArrayKlass( Klass * k );


        const char * name() const {
            return "byteArray";
        }

        // ALL FUNCTIONS BELOW THIS POINT ARE DISPATCHED FROM AN OOP
    public:
        // accessors
        int oop_size( Oop obj ) const {
            return object_size( ByteArrayOop( obj )->length() );
        }


        int oop_scavenge_contents( Oop obj );

        int oop_scavenge_tenured_contents( Oop obj );

        bool_t oop_verify( Oop obj );

        void oop_print_value_on( Oop obj, ConsoleOutputStream * stream );

        // iterators
        void oop_oop_iterate( Oop obj, OopClosure * blk );

        void oop_layout_iterate( Oop obj, ObjectLayoutClosure * blk );


        // Sizing
        int oop_header_size() const {
            return ByteArrayOopDescriptor::header_size();
        }


        // testers
        bool_t oop_is_byteArray() const {
            return true;
        }


        bool_t oop_is_indexable() const {
            return true;
        }

};

void setKlassVirtualTableFromByteArrayKlass( Klass * k );
