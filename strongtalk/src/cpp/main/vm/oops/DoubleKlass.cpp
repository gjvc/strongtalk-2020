//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/DoubleKlass.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/memory/Closure.hpp"


Oop DoubleKlass::allocateObject( bool_t permit_scavenge, bool_t tenured ) {
    st_assert( not can_inline_allocation(), "using nonstandard allocation" );
    // allocate
    Oop *result = basicAllocate( DoubleOopDescriptor::object_size(), &doubleKlassObject, permit_scavenge, tenured );
    if ( result == nullptr )
        return nullptr;
    DoubleOop obj = as_doubleOop( result );
    // header
    MemOop( obj )->initialize_header( false, doubleKlassObject );
    obj->set_value( 0.0 );
    return obj;
}


KlassOop DoubleKlass::create_subclass( MixinOop mixin, Format format ) {
    return nullptr;
}


void setKlassVirtualTableFromDoubleKlass( Klass *k ) {
    DoubleKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


void DoubleKlass::oop_short_print_on( Oop obj, ConsoleOutputStream *stream ) {
    st_assert_double( obj, "obj must be double" );
    stream->print( "%1.10gd ", DoubleOop( obj )->value() );
    oop_print_value_on( obj, stream );
}


void DoubleKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    st_assert_double( obj, "obj must be double" );
    stream->print( "%1.10gd", DoubleOop( obj )->value() );
}


// since klass is tenured scavenge operations are empty.
int DoubleKlass::oop_scavenge_contents( Oop obj ) {
    return DoubleOopDescriptor::object_size();
}


int DoubleKlass::oop_scavenge_tenured_contents( Oop obj ) {
    return DoubleOopDescriptor::object_size();
}


void DoubleKlass::oop_follow_contents( Oop obj ) {
    MemOop( obj )->follow_header();
}


void DoubleKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    MemOop( obj )->layout_iterate_header( blk );
    blk->do_double( "value", &DoubleOop( obj )->addr()->_value );
}


void DoubleKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    MemOop( obj )->oop_iterate_header( blk );
}
