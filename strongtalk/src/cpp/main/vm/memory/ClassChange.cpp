
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/ClassChange.hpp"
#include "vm/memory/Converter.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/oop/MixinOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"


void ClassChange::recustomize_methods() {
    new_mixin()->customize_for( new_klass() );
    new_mixin()->class_mixin()->customize_for( new_klass()->klass() );
}


struct KlassOopDescriptor *ClassChange::new_class_from( KlassOopDescriptor *old_klass, KlassOopDescriptor *new_super_klass, struct MixinOopDescriptor *new_mixin, Klass::Format new_format, struct MixinOopDescriptor *old_mixin ) {
    static_cast<void>(old_mixin); // unused

    Klass::Format format = ( new_format not_eq Klass::Format::special_klass ) ? new_format : old_klass->klass_part()->format();

    KlassOopDescriptor *result = new_super_klass->klass_part()->create_subclass( new_mixin, format );
    if ( result == nullptr ) {
        st_fatal( "class creation failed - internal error" );
    }

    // %cleanup code
    _new_klass = result;

    if ( old_klass->klass_part()->mixin()->primary_invocation() == old_klass ) {
        recustomize_methods();
        new_mixin->set_primary_invocation( result );
        new_mixin->class_mixin()->set_primary_invocation( result->klass() );
    }

    new_mixin->set_installed( trueObject );
    new_mixin->class_mixin()->set_installed( trueObject );

    transfer_misc( old_klass, result );
    transfer_misc( old_klass->klass(), result->klass() );

    // Copy the class variables
    for ( std::size_t i = old_klass->klass_part()->number_of_classVars(); i > 0; i-- ) {
        struct AssociationOopDescriptor *old_assoc = old_klass->klass_part()->classVar_at( i );
        struct AssociationOopDescriptor *new_assoc = result->klass_part()->local_lookup_class_var( old_assoc->key() );
        if ( new_assoc ) {
            new_assoc->set_value( old_assoc->value() );
        }
    }
    return result;
}


void ClassChange::transfer_misc( struct MemOopDescriptor *src, struct MemOopDescriptor *dst ) {
    memConverter *c = new memConverter( src->klass(), dst->klass() );
    c->transfer( src, dst );
}


memConverter *ClassChange::create_converter_for( KlassOopDescriptor *old_class, KlassOopDescriptor *new_class ) {
    switch ( new_class->klass_part()->format() ) {
        case Klass::Format::mem_klass:
            return new memConverter( old_class, new_class );
        case Klass::Format::byte_array_klass:
            return new byteArrayConverter( old_class, new_class );
        case Klass::Format::double_byte_array_klass:
            return new doubleByteArrayConverter( old_class, new_class );
        case Klass::Format::double_value_array_klass:
            return new doubleValueArrayConverter( old_class, new_class );
        case Klass::Format::klass_klass:
            return new klassConverter( old_class, new_class );
        case Klass::Format::mixin_klass:
            return new mixinConverter( old_class, new_class );
        case Klass::Format::object_array_klass:
            return new objectArrayConverter( old_class, new_class );
        case Klass::Format::weak_array_klass:
            return new objectArrayConverter( old_class, new_class );
        case Klass::Format::process_klass:
            return new processConverter( old_class, new_class );
        case Klass::Format::proxy_klass:
            return new proxyConverter( old_class, new_class );
        default:
            return nullptr;
    }
    st_fatal( "cannot create converter for type" );
    return nullptr;
}


void ClassChange::update_class_vars() {
    if ( TraceApplyChange ) {
        SPDLOG_INFO( " - updating class variables for:" );
        old_klass()->print_value();
        _console->cr();
    }

    Klass *k = old_klass()->klass_part();

    // Remove the dead entries
    for ( std::size_t i = k->number_of_classVars(); i > 0; i-- ) {
        struct AssociationOopDescriptor *assoc = k->classVar_at( i );
        if ( not new_mixin()->includes_classVar( assoc->key() ) ) {
            k->remove_classVar_at( i );
        }
    }

    // Add the new ones
    for ( std::size_t i = new_mixin()->number_of_classVars(); i > 0; i-- ) {
        struct SymbolOopDescriptor *name = new_mixin()->classVar_at( i );
        if ( not k->includes_classVar( name ) ) {
            k->add_classVar( OopFactory::new_association( name, nilObject, false ) );
        }
    }

    // Make fhe Class side inherit the Instance side class variables
    old_klass()->klass()->klass_part()->set_classVars( k->classVars() );

    old_mixin()->set_classVars( new_mixin()->classVars() );
    old_mixin()->class_mixin()->set_classVars( new_mixin()->classVars() );
}


void ClassChange::update_methods( std::int32_t instance_side ) {

    if ( TraceApplyChange ) {
        SPDLOG_INFO( " updating {}-side methods for [{}] ", instance_side ? "instance" : "class", old_klass()->print_value_string() );
    }

    if ( instance_side ) {
        old_klass()->klass_part()->flush_methods();
        old_mixin()->set_methods( new_mixin()->methods() );
    } else {
        old_klass()->klass()->klass_part()->flush_methods();
        old_mixin()->class_mixin()->set_methods( new_mixin()->class_mixin()->methods() );
    }
}


void ClassChange::update_class( std::int32_t class_vars_changed, std::int32_t instance_methods_changed, std::int32_t class_methods_changed ) {
    // The format has not changed which means we can patch the existing classes and mixins
    // We only have to change classes using the old_mixin
    if ( old_mixin() == new_mixin() ) {
        return;
    }

    // This is an invocation
    if ( class_vars_changed ) {
        update_class_vars();
    }

    if ( instance_methods_changed ) {
        update_methods( true );
    }

    if ( class_methods_changed ) {
        update_methods( false );
    }

    if ( old_klass()->klass_part()->superKlass() not_eq new_super() ) {
        old_klass()->klass_part()->set_superKlass( new_super() );
        old_klass()->klass()->klass_part()->set_superKlass( new_super()->klass() );
    }
}


void ClassChange::setup_schema_change() {

    // Setup the new super class if we have super change
    if ( _super_change ) {
        _new_super = super_change()->new_klass();
        st_assert( _new_super not_eq nullptr, "super class must exist" );
    }

    // Create new class
    _new_klass = new_class_from( old_klass(), new_super(), new_mixin(), new_format(), old_mixin() );

    // Create the converter
    _converter = create_converter_for( old_klass(), new_klass() );

    // Set forward pointer in old_class
    Reflection::forward( old_klass(), new_klass() );

    // Set forward pointer in old_class class
    Reflection::forward( old_klass()->klass(), new_klass()->klass() );

    // Set forward pointer in mixin
    Reflection::forward( old_klass()->klass_part()->mixin(), new_klass()->klass_part()->mixin() );

    // Set forward pointer in mixinClass
    Reflection::forward( old_klass()->klass()->klass_part()->mixin(), new_klass()->klass()->klass_part()->mixin() );
}


std::int32_t ClassChange::compute_needed_schema_change() {

    // Be dependent on the super change
    if ( new_super()->is_klass() and not( new_super()->klass_part()->has_same_layout_as( old_klass()->klass_part()->superKlass() ) and new_super()->klass()->klass_part()->has_same_layout_as( old_klass()->klass()->klass_part()->superKlass() ) ) ) {
        set_reason_for_schema_change( "super class has changed" );
        return true;
    }

    // Has the format changed
    if ( new_format() not_eq Klass::Format::special_klass and new_format() not_eq old_klass()->klass_part()->format() ) {
        set_reason_for_schema_change( "format of class has changed" );
        return true;
    }

    // Check if we've changed the instance variables
    if ( new_mixin()->number_of_instVars() not_eq old_mixin()->number_of_instVars() ) {
        set_reason_for_schema_change( "number of instance variables have changed" );
        return true;
    }

    for ( std::size_t i = new_mixin()->number_of_instVars(); i > 0; i-- ) {
        if ( new_mixin()->instVar_at( i ) not_eq old_mixin()->instVar_at( i ) ) {
            set_reason_for_schema_change( "instance variables have changed" );
            return true;
        }
    }

    // Check if we've changed the class instance variables
    struct MixinOopDescriptor *new_class_mixin = new_mixin()->class_mixin();
    struct MixinOopDescriptor *old_class_mixin = old_mixin()->class_mixin();

    if ( new_class_mixin->number_of_instVars() not_eq old_class_mixin->number_of_instVars() ) {
        set_reason_for_schema_change( "number of class instance variables have changed" );
        return true;
    }

    for ( std::size_t i = new_class_mixin->number_of_instVars(); i > 0; i-- ) {
        if ( new_class_mixin->instVar_at( i ) not_eq old_class_mixin->instVar_at( i ) ) {
            set_reason_for_schema_change( "class instance variables have changed" );
            return true;
        }
    }

    return false;
}


ClassChange::ClassChange( struct KlassOopDescriptor *old_klass, struct MixinOopDescriptor *new_mixin, Klass::Format new_format, struct KlassOopDescriptor *new_super ) :
    _old_klass{ old_klass },
    _new_mixin{ new_mixin },
    _new_format{ new_format },
    _new_klass{ nullptr },
    _new_super{ new_super },
    _converter{ nullptr },
    _super_change{ nullptr },
    _is_schema_change_computed{ false },
    _needs_schema_change{ 0 },
    _reason_for_schema_change{ "" } {
}


ClassChange::ClassChange( struct KlassOopDescriptor *old_klass, Klass::Format new_format ) :
    _old_klass{ old_klass },
    _new_mixin{ old_klass->klass_part()->mixin() },
    _new_format{ new_format },
    _new_klass{ nullptr },
    _new_super{ nullptr },
    _converter{ nullptr },
    _super_change{ nullptr },
    _is_schema_change_computed{ false },
    _needs_schema_change{ false },
    _reason_for_schema_change{ "" } {
}


struct KlassOopDescriptor *ClassChange::old_klass() const {
    return _old_klass;
}


struct MixinOopDescriptor *ClassChange::new_mixin() const {
    return _new_mixin;
}


Klass::Format ClassChange::new_format() const {
    return _new_format;
}


struct KlassOopDescriptor *ClassChange::new_klass() const {
    return _new_klass;
}


struct KlassOopDescriptor *ClassChange::new_super() const {
    return _new_super;
}


memConverter *ClassChange::converter() const {
    return _converter;
}


ClassChange *ClassChange::super_change() const {
    return _super_change;
}


struct MixinOopDescriptor *ClassChange::old_mixin() const {
    return _old_klass->klass_part()->mixin();
}


const char *ClassChange::reason_for_schema_change() {
    return _reason_for_schema_change;
}


void ClassChange::set_reason_for_schema_change( const char *msg ) {
    _reason_for_schema_change = msg;
}


void ClassChange::set_super_change( ClassChange *change ) {
    _super_change = change;
}


std::int32_t ClassChange::needs_schema_change() {

    if ( not _is_schema_change_computed )
        _needs_schema_change = compute_needed_schema_change();

    return _needs_schema_change;
}
