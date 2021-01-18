//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"

class ResourceArea;

class ResourceAreaChunk;


// A resource mark releases all resources allocated after it was created when the mark is deleted.  Typically used as a local variable.

class AbstractResourceMark : StackAllocatedObject {

};


class ResourceMark : StackAllocatedObject {

protected:
    static bool_t _enabled;
    ResourceArea      *_resourceArea;
    ResourceAreaChunk *_resourceAreaChunk;
    char              *_top;

public:
    ResourceMark();

    ~ResourceMark();
};


class HeapResourceMark : public CHeapAllocatedObject, public ResourceMark {
public:
    HeapResourceMark() :
            ResourceMark() {
    };


    void *operator new( std::size_t size ) {
        return CHeapAllocatedObject::operator new( size );
    }


    void operator delete( void *p ) {
        CHeapAllocatedObject::operator delete( p );
    }
};


class FinalResourceMark : public ResourceMark {
public:
    FinalResourceMark();

    ~FinalResourceMark();
};
