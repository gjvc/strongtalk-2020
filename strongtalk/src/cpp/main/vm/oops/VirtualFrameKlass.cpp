//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/VirtualFrameOopDescriptor.hpp"
#include "vm/oops/VirtualFrameKlass.hpp"
#include "vm/memory/Closure.hpp"


void setKlassVirtualTableFromVirtualFrameKlass( Klass *k ) {
    VirtualFrameKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop VirtualFrameKlass::allocateObject( bool permit_scavenge, bool tenured ) {
    KlassOop     k    = as_klassOop();
    std::int32_t size = non_indexable_size();

    // allocate
    Oop *result = basicAllocate( size, &k, permit_scavenge, tenured );
    if ( not result )
        return nullptr;
    VirtualFrameOop obj = as_vframeOop( result );

    // header
    MemOop( obj )->initialize_header( true, k );

    obj->set_process( ProcessOop( nilObject ) );
    obj->set_index( 0 );
    obj->set_time_stamp( 1 );

    // instance variables
    MemOop( obj )->initialize_body( VirtualFrameOopDescriptor::header_size(), size );
    return obj;
}


KlassOop VirtualFrameKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::vframe_klass ) {
        return VirtualFrameKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop VirtualFrameKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    VirtualFrameKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


std::int32_t VirtualFrameKlass::oop_scavenge_contents( Oop obj ) {
    std::int32_t size = non_indexable_size();
    // header + instance variables
    MemOop( obj )->scavenge_header();
    MemOop( obj )->scavenge_body( MemOopDescriptor::header_size(), size );
    return size;
}


std::int32_t VirtualFrameKlass::oop_scavenge_tenured_contents( Oop obj ) {
    std::int32_t size = non_indexable_size();
    // header + instance variables
    MemOop( obj )->scavenge_tenured_header();
    MemOop( obj )->scavenge_tenured_body( MemOopDescriptor::header_size(), size );
    return size;
}


void VirtualFrameKlass::oop_follow_contents( Oop obj ) {
    // header + instance variables
    MemOop( obj )->follow_header();
    MemOop( obj )->follow_body( MemOopDescriptor::header_size(), non_indexable_size() );
}


void VirtualFrameKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    // header
    MemOop( obj )->layout_iterate_header( blk );
    VirtualFrameOop vf = VirtualFrameOop( obj );
    blk->do_oop( "process", (Oop *) &vf->addr()->_process );
    blk->do_oop( "index", (Oop *) &vf->addr()->_index );
    blk->do_oop( "time stamp", (Oop *) &vf->addr()->_time_stamp );
    MemOop( obj )->layout_iterate_body( blk, VirtualFrameOopDescriptor::header_size(), non_indexable_size() );
}


void VirtualFrameKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    // header
    MemOop( obj )->oop_iterate_header( blk );
    VirtualFrameOop vf = VirtualFrameOop( obj );
    blk->do_oop( (Oop *) &vf->addr()->_process );
    blk->do_oop( (Oop *) &vf->addr()->_index );
    blk->do_oop( (Oop *) &vf->addr()->_time_stamp );
    // instance variables
    MemOop( obj )->oop_iterate_body( blk, VirtualFrameOopDescriptor::header_size(), non_indexable_size() );
}
