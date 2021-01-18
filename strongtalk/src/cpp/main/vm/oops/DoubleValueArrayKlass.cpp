//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/oops/DoubleValueArrayOopDescriptor.hpp"
#include "vm/oops/DoubleValueArrayKlass.hpp"
#include "vm/memory/util.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/system/sizes.hpp"


Oop DoubleValueArrayKlass::allocateObject( bool_t permit_scavenge, bool_t tenured ) {
    st_fatal( "should never call allocateObject in doubleValueArrayKlass" );
    return badOop;
}


Oop DoubleValueArrayKlass::allocateObjectSize( int size, bool_t permit_scavenge, bool_t permit_tenured ) {

    //
    KlassOop k        = as_klassOop();
    int      ni_size  = non_indexable_size();
    int      obj_size = ni_size + 1 + roundTo( size * sizeof( double ), oopSize ) / oopSize;

    // allocate
    doubleValueArrayOop obj = as_doubleValueArrayOop( Universe::allocate( obj_size, (MemOop *) &k ) );

    // header
    MemOop( obj )->initialize_header( true, k );

    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );

    // indexables
    obj->set_length( size );

    for ( int i = 1; i <= size; i++ ) {
        obj->double_at_put( i, 0.0 );
    }

    return obj;
}


KlassOop DoubleValueArrayKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::doubleValueArray_klass ) {
        return DoubleValueArrayKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop DoubleValueArrayKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    DoubleValueArrayKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


void setKlassVirtualTableFromDoubleValueArrayKlass( Klass *k ) {
    DoubleValueArrayKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


bool_t DoubleValueArrayKlass::oop_verify( Oop obj ) {
    st_assert_doubleValueArray( obj, "Argument must be doubleValueArray" );
    return doubleValueArrayOop( obj )->verify();
}


void DoubleValueArrayKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    st_assert_doubleValueArray( obj, "Argument must be doubleValueArray" );
    doubleValueArrayOop array = doubleValueArrayOop( obj );
    int                 len   = array->length();
    int                 n     = min( MaxElementPrintSize, len );
    Unimplemented();
}


void DoubleValueArrayKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    double *p = doubleValueArrayOop( obj )->double_start();
    Oop    *l = doubleValueArrayOop( obj )->length_addr();
    int len = doubleValueArrayOop( obj )->length();
    MemOopKlass::oop_layout_iterate( obj, blk );
    blk->do_oop( "length", l );
    blk->begin_indexables();
    Unimplemented();
    blk->end_indexables();
}


void DoubleValueArrayKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    Oop *l = doubleValueArrayOop( obj )->length_addr();
    MemOopKlass::oop_oop_iterate( obj, blk );
    blk->do_oop( l );
}


int DoubleValueArrayKlass::oop_scavenge_contents( Oop obj ) {
    MemOopKlass::oop_scavenge_contents( obj );
    return object_size( doubleValueArrayOop( obj )->length() );
}


int DoubleValueArrayKlass::oop_scavenge_tenured_contents( Oop obj ) {
    MemOopKlass::oop_scavenge_tenured_contents( obj );
    return object_size( doubleValueArrayOop( obj )->length() );
}
