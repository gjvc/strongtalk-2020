
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/MarkSweep.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/oops/WeakArrayKlass.hpp"
#include "vm/runtime/VMProcess.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/SymbolTable.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/WaterMark.hpp"

typedef struct {
    Oop anOop;
    Oop *oopPointer;
} oopAssoc;


class OopChunk : public ResourceObject {

private:
    oopAssoc       oop_start[1000];
    const oopAssoc *oop_end;
    oopAssoc       *next;

public:
    OopChunk() :
        oop_end{ oop_start + 1000 - 1 },// account for pre-increment in append
        next{ oop_start - 1 } {
    }
    virtual ~OopChunk() = default;
    OopChunk( const OopChunk & ) = default;
    OopChunk &operator=( const OopChunk & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    bool isFull() {
        return next >= oop_end;
    }


    Oop *append( Oop *anOop ) {
        st_assert( not isFull(), "Cannot append to full OopChunk" );
        st_assert( ( *anOop )->isMemOop(), "Must be a mem Oop" );
        next++;
        next->anOop      = *anOop;
        next->oopPointer = anOop;
        return &next->anOop;
    }


    void fixupOops() {
        oopAssoc *current = next;
        while ( current >= oop_start ) {
            st_assert( current->anOop->isMemOop(), "Fixed up Oop should be MemOop" );
            *( current->oopPointer ) = current->anOop;
            current--;
        }
    }
};


class OopRelocations : public ResourceObject {
private:
    GrowableArray<OopChunk *> *chunks;
    OopChunk                  *current;


public:
    OopRelocations() :
        chunks{ nullptr },
        current{ nullptr } {
        chunks  = new GrowableArray<OopChunk *>( 10 );
        current = new OopChunk();
        chunks->append( current );
    }


    virtual ~OopRelocations() = default;
    OopRelocations( const OopRelocations & ) = default;
    OopRelocations &operator=( const OopRelocations & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    Oop *relocate( Oop *toMove ) {
        if ( current->isFull() ) {
            current = new OopChunk();
            chunks->append( current );
        }
        return current->append( toMove );
    }


    void fixupOops() {
        while ( chunks->nonEmpty() ) {
            chunks->pop()->fixupOops();
        }
    }
};


GrowableArray<MemOop>       *MarkSweep::_stack;
GrowableArray<std::int32_t> *MarkSweep::hcode_offsets;
std::int32_t                MarkSweep::hcode_pos;
OopRelocations              *MarkSweep::_oopRelocations;


void oopVerify( Oop *p ) {
    ( *p )->verify();
}


Oop MarkSweep::collect( Oop p ) {
    FlagSetting  fl( garbageCollectionInProgress, true );
    EventMarker  em( "Garbage Collect" );
    ResourceMark resourceMark;
    TraceTime    t( "Garbage collection", PrintGC );

    std::int32_t old_used = Universe::old_gen.used();

    if ( VerifyBeforeScavenge or VerifyBeforeGC )
        Universe::verify();

    // Clear all vm inline caches
    DeltaCallCache::clearAll();

    // clear remembered set; it is used for object sizes
    Universe::remembered_set->clear();

    allocate();    // allocate stack for traversal

    mark_sweep_phase1( &p );
    mark_sweep_phase2();
    mark_sweep_phase3();

    deallocate(); // clear allocated structures

    // clear the remember set; we have no pointers from old to new
    Universe::remembered_set->clear();

    LookupCache::flush();

    if ( VerifyAfterScavenge or VerifyAfterGC ) {
        Universe::verify();
        Universe::code->oops_do( &oopVerify );
    }

    if ( PrintGC ) {
        SPDLOG_INFO( "%garbage-collection:  before [{:3f}M], after [{:3f}M]", (double) old_used / (double) ( 1024 * 1024 ), (double) Universe::old_gen.used() / (double) ( 1024 * 1024 ) );
    }

    return p;
}


void MarkSweep::allocate() {
    _stack          = new GrowableArray<MemOop>( 200 );
    hcode_offsets   = new GrowableArray<std::int32_t>( 100 );
    hcode_pos       = 0;
    _oopRelocations = new OopRelocations();
}


void MarkSweep::deallocate() {
    _stack        = nullptr;
    hcode_offsets = nullptr;
}


void MarkSweep::trace( const char *msg ) {
    if ( TraceGC )
        SPDLOG_INFO( "{}", msg );
}


MemOop MarkSweep::reverse( Oop *p ) {
    Oop obj = *p;

    // Return nullptr if non MemOop
    if ( not obj->isMemOop() )
        return nullptr;
    if ( not Oop( p )->isSmallIntegerOop() ) {// ie. not word-aligned
        p = _oopRelocations->relocate( p );
    }

    if ( MemOop( obj )->is_gc_marked() ) {
        // Reverse pointer
        *p = Oop( MemOop( obj )->mark() );
        MemOop( obj )->set_mark( p );

        return nullptr;
    } else {
        // Before the pointer reversal takes place the object size must be made accessible
        // without using the klass pointer. We store the object size partially in the
        // age field and partially in the remembered set.
        MemOop( obj )->gc_store_size();

        st_assert( MemOop(obj)->size() == MemOop(obj)->gc_retrieve_size(), "checking real against stored size" );

        // Reverse pointer
        *p = Oop( MemOop( obj )->mark() );
        MemOop( obj )->set_mark( p );

        st_assert( MemOop(obj)->klass()->isMemOop(), "just checking" );

        return MemOop( obj );
    }
}


void MarkSweep::reverse_and_push( Oop *p ) {
    MemOop m = reverse( p );
    if ( m )
        _stack->push( m );
}


void MarkSweep::reverse_and_follow( Oop *p ) {
    MemOop m = reverse( p );
    if ( m )
        m->follow_contents(); // Follow contents of the marked object
}


void MarkSweep::follow_root( Oop *p ) {
    reverse_and_follow( p );
    while ( not _stack->isEmpty() )
        _stack->pop()->follow_contents();
}


void MarkSweep::add_heap_code_offset( std::int32_t offset ) {
    hcode_offsets->push( offset );
}


std::int32_t MarkSweep::next_heap_code_offset() {
    return hcode_offsets->at( hcode_pos++ );
}


void MarkSweep::mark_sweep_phase1( Oop *p ) {

    static_cast<void>(p); // unused

    // Recursively traverse all live objects and mark them by reversing pointers.
    EventMarker em( "1 reverse pointers" );

    trace( " 1" );

    WeakArrayRegister::begin_mark_sweep();
    Processes::convert_heap_code_pointers();

    Universe::oops_do( &follow_root );

    // HeapCode pointers point to the inside of methodOops and
    // have to be converted before we can follow the processes.
    Processes::follow_roots();

    WeakArrayRegister::check_and_follow_contents();
    NotificationQueue::oops_do( &follow_root );

    vmSymbols::follow_contents();

    // finalize unused objects - must be after all other gc_mark routines!
    Universe::symbol_table->follow_used_symbols();

    st_assert( _stack->isEmpty(), "stack should be empty by now" );
}


void MarkSweep::mark_sweep_phase2() {
    // Now all live objects are marked, compute the new object addresses.
    EventMarker em( "2 compute new addresses" );
    trace( "2" );

    OldWaterMark mark = Universe::old_gen.bottom_mark();
    // %note memory must be traversed in the same order as phase3
    Universe::old_gen.prepare_for_compaction( &mark );
    Universe::new_gen.prepare_for_compaction( &mark );
}


void MarkSweep::mark_sweep_phase3() {
    EventMarker em( "3 compact heap" );
    trace( "3" );
    OldWaterMark mark = Universe::old_gen.bottom_mark();
    mark._space->initialize_threshold();
    // %note memory must be traversed in the same order as phase2
    Universe::old_gen.compact( &mark );
    Universe::new_gen.compact( &mark );

    // update non-Oop-aligned Oop locations
    _oopRelocations->fixupOops();

    // All hcode pointers can now be restored. Remember
    // we converted these pointers in phase1.
    Processes::restore_heap_code_pointers();
}
