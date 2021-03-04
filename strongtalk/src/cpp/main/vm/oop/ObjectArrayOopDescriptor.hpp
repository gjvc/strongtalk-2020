//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/klass/Klass.hpp"


// ObjectArrays are arrays containing oops

//
//
// memory layout:
//    [header      ]
//    [klass_field ]
//    [instVars    ]*
//    [length      ]
//    [elements    ]* = objs(1) .. objs(length)
//

class ObjectArrayOopDescriptor : public MemOopDescriptor {
public:
    friend ObjectArrayOop as_objectArrayOop( void *p );

    void bootstrap_object( Bootstrap *stream );


    // accessors
    ObjectArrayOopDescriptor *addr() const {
        return static_cast<ObjectArrayOopDescriptor *>( MemOopDescriptor::addr() );
    }


    bool is_within_bounds( std::size_t index ) const {
        return 1 <= index and index <= length();
    }


    Oop *addr_as_oops() const {
        return reinterpret_cast<Oop *>( addr() );
    }


    Oop *objs( std::size_t which ) const {
        return &length_addr()[ which ];
    }


    Oop *length_addr() const {
        return &addr_as_oops()[ blueprint()->non_indexable_size() ];
    }


    std::size_t length() const {
        Oop len = *length_addr();
        st_assert( len->isSmallIntegerOop(), "length of indexable should be small_int_t" );
        auto value = SmallIntegerOop( len )->value();
        return static_cast<std::int32_t>( value );
    }


    void set_length( small_int_t len ) {
        *length_addr() = smiOopFromValue( len );
    }


    Oop obj_at( std::size_t which ) const {
        st_assert( which > 0 and which <= length(), "index out of bounds" );
        return *objs( which );
    }


    void obj_at_put( std::size_t which, Oop contents, bool cs = true ) {
        st_assert( which > 0 and which <= length(), "index out of bounds" );
        st_assert( not isSymbol(), "shouldn't be modifying a canonical string" );
        st_assert( contents->verify(), "check contents" );
        if ( cs ) {
            STORE_OOP( objs( which ), contents );
        } else {
            *objs( which ) = contents;
        }
    }


    ObjectArrayOop growBy( std::int32_t increment );

    // memory operations
    bool verify();

    ObjectArrayOop copy_remove( std::int32_t from, std::int32_t number = 1 );

    ObjectArrayOop copy();

    ObjectArrayOop copy_add( Oop a );

    ObjectArrayOop copy_add_two( Oop a, Oop b );

    // primtiive operations
    void replace_from_to( std::int32_t from, std::int32_t to, ObjectArrayOop source, std::int32_t start );

    void replace_and_fill( std::int32_t from, std::int32_t start, ObjectArrayOop source );

    friend class objectArrayKlass;

private:
    // define the interval [begin .. end[ where the indexables are.
    Oop *begin_indexables() const {
        return objs( 1 );
    }


    Oop *end_indexables() const {
        return begin_indexables() + length();
    }
};


inline ObjectArrayOop as_objectArrayOop( void *p ) {
    return ObjectArrayOop( as_memOop( p ) );
}


class WeakArrayOopDescriptor : public ObjectArrayOopDescriptor {
public:
    friend WeakArrayOop as_weakArrayOop( void *p ) {
        return WeakArrayOop( as_memOop( p ) );
    }


    // accessors
    WeakArrayOopDescriptor *addr() const {
        return (WeakArrayOopDescriptor *) MemOopDescriptor::addr();
    }


    void scavenge_contents_after_registration();

    void follow_contents_after_registration();
};
