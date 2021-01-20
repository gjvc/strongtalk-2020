
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/oops/OopDescriptor.hpp"
#include "vm/memory/Generation.hpp"
#include "vm/memory/NewGeneration.hpp"
#include "vm/memory/OldGeneration.hpp"
#include "vm/memory/RememberedSet.hpp"
#include "vm/memory/SpaceSizes.hpp"


extern bool_t scavengeRequired;             // set when eden overflows
extern bool_t garbageCollectionInProgress;  // garbage collection or scavenge in progress
extern bool_t bootstrappingInProgress;      // true only at the very beginning


//
// When a new root is added to Universe remember to:
//
//   1. add a private static variable prefixed with _.
//   2. add a public static accessor function.
//   3. define the static variable in Universe.cpp.
//   4. update Universe::do_oops to iterate over the new root.
//


// classes used by the interpreter
extern "C" KlassOop smiKlassObj;
extern "C" KlassOop contextKlassObj;
extern "C" KlassOop doubleKlassObj;
extern "C" KlassOop symbolKlassObj;


// objects used by the interpreter
extern "C" Oop nilObj;
extern "C" Oop trueObj;
extern "C" Oop falseObj;


// objects used by the block primitives
extern "C" KlassOop zeroArgumentBlockKlassObj;
extern "C" KlassOop oneArgumentBlockKlassObj;
extern "C" KlassOop twoArgumentBlockKlassObj;
extern "C" KlassOop threeArgumentBlockKlassObj;
extern "C" KlassOop fourArgumentBlockKlassObj;
extern "C" KlassOop fiveArgumentBlockKlassObj;
extern "C" KlassOop sixArgumentBlockKlassObj;
extern "C" KlassOop sevenArgumentBlockKlassObj;
extern "C" KlassOop eightArgumentBlockKlassObj;
extern "C" KlassOop nineArgumentBlockKlassObj;

//
extern "C" KlassOop doubleValueArrayKlassObj;

class SymbolTable;

class AgeTable;

class Zone;

class klassOopClosure;

class OopClosure;

class Universe : AllStatic {

private:
    // special

    // Known classes in the VM
    static KlassOop _memOopKlassObj;
    static KlassOop _objArrayKlassObj;
    static KlassOop _byteArrayKlassObj;
    static KlassOop _associationKlassObj;
    static KlassOop _doubleKlassObj;
    static KlassOop _methodKlassObj;
    static KlassOop _characterKlassObj;
    static KlassOop _vframeKlassObj;

    // Known objects in tbe VM
    static ObjectArrayOop _asciiCharacters;
    static ObjectArrayOop _systemDictionaryObj;
    static ObjectArrayOop _objectIDTable;
    static ObjectArrayOop _pic_free_list;

    static Oop       _callBack_receiver;
    static SymbolOop _callBack_selector;

    static Oop       _dll_lookup_receiver;
    static SymbolOop _dll_lookup_selector;

    static MethodOop _sweeper_method; // used by Sweeper only
    static bool_t    _scavenge_blocked;

    friend class Bootstrap;

public:
    // Known classes in tbe VM
    static KlassOop smiKlassObj();
    static KlassOop contextKlassObj();
    static KlassOop doubleKlassObj();
    static KlassOop memOopKlassObj();
    static KlassOop objArrayKlassObj();
    static KlassOop byteArrayKlassObj();
    static KlassOop symbolKlassObj();
    static KlassOop associationKlassObj();
    static KlassOop zeroArgumentBlockKlassObj();
    static KlassOop oneArgumentBlockKlassObj();
    static KlassOop twoArgumentBlockKlassObj();
    static KlassOop threeArgumentBlockKlassObj();
    static KlassOop fourArgumentBlockKlassObj();
    static KlassOop fiveArgumentBlockKlassObj();
    static KlassOop sixArgumentBlockKlassObj();
    static KlassOop sevenArgumentBlockKlassObj();
    static KlassOop eightArgumentBlockKlassObj();
    static KlassOop nineArgumentBlockKlassObj();
    static KlassOop methodKlassObj();
    static KlassOop characterKlassObj();
    static KlassOop doubleValueArrayKlassObj();
    static KlassOop vframeKlassObj();

    // Known objects in tbe VM
    static Oop nilObj();
    static Oop trueObj();
    static Oop falseObj();
    static ObjectArrayOop asciiCharacters();
    static ObjectArrayOop systemDictionaryObj();
    static ObjectArrayOop pic_free_list();
    static Oop callBack_receiver();
    static SymbolOop callBack_selector();
    static void set_callBack( Oop receiver, SymbolOop selector );
    static Oop dll_lookup_receiver();
    static SymbolOop dll_lookup_selector();
    static void set_dll_lookup( Oop receiver, SymbolOop selector );
    static MethodOop sweeper_method();


    static void set_sweeper_method( MethodOop method ) {
        _sweeper_method = method;
    }


    static ObjectArrayOop objectIDTable() {
        return _objectIDTable;
    }


    static void set_objectIDTable( ObjectArrayOop array ) {
        _objectIDTable = array;
    }


    // Version numbers
    //   increment snapshot_version whenever old snapshots will break; reset
    //   it to zero when changing the minor or major version
    static std::size_t major_version() {
        return 1;
    }


    static std::size_t minor_version() {
        return 1;
    }


    static const char *beta_version() {
        return "alpha5";
    }


    static std::size_t snapshot_version() {
        return 3;
    }


    // Check root is not badOop
    static void check_root( Oop *p );

    // Iterates over roots defined in Universe
    static void roots_do( void f( Oop * ) );

    // Iterates through all oops of Universe/Zone
    static void oops_do( void f( Oop * ) );

    // Iterates over all active classes and mixins in the system.
    // Active means reachable from the clases in the system dictionary.
    static void classes_do( klassOopClosure *iterator );

    static void methods_do( void f( MethodOop method ) );

private:
    static void classes_for_do( KlassOop klass, klassOopClosure *iterator );

    static void methods_in_array_do( ObjectArrayOop array, void f( MethodOop method ) );

    static void methods_for_do( KlassOop klass, void f( MethodOop method ) );

    friend class MethodsClosure;

public:

    static NewGeneration new_gen;
    static OldGeneration old_gen;

    static SymbolTable   *symbol_table;
    static RememberedSet *remembered_set;
    static AgeTable      *age_table;
    static Zone          *code;

    // additional variables
    static std::size_t tenuring_threshold;
    static std::size_t scavengeCount;


    // Space operations
    static bool_t is_heap( Oop *p ) {
        return new_gen.contains( p ) or old_gen.contains( p );
    }


    static Oop *object_start( Oop *p );

    // relocate is used for moving objects around after reading in a snapshot
    static MemOop relocate( MemOop p );

    static bool_t verify_oop( MemOop p );


    static bool_t really_contains( void *p ) {
        return new_gen.contains( p ) or old_gen.contains( p );
    }


    static Space *spaceFor( void *p );


    static Generation *generation_containing( Oop p ) {
        return new_gen.contains( p ) ? (Generation *) &new_gen : (Generation *) &old_gen;
    }


    // allocators
    static Oop *allocate( std::size_t size, MemOop *p = nullptr, bool_t permit_scavenge = true ) {

        if ( _scavenge_blocked and can_scavenge() and permit_scavenge )
            return scavenge_and_allocate( size, (Oop *) p );

        Oop *obj = new_gen.allocate( size );
        if ( not permit_scavenge )
            return obj;

        return obj ? obj : scavenge_and_allocate( size, (Oop *) p );
    }


    static Oop *allocate_without_scavenge( std::size_t size ) {
        Oop *obj = new_gen.allocate( size );
        return obj ? obj : allocate_tenured( size );
    }


    static Oop *allocate_tenured( std::size_t size, bool_t permit_expansion = true ) {
        return old_gen.allocate( size, permit_expansion );
    }


    // Tells whether we should force a garbage collection
    static bool_t needs_garbage_collection();

    // tells if the vm is in a state where we can scavenge.
    static bool_t can_scavenge();

    // scavenging operations.
    static Oop *scavenge_and_allocate( std::size_t size, Oop *p );

    static void scavenge( Oop *p = nullptr );

    static Oop tenure( Oop p = nullptr );

    static void default_low_space_handler( Oop p = nullptr );


    static void need_scavenge() {
        if ( not scavengeRequired ) {
            scavengeRequired = true;
            // setupPreemption();
        }
    }


    static bool_t needs_scavenge() {
        return scavengeRequired;
    }


    static bool_t should_scavenge( MemOop p ) {
        return not( ( (const char *) p > Universe::old_gen._lowBoundary ) or Universe::new_gen.to()->contains( p ) );
    }


    static Oop *allocate_in_survivor_space( MemOop p, std::size_t size, bool_t &is_new );


    static std::size_t free() {
        return old_gen.free();
    }


private:
    static void get_space_sizes();

    static char *check_eden_size( SpaceSizes &snap_sizes );

    static char *check_surv_size( SpaceSizes &snap_sizes );

    static char *check_old_size( SpaceSizes &snap_sizes );

public:
    static void genesis();

    static SpaceSizes current_sizes;

public:

    // operations: we need extras because of include file orderings
    static void store( Oop *p, SMIOop contents ) {
        *(SMIOop *) p = contents;
    }


    static void store( Oop *p, Oop contents, bool_t cs = true );

    static void cleanup_after_bootstrap();

    static void switch_pointers( Oop from, Oop to );

    static void verify( bool_t postScavenge = false );

    // printing operations
    static void print();

    static void print_layout();

    static void decode_methods();

    static void objectSizeHistogram( int maxSize );

    // Iterator
    static void object_iterate( ObjectClosure *blk );

    static void root_iterate( OopClosure *blk );

    // System dictionary manipulation
    static void add_global( Oop value );

    static void remove_global_at( int index );

public:
    static char *printAddr; // used for debug printing

    static void printRegion( const char *&caddr, int count = 16 );

    // for debugging
    static void print_klass_name( KlassOop k );

    static const char *klass_name( KlassOop k );

    static KlassOop method_holder_of( MethodOop m );

    static SymbolOop find_global_key_for( Oop value, bool_t *meta );

    static Oop find_global( const char *name, bool_t must_be_constant = false );

    static AssociationOop find_global_association( const char *name );

public:
    static void scavenge_oop( Oop *p );

    // flushes inline caches in all methodOops
    static void flush_inline_caches_in_methods();

    // clean all inline caches in methodOops and nativeMethods
    static void cleanup_all_inline_caches();

private:
    static void flush_inline_caches_in_method( MethodOop method );

public:
    static void methodOops_do( void f( MethodOop ) );

    static bool_t on_page_boundary( void *addr );

    static std::size_t page_size();
};

#define STORE_OOP( ADDR, VALUE ) Universe::store((Oop*) ADDR, (Oop) VALUE)


inline void scavenge_oop( Oop *p ) {
    *p = ( *p )->scavenge();
}


inline void scavenge_tenured_oop( Oop *p ) {
    scavenge_oop( p );
    if ( ( *p )->is_new() ) {
        Universe::remembered_set->record_store( p );
    }
}


inline Oop *min( Oop *a, Oop *b ) {
    return a > b ? b : a;
}


inline Oop *max( Oop *a, Oop *b ) {
    return a > b ? a : b;
}
