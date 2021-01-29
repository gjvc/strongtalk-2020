
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


extern bool scavengeRequired;             // set when eden overflows
extern bool garbageCollectionInProgress;  // garbage collection or scavenge in progress
extern bool bootstrappingInProgress;      // true only at the very beginning


//
// When a new root is added to Universe remember to:
//
//   1. add a private static variable prefixed with _.
//   2. add a public static accessor function.
//   3. define the static variable in Universe.cpp.
//   4. update Universe::do_oops to iterate over the new root.
//


// classes used by the interpreter
extern "C" KlassOop smiKlassObject;
extern "C" KlassOop contextKlassObject;
extern "C" KlassOop doubleKlassObject;
extern "C" KlassOop symbolKlassObject;


// objects used by the interpreter
extern "C" Oop nilObject;
extern "C" Oop trueObject;
extern "C" Oop falseObject;


// objects used by the block primitives
extern "C" KlassOop zeroArgumentBlockKlassObject;
extern "C" KlassOop oneArgumentBlockKlassObject;
extern "C" KlassOop twoArgumentBlockKlassObject;
extern "C" KlassOop threeArgumentBlockKlassObject;
extern "C" KlassOop fourArgumentBlockKlassObject;
extern "C" KlassOop fiveArgumentBlockKlassObject;
extern "C" KlassOop sixArgumentBlockKlassObject;
extern "C" KlassOop sevenArgumentBlockKlassObject;
extern "C" KlassOop eightArgumentBlockKlassObject;
extern "C" KlassOop nineArgumentBlockKlassObject;

//
extern "C" KlassOop doubleValueArrayKlassObject;

class SymbolTable;

class AgeTable;

class Zone;

class klassOopClosure;

class OopClosure;

class Universe : AllStatic {

private:
    // special

    // Known classes in the VM
    static KlassOop _memOopKlassObject;
    static KlassOop _objArrayKlassObject;
    static KlassOop _byteArrayKlassObject;
    static KlassOop _associationKlassObject;
    static KlassOop _doubleKlassObject;
    static KlassOop _methodKlassObject;
    static KlassOop _characterKlassObject;
    static KlassOop _vframeKlassObject;

    // Known objects in tbe VM
    static ObjectArrayOop _asciiCharacters;
    static ObjectArrayOop _systemDictionaryObject;
    static ObjectArrayOop _objectIDTable;
    static ObjectArrayOop _pic_free_list;

    static Oop       _callBack_receiver;
    static SymbolOop _callBack_selector;

    static Oop       _dll_lookup_receiver;
    static SymbolOop _dll_lookup_selector;

    static MethodOop _sweeper_method; // used by Sweeper only
    static bool    _scavenge_blocked;

    friend class Bootstrap;

public:
    // Known classes in tbe VM
    static KlassOop smiKlassObject();
    static KlassOop contextKlassObject();
    static KlassOop doubleKlassObject();
    static KlassOop memOopKlassObject();
    static KlassOop objArrayKlassObject();
    static KlassOop byteArrayKlassObject();
    static KlassOop symbolKlassObject();
    static KlassOop associationKlassObject();
    static KlassOop zeroArgumentBlockKlassObject();
    static KlassOop oneArgumentBlockKlassObject();
    static KlassOop twoArgumentBlockKlassObject();
    static KlassOop threeArgumentBlockKlassObject();
    static KlassOop fourArgumentBlockKlassObject();
    static KlassOop fiveArgumentBlockKlassObject();
    static KlassOop sixArgumentBlockKlassObject();
    static KlassOop sevenArgumentBlockKlassObject();
    static KlassOop eightArgumentBlockKlassObject();
    static KlassOop nineArgumentBlockKlassObject();
    static KlassOop methodKlassObject();
    static KlassOop characterKlassObject();
    static KlassOop doubleValueArrayKlassObject();
    static KlassOop vframeKlassObject();

    // Known objects in tbe VM
    static Oop nilObject();
    static Oop trueObject();
    static Oop falseObject();
    static ObjectArrayOop asciiCharacters();
    static ObjectArrayOop systemDictionaryObject();
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
    static std::int32_t major_version() {
        return 1;
    }


    static std::int32_t minor_version() {
        return 1;
    }


    static const char *beta_version() {
        return "alpha5";
    }


    static std::int32_t snapshot_version() {
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
    static std::int32_t tenuring_threshold;
    static std::int32_t scavengeCount;


    // Space operations
    static bool is_heap( Oop *p ) {
        return new_gen.contains( p ) or old_gen.contains( p );
    }


    static Oop *object_start( Oop *p );

    // relocate is used for moving objects around after reading in a snapshot
    static MemOop relocate( MemOop p );

    static bool verify_oop( MemOop p );


    static bool really_contains( void *p ) {
        return new_gen.contains( p ) or old_gen.contains( p );
    }


    static Space *spaceFor( void *p );


    static Generation *generation_containing( Oop p ) {
        return new_gen.contains( p ) ? (Generation *) &new_gen : (Generation *) &old_gen;
    }


    // allocators
    static Oop *allocate( std::int32_t size, MemOop *p = nullptr, bool permit_scavenge = true ) {

        if ( _scavenge_blocked and can_scavenge() and permit_scavenge )
            return scavenge_and_allocate( size, (Oop *) p );

        Oop *obj = new_gen.allocate( size );
        if ( not permit_scavenge )
            return obj;

        return obj ? obj : scavenge_and_allocate( size, (Oop *) p );
    }


    static Oop *allocate_without_scavenge( std::int32_t size ) {
        Oop *obj = new_gen.allocate( size );
        return obj ? obj : allocate_tenured( size );
    }


    static Oop *allocate_tenured( std::int32_t size, bool permit_expansion = true ) {
        return old_gen.allocate( size, permit_expansion );
    }


    // Tells whether we should force a garbage collection
    static bool needs_garbage_collection();

    // tells if the vm is in a state where we can scavenge.
    static bool can_scavenge();

    // scavenging operations.
    static Oop *scavenge_and_allocate( std::int32_t size, Oop *p );

    static void scavenge( Oop *p = nullptr );

    static Oop tenure( Oop p = nullptr );

    static void default_low_space_handler( Oop p = nullptr );


    static void need_scavenge() {
        if ( not scavengeRequired ) {
            scavengeRequired = true;
            // setupPreemption();
        }
    }


    static bool needs_scavenge() {
        return scavengeRequired;
    }


    static bool should_scavenge( MemOop p ) {
        return not( ( (const char *) p > Universe::old_gen._lowBoundary ) or Universe::new_gen.to()->contains( p ) );
    }


    static Oop *allocate_in_survivor_space( MemOop p, std::int32_t size, bool &is_new );


    static std::int32_t free() {
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


    static void store( Oop *p, Oop contents, bool cs = true );

    static void cleanup_after_bootstrap();

    static void switch_pointers( Oop from, Oop to );

    static void verify( bool postScavenge = false );

    // printing operations
    static void print();

    static void print_layout();

    static void decode_methods();

    static void objectSizeHistogram( std::int32_t maxSize );

    // Iterator
    static void object_iterate( ObjectClosure *blk );

    static void root_iterate( OopClosure *blk );

    // System dictionary manipulation
    static void add_global( Oop value );

    static void remove_global_at( std::int32_t index );

public:
    static char *printAddr; // used for debug printing

    static void printRegion( const char *&caddr, std::int32_t count = 16 );

    // for debugging
    static void print_klass_name( KlassOop k );

    static const char *klass_name( KlassOop k );

    static KlassOop method_holder_of( MethodOop m );

    static SymbolOop find_global_key_for( Oop value, bool *meta );

    static Oop find_global( const char *name, bool must_be_constant = false );

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

    static bool on_page_boundary( void *addr );

    static std::int32_t page_size();
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
