
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"


// A resource mark releases all resources allocated after it was created when the mark is deleted.  Typically used as a local variable.

class AbstractResourceMark : StackAllocatedObject {
private:
public:
    AbstractResourceMark() = default;
    virtual ~AbstractResourceMark();
    AbstractResourceMark( const AbstractResourceMark & ) = default;
    AbstractResourceMark &operator=( const AbstractResourceMark & ) = default;


    void operator delete( void *ptr ) { (void)ptr; }


};


//
class ResourceArea;

class ResourceAreaChunk;


class ResourceMark : StackAllocatedObject {
public:

    ResourceMark();
    virtual ~ResourceMark();
    ResourceMark( const ResourceMark & ) = default;
    ResourceMark &operator=( const ResourceMark & ) = default;


    void operator delete( void *ptr ) { (void)ptr; }


protected:
    static bool       _enabled;
    ResourceArea      *_resourceArea;
    ResourceAreaChunk *_resourceAreaChunk;
    char              *_top;

};


//
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


    void operator delete( void *p ) {}

};
