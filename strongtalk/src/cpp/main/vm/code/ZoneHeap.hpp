//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/system/os.hpp"
#include "vm/code/ZoneHeap.hpp"
#include "vm/runtime/flags.hpp"

// Basic heap management
// maintains a map of the heap + free lists to reduce fragmentation
// allocations are in multiples of block size (2**k)

//class HeapChunk;
//
//class ChunkKlass;
//
//class FreeList;


// -----------------------------------------------------------------------------

class HeapChunk : public ValueObject {    // a heap chunk is a consecutive sequence of blocks

protected:
    HeapChunk *_next, *_prev;  // doubly-linked ring
    void initialize() {
        _next = _prev = this;
        size  = 0;
    }


public:
    std::size_t size;    // size in blocks (only for heterogenuous list)

    HeapChunk() {
        initialize();
    }


    HeapChunk *next() const {
        return _next;
    }


    HeapChunk *prev() const {
        return _prev;
    }


    void insert( HeapChunk *other ) {
        other->_next = _next;
        _next->_prev = other;
        _next = other;
        other->_prev = this;
    }


    void remove() {
        _next->_prev = _prev;
        _prev->_next = _next;
        initialize();
    }
};

class FreeList : private HeapChunk {
public:
    void clear() {
        initialize();
    }


    HeapChunk *anchor() const {
        return (HeapChunk *) this;
    }


    bool_t isEmpty() const {
        return next() == anchor();
    }


    void append( HeapChunk *h );

    void remove( HeapChunk *h );

    HeapChunk *get();

    int length() const;
};


// -----------------------------------------------------------------------------

enum class chunkState {
    ZeroDistance = 0,    //
    MaxDistance  = 128,  //
    unused       = 128,  //
    unusedOvfl   = 190,  //
    used         = 192,  //
    usedOvfl     = 254,  //
    invalid      = 255   //
};


// The other bytes hold the distance to the chunk header (or an approximation
// thereof); headers are found by following the distance pointers downwards

constexpr int minHeaderSize = 1;
constexpr int maxHeaderSize = 4;


constexpr int MaxDistLog    = log2( static_cast<double>( chunkState::MaxDistance ) );
constexpr int maxOneByteLen = ( static_cast<std::size_t>( chunkState::usedOvfl ) - static_cast<std::size_t>(chunkState::used ) );

class ChunkKlass;

ChunkKlass *asChunkKlass( std::uint8_t *c );


class ChunkKlass {

private:
    std::uint8_t c( int which ) {
        return ( (std::uint8_t *) this )[ which ];
    }


    std::uint8_t n( int which ) {
        return c( which ) - static_cast<std::uint8_t>( chunkState::unused );
    }


public:
    ChunkKlass() {
        st_fatal( "shouldn't create" );
    }


    std::uint8_t *asByte() {
        return (std::uint8_t *) this;
    }


    void markSize( int nChunks, chunkState s );


    void markUsed( int nChunks ) {
        markSize( nChunks, chunkState::used );
    }


    void markUnused( int nChunks ) {
        markSize( nChunks, chunkState::unused );
    }


    ChunkKlass *findStart( ChunkKlass *mapStart, ChunkKlass *mapEnd );

    bool_t verify();

    void print();

    bool_t isValid();


    void invalidate() {
        asByte()[ 0 ] = static_cast<std::uint8_t>( chunkState::invalid );
    }


    chunkState state() {
        return chunkState( c( 0 ) );
    }


    bool_t isUsed() {
        return state() >= chunkState::used;
    }


    bool_t isUnused() {
        return not isUsed();
    }


    int headerSize() {        // size of header in bytes
        int ovfl = static_cast<std::size_t>( isUsed() ? chunkState::usedOvfl : chunkState::unusedOvfl );
        return c( 0 ) == ovfl ? maxHeaderSize : minHeaderSize;
    }


    std::size_t size() {        // size of this block
        int ovfl = static_cast<std::size_t>( isUsed() ? chunkState::usedOvfl : chunkState::unusedOvfl );
        int len;
        st_assert( c( 0 ) not_eq static_cast<std::uint8_t>( chunkState::invalid ) and c( 0 ) >= static_cast<std::uint8_t>( chunkState::MaxDistance ), "invalid chunk" );
        if ( c( 0 ) not_eq ovfl ) {
            len = c( 0 ) + 1 - ( isUsed() ? static_cast<std::uint8_t>( chunkState::used ) : static_cast<std::uint8_t>( chunkState::unused ) );
        } else {
            len = ( ( ( n( 1 ) << MaxDistLog ) + n( 2 ) ) << MaxDistLog ) + n( 3 );
        }
        st_assert( len > 0, "negative/zero chunk length" );
        return len;
    }


    bool_t contains( std::uint8_t *p ) {
        return asByte() <= p and p < asByte() + size();
    }


    ChunkKlass *next() {
        return asChunkKlass( asByte() + size() );
    }


    ChunkKlass *prev() {
        ChunkKlass *p = asChunkKlass( asByte() - 1 );
        int ovfl = static_cast<std::size_t>( p->isUsed() ? chunkState::usedOvfl : chunkState::unusedOvfl );
        int len;
        if ( c( -1 ) not_eq ovfl ) {
            len = p->size();
        } else {
            len = ( ( ( n( -4 ) << MaxDistLog ) + n( -3 ) ) << MaxDistLog ) + n( -2 );
        }
        st_assert( len > 0, "negative/zero chunk length" );
        return asChunkKlass( asByte() - len );
    }
};



// -----------------------------------------------------------------------------

class ZoneHeap : public CHeapAllocatedObject {

protected:
    std::size_t size;                   // total size in bytes

public:
    int blockSize;              // allocation unit in bytes (must be power of 2)
    int nfree;                  // number of free lists

protected:
    int log2BS;         // log2(blockSize)
    int _bytesUsed;     // used bytes (rounded to block size)
    int _total;         // total bytes allocated so far
    int _ifrag;         // bytes wasted by internal fragmentation
    const char *_base;         // for deallocation
    const char *base;          // base addr of heap (aligned to block size)

    ChunkKlass *_heapKlass;     // map of heap (1 byte / block)
    FreeList   *_freeList;      // array of free lists for different chunk sizes
    FreeList   *_bigList;       // list of all big free blocks
    ChunkKlass *_lastCombine;   // result of last block combination

public:
    ZoneHeap *_newHeap;                 // only set when growing a heap (i.e. replacing it)
    bool_t _combineOnDeallocation;    // do eager block combination on deallocs?

public:
    ZoneHeap( int s, int bs );

    ~ZoneHeap();

    // Initializes the Heap
    void clear();

    // Allocation
    void *allocate( int wantedBytes );    // returns nullptr if allocation failed
    void deallocate( void *p, int bytes );

    // Compaction
    const char *compact( void move( const char *from, char *to, int nbytes ) );    // returns first free byte

    // Sizes
    int capacity() const {
        return size;
    }


    int usedBytes() const {
        return _bytesUsed;
    }


    int freeBytes() const {
        return size - _bytesUsed;
    }


    // Fragmentation
    double intFrag() const {
        return usedBytes() ? (float) _ifrag / usedBytes() : 0;
    }


    double extFrag() const {
        return usedBytes() ? 1.0 - (float) usedBytes() / capacity() : 0;
    }


    // Location
    const char *startAddr() const {
        return base;
    }


    const char *endAddr() const {
        return startAddr() + capacity();
    }


    // Queries
    bool_t contains( const void *p ) const;

    const void *firstUsed() const; // Address of first used object
    const void *nextUsed( const void *prev ) const;
    const void *findStartOfBlock( const void *start ) const;
    std::size_t sizeOfBlock( void *nm ) const;

    // Misc.
    void verify() const;

    void print() const;

protected:
    int mapSize() const {
        return size >> log2BS;
    }


    const char *blockAddr( ChunkKlass *m ) const {
        std::uint8_t *fm = (std::uint8_t *) _heapKlass;
        std::uint8_t *bm = (std::uint8_t *) m;
        st_assert( bm >= fm and bm < fm + mapSize(), "not a heapKlass entry" );
        return base + ( ( bm - fm ) << log2BS );
    }


    ChunkKlass *mapAddr( const void *p ) const {
        const char *pp = (const char *) p;
        st_assert( pp >= base and pp < base + size, "not in this heap" );
        st_assert( int(pp) % blockSize == 0, "must be block-aligned" );
        std::uint8_t *fm = (std::uint8_t *) _heapKlass;
        return (ChunkKlass *) ( fm + ( ( pp - base ) >> log2BS ) );
    }


    ChunkKlass *heapEnd() const {
        return (ChunkKlass *) ( (std::uint8_t *) _heapKlass + mapSize() );
    }


    // Free list management
    void *allocFromLists( int wantedBytes );

    bool_t addToFreeList( ChunkKlass *m );

    void removeFromFreeList( ChunkKlass *m );

    int combineAll();

    int combine( HeapChunk *&m );
};
