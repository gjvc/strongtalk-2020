//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ObjectArrayKlass.hpp"
#include "vm/oops/WeakArrayKlass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/memory/MarkSweep.hpp"


Oop ObjectArrayKlass::allocateObjectSize( std::size_t size, bool_t permit_scavenge, bool_t tenured ) {
    KlassOop k        = as_klassOop();
    int      ni_size  = non_indexable_size();
    int      obj_size = ni_size + 1 + size;

    // allocate
    Oop *result = tenured ? Universe::allocate_tenured( obj_size, permit_scavenge ) : Universe::allocate( obj_size, (MemOop *) &k, permit_scavenge );
    if ( not result )
        return nullptr;

    ObjectArrayOop obj = as_objArrayOop( result );
    // header
    MemOop( obj )->initialize_header( has_untagged_contents(), k );

    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );

    // indexables
    Oop *base = (Oop *) obj->addr();
    Oop *end  = base + obj_size;
    // %optimized 'obj->set_length(size)'
    base[ ni_size ] = smiOopFromValue( size );
    MemOop( obj )->initialize_body( ni_size + 1, obj_size );

    return obj;
}


KlassOop ObjectArrayKlass::create_subclass( MixinOop mixin, Format format ) {

    if ( format == Format::weakArray_klass )
        return WeakArrayKlass::create_class( as_klassOop(), mixin );

    if ( format == Format::mem_klass or format == Format::objArray_klass ) {
        return ObjectArrayKlass::create_class( as_klassOop(), mixin );
    }

    return nullptr;
}


KlassOop ObjectArrayKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    ObjectArrayKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


ObjectArrayOop ObjectArrayKlass::allocate_tenured_pic( std::size_t size ) {
    KlassOop k        = Universe::objArrayKlassObj();
    int      ni_size  = k->klass_part()->non_indexable_size();
    int      obj_size = ni_size + 1 + size;

    // allocate
    ObjectArrayOop obj = as_objArrayOop( Universe::allocate_tenured( obj_size ) );

    // header
    MemOop( obj )->initialize_header( k->klass_part()->has_untagged_contents(), k );

    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );

    // indexables
    Oop *base = (Oop *) obj->addr();
    Oop *end  = base + obj_size;
    // %optimized 'obj->set_length(size)'
    base[ ni_size ] = smiOopFromValue( size );
    MemOop( obj )->initialize_body( ni_size + 1, obj_size );

    return obj;
}


void setKlassVirtualTableFromObjArrayKlass( Klass *k ) {
    ObjectArrayKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


void ObjectArrayKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {

    // Retrieve length information in case the iterator mutates the object
    Oop *p = ObjectArrayOop( obj )->objs( 0 );
    int len = ObjectArrayOop( obj )->length();

    // header + instance variables
    MemOopKlass::oop_layout_iterate( obj, blk );
    // indexables
    blk->do_oop( "length", p++ );
    blk->begin_indexables();
    for ( std::size_t i = 1; i <= len; i++ ) {
        blk->do_indexable_oop( i, p++ );
    }
    blk->end_indexables();
}


void ObjectArrayKlass::oop_short_print_on( Oop obj, ConsoleOutputStream *stream ) {

    const int MaxPrintLen = 255;    // to prevent excessive output -Urs
    st_assert_objArray( obj, "Argument must be objArray" );
    ObjectArrayOop array = ObjectArrayOop( obj );
    int            len   = array->length();
    int            n     = min( MaxElementPrintSize, len );
    stream->print( "'" );

    for ( std::size_t i = 1; i <= n and stream->position() < MaxPrintLen; i++ ) {
        array->obj_at( i )->print_value_on( stream );
        stream->print( ", " );
    }
    if ( n < len )
        stream->print( "... " );
    else
        stream->print( "' " );
    oop_print_value_on( obj, stream );
}


void ObjectArrayKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    // Retrieve length information in case the iterator mutates the object
    Oop *p = ObjectArrayOop( obj )->objs( 0 );
    int len = ObjectArrayOop( obj )->length();

    // header + instance variables
    MemOopKlass::oop_oop_iterate( obj, blk );
    // indexables
    blk->do_oop( p++ );
    for ( std::size_t i = 1; i <= len; i++ ) {
        blk->do_oop( p++ );
    }
}


int ObjectArrayKlass::oop_scavenge_contents( Oop obj ) {

    // header + instance variables
    MemOopKlass::oop_scavenge_contents( obj );

    // indexables
    ObjectArrayOop o = ObjectArrayOop( obj );
    Oop *base = o->objs( 1 );
    Oop *end  = base + o->length();
    while ( base < end ) {
        scavenge_oop( base++ );
    }

    return object_size( o->length() );
}


int ObjectArrayKlass::oop_scavenge_tenured_contents( Oop obj ) {

    // header + instance variables
    MemOopKlass::oop_scavenge_tenured_contents( obj );

    // indexables
    ObjectArrayOop o = ObjectArrayOop( obj );
    Oop *base = o->objs( 1 );
    Oop *end  = base + o->length();
    while ( base < end )
        scavenge_tenured_oop( base++ );

    return object_size( o->length() );
}


void ObjectArrayKlass::oop_follow_contents( Oop obj ) {

    // Retrieve length information since header information  mutates the object
    Oop *base = ObjectArrayOop( obj )->objs( 1 );
    Oop *end  = base + ObjectArrayOop( obj )->length();

    // header + instance variables
    MemOopKlass::oop_follow_contents( obj );

    // indexables
    while ( base < end )
        MarkSweep::reverse_and_push( base++ );
}
