//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/oops/AssociationKlass.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"


void setKlassVirtualTableFromAssociationKlass( Klass *k ) {
    AssociationKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop AssociationKlass::allocateObject( bool permit_scavenge, bool tenured ) {
    KlassOop k    = as_klassOop();
    std::int32_t      size = non_indexable_size();

    // allocate
    Oop *result = Universe::allocate_tenured( size, permit_scavenge );
    if ( result == nullptr and not permit_scavenge )
        return nullptr;

    AssociationOop obj = as_associationOop( result );
    // header
    MemOop( obj )->initialize_header( has_untagged_contents(), k );
    obj->set_key( SymbolOop( nilObject ) );
    obj->set_value( nilObject );
    obj->set_is_constant( false );

    // instance variables
    obj->initialize_body( AssociationOopDescriptor::header_size(), size );
    return obj;
}


KlassOop AssociationKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::association_klass ) {
        return AssociationKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop AssociationKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    AssociationKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


bool AssociationKlass::oop_verify( Oop obj ) {
    bool flag = MemOop( obj )->verify();
    return flag;
}


void AssociationKlass::oop_short_print_on( Oop obj, ConsoleOutputStream *stream ) {
    AssociationOop assoc = AssociationOop( obj );
    stream->print( "{" );
    assoc->key()->print_symbol_on( stream );
    stream->print( ", " );
    assoc->value()->print_value_on( stream );
    if ( assoc->is_constant() ) {
        stream->print( " (constant)" );
    }
    stream->print( "} " );
}


void AssociationKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    AssociationOop assoc = AssociationOop( obj );
    if ( PrintObjectID ) {
        MemOop( obj )->print_id_on( stream );
        stream->print( "-" );
    }
    print_name_on( stream );
    stream->print( " {" );
    assoc->key()->print_symbol_on( stream );
    stream->print( ", " );
    assoc->value()->print_value_on( stream );
    if ( assoc->is_constant() ) {
        stream->print( " (constant)" );
    }
    stream->print( "}" );
    if ( PrintOopAddress )
        stream->print( " (0x{0:x})", this );
}


std::int32_t AssociationKlass::oop_scavenge_contents( Oop obj ) {
    std::int32_t size = non_indexable_size();
    // header + instance variables
    MemOop( obj )->scavenge_header();
    MemOop( obj )->scavenge_body( MemOopDescriptor::header_size(), size );
    return size;
}


std::int32_t AssociationKlass::oop_scavenge_tenured_contents( Oop obj ) {
    std::int32_t size = non_indexable_size();
    // header + instance variables
    MemOop( obj )->scavenge_tenured_header();
    MemOop( obj )->scavenge_tenured_body( MemOopDescriptor::header_size(), size );
    return size;
}


void AssociationKlass::oop_follow_contents( Oop obj ) {
    // header + instance variables
    MemOop( obj )->follow_header();
    MemOop( obj )->follow_body( MemOopDescriptor::header_size(), non_indexable_size() );
}


void AssociationKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    // header
    MemOop( obj )->layout_iterate_header( blk );
    AssociationOop assoc = AssociationOop( obj );
    blk->do_oop( "key", (Oop *) &assoc->addr()->_key );
    blk->do_oop( "value", (Oop *) &assoc->addr()->_value );
    blk->do_oop( "is_constant", (Oop *) &assoc->addr()->_is_constant );
    // instance variables
    MemOop( obj )->layout_iterate_body( blk, AssociationOopDescriptor::header_size(), non_indexable_size() );
}


void AssociationKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {

    // header
    MemOop( obj )->oop_iterate_header( blk );

    //
    AssociationOop assoc = AssociationOop( obj );
    blk->do_oop( (Oop *) &assoc->addr()->_key );
    blk->do_oop( (Oop *) &assoc->addr()->_value );
    blk->do_oop( (Oop *) &assoc->addr()->_is_constant );

    // instance variables
    MemOop( obj )->oop_iterate_body( blk, AssociationOopDescriptor::header_size(), non_indexable_size() );
}
