
//
//
//
//


#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/BlockClosureKlass.hpp"
#include "ContextKlass.hpp"


void setKlassVirtualTableFromContextKlass( Klass *k ) {
    ContextKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop ContextKlass::allocateObjectSize( std::int32_t num_of_temps, bool permit_scavenge, bool tenured ) {
    KlassOop     k        = as_klassOop();
    std::int32_t obj_size = ContextOopDescriptor::header_size() + num_of_temps;
    // allocate
    ContextOop   obj      = as_contextOop( Universe::allocate( obj_size, (MemOop *) &k ) );
    // header
    obj->set_klass_field( k );
    //%clean the up later
    //  hash value must by convention be different from 0 (check markOop.hpp)
    obj->set_mark( MarkOopDescriptor::tagged_prototype()->set_hash( num_of_temps + 1 ) );
    // indexables
    MemOop( obj )->initialize_body( ContextOopDescriptor::header_size(), obj_size );
    return obj;
}


KlassOop ContextKlass::create_subclass( MixinOop mixin, Format format ) {
    return nullptr;
}


ContextOop ContextKlass::allocate_context( std::int32_t num_of_temps ) {
    ContextKlass *ck = (ContextKlass *) contextKlassObject->klass_part();
    return ContextOop( ck->allocateObjectSize( num_of_temps ) );
}


std::int32_t ContextKlass::oop_scavenge_contents( Oop obj ) {
    std::int32_t size = ContextOop( obj )->object_size();
    // header
    MemOop( obj )->scavenge_header();
    scavenge_oop( (Oop *) &ContextOop( obj )->addr()->_parent );
    // temporaries
    MemOop( obj )->scavenge_body( ContextOopDescriptor::header_size(), size );
    return size;
}


std::int32_t ContextKlass::oop_scavenge_tenured_contents( Oop obj ) {
    std::int32_t size = ContextOop( obj )->object_size();
    // header
    MemOop( obj )->scavenge_tenured_header();
    scavenge_tenured_oop( (Oop *) &ContextOop( obj )->addr()->_parent );
    // temporaries
    MemOop( obj )->scavenge_tenured_body( ContextOopDescriptor::header_size(), size );
    return size;
}


void ContextKlass::oop_follow_contents( Oop obj ) {
    // header
    MemOop( obj )->follow_header();
    MarkSweep::reverse_and_push( (Oop *) &ContextOop( obj )->addr()->_parent );
    // temporaries

    // we have to find the header word in order to compute object size.
    // %implementation note:
    //   implement this another way if possible
    Oop *root_or_mark = (Oop *) MemOop( obj )->mark();
    while ( not Oop( root_or_mark )->is_mark() ) {
        root_or_mark = (Oop *) *root_or_mark;
    }
    std::int32_t len = MarkOop( root_or_mark )->hash() - 1;
    MemOop( obj )->follow_body( ContextOopDescriptor::header_size(), ContextOopDescriptor::header_size() + len );
}


void ContextKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    // header
    MemOop( obj )->oop_iterate_header( blk );
    blk->do_oop( (Oop *) &ContextOop( obj )->addr()->_parent );
    // temporaries
    MemOop( obj )->oop_iterate_body( blk, ContextOopDescriptor::header_size(), oop_size( obj ) );
}


void ContextKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    // header
    MemOop( obj )->layout_iterate_header( blk );
    blk->do_oop( "home", (Oop *) &ContextOop( obj )->addr()->_parent );
    // temporaries
    MemOop( obj )->layout_iterate_body( blk, ContextOopDescriptor::header_size(), oop_size( obj ) );
}


std::int32_t ContextKlass::oop_size( Oop obj ) const {
    return ContextOop( obj )->object_size();
}


void ContextKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    MemOopKlass::oop_print_value_on( obj, stream );
    st_assert( obj->is_context(), "must be context" );
    ContextOop con = ContextOop( obj );
    stream->print( "{" );
    con->print_home_on( stream );
    stream->print( " |%d}", con->length() );
}


void ContextKlass::oop_print_on( Oop obj, ConsoleOutputStream *stream ) {
    MemOopKlass::oop_print_value_on( obj, stream );
    stream->cr();
    st_assert( obj->is_context(), "must be context" );
    ContextOop con = ContextOop( obj );
    stream->print( " home: " );
    con->print_home_on( stream );
    stream->cr();
    for ( std::int32_t i = 0; i < con->length(); i++ ) {
        stream->print( " - %d: ", i );
        con->obj_at( i )->print_value_on( stream );
        stream->cr();
    }
}
