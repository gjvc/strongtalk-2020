//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/code/CodeTable.hpp"
#include "vm/code/ZoneHeap.hpp"
#include "vm/code/JumpTable.hpp"




// The zone implements the code cache for optimized methods and contains:
//   1) a lookup table:      "Lookup key -> NativeMethod"
//   2) the optimized methods (nativeMethods).

// Implementation:
//   - Each compiled method occupies one chunk of memory.
//   - Like the offset table in oldspace the zone has at table for
//     locating a method given a address of an instruction.


class PolymorphicInlineCache;

class Zone : public CHeapAllocatedObject {

public:
    ZoneHeap  *_methodHeap;     // Contains all nativeMethods
    ZoneHeap  *_picHeap;        // Contains all compiled CompiledPICs
    CodeTable *_methodTable;    // Hash table: LookupKey -> NativeMethod
    JumpTable _jumpTable;       // Contains all jump entries

public:
    // returns the optimized method matching the lookup key, otherwise nullptr
    NativeMethod *lookup( const LookupKey *key ) {
        return _methodTable->lookup( key );
    }


protected:
    NativeMethod *LRUhand;          // for LRU algorithm; sweeps through iZone
    bool         _needsCompaction;  //
    bool         _needsLRUSweep;    //
    bool         _needsSweep;       //

    std::int32_t compactTime;       // time of last compaction
    std::int32_t compactDuration;   // duration of last compaction
    double       minFreeFrac;       // fraction of free Space needed at compaction time


public:
    Zone( std::int32_t &size );

    Zone() = default;
    virtual ~Zone() = default;
    Zone( const Zone & ) = default;
    Zone &operator=( const Zone & ) = default;
    void operator delete( void *ptr ) { (void)ptr; }

    void *operator new( std::size_t size ) {
        return AllocateHeap( size, "NativeMethod zone header" );
    }


    void clear();


    std::int32_t capacity() const {
        return _methodHeap->capacity();
    }


    JumpTable *jump_table() const {
        return (JumpTable *) &_jumpTable;
    }


    void verify_if_often();

    std::int32_t used();


    std::int32_t numberOfNativeMethods() const {
        return jump_table()->usedIDs;
    }


    NativeMethod *allocate( std::int32_t size );

    void free( NativeMethod *m );

    void addToCodeTable( NativeMethod *nm );

    void compact( bool forced = false );


    bool needsCompaction() const {
        return _needsCompaction;
    }


    bool needsWork() const {
        return needsCompaction() or _needsSweep;
    }


    bool needsSweep() const {
        return _needsSweep;
    }


    void doWork();

    void doSweep();

    void flush();

    void flushZombies( bool deoptimize = true );

    void flushUnused();

    // Clear all inline caches of all nativeMethods
    void clear_inline_caches();

    void cleanup_inline_caches();

    std::int32_t findReplCandidates( std::int32_t needed );

    bool isDeltaPC( void *p ) const;


    bool contains( const void *p ) const {
        return _methodHeap->contains( p );
    }


    NativeMethod *findNativeMethod( const void *start ) const;

    NativeMethod *findNativeMethod_maybe( void *start ) const;    // slow!

    void nativeMethods_do( void f( NativeMethod *nm ) );

    void PICs_do( void f( PolymorphicInlineCache *pic ) );

    // Iterates over all oops is the zone
    void oops_do( void f( Oop * ) );

    void switch_pointers( Oop from, Oop to );

    void verify();

    void print();

    void print_NativeMethod_histogram( std::int32_t size );


    NativeMethod *first_nm() const {
        return (NativeMethod *) ( _methodHeap->firstUsed() );
    }


    NativeMethod *next_nm( NativeMethod *p ) const {
        return (NativeMethod *) ( _methodHeap->nextUsed( p ) );
    }


    PolymorphicInlineCache *first_pic() const {
        return (PolymorphicInlineCache *) ( _picHeap->firstUsed() );
    }


    PolymorphicInlineCache *next_pic( PolymorphicInlineCache *p ) const {
        return (PolymorphicInlineCache *) ( _picHeap->nextUsed( p ) );
    }


    const char *instsStart();

    std::int32_t instsSize();

    std::int32_t LRU_time();

    std::int32_t sweeper( std::int32_t maxVisit, std::int32_t maxReclaim, std::int32_t *nvisited = nullptr, std::int32_t *nbytesReclaimed = nullptr );

    std::int32_t nextNativeMethodID();

public:
    void mark_dependents_for_deoptimization();

    void mark_all_for_deoptimization();

    void unmark_all_for_deoptimization();

    void make_marked_nativeMethods_zombies();

protected:
    void print_helper( bool stats );

    void adjustPolicy();

    std::int32_t flushNextMethod( std::int32_t needed );

    inline NativeMethod *next_circular_nm( NativeMethod *nm );

    friend void moveInsts( const char *from, char *to, std::int32_t size );

    friend void printAllNativeMethods();

    friend void sweepTrigger();
};


// holds usage information for nativeMethods (or index of next free NativeMethod ID if not assigned to any NativeMethod)
class LRUcount : ValueObject {
public:
    std::uint16_t unused;       // NativeMethod prologue clears BOTH fields to 0
    std::uint16_t lastUsed;     // time of last use

    void set( std::int32_t i ) {
        *(std::int32_t *) this = i;
    }


private:
    LRUcount() :
        unused{},
        lastUsed{} {
        ShouldNotCallThis();
    } // shouldn't create


};

extern LRUcount     *LRUtable;      // for optimized methods
extern std::int32_t *LRUflag;       // == LRUtable, just different type for convenience
