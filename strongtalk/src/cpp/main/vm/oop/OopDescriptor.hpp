
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oop/Oop.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


//
// oopDescriptor ("object-orientated pointer descriptor") is the top of the Oop hierarchy.
//

// The "*Descriptor" classes describe the format of ST objects so the fields can be accessed from C++.
// "*Oop" pointers to "*Descriptor" structures (e.g., Oop, proxyOop) are TAGGED and thus should not be used to access the fields.
// Instead, convert the xxxOop to a xxxDescriptor* with the ->addr() function, then work with the xxxDescriptor* pointer.

// xxxOop pointers are tagged.
// xxxDescriptor* pointers are not tagged.
// convert xxxOop to a xxxDescriptor* with the xxxOop->addr() function

// NB: the above is true only for memOops


//
class Klass;
class Generation;
extern "C" Oop nilObject;

typedef class ScopeDescriptorNode *ScopeInfo;

//
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


    bool isSmallIntegerOop() const {
        return tag() == INTEGER_TAG;
    }


    bool isMemOop() const {
        return tag() == MEMOOP_TAG;
    }


    bool isMarkOop() const {
        return tag() == MARK_TAG;
    }


    // tag dispatchers (inlined in Oop.inline.h)
    Klass *blueprint() const;

    KlassOop klass() const;

    small_int_t identity_hash();

    // memory management
    Oop scavenge();

    Oop relocate();

    // generation testers (inlined in Oop.inline.h)
    bool is_old() const;

    bool is_new() const;

    Generation *my_generation();

    // type test operations (inlined in Oop.inline.h)
    bool isDouble() const;

    bool is_block() const;

    bool isByteArray() const;

    bool isDoubleByteArray() const;

    bool isDoubleValueArray() const;

    bool isSymbol() const;

    bool isObjectArray() const;

    bool is_weakArray() const;

    bool is_klass() const;

    bool is_process() const;

    bool is_VirtualFrame() const;

    bool is_method() const;

    bool is_proxy() const;

    bool is_mixin() const;

    bool is_association() const;

    bool is_context() const;

    bool is_indexable() const;


    // Returns is the Oop is the nil object
    bool isNilObject() const {
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
