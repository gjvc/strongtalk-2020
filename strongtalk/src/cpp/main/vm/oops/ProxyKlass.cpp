//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ProxyKlass.hpp"


void setKlassVirtualTableFromProxyKlass( Klass * k ) {
    ProxyKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop ProxyKlass::allocateObject( bool_t permit_scavenge, bool_t tenured ) {
    KlassOop k    = as_klassOop();
    int      size = non_indexable_size();
    // allocate
    Oop * result = basicAllocate( size, &k, permit_scavenge, tenured );
    if ( not result )
        return nullptr;
    ProxyOop obj = as_proxyOop( result );
    // header
    MemOop( obj )->initialize_header( true, k );
    obj->set_pointer( nullptr );
    // instance variables
    MemOop( obj )->initialize_body( ProxyOopDescriptor::header_size(), size );
    return obj;
}


KlassOop ProxyKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::proxy_klass ) {
        return ProxyKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop ProxyKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    ProxyKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


int ProxyKlass::oop_scavenge_contents( Oop obj ) {
    int size = non_indexable_size();
    // header
    MemOop( obj )->scavenge_header();
    // instance variables
    MemOop( obj )->scavenge_body( ProxyOopDescriptor::header_size(), size );
    return size;
}


int ProxyKlass::oop_scavenge_tenured_contents( Oop obj ) {
    int size = non_indexable_size();
    // header
    MemOop( obj )->scavenge_tenured_header();
    // instance variables
    MemOop( obj )->scavenge_tenured_body( ProxyOopDescriptor::header_size(), size );
    return size;
}


void ProxyKlass::oop_follow_contents( Oop obj ) {
    // header
    MemOop( obj )->follow_header();
    // instance variables
    MemOop( obj )->follow_body( ProxyOopDescriptor::header_size(), non_indexable_size() );
}


void ProxyKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure * blk ) {
    // header
    MemOop( obj )->layout_iterate_header( blk );
    blk->do_long( "pointer", &ProxyOop( obj )->addr()->_pointer );
    // instance variables
    MemOop( obj )->layout_iterate_body( blk, ProxyOopDescriptor::header_size(), non_indexable_size() );
}


void ProxyKlass::oop_oop_iterate( Oop obj, OopClosure * blk ) {
    // header
    MemOop( obj )->oop_iterate_header( blk );
    // instance variables
    MemOop( obj )->oop_iterate_body( blk, ProxyOopDescriptor::header_size(), non_indexable_size() );
}
