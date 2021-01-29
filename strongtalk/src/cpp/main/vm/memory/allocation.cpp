
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "allocation.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/runtime/ResourceObject.hpp"


// -----------------------------------------------------------------------------

void PrintableResourceObject::print_short() {
    print();
}


void PrintableCHeapAllocatedObject::print_short() {
    print();
}


void PrintableStackAllocatedObject::print_short() {
    print();
}


void *CHeapAllocatedObject::operator new( std::size_t size ) {
    return (void *) AllocateHeap( size, "operator-new" );
}


void CHeapAllocatedObject::operator delete( void *p ) {
    st_assert( not resources.contains( (const char *) p ), "CHeapAllocatedObject should not be in resource area" );
    FreeHeap( p );
}



// -----------------------------------------------------------------------------

void *StackAllocatedObject::operator new( std::size_t size ) {
    ShouldNotCallThis();
    return nullptr;
}


void StackAllocatedObject::operator delete( void *p ) {
    ShouldNotCallThis();
}


// -----------------------------------------------------------------------------

void *ValueObject::operator new( std::size_t size ) throw() {
    ShouldNotCallThis();
    return nullptr;
}


void ValueObject::operator delete( void *p ) {
    ShouldNotCallThis();
}
