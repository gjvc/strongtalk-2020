
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#pragma once

#include "vm/runtime/ResourceArea.hpp"

/*
  All classes in the virtual machine must be subclassed by one of the following allocation classes:

   "For objects allocated in the resource area."
   - ResourceObject
     - PrintableResourceObject
*/

/*
   "For objects allocated in the C-heap (managed by malloc & free)."
   - CHeapAllocatedObject
     - PrintableCHeapAllocatedObject

   "For objects allocated on the stack."
   - StackAllocatedObject
     - PrintableStackAllocatedObject

   "For embedded objects."
   - ValueObject

   "For classes used as name spaces."
   - AllStatic

   "The printable subclasses are used for debugging and define virtual
    member functions for printing. Classes that avoid allocating the
    vtbl entries in the objects should therefore not the printable subclasses"
*/


class ResourceObject {

    public:

        void * operator new( std::size_t size, bool_t on_C_heap = false );

        void operator delete( void * p, int ); // use explicit free() to deallocate heap-allocated objects

};


// Base class for objects allocated in the resource area with printing behavior.
class PrintableResourceObject : public ResourceObject {

    public:
        virtual void print() = 0;

        virtual void print_short();
};
