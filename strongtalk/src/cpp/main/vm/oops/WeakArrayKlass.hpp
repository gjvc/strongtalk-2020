//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/oops/ObjectArrayKlass.hpp"
#include "vm/utilities/GrowableArray.hpp"


class WeakArrayKlass : public ObjectArrayKlass {
public:
    friend void setKlassVirtualTableFromWeakArrayKlass( Klass *k );


    const char *name() const {
        return "weakArray";
    }


    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );

    static KlassOop create_class( KlassOop super_class, MixinOop mixin );


    // Format
    Format format() {
        return Format::weakArray_klass;
    }

    // ALL FUNCTIONS BELOW THIS POINT ARE DISPATCHED FROM AN OOP

    // memory operations
    int oop_scavenge_contents( Oop obj );

    int oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );


    // testers
    bool_t oop_is_weakArray() const {
        return true;
    }


    // iterators
    void oop_oop_iterate( Oop obj, OopClosure *blk );

    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );


    // Sizing
    int oop_header_size() const {
        return WeakArrayOopDescriptor::header_size();
    }
};

void setKlassVirtualTableFromWeakArrayKlass( Klass *k );
// The weak array register is used during memory management to
// split the object scanning into two parts:
//   1. Transively traverse all object except the indexable part
//      of weakArrays. Then a weakArray is encountered it is registered
//   2. Using the registered weakArrays continue the transitive traverse.
// Inbetween we can easily compute the set of object with a
// near death experience.
//
// Scavenge and Mark Sweep use to disjunct parts of the interface.

// Implementation note:
//  During phase1 of Mark Sweep pointers are reversed and a objects
//  structure cannot be used (the klass pointer is gone). This makes
//  it necessary to register weakArrays along with their non indexable sizes.
//  'nis' contains the non indexable sizes.

// Interface for weak array support
class WeakArrayRegister : AllStatic {
public:
    // Scavenge interface
    static void begin_scavenge();

    static bool_t scavenge_register( WeakArrayOop obj );

    static void check_and_scavenge_contents();

    // Mark sweep interface
    static void begin_mark_sweep();

    static bool_t mark_sweep_register( WeakArrayOop obj, int non_indexable_size );

    static void check_and_follow_contents();

private:
    // Variables
    static bool_t during_registration;
    static GrowableArray<WeakArrayOop> *weakArrays;
    static GrowableArray<int>          *nis;

    // Scavenge operations
    static void scavenge_contents();

    static inline bool_t scavenge_is_near_death( Oop obj );

    static void scavenge_check_for_dying_objects();

    // Mark sweep operations
    static void follow_contents();

    static inline bool_t mark_sweep_is_near_death( Oop obj );

    static void mark_sweep_check_for_dying_objects();
};

// The NotificationQueue holds references to weakArrays
// containing object with a near death experience.
class NotificationQueue : AllStatic {
public:
    // Queue operations
    static void mark_elements();   // Marks all elements as queued (by using the sentinel bit)
    static void clear_elements();  // Reset the sentinel bit

    static bool_t is_empty();

    static Oop get();

    static void put( Oop obj );

    static void put_if_absent( Oop obj );

    // Memory management
    static void oops_do( void f( Oop * ) );

private:
    static Oop *array;
    static int size;
    static int first;
    static int last;

    static int succ( int index );
};
