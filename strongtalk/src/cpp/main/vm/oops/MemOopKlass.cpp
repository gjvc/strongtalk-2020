//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/ObjectArrayKlass.hpp"
#include "vm/oops/ByteArrayKlass.hpp"
#include "vm/oops/WeakArrayKlass.hpp"
#include "vm/oops/DoubleByteArrayKlass.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/MixinKlass.hpp"
#include "vm/oops/ProxyKlass.hpp"
#include "vm/oops/ProcessKlass.hpp"
#include "vm/memory/PrintObjectClosure.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/vmSymbols.hpp"


void setKlassVirtualTableFromMemOopKlass( Klass *k ) {
    MemOopKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


std::int32_t MemOopKlass::oop_scavenge_contents( Oop obj ) {
    std::int32_t size = non_indexable_size();
    // header
    MemOop( obj )->scavenge_header();
    // instance variables
    MemOop( obj )->scavenge_body( MemOopDescriptor::header_size(), size );
    return size;
}


std::int32_t MemOopKlass::oop_scavenge_tenured_contents( Oop obj ) {
    std::int32_t size = non_indexable_size();
    // header
    MemOop( obj )->scavenge_tenured_header();
    // instance variables
    MemOop( obj )->scavenge_tenured_body( MemOopDescriptor::header_size(), size );
    return size;
}


void MemOopKlass::oop_follow_contents( Oop obj ) {
    // header
    MemOop( obj )->follow_header();
    // instance variables
    MemOop( obj )->follow_body( MemOopDescriptor::header_size(), non_indexable_size() );
}


bool MemOopKlass::oop_verify( Oop obj ) {
    return Universe::verify_oop( MemOop( obj ) );
}


void MemOopKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    // header
    MemOop( obj )->layout_iterate_header( blk );
    // instance variables
    MemOop( obj )->layout_iterate_body( blk, MemOopDescriptor::header_size(), non_indexable_size() );
}


void MemOopKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    // header
    MemOop( obj )->oop_iterate_header( blk );
    // instance variables
    MemOop( obj )->oop_iterate_body( blk, MemOopDescriptor::header_size(), non_indexable_size() );
}


void MemOopKlass::oop_print_on( Oop obj, ConsoleOutputStream *stream ) {
    PrintObjectClosure blk( stream );
    blk.do_object( MemOop( obj ) );
    MemOop( obj )->layout_iterate( &blk );
}


void MemOopKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    if ( obj == nilObject )
        stream->print( "nil" );
    else if ( obj == trueObject )
        stream->print( "true" );
    else if ( obj == falseObject )
        stream->print( "false" );
    else {
        if ( PrintObjectID ) {
            MemOop( obj )->print_id_on( stream );
            stream->print( "-" );
        }
        print_name_on( stream );
    }
    if ( PrintOopAddress )
        stream->print( " (0x{0:x})", this );
}


Oop MemOopKlass::allocateObject( bool permit_scavenge, bool tenured ) {
    KlassOop k    = as_klassOop();
    std::int32_t      size = non_indexable_size();

    Oop *result = basicAllocate( size, &k, permit_scavenge, tenured );
    if ( not result )
        return nullptr;
    // allocate
    MemOop obj = as_memOop( result );
    // header
    obj->initialize_header( has_untagged_contents(), k );
    // instance variables
    obj->initialize_body( MemOopDescriptor::header_size(), size );
    return obj;
}


Oop MemOopKlass::allocateObjectSize( std::int32_t size, bool permit_scavenge, bool permit_tenured ) {
    return markSymbol( vmSymbols::not_indexable() );
}


KlassOop MemOopKlass::create_subclass( MixinOop mixin, KlassOop instSuper, KlassOop metaClass, Format format ) {
    MemOopKlass o;
    return create_generic_class( as_klassOop(), instSuper, metaClass, mixin, o.vtbl_value() );
}


KlassOop MemOopKlass::create_subclass( MixinOop mixin, Format format ) {

    st_assert( can_be_subclassed(), "must be able to subclass this" );
    if ( format == Format::mem_klass )
        return MemOopKlass::create_class( as_klassOop(), mixin );

    if ( format == Format::objArray_klass )
        return ObjectArrayKlass::create_class( as_klassOop(), mixin );
    if ( format == Format::byteArray_klass )
        return ByteArrayKlass::create_class( as_klassOop(), mixin );
    if ( format == Format::doubleByteArray_klass )
        return DoubleByteArrayKlass::create_class( as_klassOop(), mixin );
    if ( format == Format::weakArray_klass )
        return WeakArrayKlass::create_class( as_klassOop(), mixin );

    if ( number_of_instance_variables() > 0 ) {
        spdlog::warn( "super class has instance variables when mixing in special mixin" );
        return nullptr;
    }

    if ( format == Format::mixin_klass )
        return MixinKlass::create_class( as_klassOop(), mixin );
    if ( format == Format::proxy_klass )
        return ProxyKlass::create_class( as_klassOop(), mixin );
    if ( format == Format::process_klass )
        return ProcessKlass::create_class( as_klassOop(), mixin );
    return nullptr;
}


KlassOop MemOopKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    MemOopKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


Oop MemOopKlass::oop_shallow_copy( Oop obj, bool tenured ) {
    // Do not copy oddballs (nil, true, false)
    if ( obj == nilObject )
        return obj;
    if ( obj == trueObject )
        return obj;
    if ( obj == falseObject )
        return obj;

    std::int32_t len = MemOop( obj )->size();
    // Important to preserve obj (in case of scavenge).
    Oop *clone = tenured ? Universe::allocate_tenured( len ) : Universe::allocate( len, (MemOop *) &obj );
    Oop *to    = clone;
    Oop *from  = (Oop *) MemOop( obj )->addr();
    Oop *end   = to + len;
    while ( to < end )
        *to++ = *from++;

    if ( not as_memOop( clone )->is_new() ) {
        // Remember to update the remembered set if the clone is in old Space.
        // Note:
        //   should we do something special for arrays.
        Universe::remembered_set->record_store( clone );
    }
    return as_memOop( clone );
}
