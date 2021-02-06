
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


//
// A Klass is the the part of the klassOop that provides:
//
//  1: language-level class object (method dictionary etc.)
//  2: provide vm dispatch behavior for the object
//
// Both functions are combined into one C++ class.
//     The top level class "Klass" implements number 1 above.
//     All subclasses provide extra virtual functions for number 2 above.
//
//
//
// One reason for the Oop/klass dichotomy in the implementation is that we don't want a C++ vtbl pointer in every object.
// Thus, normal oops don't have any virtual functions.
// Instead, they forward all "virtual" functions to their klass, which does have a vtbl and does the C++ dispatch depending on the object's actual type.
// (See Oop.inline.h for some of the forwarding code.)
//
// ALL FUNCTIONS IMPLEMENTING THIS DISPATCH ARE PREFIXED WITH "oop_"!
//

class Bootstrap;

// Microsoft C++ 2.0 always forces the vtbl at offset 0.
constexpr std::int32_t VTBL_OFFSET = 0;

//  Klass layout:
//    [_vtbl                 ]
//    [_non_indexable_size   ]
//    [_has_untagged_contents]  can be avoided if prototype is stored.
//    [_classVars            ]  class variables copied from mixin
//    [_methods              ]  customized methods from the mixin
//    [_superKlass           ]  super
//    [_mixin                ]  the mixin for the class

class Klass : ValueObject {

protected:
    SMIOop         _non_indexable_size;
    SMIOop         _has_untagged_contents;
    ObjectArrayOop _classVars;
    ObjectArrayOop _methods;
    KlassOop       _superKlass;
    MixinOop       _mixin;

public:
    Klass() :
        ValueObject(),
        _methods{ nullptr },
        _classVars{ nullptr },
        _has_untagged_contents{ nullptr },
        _mixin{ nullptr },
        _non_indexable_size{ nullptr },
        _superKlass{ nullptr } {

    }


    friend KlassOop as_klassOop( void *p );


    // Returns the enclosing klassOop
    KlassOop as_klassOop() const {
        // see klassOop.hpp for layout.
        return (KlassOop) ( ( (const char *) this ) - sizeof( MemOopDescriptor ) + MEMOOP_TAG );
    }


    smi_t non_indexable_size() const {
        return _non_indexable_size->value();
    }


    void set_non_indexable_size( smi_t size ) {
        _non_indexable_size = smiOopFromValue( size );
    }


    bool has_untagged_contents() const {
        return _has_untagged_contents == smiOop_one;
    }


    void set_untagged_contents( bool v ) {
        _has_untagged_contents = v ? smiOop_one : smiOop_zero;
    }


    ObjectArrayOop classVars() const {
        return _classVars;
    }


    void set_classVars( ObjectArrayOop c ) {
        STORE_OOP( &_classVars, c );
    }


    ObjectArrayOop methods() const {
        return _methods;
    }


    void set_methods( ObjectArrayOop m ) {
        STORE_OOP( &_methods, m );
    }


    KlassOop superKlass() const {
        return _superKlass;
    }


    void set_superKlass( KlassOop super ) {
        STORE_OOP( &_superKlass, super );
    }


    MixinOop mixin() const {
        return _mixin;
    }


    void set_mixin( MixinOop m ) {
        STORE_OOP( &_mixin, m );
    }


    // Tells whether here is a super class
    bool has_superKlass() const {
        return Oop( superKlass() ) not_eq nilObject;
    }


public:
    std::int32_t number_of_methods() const;                  // Returns the number of methods in this class.
    MethodOop method_at( std::int32_t index ) const;         // Returns the method at index.
    void add_method( MethodOop method );            // Adds or overwrites with method.
    MethodOop remove_method_at( std::int32_t index );        // Removes method at index and returns the removed method.

    std::int32_t number_of_classVars() const;                // Returns the number of class variables.
    AssociationOop classVar_at( std::int32_t index ) const;  // Returns the class variable at index.
    void add_classVar( AssociationOop assoc );      // Adds or overwrites class variable.
    AssociationOop remove_classVar_at( std::int32_t index ); // Removes class variable at index and returns the removed association.
    bool includes_classVar( SymbolOop name );     // Tells whether the name is present

    // virtual pointer value
    std::int32_t vtbl_value() const {
        return ( (std::int32_t *) this )[ VTBL_OFFSET ];
    }


    void set_vtbl_value( std::int32_t vtbl ) {
        st_assert( vtbl % 4 == 0, "vtbl should be aligned" ); // XXX hard-coded alignment value
        ( (std::int32_t *) this )[ VTBL_OFFSET ] = vtbl;
    }


    void bootstrap_klass_part_one( Bootstrap *bs );

    void bootstrap_klass_part_two( Bootstrap *bs );

public:
    // After reading the snapshot the klass has to be fixed e.g. vtbl initialized!
    void fixup_after_snapshot_read();  // must not be virtual; vtbl not fixed yet

    // allocation operations
    virtual bool can_inline_allocation() const {
        return false;
    }
    // If this returns true, the compiler may inline the allocation code.
    // This means that the actual definition of allocate() is ignored!!
    // Fix this compare member function pointers (9/9-1994)

    // Reflective properties
    virtual bool can_have_instance_variables() const {
        return false;
    }


    virtual bool can_be_subclassed() const {
        return false;
    }


    bool is_specialized_class() const;

    // Tells whether this is a named class
    bool is_named_class() const;


    // allocation operations
    std::int32_t size() const {
        return sizeof( Klass ) / sizeof( Oop );
    }


    virtual Oop allocateObject( bool permit_scavenge = true, bool tenured = false );

    virtual Oop allocateObjectSize( std::int32_t size, bool permit_scavenge = true, bool tenured = false );

    // KlassFormat
    enum class Format {
        no_klass,               //
        mem_klass,              //
        association_klass,      //
        blockClosure_klass,     //
        byteArray_klass,        //
        symbol_klass,           //
        context_klass,          //
        doubleByteArray_klass,  //
        doubleValueArray_klass, //
        double_klass,           //
        klass_klass,            //
        method_klass,           //
        mixin_klass,            //
        objArray_klass,         //
        weakArray_klass,        //
        process_klass,          //
        vframe_klass,           //
        proxy_klass,            //
        smi_klass,              //
        special_klass           //
    };


    // format
    virtual Format format() {
        return Format::no_klass;
    }


    static Format format_from_symbol( SymbolOop format );

    static const char *name_from_format( Format format );

    // Tells whether the two klass have same layout (format and instance variables)
    bool has_same_layout_as( KlassOop klass );

    bool has_same_inst_vars_as( KlassOop klass );

    // creates invocation
    virtual KlassOop create_subclass( MixinOop mixin, Format format );

    // create invocation (receiver as metaclass superclass)
    virtual KlassOop create_subclass( MixinOop mixin, KlassOop instSuper, KlassOop metaClass, Format format );

protected:
    static KlassOop create_generic_class( KlassOop super_class, MixinOop mixin, std::int32_t vtbl );

    static KlassOop create_generic_class( KlassOop superMetaClass, KlassOop superClass, KlassOop metaMetaClass, MixinOop mixin, std::int32_t vtbl );

public:

    virtual const char *name() const {
        return "";
    }


    void print_klass();

    char *delta_name(); // the Smalltalk name of the class or nullptr
    void print_name_on( ConsoleOutputStream *stream );

    // Methods
    inline MethodOop local_lookup( SymbolOop selector );

    MethodOop lookup( SymbolOop selector );

    bool is_method_holder_for( MethodOop method );

    KlassOop lookup_method_holder_for( MethodOop method );

    // Reflective operation
    void flush_methods();

    // Class variables
    AssociationOop local_lookup_class_var( SymbolOop name );

    AssociationOop lookup_class_var( SymbolOop name );

    // Instance variables

    // Returns the word offset for an instance variable.
    // -1 is returned if the search failed.
    std::int32_t lookup_inst_var( SymbolOop name ) const;

    // Returns the name of the instance variable at offset.
    // nullptr is returned if the search failed.
    SymbolOop inst_var_name_at( std::int32_t offset ) const;

    // Compute the number of instance variables based on the mixin,
    std::int32_t number_of_instance_variables() const;

    // Schema change support
    void mark_for_schema_change();

    bool is_marked_for_schema_change();

    void initialize();

    // ALL FUNCTIONS BELOW THIS POINT ARE DISPATCHED FROM AN OOP
    // These functions describe behavior for the Oop not the KLASS.

public:
    // actual Oop size of obj in memory
    virtual std::int32_t oop_size( Oop obj ) const {
        static_cast<void>(obj); // unused
        return non_indexable_size();
    }


    // Returns the header size for an instance of this klass
    virtual std::int32_t oop_header_size() const {
        return 0;
    }


    // memory operations
    virtual bool oop_verify( Oop obj );

    virtual std::int32_t oop_scavenge_contents( Oop obj );

    virtual std::int32_t oop_scavenge_tenured_contents( Oop obj );

    virtual void oop_follow_contents( Oop obj );


    // type testing operations
    virtual bool oop_is_smi() const {
        return false;
    }


    virtual bool oop_is_double() const {
        return false;
    }


    virtual bool oop_is_block() const {
        return false;
    }


    virtual bool oop_is_byteArray() const {
        return false;
    }


    virtual bool oop_is_doubleByteArray() const {
        return false;
    }


    virtual bool oop_is_doubleValueArray() const {
        return false;
    }


    virtual bool oop_is_symbol() const {
        return false;
    }


    virtual bool oop_is_objArray() const {
        return false;
    }


    virtual bool oop_is_weakArray() const {
        return false;
    }


    virtual bool oop_is_klass() const {
        return false;
    }


    virtual bool oop_is_process() const {
        return false;
    }


    virtual bool oop_is_vframe() const {
        return false;
    }


    virtual bool oop_is_method() const {
        return false;
    }


    virtual bool oop_is_proxy() const {
        return false;
    }


    virtual bool oop_is_mixin() const {
        return false;
    }


    virtual bool oop_is_association() const {
        return false;
    }


    virtual bool oop_is_context() const {
        return false;
    }


    virtual bool oop_is_message() const {
        return false;
    }


    virtual bool oop_is_indexable() const {
        return false;
    }


    // Dispatched primitives
    virtual Oop oop_primitive_allocate( Oop obj, bool allow_scavenge = true, bool tenured = false );

    virtual Oop oop_primitive_allocate_size( Oop obj, std::int32_t size );

    virtual Oop oop_shallow_copy( Oop obj, bool tenured );

    // printing operations
    virtual void oop_print_on( Oop obj, ConsoleOutputStream *stream );

    virtual void oop_short_print_on( Oop obj, ConsoleOutputStream *stream );

    virtual void oop_print_value_on( Oop obj, ConsoleOutputStream *stream );

    // iterators
    virtual void oop_oop_iterate( Oop obj, OopClosure *blk );

    virtual void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );

    friend class KlassKlass;

    friend class KlassOopDescriptor;
};


inline KlassOop as_klassOop( void *p ) {
    return KlassOop( as_memOop( p ) );
}
