
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/Oop.hpp"


class OopDescriptor {

protected:

public:
    // The _mark instance variable is here rather than in MemOop (where it belongs) because many C++ compilers have trouble with empty objects (size 0),
    // i.e., give them nonzero length which messes up all the subclasses.

    // So, to be perfectly clear, not all oopDescriptor classes truly have a _mark word; only the ones below MemOop do.
    MarkOop _mark;

public:
    // Called during bootstrappingInProgress for computing vtbl values see (create_*Klass)
    OopDescriptor();


    // tag checks
    std::int32_t tag() const {
        return maskBits( std::int32_t( this ), TAG_MASK );
    }


    bool is_smi() const {
        return tag() == INTEGER_TAG;
    }


    bool is_mem() const {
        return tag() == MEMOOP_TAG;
    }


    bool is_mark() const {
        return tag() == MARK_TAG;
    }


    // tag dispatchers (inlined in Oop.inline.h)
    Klass *blueprint() const;

    KlassOop klass() const;

    smi_t identity_hash();

    // memory management
    Oop scavenge();

    Oop relocate();

    // generation testers (inlined in Oop.inline.h)
    bool is_old() const;

    bool is_new() const;

    Generation *my_generation();

    // type test operations (inlined in Oop.inline.h)
    bool is_double() const;

    bool is_block() const;

    bool is_byteArray() const;

    bool is_doubleByteArray() const;

    bool is_doubleValueArray() const;

    bool is_symbol() const;

    bool is_objArray() const;

    bool is_weakArray() const;

    bool is_klass() const;

    bool is_process() const;

    bool is_vframe() const;

    bool is_method() const;

    bool is_proxy() const;

    bool is_mixin() const;

    bool is_association() const;

    bool is_context() const;

    bool is_indexable() const;


    // Returns is the Oop is the nil object
    bool is_nil() const {
        return this == nilObject;
    }


    // Primitives
    Oop primitive_allocate( bool allow_scavenge = true, bool tenured = false );

    Oop primitive_allocate_size( std::int32_t size );

    Oop shallow_copy( bool tenured );

    bool verify();

    // printing functions for VM debugging
    void print_on( ConsoleOutputStream *stream );        // First level print
    void print_value_on( ConsoleOutputStream *stream );  // Prints Oop as <ClassName>(<objectID>).

    // printing on default output stream
    void print();

    void print_value();

    // return the print strings
    const char *toString();

    char *print_value_string();

    friend class memOopKlass;
};
