//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"


std::int32_t MixinOopDescriptor::inst_var_offset( SymbolOop name, std::int32_t non_indexable_size ) const {
    ObjectArrayOop     array  = instVars();
    std::int32_t       length = array->length();
    for ( std::int32_t index  = 1; index <= length; index++ ) {
        if ( array->obj_at( index ) == name ) {
            return non_indexable_size - ( length - index + 1 );
        }
    }
    return -1;
}


void MixinOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    MemOopDescriptor::bootstrap_header( stream );
    stream->read_oop( (Oop *) &addr()->_methods );
    stream->read_oop( (Oop *) &addr()->_inst_vars );
    stream->read_oop( (Oop *) &addr()->_class_vars );
    stream->read_oop( (Oop *) &addr()->_primary_invocation );
    stream->read_oop( (Oop *) &addr()->_class_mixin );
    stream->read_oop( (Oop *) &addr()->_installed );
    MemOopDescriptor::bootstrap_body( stream, header_size() );
}


std::int32_t MixinOopDescriptor::number_of_methods() const {
    return methods()->length();
}


MethodOop MixinOopDescriptor::method_at( std::int32_t index ) const {
    return MethodOop( methods()->obj_at( index ) );
}


void MixinOopDescriptor::add_method( MethodOop method ) {
    ObjectArrayOop     old_array = methods();
    SymbolOop          selector  = method->selector();
    // Find out if a method with the same selector exists.
    for ( std::int32_t index     = 1; index <= old_array->length(); index++ ) {
        st_assert( old_array->obj_at( index )->is_method(), "must be method" );
        MethodOop m = MethodOop( old_array->obj_at( index ) );
        if ( m->selector() == selector ) {
            ObjectArrayOop new_array = old_array->copy();
            new_array->obj_at_put( index, method );
            set_methods( new_array );
            return;
        }
    }
    // Extend the array
    set_methods( old_array->copy_add( method ) );
}


MethodOop MixinOopDescriptor::remove_method_at( std::int32_t index ) {
    MethodOop method = method_at( index );
    set_methods( methods()->copy_remove( index ) );
    return method;
}


bool MixinOopDescriptor::includes_method( MethodOop method ) {
    ObjectArrayOop     array = methods();
    for ( std::int32_t index = 1; index <= array->length(); index++ ) {
        MethodOop m = MethodOop( array->obj_at( index ) );
        if ( m == method )
            return true;
    }
    return false;
}


std::int32_t MixinOopDescriptor::number_of_instVars() const {
    return instVars()->length();
}


SymbolOop MixinOopDescriptor::instVar_at( std::int32_t index ) const {
    return SymbolOop( instVars()->obj_at( index ) );
}


void MixinOopDescriptor::add_instVar( SymbolOop name ) {
    ObjectArrayOop     old_array = instVars();
    // Find out if it already exists.
    for ( std::int32_t index     = 1; index <= old_array->length(); index++ ) {
        st_assert( old_array->obj_at( index )->isSymbol(), "must be symbol" );
        if ( old_array->obj_at( index ) == name )
            return;
    }
    // Extend the array
    set_instVars( old_array->copy_add( name ) );
}


SymbolOop MixinOopDescriptor::remove_instVar_at( std::int32_t index ) {
    SymbolOop name = instVar_at( index );
    set_instVars( instVars()->copy_remove( index, 1 ) );
    return name;
}


bool MixinOopDescriptor::includes_instVar( SymbolOop name ) {
    ObjectArrayOop     array = instVars();
    for ( std::int32_t index = 1; index <= array->length(); index++ ) {
        SymbolOop elem = SymbolOop( array->obj_at( index ) );
        if ( elem == name )
            return true;
    }
    return false;
}


std::int32_t MixinOopDescriptor::number_of_classVars() const {
    return classVars()->length();
}


SymbolOop MixinOopDescriptor::classVar_at( std::int32_t index ) const {
    return SymbolOop( classVars()->obj_at( index ) );
}


void MixinOopDescriptor::add_classVar( SymbolOop name ) {
    ObjectArrayOop     old_array = classVars();
    // Find out if it already exists.
    for ( std::int32_t index     = 1; index <= old_array->length(); index++ ) {
        SymbolOop elem = SymbolOop( old_array->obj_at( index ) );
        if ( elem == name )
            return;
    }
    // Extend the array
    set_classVars( old_array->copy_add( name ) );
}


SymbolOop MixinOopDescriptor::remove_classVar_at( std::int32_t index ) {
    SymbolOop name = classVar_at( index );
    set_classVars( classVars()->copy_remove( index ) );
    return name;
}


bool MixinOopDescriptor::includes_classVar( SymbolOop name ) {
    ObjectArrayOop     array = classVars();
    for ( std::int32_t index = 1; index <= array->length(); index++ ) {
        SymbolOop elem = SymbolOop( array->obj_at( index ) );
        if ( elem == name )
            return true;
    }
    return false;
}


bool MixinOopDescriptor::is_installed() const {
    if ( installed() == trueObject )
        return true;
    st_assert( installed() == falseObject, "verify installed" );
    return false;
}


bool MixinOopDescriptor::has_primary_invocation() const {
    return primary_invocation()->is_klass();
}


void MixinOopDescriptor::apply_mixin( MixinOop m ) {
    set_methods( m->methods() );
}


void MixinOopDescriptor::customize_for( KlassOop klass ) {
    ObjectArrayOop     array = methods();
    for ( std::int32_t index = 1; index <= array->length(); index++ ) {
        MethodOop m = MethodOop( array->obj_at( index ) );
        m->customize_for( klass, this );
    }
}


void MixinOopDescriptor::uncustomize_methods() {
    ObjectArrayOop     array = methods();
    for ( std::int32_t index = 1; index <= array->length(); index++ ) {
        MethodOop m = MethodOop( array->obj_at( index ) );
        m->uncustomize_for( this );
    }
}
