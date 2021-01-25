//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"


// mixin objects holds the description of a class
// All classes are results of mixin invocations.

// memory layout:
//    [header            ]
//    [klass_field       ]-  (Class)
//    [methods           ]-  (Array[Method])
//    [instVarDict       ]-  (Array[Symbol])
//    [class variables   ]-  (Array[Symbol])
//    [primary invocation]-  (Class)
//    [class mixin       ]-  (Mixin)
//    [instance variables]*

class MixinOopDescriptor : public MemOopDescriptor {

private:
    ObjectArrayOop _methods;
    ObjectArrayOop _inst_vars;          // Description of instance variables
    ObjectArrayOop _class_vars;         // Description of class variables
    KlassOop       _primary_invocation; // Points to the primary invocation
    MixinOop       _class_mixin;        // Mixin for the class part
    Oop            _installed;          // Tells whether the mixin has been installed

protected:
    MixinOopDescriptor *addr() const {
        return (MixinOopDescriptor *) MemOopDescriptor::addr();
    }


public:
    friend MixinOop as_mixinOop( void *p );


    // sizing
    static std::int32_t header_size() {
        return sizeof( MixinOopDescriptor ) / oopSize;
    }


    // accessors
    ObjectArrayOop methods() const {
        return addr()->_methods;
    }


    void set_methods( ObjectArrayOop m ) {
        STORE_OOP( &addr()->_methods, m );
    }


    ObjectArrayOop instVars() const {
        return addr()->_inst_vars;
    }


    void set_instVars( ObjectArrayOop i ) {
        STORE_OOP( &addr()->_inst_vars, i );
    }


    ObjectArrayOop classVars() const {
        return addr()->_class_vars;
    }


    void set_classVars( ObjectArrayOop c ) {
        STORE_OOP( &addr()->_class_vars, c );
    }


    KlassOop primary_invocation() const {
        return addr()->_primary_invocation;
    }


    void set_primary_invocation( KlassOop k ) {
        STORE_OOP( &addr()->_primary_invocation, k );
    }


    MixinOop class_mixin() const {
        return addr()->_class_mixin;
    }


    void set_class_mixin( MixinOop m ) {
        STORE_OOP( &addr()->_class_mixin, m );
    }


    Oop installed() const {
        return addr()->_installed;
    }


    void set_installed( Oop b ) {
        STORE_OOP( &addr()->_installed, b );
    }


    // primitive operations
    std::int32_t number_of_methods() const;       // Return the number of methods.
    MethodOop method_at( std::int32_t index ) const;       // Return the method at index.
    void add_method( MethodOop method );      // Add/overwrite method.
    MethodOop remove_method_at( std::int32_t index );       // Remove and return the method at index.
    bool_t includes_method( MethodOop method ); // Remove and return the class variable at index.

    std::int32_t number_of_instVars() const;       // Return the number of instance variables.
    SymbolOop instVar_at( std::int32_t index ) const;       // Return the instance variable at index.
    void add_instVar( SymbolOop name );       // Add instance variable.
    SymbolOop remove_instVar_at( std::int32_t index );      // Remove and return the instance variable at index.
    bool_t includes_instVar( SymbolOop name );  // Tells whether the name is present as an instance variable name.

    std::int32_t number_of_classVars() const;      // Return the number of class variables.
    SymbolOop classVar_at( std::int32_t index ) const;      // Return the class variable at index.
    void add_classVar( SymbolOop name );      // Add class variable.
    SymbolOop remove_classVar_at( std::int32_t index );     // Remove and return the class variable at index.
    bool_t includes_classVar( SymbolOop name ); // Tells whether the name is present

    // Returns the offset of an instance variable.
    // -1 is returned if inst var is not present in mixin.
    std::int32_t inst_var_offset( SymbolOop name, std::int32_t non_indexable_size ) const;

    // Reflective operation
    void apply_mixin( MixinOop m );

    void customize_for( KlassOop klass );

    void uncustomize_methods();

    // Tells whether the mixin has been installed
    bool_t is_installed() const;

    // Tells whether the mixin has been installed
    bool_t has_primary_invocation() const;

    // bootstrappingInProgress
    void bootstrap_object( Bootstrap *stream );

    friend class MixinKlass;
};


inline MixinOop as_mixinOop( void *p ) {
    return MixinOop( as_memOop( p ) );
}
