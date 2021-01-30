//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


class ByteArrayKlass : public MemOopKlass {

public:
    // allocation properties
    bool can_inline_allocation() const {
        return false;
    }


    // Return the Oop size for a ByteArrayOop
    std::int32_t object_size( std::int32_t number_of_bytes ) const {
        return non_indexable_size() + 1 + roundTo( number_of_bytes, OOP_SIZE ) / OOP_SIZE;
    }


    // Layout
    std::int32_t length_offset() const {
        return non_indexable_size();
    }


    std::int32_t array_offset() const {
        return non_indexable_size() + 1;
    }


    // creation operations
    Oop allocateObject( bool permit_scavenge = true, bool tenured = false );

    Oop allocateObjectSize( std::int32_t bytes, bool permit_scavenge = true, bool tenured = false );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );

    static KlassOop create_class( KlassOop super_class, MixinOop mixin );


    // Format
    Format format() {
        return Format::byteArray_klass;
    }


    // Initialize the object
    void initialize_object( ByteArrayOop obj, const char *value, std::int32_t len );

    friend void setKlassVirtualTableFromByteArrayKlass( Klass *k );


    const char *name() const {
        return "byteArray";
    }

    // ALL FUNCTIONS BELOW THIS POINT ARE DISPATCHED FROM AN OOP
public:
    // accessors
    std::int32_t oop_size( Oop obj ) const {
        return object_size( ByteArrayOop( obj )->length() );
    }


    std::int32_t oop_scavenge_contents( Oop obj );

    std::int32_t oop_scavenge_tenured_contents( Oop obj );

    bool oop_verify( Oop obj );

    void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );

    // iterators
    void oop_oop_iterate( Oop obj, OopClosure *blk );

    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );


    // Sizing
    std::int32_t oop_header_size() const {
        return ByteArrayOopDescriptor::header_size();
    }


    // testers
    bool oop_is_byteArray() const {
        return true;
    }


    bool oop_is_indexable() const {
        return true;
    }

};

void setKlassVirtualTableFromByteArrayKlass( Klass *k );
