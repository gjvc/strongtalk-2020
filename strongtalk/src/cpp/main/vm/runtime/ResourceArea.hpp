
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"

#include <cstring>


class ResourceAreaChunk : public PrintableCHeapAllocatedObject {

private:
    friend class ResourceMark;

    friend class ResourceArea;

    friend class Resources;

    char              *_bottom;
    char              *_top;
    char              *_firstFree;
    ResourceAreaChunk *_prev;

    std::int32_t _allocated;     // Allocated bytes in this and previous chunks.
    std::int32_t _previous_used; // Used bytes in previous chunks.

    void clear( char *start, char *end ) {
        memset( start, 33, end - start );
    }


    void clear() {
        clear( _bottom, _firstFree );
    }


    void freeTo( char *new_first_free );

public:
    char *allocate_bytes( std::int32_t size );

    ResourceAreaChunk( std::int32_t min_capacity, ResourceAreaChunk *previous );

    ~ResourceAreaChunk();

    void initialize( ResourceAreaChunk *previous );


    std::int32_t capacity() {
        return _top - _bottom;
    }


    std::int32_t used() {
        return _firstFree - _bottom;
    }


    bool contains( void *p ) {
        if ( p >= (void *) _bottom and p < (void *) _top )
            return true;
        else if ( _prev )
            return _prev->contains( p );
        else
            return false;
    }


    void print();

    void print_short();

protected:
    void print_alloc( const char *addr, std::int32_t size );
};


class ResourceArea {

public:
    ResourceAreaChunk *_resourceAreaChunk;  // current chunk
    std::int32_t               _nestingLevel;        // current # of nested ResourceMarks (will warn if alloc with nesting == 0)

public:

    ResourceArea();

    ~ResourceArea();

    char *allocate_more_bytes( std::int32_t size );

    char *allocate_bytes( std::int32_t size );


    std::int32_t capacity() {
        return _resourceAreaChunk ? _resourceAreaChunk->_allocated : 0;
    }


    std::int32_t used();


    bool contains( void *p ) {
        return _resourceAreaChunk not_eq nullptr and _resourceAreaChunk->contains( p );
    }
};






// A NoGCVerifier makes sure that between its creation and deletion there are no scavenges.
// Typically used as a local variable.

class NoGCVerifier : StackAllocatedObject {
private:
    std::int32_t old_scavenge_count;
public:
    NoGCVerifier();

    ~NoGCVerifier();
};


class Resources {

private:
    ResourceAreaChunk *freeChunks;          // list of unused chunks
    std::int32_t               _allocated;           // total number of bytes allocated
    bool            _in_consistent_state; //
    ResourceAreaChunk *getFromFreeList( std::int32_t min_capacity );

public:
    Resources();

    ResourceAreaChunk *new_chunk( std::int32_t min_capacity, ResourceAreaChunk *area );

    void addToFreeList( ResourceAreaChunk *c );


    bool in_consistent_state() {
        return _in_consistent_state;
    }


    bool contains( const char *p );

    std::int32_t capacity();

    std::int32_t used();
};


// -----------------------------------------------------------------------------

char *AllocateHeap( std::int32_t size, const char *name );

void FreeHeap( void *p );

char *AllocatePageAligned( std::int32_t size, const char *name );

extern Resources    resources;
extern ResourceArea resource_area;

char *allocateResource( std::int32_t size );

// base class ResourceObject is at the end of the file because it uses ResourceArea Base class for objects allocated in the resource area per default.
// Optionally, objects may be allocated on the C heap with new(true) Foo(...)


// -----------------------------------------------------------------------------

template<typename T>
T *new_resource_array( std::int32_t size ) {
    return reinterpret_cast<T *>( allocateResource( size * sizeof( T ) ));
}


template<typename T>
T *new_c_heap_array( std::int32_t size ) {
    return reinterpret_cast<T *>( malloc( ( size ) * sizeof( T ) ));
}


// -----------------------------------------------------------------------------


// One of the following macros must be used when allocating an array to
// determine which area the array should reside in.
#define NEW_RESOURCE_ARRAY( type, size ) \
    (type*) allocateResource( (size) * sizeof(type))

#define NEW_C_HEAP_ARRAY( type, size ) \
    (type*) malloc( (size) * sizeof(type)); //XSTR(type) " in " __FILE__)

#define NEW_RESOURCE_OBJ( type ) NEW_RESOURCE_ARRAY( type, 1 )
#define NEW_C_HEAP_OBJ( type )   NEW_C_HEAP_ARRAY( type, 1 )
