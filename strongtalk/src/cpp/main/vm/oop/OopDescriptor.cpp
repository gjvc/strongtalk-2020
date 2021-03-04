
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oop/OopDescriptor.hpp"
#include "vm/oop/SMIOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/utility/StringOutputStream.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


// Called during bootstrappingInProgress for computing vtbl values see (create_*Klass)
OopDescriptor::OopDescriptor() : _mark{ nullptr } {
    if ( not bootstrappingInProgress ) {
        ShouldNotCallThis();
    }
}


void OopDescriptor::print_value_on( ConsoleOutputStream *stream ) {

    if ( isMarkOop() ) {
        MarkOop( this )->print_on( stream );
    } else if ( isSmallIntegerOop() ) {
        SmallIntegerOop( this )->print_on( stream );
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
    if ( isMarkOop() ) {
        MarkOop( this )->print_on( stream );
    } else if ( isSmallIntegerOop() ) {
        SmallIntegerOop( this )->print_on( stream );
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
    if ( isMemOop() )
        return MemOop( this )->klass_field();
    st_assert( isSmallIntegerOop(), "tag must be small_int_t" );
    return smiKlassObject;
}


Klass *OopDescriptor::blueprint() const {
    return klass()->klass_part();
}


small_int_t OopDescriptor::identity_hash() {
    if ( isSmallIntegerOop() )
        return SmallIntegerOop( this )->identity_hash();
    st_assert( isMemOop(), "tag must be mem" );
    return MemOop( this )->identity_hash();
}


Oop OopDescriptor::scavenge() {
    return isMemOop() ? MemOop( this )->scavenge() : this;
}


Oop OopDescriptor::relocate() {
    // FIX LATER
    return this;
}


// generation testers
bool OopDescriptor::is_old() const {
    return isMemOop() and MemOop( this )->is_old();
}


bool OopDescriptor::is_new() const {
    return isMemOop() and MemOop( this )->is_new();
}


Generation *OopDescriptor::my_generation() {
    return Universe::generation_containing( this );
}


// type test operations
bool OopDescriptor::isDouble() const {
    return isMemOop() and MemOop( this )->klass_field() == doubleKlassObject;
}


bool OopDescriptor::is_block() const {
    return isMemOop() and MemOop( this )->blueprint()->oop_is_block();
}


bool OopDescriptor::isByteArray() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsByteArray();
}


bool OopDescriptor::isDoubleByteArray() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsDoubleByteArray();
}


bool OopDescriptor::isDoubleValueArray() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsDoubleValueArray();
}


bool OopDescriptor::isSymbol() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsSymbol();
}


bool OopDescriptor::isObjectArray() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsObjectArray();
}


bool OopDescriptor::is_weakArray() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsWeakArray();
}


bool OopDescriptor::is_association() const {
    return isMemOop() and MemOop( this )->blueprint()->oop_is_association();
}


bool OopDescriptor::is_context() const {
    return isMemOop() and MemOop( this )->blueprint()->oop_is_context();
}


bool OopDescriptor::is_klass() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsKlass();
}


bool OopDescriptor::is_proxy() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsProxy();
}


bool OopDescriptor::is_mixin() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsMixin();
}


bool OopDescriptor::is_process() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsProcess();
}


bool OopDescriptor::is_VirtualFrame() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsVirtualFrame();
}


bool OopDescriptor::is_method() const {
    return isMemOop() and MemOop( this )->blueprint()->oopIsMethod();
}


bool OopDescriptor::is_indexable() const {
    return isMemOop() and MemOop( this )->blueprint()->oop_is_indexable();
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
