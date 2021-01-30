
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/OopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utilities/OutputStream.hpp"


// Called during bootstrappingInProgress for computing vtbl values see (create_*Klass)
OopDescriptor::OopDescriptor() {
    if ( not bootstrappingInProgress ) ShouldNotCallThis();
}


void OopDescriptor::print_value_on( ConsoleOutputStream *stream ) {
    if ( is_mark() ) {
        MarkOop( this )->print_on( stream );
    } else if ( is_smi() ) {
        SMIOop( this )->print_on( stream );
    } else {
        // In the debug version unused Space is cleared after scavenge.
        // This means if we try printing an Oop pointing to unused Space
        // its klass() is nullptr.
        // The following hack can print such oops.
        if ( klass()->addr() == nullptr ) {
            stream->print( "Wrong Oop(0x%lx)", this );
        } else {
            blueprint()->oop_print_value_on( this, stream );
        }
    }
}


void OopDescriptor::print_on( ConsoleOutputStream *stream ) {
    if ( is_mark() ) {
        MarkOop( this )->print_on( stream );
    } else if ( is_smi() ) {
        SMIOop( this )->print_on( stream );
    } else {
        MemOop( this )->print_on( stream );
    }
}


void OopDescriptor::print() {
    print_on( _console );
}


void OopDescriptor::print_value() {
    print_value_on( _console );
}


const char *OopDescriptor::toString() {
    StringOutputStream *stream = new StringOutputStream( 50 );
    print_on( stream );
    return stream->as_string();
}


char *OopDescriptor::print_value_string() {
    StringOutputStream *stream = new StringOutputStream( 50 );
    print_value_on( stream );
    return stream->as_string();
}


KlassOop OopDescriptor::klass() const {
    if ( is_mem() )
        return MemOop( this )->klass_field();
    st_assert( is_smi(), "tag must be smi_t" );
    return smiKlassObject;
}


Klass *OopDescriptor::blueprint() const {
    return klass()->klass_part();
}


smi_t OopDescriptor::identity_hash() {
    if ( is_smi() )
        return SMIOop( this )->identity_hash();
    st_assert( is_mem(), "tag must be mem" );
    return MemOop( this )->identity_hash();
}


Oop OopDescriptor::scavenge() {
    return is_mem() ? MemOop( this )->scavenge() : this;
}


Oop OopDescriptor::relocate() {
    // FIX LATER
    return this;
}


// generation testers
bool OopDescriptor::is_old() const {
    return is_mem() and MemOop( this )->is_old();
}


bool OopDescriptor::is_new() const {
    return is_mem() and MemOop( this )->is_new();
}


Generation *OopDescriptor::my_generation() {
    return Universe::generation_containing( this );
}


// type test operations
bool OopDescriptor::is_double() const {
    return is_mem() and MemOop( this )->klass_field() == doubleKlassObject;
}


bool OopDescriptor::is_block() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_block();
}


bool OopDescriptor::is_byteArray() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_byteArray();
}


bool OopDescriptor::is_doubleByteArray() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_doubleByteArray();
}


bool OopDescriptor::is_doubleValueArray() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_doubleValueArray();
}


bool OopDescriptor::is_symbol() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_symbol();
}


bool OopDescriptor::is_objArray() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_objArray();
}


bool OopDescriptor::is_weakArray() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_weakArray();
}


bool OopDescriptor::is_association() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_association();
}


bool OopDescriptor::is_context() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_context();
}


bool OopDescriptor::is_klass() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_klass();
}


bool OopDescriptor::is_proxy() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_proxy();
}


bool OopDescriptor::is_mixin() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_mixin();
}


bool OopDescriptor::is_process() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_process();
}


bool OopDescriptor::is_vframe() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_vframe();
}


bool OopDescriptor::is_method() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_method();
}


bool OopDescriptor::is_indexable() const {
    return is_mem() and MemOop( this )->blueprint()->oop_is_indexable();
}


// Primitives
bool OopDescriptor::verify() {
    return blueprint()->oop_verify( this );
}


Oop OopDescriptor::primitive_allocate( bool allow_scavenge, bool tenured ) {
    return blueprint()->oop_primitive_allocate( this, allow_scavenge, tenured );
}


Oop OopDescriptor::primitive_allocate_size( std::int32_t size ) {
    return blueprint()->oop_primitive_allocate_size( this, size );
}


Oop OopDescriptor::shallow_copy( bool tenured ) {
    return blueprint()->oop_shallow_copy( this, tenured );
}
