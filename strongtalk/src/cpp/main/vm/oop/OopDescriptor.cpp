
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oop/OopDescriptor.hpp"
#include "vm/oop/SmallIntegerOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/utility/StringOutputStream.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


//// Called during bootstrappingInProgress for computing vtbl values see (create_*Klass)
//OopDescriptor::OopDescriptor() {
//    if ( not bootstrappingInProgress ) {
//        ShouldNotCallThis();
//    }
//}


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
        reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->print_on( stream );
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
        return reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->klass_field();
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
    return reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->identity_hash();
}


Oop OopDescriptor::scavenge() {
    return isMemOop() ? reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->scavenge() : this;
}


Oop OopDescriptor::relocate() {
    // FIX LATER
    return this;
}


// generation testers
bool OopDescriptor::is_old() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->is_old();
}


bool OopDescriptor::is_new() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->is_new();
}


Generation *OopDescriptor::my_generation() {
    return Universe::generation_containing( this );
}


// type test operations
bool OopDescriptor::isDouble() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->klass_field() == doubleKlassObject;
}


bool OopDescriptor::is_block() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsBlock();
}


bool OopDescriptor::isByteArray() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsByteArray();
}


bool OopDescriptor::isDoubleByteArray() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsDoubleByteArray();
}


bool OopDescriptor::isDoubleValueArray() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsDoubleValueArray();
}


bool OopDescriptor::isSymbol() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsSymbol();
}


bool OopDescriptor::isObjectArray() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsObjectArray();
}


bool OopDescriptor::is_weakArray() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsWeakArray();
}


bool OopDescriptor::is_association() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsAssociation();
}


bool OopDescriptor::is_context() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsContext();
}


bool OopDescriptor::is_klass() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsKlass();
}


bool OopDescriptor::is_proxy() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsProxy();
}


bool OopDescriptor::is_mixin() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsMixin();
}


bool OopDescriptor::is_process() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsProcess();
}


bool OopDescriptor::is_VirtualFrame() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsVirtualFrame();
}


bool OopDescriptor::is_method() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsMethod();
}


bool OopDescriptor::is_indexable() const {
    return isMemOop() and reinterpret_cast<MemOop>( const_cast<Oop>( this ) )->blueprint()->oopIsIndexable();
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
