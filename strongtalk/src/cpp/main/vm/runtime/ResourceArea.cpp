//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceArea.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/system/os.hpp"
#include "vm/memory/util.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/system/sizes.hpp"



// -----------------------------------------------------------------------------

// The resource area holds temporary data structures of the VM.
// Things in the resource area can be deallocated very efficiently using ResourceMarks.
// (The destructor of a ResourceMark will deallocate everything that was created since the ResourceMark was created.)

constexpr int min_resource_free_size  = 32 * 1024;
constexpr int min_resource_chunk_size = 256 * 1024;


ResourceAreaChunk::ResourceAreaChunk( int min_capacity, ResourceAreaChunk * previous ) {

    int size = max( min_capacity + min_resource_free_size, min_resource_chunk_size );
    _bottom = ( char * ) AllocateHeap( size, "resourceAreaChunk" );
    _top    = _bottom + size;

//    _console->print_cr( "%ResourceAreaChunk-allocated [0x%08x] ", resources.capacity() );
//    _console->print_cr( "%ResourceAreaChunk-used [0x%08x] ", resources.used() );
//    _console->print_cr( "%ResourceAreaChunk-size [0x%08x] ", size );

    _console->print_cr( "%%ResourceAreaChunk-create size [%d], [%d] used out of [%d] ", size, resources.used(), resources.capacity() );

    initialize( previous );
}


void ResourceAreaChunk::initialize( ResourceAreaChunk * previous ) {

    _firstFree = _bottom;
    _prev      = previous;

    _allocated     = capacity() + ( _prev ? _prev->_allocated : 0 );
    _previous_used = _prev ? ( _prev->_previous_used + used() ) : 0;
}


ResourceAreaChunk::~ResourceAreaChunk() {
    FreeHeap( _bottom );
}


void ResourceAreaChunk::print() {
    if ( _prev )
        _prev->print();
    print_short();
    lprintf( ": _bottom [%#lx], _top [%#lx], _prev [%#lx]\n", _bottom, _top, _prev );
}


void ResourceAreaChunk::print_short() {
    _console->print( "ResourceAreaChunk [%#lx]", this );
}


void ResourceAreaChunk::print_alloc( const char * addr, int size ) {
    _console->print_cr( "allocating %ld bytes at %#lx", size, addr );
}


ResourceArea::ResourceArea() {
    _resourceAreaChunk = nullptr;
    _nestingLevel      = 0;
}


ResourceArea::~ResourceArea() {
    // deallocate all chunks
    ResourceAreaChunk       * prevc;
    for ( ResourceAreaChunk * c = _resourceAreaChunk; c not_eq nullptr; c = prevc ) {
        prevc = c->_prev;
        resources.addToFreeList( c );
    }
}


char * ResourceArea::allocate_more_bytes( int size ) {
    _resourceAreaChunk = resources.new_chunk( size, _resourceAreaChunk );
    char * p = _resourceAreaChunk->allocate_bytes( size );
    st_assert( p, "Nothing returned" );
    return p;
}


int ResourceArea::used() {
    if ( _resourceAreaChunk == nullptr )
        return 0;
    return _resourceAreaChunk->used() + ( _resourceAreaChunk->_prev ? _resourceAreaChunk->_prev->_previous_used : 0 );
}


char * ResourceArea::allocate_bytes( int size ) {

    if ( size < 0 ) {
        st_fatal( "negative size in allocate_bytes" );
    }
    st_assert( size >= 0, "negative size in allocate_bytes" );

    // NB: don't make it a fatal error -- otherwise, if you call certain functions
    // from the debugger, it might report a leak since there might not be a
    // ResourceMark.

    // However, in all other situations, calling allocate_bytes with nesting == 0
    // is a definitive memory leak.  -Urs 10/95

//            static int warned = 0;    // to suppress multiple warnings (e.g. when allocating from the debugger)
//            if (nesting < 1 and not warned++) error("memory leak: allocating w/o ResourceMark!");

    if ( size == 0 ) {
        // want to return an invalid pointer for a zero-sized allocation,
        // but not nullptr, because routines may want to use nullptr for failure.
        // gjvc: but the above reason doesn't make much sense -- a zero-sized allocation is immediately useless.
        return ( char * ) 1;
    }

    size = roundTo( size, oopSize );
    char * p;
    if ( _resourceAreaChunk and ( p = _resourceAreaChunk->allocate_bytes( size ) ) )
        return p;
    return allocate_more_bytes( size );
}


// -----------------------------------------------------------------------------

int Resources::capacity() {
    return _allocated;
}


int Resources::used() {
    return resource_area.used();
}


static bool_t in_rsrc;
static const char * p_rsrc;


bool_t Resources::contains( const char * p ) {
    in_rsrc = false;
    p_rsrc  = p;
    // FIX LATER  processes->processesDo(rsrcf2);
    return in_rsrc;
}


void Resources::addToFreeList( ResourceAreaChunk * c ) {
    if ( ZapResourceArea )
        c->clear();
    c->_prev = freeChunks;
    freeChunks = c;
}


ResourceAreaChunk * Resources::getFromFreeList( int min_capacity ) {
    if ( not freeChunks )
        return nullptr;

    // Handle the first element special
    if ( freeChunks->capacity() >= min_capacity ) {
        ResourceAreaChunk * res = freeChunks;
        freeChunks = freeChunks->_prev;
        return res;
    }

    ResourceAreaChunk * cursor = freeChunks;
    while ( cursor->_prev ) {
        if ( cursor->_prev->capacity() >= min_capacity ) {
            ResourceAreaChunk * res = cursor->_prev;
            cursor->_prev = cursor->_prev->_prev;
            return res;
        }
        cursor = cursor->_prev;
    }

    // No suitable chunk found
    return nullptr;
}


ResourceAreaChunk * Resources::new_chunk( int min_capacity, ResourceAreaChunk * previous ) {

    _in_consistent_state = false;
    ResourceAreaChunk * res = getFromFreeList( min_capacity );
    if ( res ) {
        res->initialize( previous );
    } else {
        res = new ResourceAreaChunk( min_capacity, previous );
        _allocated += res->capacity();
        if ( PrintResourceChunkAllocation ) {
            _console->print_cr( "*allocating new resource area chunk of >=0x%08x bytes, new total = 0x%08x bytes", min_capacity, _allocated );
        }
    }

    _in_consistent_state = true;

    st_assert( res, "just checking" );

    return res;
}


// -----------------------------------------------------------------------------

char * ResourceAreaChunk::allocate_bytes( int size ) {

    char * p = _firstFree;
    if ( _firstFree + size <= _top ) {
        if ( PrintResourceAllocation ) {
            print_alloc( p, size );
        }
        _firstFree += size;
        return p;
    } else {
        return nullptr;
    }

}


void ResourceAreaChunk::freeTo( char * new_first_free ) {
    st_assert( new_first_free <= _firstFree, "unfreeing in resource area" );
    if ( ZapResourceArea )
        clear( new_first_free, _firstFree );
    _firstFree = new_first_free;
}


Resources::Resources() {
    _allocated           = 0;
    _in_consistent_state = true;
}


// -----------------------------------------------------------------------------

NoGCVerifier::NoGCVerifier() {
    old_scavenge_count = Universe::scavengeCount;
}


NoGCVerifier::~NoGCVerifier() {
    if ( old_scavenge_count not_eq Universe::scavengeCount ) {
        warning( "scavenge in a NoGCVerifier secured function" );
    }
}


char * AllocatePageAligned( int size, const char * name ) {
    int page_size = Universe::page_size();
    char * block = ( char * ) align( os::malloc( size + page_size ), page_size );
    if ( PrintHeapAllocation )
        lprintf( "Malloc (page-aligned) %s: 0x%08x = %#lx\n", name, size, block );

    return block;
}


char * AllocateHeap( int size, const char * name ) {
    if ( PrintHeapAllocation )
        lprintf( "Heap %7d %s\n", size, name );
    return ( char * ) os::malloc( size );
}


void FreeHeap( void * p ) {
    os::free( p );
}


char * allocateResource( int size ) {
    return resource_area.allocate_bytes( size );
}


// The global "operator new" should never be called since it will usually initiate a memory leak.
// Use "CHeapAllocatedObject" as the base class of such objects to make it explicit that they're allocated on the C heap.

// Commented out to prevent conflict with dynamically loaded routines.

//void * operator new( size_t size ) {
//    fatal( "should not call global (default) operator new" );
//    return ( void * ) AllocateHeap( size, "global operator new" );
//}


// -----------------------------------------------------------------------------

Resources    resources;
ResourceArea resource_area;
