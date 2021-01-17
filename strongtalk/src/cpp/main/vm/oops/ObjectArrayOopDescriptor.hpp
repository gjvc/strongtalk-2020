//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/Klass.hpp"
#include "vm/oops/MemOopDescriptor.hpp"


// objArrays are arrays containing oops

// memory layout:
//    [header      ]
//    [klass_field ]
//    [instVars    ]*
//    [length      ]
//    [elements    ]* = objs(1) .. objs(length)

class ObjectArrayOopDescriptor : public MemOopDescriptor {
    public:
        friend ObjectArrayOop as_objArrayOop( void * p );

        void bootstrap_object( Bootstrap * stream );


        // accessors
        ObjectArrayOopDescriptor * addr() const {
            return ( ObjectArrayOopDescriptor * ) MemOopDescriptor::addr();
        }


        bool_t is_within_bounds( int index ) const {
            return 1 <= index and index <= length();
        }


        Oop * addr_as_oops() const {
            return ( Oop * ) addr();
        }


        Oop * objs( int which ) const {
            return &length_addr()[ which ];
        }


        Oop * length_addr() const {
            return &addr_as_oops()[ blueprint()->non_indexable_size() ];
        }


        smi_t length() const {
            Oop len = *length_addr();
            st_assert( len->is_smi(), "length of indexable should be smi_t" );
            return SMIOop( len )->value();
        }


        void set_length( smi_t len ) {
            *length_addr() = smiOopFromValue( len );
        }


        Oop obj_at( int which ) const {
            st_assert( which > 0 and which <= length(), "index out of bounds" );
            return *objs( which );
        }


        void obj_at_put( int which, Oop contents, bool_t cs = true ) {
            st_assert( which > 0 and which <= length(), "index out of bounds" );
            st_assert( not is_symbol(), "shouldn't be modifying a canonical string" );
            st_assert( contents->verify(), "check contents" );
            if ( cs ) {
                STORE_OOP( objs( which ), contents );
            } else {
                *objs( which ) = contents;
            }
        }


        ObjectArrayOop growBy( int increment );

        // memory operations
        bool_t verify();

        ObjectArrayOop copy_remove( int from, int number = 1 );

        ObjectArrayOop copy();

        ObjectArrayOop copy_add( Oop a );

        ObjectArrayOop copy_add_two( Oop a, Oop b );

        // primtiive operations
        void replace_from_to( int from, int to, ObjectArrayOop source, int start );

        void replace_and_fill( int from, int start, ObjectArrayOop source );

        friend class objArrayKlass;

    private:
        // define the interval [begin .. end[ where the indexables are.
        Oop * begin_indexables() const {
            return objs( 1 );
        }


        Oop * end_indexables() const {
            return begin_indexables() + length();
        }
};


inline ObjectArrayOop as_objArrayOop( void * p ) {
    return ObjectArrayOop( as_memOop( p ) );
}


class WeakArrayOopDescriptor : public ObjectArrayOopDescriptor {
    public:
        friend WeakArrayOop as_weakArrayOop( void * p ) {
            return WeakArrayOop( as_memOop( p ) );
        }


        // accessors
        WeakArrayOopDescriptor * addr() const {
            return ( WeakArrayOopDescriptor * ) MemOopDescriptor::addr();
        }


        void scavenge_contents_after_registration();

        void follow_contents_after_registration();
};
