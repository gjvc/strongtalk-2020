//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/KlassKlass.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/utilities/objectIDTable.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/vmSymbols.hpp"


void setKlassVirtualTableFromKlassKlass( Klass *k ) {
    KlassKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop KlassKlass::allocateObject( bool_t permit_scavenge, bool_t tenured ) {
    // allocate
    MemOop obj = as_memOop( Universe::allocate_tenured( non_indexable_size() ) );
    // header + instance variables
    obj->initialize_header( has_untagged_contents(), as_klassOop() );
    obj->initialize_body( KlassOopDescriptor::header_size(), non_indexable_size() );
    return obj;
}


KlassOop KlassKlass::create_subclass( MixinOop mixin, Format format ) {
    KlassKlass o;
    return create_generic_class( as_klassOop(), mixin, o.vtbl_value() );
}


int KlassKlass::oop_scavenge_contents( Oop obj ) {
    int size = non_indexable_size();

    // header
    MemOop( obj )->scavenge_header();
    Klass *k = KlassOop( obj )->klass_part();
    scavenge_oop( (Oop *) &k->_non_indexable_size );
    scavenge_oop( (Oop *) &k->_has_untagged_contents );
    scavenge_oop( (Oop *) &k->_classVars );
    scavenge_oop( (Oop *) &k->_methods );
    scavenge_oop( (Oop *) &k->_superKlass );
    scavenge_oop( (Oop *) &k->_mixin );

    // instance variables
    MemOop( obj )->scavenge_body( KlassOopDescriptor::header_size(), size );
    return size;
}


int KlassKlass::oop_scavenge_tenured_contents( Oop obj ) {
    int size = non_indexable_size();

    // header
    MemOop( obj )->scavenge_tenured_header();
    Klass *k = KlassOop( obj )->klass_part();
    scavenge_tenured_oop( (Oop *) &k->_non_indexable_size );
    scavenge_tenured_oop( (Oop *) &k->_has_untagged_contents );
    scavenge_tenured_oop( (Oop *) &k->_classVars );
    scavenge_tenured_oop( (Oop *) &k->_methods );
    scavenge_tenured_oop( (Oop *) &k->_superKlass );
    scavenge_tenured_oop( (Oop *) &k->_mixin );

    // instance variables
    MemOop( obj )->scavenge_tenured_body( KlassOopDescriptor::header_size(), size );
    return size;
}


void KlassKlass::oop_follow_contents( Oop obj ) {

    // header
    MemOop( obj )->follow_header();
    Klass *k = KlassOop( obj )->klass_part();
    MarkSweep::reverse_and_push( (Oop *) &k->_non_indexable_size );
    MarkSweep::reverse_and_push( (Oop *) &k->_has_untagged_contents );
    MarkSweep::reverse_and_push( (Oop *) &k->_classVars );
    MarkSweep::reverse_and_push( (Oop *) &k->_methods );
    MarkSweep::reverse_and_push( (Oop *) &k->_superKlass );
    MarkSweep::reverse_and_push( (Oop *) &k->_mixin );

    // instance variables
    MemOop( obj )->follow_body( KlassOopDescriptor::header_size(), non_indexable_size() );
}


bool_t KlassKlass::oop_verify( Oop obj ) {
    st_assert( obj->is_klass(), "must be class" );
    Klass *k = KlassOop( obj )->klass_part();

//    if ( not k->oop_is_smi() ) {
//        int a = k->non_indexable_size();
//        int b = k->oop_header_size();
//        int c = k->number_of_instance_variables();
//        if ( a not_eq b + c ) {
//            warning( "inconsistent non indexable size" );
//        }
//    }

    bool_t flag = MemOop( obj )->verify();
    return flag;
}


void KlassKlass::oop_print_on( Oop obj, ConsoleOutputStream *stream ) {
    MemOopKlass::oop_print_on( obj, stream );
}


void KlassKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    if ( PrintObjectID ) {
        MemOop( obj )->print_id_on( stream );
        stream->print( "-" );
    }
    KlassOop( obj )->klass_part()->print_name_on( stream );
    stream->print( " class" );
}


void KlassKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {

    // header
    MemOop( obj )->layout_iterate_header( blk );
    Klass *k = KlassOop( obj )->klass_part();
    blk->do_oop( "size", (Oop *) &k->_non_indexable_size );
    blk->do_oop( "untag", (Oop *) &k->_has_untagged_contents );
    blk->do_oop( "classVars", (Oop *) &k->_classVars );
    blk->do_oop( "methods", (Oop *) &k->_methods );
    blk->do_oop( "super", (Oop *) &k->_superKlass );
    blk->do_oop( "mixin", (Oop *) &k->_mixin );

    // instance variables
    MemOop( obj )->layout_iterate_body( blk, KlassOopDescriptor::header_size(), non_indexable_size() );
}


void KlassKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {

    // header
    MemOop( obj )->oop_iterate_header( blk );
    Klass *k = KlassOop( obj )->klass_part();
    blk->do_oop( (Oop *) &k->_non_indexable_size );
    blk->do_oop( (Oop *) &k->_has_untagged_contents );
    blk->do_oop( (Oop *) &k->_classVars );
    blk->do_oop( (Oop *) &k->_methods );
    blk->do_oop( (Oop *) &k->_superKlass );
    blk->do_oop( (Oop *) &k->_mixin );

    // instance variables
    MemOop( obj )->oop_iterate_body( blk, KlassOopDescriptor::header_size(), non_indexable_size() );
}


Oop KlassKlass::oop_primitive_allocate( Oop obj, bool_t allow_scavenge, bool_t tenured ) {
    return KlassOop( obj )->klass_part()->allocateObject( allow_scavenge, tenured );
}


Oop KlassKlass::oop_primitive_allocate_size( Oop obj, int size ) {
    return KlassOop( obj )->klass_part()->allocateObjectSize( size );
}


Oop KlassKlass::oop_shallow_copy( Oop obj, bool_t tenured ) {
    return markSymbol( vmSymbols::not_clonable() );
}
