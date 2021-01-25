
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"


// -----------------------------------------------------------------------------

typedef class OopDescriptor *Oop;
constexpr std::int32_t oopSize = sizeof( Oop );


// -----------------------------------------------------------------------------

typedef Oop     (__CALLING_CONVENTION *primitiveFunctionType)( ... );
//typedef void    (__CALLING_CONVENTION * oopsDoFn)( Oop * p );
typedef void    (__CALLING_CONVENTION *doFn)();


// -----------------------------------------------------------------------------

typedef class OopDescriptor                 *Oop;
typedef class MarkOopDescriptor             *MarkOop;
typedef class MemOopDescriptor              *MemOop;
typedef class AssociationOopDescriptor      *AssociationOop;
typedef class BlockClosureOopDescriptor     *BlockClosureOop;
typedef class ByteArrayOopDescriptor        *ByteArrayOop;
typedef class SymbolOopDescriptor           *SymbolOop;
typedef class ContextOopDescriptor          *ContextOop;
typedef class DoubleByteArrayOopDescriptor  *DoubleByteArrayOop;
typedef class DoubleOopDescriptor           *DoubleOop;
typedef class DoubleValueArrayOopDescriptor *doubleValueArrayOop;
typedef class KlassOopDescriptor            *KlassOop;
typedef class MethodOopDescriptor           *MethodOop;
typedef class MixinOopDescriptor            *MixinOop;
typedef class ObjectArrayOopDescriptor      *ObjectArrayOop;
typedef class WeakArrayOopDescriptor        *WeakArrayOop;
typedef class ProcessOopDescriptor          *ProcessOop;
typedef class ProxyOopDescriptor            *ProxyOop;
typedef class VirtualFrameOopDescriptor     *VirtualFrameOop;
typedef class SMIOopDescriptor              *SMIOop;


// -----------------------------------------------------------------------------

typedef class ScopeDescriptorNode *ScopeInfo;

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

extern "C" Oop nilObject;

class Generation;

class Klass;

class ConsoleOutputStream;

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


    bool_t is_smi() const {
        return tag() == INTEGER_TAG;
    }


    bool_t is_mem() const {
        return tag() == MEMOOP_TAG;
    }


    bool_t is_mark() const {
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
    bool_t is_old() const;

    bool_t is_new() const;

    Generation *my_generation();

    // type test operations (inlined in Oop.inline.h)
    bool_t is_double() const;

    bool_t is_block() const;

    bool_t is_byteArray() const;

    bool_t is_doubleByteArray() const;

    bool_t is_doubleValueArray() const;

    bool_t is_symbol() const;

    bool_t is_objArray() const;

    bool_t is_weakArray() const;

    bool_t is_klass() const;

    bool_t is_process() const;

    bool_t is_vframe() const;

    bool_t is_method() const;

    bool_t is_proxy() const;

    bool_t is_mixin() const;

    bool_t is_association() const;

    bool_t is_context() const;

    bool_t is_indexable() const;


    // Returns is the Oop is the nil object
    bool_t is_nil() const {
        return this == nilObject;
    }


    // Primitives
    Oop primitive_allocate( bool_t allow_scavenge = true, bool_t tenured = false );

    Oop primitive_allocate_size( std::int32_t size );

    Oop shallow_copy( bool_t tenured );

    bool_t verify();

    // printing functions for VM debugging
    void print_on( ConsoleOutputStream *stream );        // First level print
    void print_value_on( ConsoleOutputStream *stream );  // Prints Oop as <ClassName>(<objectID>).

    // printing on default output stream
    void print();

    void print_value();

    // return the print strings
    char *print_string();

    char *print_value_string();

    friend class memOopKlass;
};
