
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/OopDescriptor.hpp"
#include "vm/oops/MarkOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"


// memOops are all OOPs that actually take up Space in the heap (not immediate like SMIs)
// memOops are tagged (Smalltalk OOPs) but MemOopDescriptor* s are untagged C pointers.
// (see OopDescriptor.hpp)

class ObjectLayoutClosure;

class OopClosure;

class Bootstrap;


MemOop as_memOop( void *p );


class MemOopDescriptor : public OopDescriptor {

protected:
    // instance variable
    // markOop _mark;			// see comment in Oop.hpp
    KlassOop _klass_field;        // the receiver's class

public:
    // returns the header size of a MemOop
    static int header_size() {
        return sizeof( MemOopDescriptor ) / oopSize;
    }


    // field offsets for code generation
    static int mark_byte_offset() {
        return ( 0 * oopSize ) - MEMOOP_TAG;
    }


    static int klass_byte_offset() {
        return ( 1 * oopSize ) - MEMOOP_TAG;
    }


    // coercions
    friend MemOop as_memOop( void *p );


    // conversion from MemOop to MemOopDescriptor*
    MemOopDescriptor *addr() const {
        return reinterpret_cast<MemOopDescriptor *>( reinterpret_cast<uintptr_t>( this ) - MEMOOP_TAG );
    }


    // Space operations, is_old/new work w/o conversion to memOopDescriptor*
    // since Oop > pointer (MEMOOP_TAG >= 0)!
    bool_t is_old() const {
        return (const char *) this >= Universe::old_gen._lowBoundary;
    }


    bool_t is_new() const {
        return (const char *) this < Universe::new_gen._highBoundary;
    }


    // mark accessors
    MarkOop mark() const {
        return addr()->_mark;
    }


    void set_mark( MarkOop m ) {
        addr()->_mark = m;
    }


    void set_mark( MemOop p ) {
        set_mark( MarkOop( p ) );
    }


    void set_mark( Oop *p ) {
        set_mark( MarkOop( p ) );
    }


    Oop *klass_addr() const {
        st_assert( addr() != nullptr, "MemOopDescriptor::klass_addr():  addr() returned nullptr" );
        return (Oop *) &addr()->_klass_field;
    }


    void set_klass_field( KlassOop k, bool_t cs = true ) {
        // %optimization
        //   since klasses are tenured the store check can be avoided
        addr()->_klass_field = k;
    }


    KlassOop klass_field() const {
        return addr()->_klass_field;
    }


    Klass *blueprint() const {
//            return klass_field()->klass_part(); // Member access into incomplete type 'class KlassOopDescriptor'
        // "return klass_field()->klass_part();" has #include problems
        return (Klass *) ( ( (const char *) klass_field() ) + sizeof( MemOopDescriptor ) - MEMOOP_TAG );
    }

    // mark operations

    // use this after a copy to get a new mark
    void init_mark() {
        set_mark( MarkOopDescriptor::tagged_prototype() );
    }


    void init_untagged_contents_mark() {
        set_mark( MarkOopDescriptor::untagged_prototype() );
    }


    void mark_as_dying() {
        set_mark( mark()->set_near_death() );
    }


    // Notification queue support
    bool_t is_queued() const {
        return mark()->is_queued();
    }


    void set_queued() {
        set_mark( mark()->set_queued() );
    }


    void clear_queued() {
        set_mark( mark()->clear_queued() );
    }


    // mark operations
    smi_t identity_hash();

    void set_identity_hash( smi_t );

    // memory operations
    Oop scavenge();

    void follow_contents();

    // scavenge the header part (see oop_scavenge_contents)
    void scavenge_header();

    // scavenge the body [begin..[end
    void scavenge_body( int begin, int end );


    void scavenge_tenured_header() {
    }


    void scavenge_tenured_body( int begin, int end );

    // Scavenge all pointers in this object and return the Oop size
    int scavenge_contents();

    // Scavenge all pointers in this object and return the Oop size
    // has_new_pointers reports if the object has pointers to new Space.
    int scavenge_tenured_contents();

    // Copy this object to survivor Space and return the new address
    // (called by scavenge)
    Oop copy_to_survivor_space();

    // MarkSweep support
    void follow_header();

    void follow_body( int begin, int end );

    // support for iterating through all oops of an object (see oop_oop_iterate).
    void oop_iterate_header( OopClosure *blk );

    void oop_iterate_body( OopClosure *blk, int begin, int end );

    // support for iterate the layout of an object (see oop_layout_iterate).
    void layout_iterate_header( ObjectLayoutClosure *blk );

    void layout_iterate_body( ObjectLayoutClosure *blk, int begin, int end );

    // support for initializing objects (see allocateObject[Size]).
    void initialize_header( bool_t has_untagged, KlassOop klass );

    void initialize_body( int begin, int end );

    bool_t verify();


    // forwarding operations
    bool_t is_forwarded() {
        return mark()->is_mem();
    }


    void forward_to( MemOop p ) {
        st_assert( p->is_mem(), "forwarding to something that's not a MemOop" );
        set_mark( p );
    }


    MemOop forwardee() {
        return MemOop( mark() );
    }


    // marking operations
    bool_t is_gc_marked() {
        return not( mark()->is_mark() and mark()->has_sentinel() );
    } // Changed from mark()->is_smi(), Lars

//        bool_t is_gc_marked() { return not mark()->has_sentinel(); } // Changed from mark()->is_smi(), Lars

    // GC operations (see discussion in Universe.cpp for rationale)
    void gc_store_size();            // Store object size in age field and remembered set
    int gc_retrieve_size();          // Retrieve object size from age field and remembered set

    // accessors
    Oop *oops( int which = 0 ) {
        return &( (Oop *) addr() )[ which ];
    }


    Oop raw_at( int which ) {
        return *oops( which );
    }


    void raw_at_put( int which, Oop contents, bool_t cs = true );

    // accessing instance variables
    bool_t is_within_instVar_bounds( int index );

    Oop instVarAt( int index );

    Oop instVarAtPut( int index, Oop value );

    // iterators
    void oop_iterate( OopClosure *blk );

    void layout_iterate( ObjectLayoutClosure *blk );

    // Returns the Oop size of this object
    std::size_t size() const;

    // printing operation
    void print_id_on( ConsoleOutputStream *stream );

    void print_on( ConsoleOutputStream *stream );

    // bootstrappingInProgress operations
    void bootstrap_object( Bootstrap *stream );

    void bootstrap_header( Bootstrap *stream );

    void bootstrap_body( Bootstrap *stream, int h_size );

    friend class MemOopKlass;
};
