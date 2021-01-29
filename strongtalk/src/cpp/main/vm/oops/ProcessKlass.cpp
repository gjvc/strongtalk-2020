//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ProcessKlass.hpp"


void setKlassVirtualTableFromProcessKlass( Klass *k ) {
    ProcessKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop ProcessKlass::allocateObject( bool permit_scavenge, bool tenured ) {
    KlassOop     k    = as_klassOop();
    std::int32_t size = non_indexable_size();

    // allocate
    Oop *result = basicAllocate( size, &k, permit_scavenge, tenured );
    if ( not result )
        return nullptr;
    ProcessOop obj = as_processOop( result );

    // header
    MemOop( obj )->initialize_header( true, k );
    obj->set_process( nullptr );

    // instance variables
    MemOop( obj )->initialize_body( ProcessOopDescriptor::header_size(), size );
    return obj;
}


KlassOop ProcessKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::process_klass ) {
        return ProcessKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop ProcessKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    ProcessKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


std::int32_t ProcessKlass::oop_scavenge_contents( Oop obj ) {
    std::int32_t size = non_indexable_size();
    // header
    MemOop( obj )->scavenge_header();
    // instance variables
    MemOop( obj )->scavenge_body( ProcessOopDescriptor::header_size(), size );
    return size;
}


std::int32_t ProcessKlass::oop_scavenge_tenured_contents( Oop obj ) {
    std::int32_t size = non_indexable_size();
    // header
    MemOop( obj )->scavenge_tenured_header();
    // instance variables
    MemOop( obj )->scavenge_tenured_body( ProcessOopDescriptor::header_size(), size );
    return size;
}


void ProcessKlass::oop_follow_contents( Oop obj ) {
    // header
    MemOop( obj )->follow_header();
    // instance variables
    MemOop( obj )->follow_body( ProcessOopDescriptor::header_size(), non_indexable_size() );
}


void ProcessKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    // header
    MemOop( obj )->layout_iterate_header( blk );
    blk->do_long( "process", (void **) &ProcessOop( obj )->addr()->_process );
    // instance variables
    MemOop( obj )->layout_iterate_body( blk, ProcessOopDescriptor::header_size(), non_indexable_size() );
}


void ProcessKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    // header
    MemOop( obj )->oop_iterate_header( blk );
    // instance variables
    MemOop( obj )->oop_iterate_body( blk, ProcessOopDescriptor::header_size(), non_indexable_size() );
}
