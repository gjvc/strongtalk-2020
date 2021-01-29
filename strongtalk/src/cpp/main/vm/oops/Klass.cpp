//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/Klass.hpp"
#include "vm/oops/KlassKlass.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/system/asserts.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"

void Klass::initialize() {
    set_untagged_contents( false );
    set_classVars( ObjectArrayOop( oopFactory::new_objArray( std::int32_t{0} ) ) );
    set_methods( ObjectArrayOop( oopFactory::new_objArray( std::int32_t{0} ) ) );
    set_superKlass( KlassOop( nilObject ) );
    set_mixin( MixinOop( nilObject ) );
}


Oop Klass::allocateObject( bool permit_scavenge, bool tenured ) {
    return markSymbol( vmSymbols::not_oops() );
}


Oop Klass::allocateObjectSize( std::int32_t size, bool permit_scavenge, bool permit_tenured ) {
    return markSymbol( vmSymbols::not_oops() );
}


void Klass::mark_for_schema_change() {
    set_mixin( nullptr );
}


bool Klass::is_marked_for_schema_change() {
    return mixin() == nullptr;
}


Klass::Format Klass::format_from_symbol( SymbolOop format ) {
    if ( format->equals( "Oops" ) )
        return Klass::Format::mem_klass;
    if ( format->equals( "ExternalProxy" ) )
        return Klass::Format::proxy_klass;
//     if (format->equals("Process"))
//        return Klass::Format::process_klass;
    if ( format->equals( "IndexedInstanceVariables" ) )
        return Klass::Format::objArray_klass;
    if ( format->equals( "IndexedByteInstanceVariables" ) )
        return Klass::Format::byteArray_klass;
    if ( format->equals( "IndexedDoubleByteInstanceVariables" ) )
        return Klass::Format::doubleByteArray_klass;
    if ( format->equals( "IndexedNextOfKinInstanceVariables" ) )
        return Klass::Format::weakArray_klass;
    if ( format->equals( "Special" ) )
        return Klass::Format::special_klass;
    return Klass::Format::no_klass;
}


const char *Klass::name_from_format( Format format ) {
    switch ( format ) {
        case Format::mem_klass:
            return "Oops";
        case Format::proxy_klass:
            return "ExternalProxy";
            // case Format::process_klass:
            // return "Process";
        case Format::objArray_klass:
            return "IndexedInstanceVariables";
        case Format::byteArray_klass:
            return "IndexedByteInstanceVariables";
        case Format::doubleByteArray_klass:
            return "IndexedDoubleByteInstanceVariables";
        case Format::weakArray_klass:
            return "IndexedNextOfKinInstanceVariables";
    }
    return "Special";
}


bool Klass::has_same_layout_as( KlassOop klass ) {

    st_assert( klass->is_klass(), "argument must be klass" );

    // Check equality
    if ( as_klassOop() == klass )
        return true;

    // Check format
    if ( format() not_eq klass->klass_part()->format() )
        return false;

    // Check instance size
    if ( non_indexable_size() not_eq klass->klass_part()->non_indexable_size() )
        return false;

    // Check instance variables
    for ( std::int32_t i = oop_header_size(); i < non_indexable_size(); i++ ) {
        if ( inst_var_name_at( i ) not_eq klass->klass_part()->inst_var_name_at( i ) )
            return false;
    }

    return true;
}


bool Klass::has_same_inst_vars_as( KlassOop klass ) {

    st_assert( klass->is_klass(), "argument must be klass" );

    // Check equality
    if ( as_klassOop() == klass )
        return true;

    Klass *classPart = klass->klass_part();
    // Check instance size
    std::int32_t ivars      = number_of_instance_variables();
    std::int32_t classIvars = classPart->number_of_instance_variables();
    if ( ivars not_eq classIvars )
        return false;

    // Check instance variables
    for ( std::int32_t offset = 1; offset <= number_of_instance_variables(); offset++ ) {
        if ( inst_var_name_at( non_indexable_size() - offset ) not_eq classPart->inst_var_name_at( classPart->non_indexable_size() - offset ) )
            return false;
    }

    return true;
}


KlassOop Klass::create_subclass( MixinOop mixin, Format format ) {
    ShouldNotCallThis();
    return nullptr;
}


KlassOop Klass::create_subclass( MixinOop mixin, KlassOop instSuper, KlassOop metaClass, Format format ) {
    ShouldNotCallThis();
    return nullptr;
}


KlassOop Klass::create_generic_class( KlassOop superMetaClass, KlassOop superClass, KlassOop metaMetaClass, MixinOop mixin, std::int32_t vtbl ) {

    BlockScavenge bs;

    st_assert( mixin->classVars()->is_objArray(), "checking instance side class var names" );
    st_assert( mixin->class_mixin()->classVars()->is_objArray(), "checking class side class var names" );
    st_assert( mixin->class_mixin()->classVars()->length() == 0, "checking class side class var names" );

    ObjectArrayOop class_vars = oopFactory::new_objArray( mixin->number_of_classVars() );
    std::int32_t            length     = mixin->number_of_classVars();

    for ( std::int32_t index = 1; index <= length; index++ ) {
        AssociationOop assoc = oopFactory::new_association( mixin->classVar_at( index ), nilObject, false );
        class_vars->obj_at_put( index, assoc );
    }

    // Meta class
    KlassOop meta_klass = KlassOop( metaMetaClass->klass_part()->allocateObject() );
    Klass *mk = meta_klass->klass_part();
    mk->set_untagged_contents( false );
    mk->set_classVars( class_vars );
    mk->set_methods( oopFactory::new_objArray( std::int32_t{0} ) );
    mk->set_superKlass( superMetaClass );
    mk->set_mixin( mixin->class_mixin() );
    mk->set_non_indexable_size( KlassOopDescriptor::header_size() + mk->number_of_instance_variables() );
    setKlassVirtualTableFromKlassKlass( mk );

    KlassOop klass = KlassOop( mk->allocateObject() );
    Klass *k = klass->klass_part();

    k->set_untagged_contents( false );
    k->set_classVars( class_vars );
    k->set_methods( oopFactory::new_objArray( std::int32_t{0} ) );
    k->set_superKlass( superClass );
    k->set_mixin( mixin );
    k->set_vtbl_value( vtbl );
    k->set_non_indexable_size( k->oop_header_size() + k->number_of_instance_variables() );

    return klass;
}


KlassOop Klass::create_generic_class( KlassOop super_class, MixinOop mixin, std::int32_t vtbl ) {
    return create_generic_class( super_class->klass(), super_class, super_class->klass()->klass(), mixin, vtbl );
}


SymbolOop Klass::inst_var_name_at( std::int32_t offset ) const {
    Klass *current_klass = (Klass *) this;
    std::int32_t current_offset = non_indexable_size();
    do {
        MixinOop  m = current_klass->mixin();
        for ( std::int32_t i = m->number_of_instVars(); i > 0; i-- ) {
            current_offset--;
            if ( offset == current_offset )
                return m->instVar_at( i );
        }
    } while ( current_klass = ( current_klass->superKlass() == nilObject ? nullptr : current_klass->superKlass()->klass_part() ) );

    return nullptr;
}


std::int32_t Klass::lookup_inst_var( SymbolOop name ) const {
    std::int32_t offset = mixin()->inst_var_offset( name, non_indexable_size() );
    if ( offset >= 0 )
        return offset;
    return has_superKlass() ? superKlass()->klass_part()->lookup_inst_var( name ) : -1;
}


std::int32_t Klass::number_of_instance_variables() const {
    return mixin()->number_of_instVars() + ( has_superKlass() ? superKlass()->klass_part()->number_of_instance_variables() : 0 );
}

// OPERATION FOR METHODS

std::int32_t Klass::number_of_methods() const {
    return methods()->length();
}


MethodOop Klass::method_at( std::int32_t index ) const {
    return MethodOop( methods()->obj_at( index ) );
}


void Klass::add_method( MethodOop method ) {
    ObjectArrayOop old_array = methods();
    SymbolOop      selector  = method->selector();

    // Find out if a method with the same selector exists.
    for ( std::int32_t index = 1; index <= old_array->length(); index++ ) {
        st_assert( old_array->obj_at( index )->is_method(), "must be method" );
        MethodOop m = MethodOop( old_array->obj_at( index ) );
        if ( m->selector() == selector ) {
            old_array->obj_at_put( index, method );
            return;
        }
    }

    // Extend the array
    set_methods( methods()->copy_add( method ) );
}


MethodOop Klass::remove_method_at( std::int32_t index ) {
    MethodOop method = method_at( index );
    set_methods( methods()->copy_remove( index ) );
    return method;
}


std::int32_t Klass::number_of_classVars() const {
    return classVars()->length();
}


AssociationOop Klass::classVar_at( std::int32_t index ) const {
    return AssociationOop( classVars()->obj_at( index ) );
}


void Klass::add_classVar( AssociationOop assoc ) {
    ObjectArrayOop array = classVars();

    // Find out if it already exists.
    for ( std::int32_t index = 1; index <= array->length(); index++ ) {
        st_assert( array->obj_at( index )->is_association(), "must be symbol" );
        AssociationOop elem = AssociationOop( array->obj_at( index ) );
        if ( elem->key() == assoc->key() )
            return;
    }

    // Extend the array
    set_classVars( array->copy_add( assoc ) );
}


AssociationOop Klass::remove_classVar_at( std::int32_t index ) {
    AssociationOop assoc = classVar_at( index );
    set_classVars( classVars()->copy_remove( index ) );
    return assoc;
}


bool Klass::includes_classVar( SymbolOop name ) {
    ObjectArrayOop array = classVars();
    for ( std::int32_t      index = 1; index <= array->length(); index++ ) {
        AssociationOop elem = AssociationOop( array->obj_at( index ) );
        if ( elem->key() == name )
            return true;
    }
    return false;
}


AssociationOop Klass::local_lookup_class_var( SymbolOop name ) {
    ObjectArrayOop array = classVars();
    for ( std::int32_t      index = 1; index <= array->length(); index++ ) {
        st_assert( array->obj_at( index )->is_association(), "must be symbol" );
        AssociationOop elem = AssociationOop( array->obj_at( index ) );
        if ( elem->key() == name )
            return elem;
    }
    return nullptr;
}


AssociationOop Klass::lookup_class_var( SymbolOop name ) {
    AssociationOop result = local_lookup_class_var( name );
    if ( result )
        return result;
    result = ( superKlass() == nilObject ) ? nullptr : superKlass()->klass_part()->lookup_class_var( name );
    return result;
}


MethodOop Klass::local_lookup( SymbolOop selector ) {
    ObjectArrayOop array;
    std::int32_t            length;
    Oop *current;

    // Find out if there is a customized method matching the selector.
    array  = methods();
    length = array->length();
    if ( length > 0 ) {
        current = array->objs( 1 );
        do {
            MethodOop method = MethodOop( *current++ );
            st_assert( method->is_method(), "must be method" );
            if ( method->selector() == selector )
                return method;
        } while ( --length > 0 );
    }

    st_assert( mixin()->is_mixin(), "mixin must exist" );

    array   = mixin()->methods();
    length  = array->length();
    current = array->objs( 1 );

    while ( length-- > 0 ) {
        MethodOop method = MethodOop( *current++ );
        st_assert( method->is_method(), "must be method" );
        if ( method->selector() == selector ) {
            if ( method->should_be_customized() ) {
                if ( as_klassOop() == mixin()->primary_invocation() ) {
                    if ( not method->is_customized() )
                        method->customize_for( as_klassOop(), mixin() );
                    return method;
                } else {
                    BlockScavenge bs;
                    // Make customized version for klass
                    MethodOop     new_method = method->copy_for_customization();
                    new_method->customize_for( as_klassOop(), mixin() );
                    add_method( new_method );
                    return new_method;
                }
            }
            return method;
        }
    }
    return nullptr;
}


MethodOop Klass::lookup( SymbolOop selector ) {
    Klass *current = this;
    while ( true ) {
        MethodOop result = current->local_lookup( selector );
        if ( result )
            return result;
        if ( current->superKlass() == nilObject ) {
            ResourceMark         rm;
            MissingMethodBuilder builder( selector );
            builder.build();
            MethodOop method = builder.method();
            current->add_method( method );
            return method;
        }
        current = current->superKlass()->klass_part();
    }
    return nullptr;
}


bool Klass::is_method_holder_for( MethodOop method ) {

    // Always use the home  of the method in case of a blockMethod
    MethodOop m = method->home();

    ObjectArrayOop array = methods();
    // Find out if a method with the same selector exists.
    for ( std::int32_t      index = 1; index <= array->length(); index++ ) {
        st_assert( array->obj_at( index )->is_method(), "must be method" );
        if ( MethodOop( array->obj_at( index ) ) == m )
            return true;
    }

    // Try the mixin
    if ( Oop( mixin() ) not_eq nilObject ) {
        st_assert( mixin()->is_mixin(), "must be mixin" );
        array = mixin()->methods();

        for ( std::int32_t i = 1; i <= array->length(); i++ ) {
            st_assert( array->obj_at( i )->is_method(), "must be method" );
            if ( MethodOop( array->obj_at( i ) ) == m )
                return true;
        }
    }

    return false;
}


KlassOop Klass::lookup_method_holder_for( MethodOop method ) {
    if ( is_method_holder_for( method ) )
        return as_klassOop();
    return ( superKlass() == nilObject ) ? nullptr : superKlass()->klass_part()->lookup_method_holder_for( method );
}


void Klass::flush_methods() {
    set_methods( oopFactory::new_objArray( std::int32_t{0} ) );
}


bool Klass::is_specialized_class() const {
    MemOopKlass m;
    return name() == m.name();
}


bool Klass::is_named_class() const {
    return mixin()->primary_invocation() == as_klassOop();
}


void Klass::print_klass() {
    spdlog::info( "%sKlass (%s)", name(), name_from_format( format() ) );
}


char *Klass::delta_name() {
    bool    meta   = false;
    std::int32_t       offset = as_klassOop()->blueprint()->lookup_inst_var( oopFactory::new_symbol( "name" ) );
    SymbolOop name   = nullptr;

    if ( offset >= 0 ) {
        name     = SymbolOop( as_klassOop()->raw_at( offset ) );
        if ( not name->is_symbol() )
            name = nullptr;
    }

    if ( name == nullptr ) {
        name = Universe::find_global_key_for( as_klassOop(), &meta );
        if ( not name )
            return nullptr;
    }

    std::int32_t length = name->length() + ( meta ? 7 : 1 );
    char *toReturn = new_resource_array<char>( length );
    strncpy( toReturn, name->chars(), name->length() );

    if ( meta )
        strcpy( toReturn + name->length(), " class" );
    toReturn[ length - 1 ] = '\0';

    return toReturn;
}


void Klass::print_name_on( ConsoleOutputStream *stream ) {
    std::int32_t       offset = as_klassOop()->blueprint()->lookup_inst_var( oopFactory::new_symbol( "name" ) );
    SymbolOop name   = nullptr;

    if ( offset >= 0 ) {
        name     = SymbolOop( as_klassOop()->raw_at( offset ) );
        if ( not name->is_symbol() )
            name = nullptr;
    }

    if ( name not_eq nullptr )
        name->print_symbol_on( stream );
    else {
        bool meta;
        name = Universe::find_global_key_for( as_klassOop(), &meta );
        if ( name ) {
            name->print_symbol_on( stream );
            if ( meta )
                stream->print( " class" );
        } else {
            stream->print( "??" );
        }
    }
}


std::int32_t Klass::oop_scavenge_contents( Oop obj ) {
    st_fatal( "should not call Klass::oop_scavenge_contents" );
    return 0;
}


std::int32_t Klass::oop_scavenge_tenured_contents( Oop obj ) {
    st_fatal( "should not call Klass::oop_scavenge_promotion" );
    return 0;
}


void Klass::oop_follow_contents( Oop obj ) {
    st_fatal( "should not call Klass::oop_follow_contents" );
}


void Klass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    st_fatal( "should not call layout_iterate on Klass" );
}


void Klass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    st_fatal( "should not call oop_iterate on Klass" );
}


void Klass::oop_print_on( Oop obj, ConsoleOutputStream *stream ) {
    st_fatal( "should not call Klass::oop_print_on" );
}


void Klass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    oop_print_on( obj, stream );
}


void Klass::oop_short_print_on( Oop obj, ConsoleOutputStream *stream ) {
    if ( obj == trueObject ) {
        stream->print( "true" );
    } else if ( obj == falseObject ) {
        stream->print( "false" );
    } else if ( obj == nilObject ) {
        stream->print( "nil" );
    } else {
        oop_print_value_on( obj, stream );
    }
}


bool Klass::oop_verify( Oop obj ) {
    // FIX LATER
    return true;
}


void Klass::bootstrap_klass_part_one( Bootstrap *stream ) {
    stream->read_oop( reinterpret_cast<Oop *>(&_non_indexable_size) );
    stream->read_oop( reinterpret_cast<Oop *>(&_has_untagged_contents) );
}


void Klass::bootstrap_klass_part_two( Bootstrap *stream ) {
    stream->read_oop( reinterpret_cast<Oop *>(&_classVars) );
    stream->read_oop( reinterpret_cast<Oop *>(&_methods) );
    stream->read_oop( reinterpret_cast<Oop *>(&_superKlass) );
    stream->read_oop( reinterpret_cast<Oop *>(&_mixin) );
}


Oop Klass::oop_primitive_allocate( Oop obj, bool allow_scavenge, bool tenured ) {
    return markSymbol( vmSymbols::not_klass() );
}


Oop Klass::oop_primitive_allocate_size( Oop obj, std::int32_t size ) {
    return markSymbol( vmSymbols::not_klass() );
}


Oop Klass::oop_shallow_copy( Oop obj, bool tenured ) {
    return markSymbol( vmSymbols::not_oops() );
}
