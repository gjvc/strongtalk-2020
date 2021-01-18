//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/code/CodeTable.hpp"
#include "vm/code/ZoneHeap.hpp"
#include "vm/code/JumpTable.hpp"



// Handle "self-modifying" code on processors with separate I-caches.
// To minimize the performance penalty of the flushes, the VM always does a sequence of selective flushes followed by a flushICache().
// If the CPU supports line-by-line flushes, implement flushICacheWord, otherwise flushICache.

#define HAVE_LINE_FLUSH        /* x86: don't need any flushing */


// For processors with a small I-cache / without selective cache invalidation,
// define flushICache to flush the entire I-cache.  Otherwise, make it a no-op.
#ifdef HAVE_LINE_FLUSH


inline void flushICache() {
}


#else
void flushICache();
#endif

// For processors with selective cache invalidation, define the following two routines:
#ifdef HAVE_LINE_FLUSH

void flushICacheWord( void * addr );                 // flush one word (instruction)
void flushICacheRange( void * start, void * end );    // flush range [start, end)
#else
inline void flushICacheWord(void * addr) {}
inline void flushICacheRange(void * start, void * end) {}
#endif





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
        ZoneHeap  * _methodHeap;     // Contains all nativeMethods
        ZoneHeap  * _picHeap;        // Contains all compiled CompiledPICs
        CodeTable * _methodTable;    // Hash table: LookupKey -> NativeMethod
        JumpTable _jumpTable;       // Contains all jump entries

    public:
        // returns the optimized method matching the lookup key, otherwise nullptr
        NativeMethod * lookup( const LookupKey * key ) {
            return _methodTable->lookup( key );
        }


    protected:
        NativeMethod * LRUhand;          // for LRU algorithm; sweeps through iZone
        bool_t _needsCompaction;  //
        bool_t _needsLRUSweep;    //
        bool_t _needsSweep;       //

        int    compactTime;                // time of last compaction
        int    compactDuration;            // duration of last compaction
        double minFreeFrac;             // fraction of free Space needed at compaction time


    public:
        Zone( int & size );


        void * operator new( std::size_t size ) {
            return AllocateHeap( size, "NativeMethod zone header" );
        }


        void clear();


        int capacity() const {
            return _methodHeap->capacity();
        }


        JumpTable * jump_table() const {
            return ( JumpTable * ) &_jumpTable;
        }


        void verify_if_often();

        int used();


        int numberOfNativeMethods() const {
            return jump_table()->usedIDs;
        }


        NativeMethod * allocate( int size );

        void free( NativeMethod * m );

        void addToCodeTable( NativeMethod * nm );

        void compact( bool_t forced = false );


        bool_t needsCompaction() const {
            return _needsCompaction;
        }


        bool_t needsWork() const {
            return needsCompaction() or _needsSweep;
        }


        bool_t needsSweep() const {
            return _needsSweep;
        }


        void doWork();

        void doSweep();

        void flush();

        void flushZombies( bool_t deoptimize = true );

        void flushUnused();

        // Clear all inline caches of all nativeMethods
        void clear_inline_caches();

        void cleanup_inline_caches();

        int findReplCandidates( int needed );

        bool_t isDeltaPC( void * p ) const;


        bool_t contains( const void * p ) const {
            return _methodHeap->contains( p );
        }


        NativeMethod * findNativeMethod( const void * start ) const;

        NativeMethod * findNativeMethod_maybe( void * start ) const;    // slow!

        void nativeMethods_do( void f( NativeMethod * nm ) );

        void PICs_do( void f( PolymorphicInlineCache * pic ) );

        // Iterates over all oops is the zone
        void oops_do( void f( Oop * ) );

        void switch_pointers( Oop from, Oop to );

        void verify();

        void print();

        void print_NativeMethod_histogram( int size );


        NativeMethod * first_nm() const {
            return ( NativeMethod * ) ( _methodHeap->firstUsed() );
        }


        NativeMethod * next_nm( NativeMethod * p ) const {
            return ( NativeMethod * ) ( _methodHeap->nextUsed( p ) );
        }


        PolymorphicInlineCache * first_pic() const {
            return ( PolymorphicInlineCache * ) ( _picHeap->firstUsed() );
        }


        PolymorphicInlineCache * next_pic( PolymorphicInlineCache * p ) const {
            return ( PolymorphicInlineCache * ) ( _picHeap->nextUsed( p ) );
        }


        const char * instsStart();

        int instsSize();

        int LRU_time();

        int sweeper( int maxVisit, int maxReclaim, int * nvisited = nullptr, int * nbytesReclaimed = nullptr );

        int nextNativeMethodID();

    public:
        void mark_dependents_for_deoptimization();

        void mark_all_for_deoptimization();

        void unmark_all_for_deoptimization();

        void make_marked_nativeMethods_zombies();

    protected:
        void print_helper( bool_t stats );

        void adjustPolicy();

        int flushNextMethod( int needed );

        inline NativeMethod * next_circular_nm( NativeMethod * nm );

        friend void moveInsts( const char * from, char * to, int size );

        friend void printAllNativeMethods();

        friend void sweepTrigger();
};


// holds usage information for nativeMethods (or index of next free NativeMethod ID if not assigned to any NativeMethod)
class LRUcount : ValueObject {
    public:
        std::uint16_t unused;    // NativeMethod prologue clears BOTH fields to 0
        std::uint16_t lastUsed;    // time of last use

        LRUcount() {
            ShouldNotCallThis();
        } // shouldn't create
        void set( int i ) {
            *( int * ) this = i;
        }
};

extern LRUcount * LRUtable;      // for optimized methods
extern int      * LRUflag;       // == LRUtable, just different type for convenience
