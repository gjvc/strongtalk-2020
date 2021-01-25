//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"


//
// Associations are cons cells (key, value) used in the Delta system dictionary.
//
//  - constAssociationOops are immutable.
//  - associationOops      are mutable.
//

// memory layout:
//  [header      ]
//  [klass_field ]
//  [key         ]
//  [value       ]
//  [is_constant ]

class AssociationOopDescriptor : public MemOopDescriptor {

protected:
    SymbolOop _key;
    Oop       _value;
    Oop       _is_constant;

public:
    AssociationOopDescriptor *addr() const {
        return (AssociationOopDescriptor *) MemOopDescriptor::addr();
    }


    friend AssociationOop as_associationOop( void *p );


    // sizing
    static std::int32_t header_size() {
        return sizeof( AssociationOopDescriptor ) / oopSize;
    }


    void bootstrap_object( Bootstrap *stream );


    SymbolOop key() const {
        return addr()->_key;
    }


    void set_key( SymbolOop k ) {
        STORE_OOP( &addr()->_key, k );
    }


    Oop value() const {
        return addr()->_value;
    }


    void set_value( Oop v ) {
        STORE_OOP( &addr()->_value, v );
    }


    bool_t is_constant() const {
        return addr()->_is_constant == trueObject;
    }


    void set_is_constant( bool_t v );


    static std::int32_t key_offset() {
        return 2;
    } // offset of the key field in words

    static std::int32_t value_offset() {
        return 3;
    } // offset of the value field in words

    static std::int32_t is_constant_offset() {
        return 4;
    } // offset of the is_constant field in words

    friend class AssociationKlass;
};


inline AssociationOop as_associationOop( void *p ) {
    return AssociationOop( as_memOop( p ) );
}
