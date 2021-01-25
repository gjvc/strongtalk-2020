
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/runtime/VirtualSpace.hpp"
#include "vm/runtime/ReservedSpace.hpp"
#include "vm/oops/OopDescriptor.hpp"

// A generation is a bunch of spaces of similarly-aged objects

class Generation : ValueObject {

private:
    friend class RememberedSet;

    friend class Universe;

    friend class MarkSweep;

    friend class MemOopDescriptor;

    friend class ByteArrayOopDescriptor;

    friend class OldGeneration;

protected:
    // Minimum and maximum addresses, used by card marking code.
    // Must not overlap with address ranges of other generation(s).
    const char *_lowBoundary;
    const char *_highBoundary;
    VirtualSpace _virtualSpace;

public:
    // Space enquiries
    virtual std::int32_t capacity() = 0;

    virtual std::int32_t used() = 0;

    virtual std::int32_t free() = 0;

    void print();
};


// ------------------------------------------------------------------------------


// ensure that you surround the call with {} to prevent s leaking out!
#define FOR_EACH_OLD_SPACE( s ) \
    for ( OldSpace *s = Universe::old_gen._firstSpace; s not_eq nullptr; s = s->_nextSpace )

//inline void FOR_EACH_OLD_SPACE( const auto & s ) {
//    for ( OldSpace *s = Universe::old_gen._firstSpace; s not_eq nullptr; s = s->_nextSpace )
//}

inline void SCAVENGE_TEMPLATE( const auto &p ) {
    *( (Oop *) p ) = Oop( *p )->scavenge();
}


inline void VERIFY_TEMPLATE( const auto &p ) {
    if ( not Oop( *p )->verify() ) {
        lprintf( "\tof object at %#lx\n", p );
    }
}


inline void RELOCATE_TEMPLATE( const auto &p ) {
    *( (Oop *) p ) = Oop( *p )->relocate();
}


inline void SPACE_VERIFY_TEMPLATE( const auto &s ) {
    s->verify();
}


#define SWITCH_POINTERS_TEMPLATE( p ) \
    if ((Oop) *p == (Oop) from) *((Oop*) p) = (Oop) to;


#define APPLY_TO_YOUNG_SPACES( t ) \
    t( new_gen.eden() ) \
    t( new_gen.from() ) \
    t( new_gen.to() )

#define APPLY_TO_OLD_SPACES( t ) \
    { FOR_EACH_OLD_SPACE(s) { t(s); } }



// ------------------------------------------------------------------------------

#define APPLY_TO_YOUNG_SPACE_NAMES( t ) \
    t( eden() ) \
    t( from() ) \
    t( to() )

#define OOPS_DO_TEMPLATE( p, f ) \
    (*f)((Oop*)p);

#define APPLY_TO_SPACES( t ) \
    APPLY_TO_YOUNG_SPACES( t ) \
    APPLY_TO_OLD_SPACES( t )

#define YOUNG_SPACE_COMPACT_TEMPLATE( s ) \
    c2= s; s->compact(c2, d);

#define OLD_SPACE_COMPACT_TEMPLATE( s ) \
    s->compact(c2, d);

#define SPACE_RELOCATE_TEMPLATE( s ) \
    s->relocate();

#define SPACE_NEED_TO_RELOCATE_TEMPLATE( s ) \
    need_to_relocate |= s->need_to_relocate();

#define SPACE_FIXUP_KILLABLES_TEMPLATE( s ) \
    s->fixup_killables(okZone);

#define SPACE_OOP_RELOCATE_TEMPLATE( s ) \
    if (s->old_contains(p)) return s->relocate_objs(p);

#define SPACE_VERIFY_OOP_TEMPLATE( s ) \
    if (s->contains(p)) return true;
