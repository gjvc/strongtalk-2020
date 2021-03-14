
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"


/*
  All classes in the virtual machine must be subclassed by one of the following allocation classes:

   For objects allocated in the resource area.
   - ResourceObject
     - PrintableResourceObject

   For objects allocated in the C-heap (managed by: free & malloc).
   - CHeapAllocatedObject
     - PrintableCHeapAllocatedObject

   For objects allocated on the stack.
   - StackAllocatedObject
     - PrintableStackAllocatedObject

   For embedded objects.
   - ValueObject

   For classes used as name spaces.
   - AllStatic

   The printable subclasses are used for debugging and define virtual
    member functions for printing. Classes that avoid allocating the
    vtbl entries in the objects should therefore not the printable subclasses
*/


// Base class for objects used as value objects.
// Calling new or delete will result in fatal error.
class ValueObject {
public:
//    ValueObject() = default;                     //  constructor causes problems
    void *operator new( std::size_t size ) throw(); //  operator new() is ok
    void operator delete( void *p );                //  operator delete() is ok
//    virtual ~ValueObject() = default;               //  destructor causes problems

};


// Base class for classes that constitute name spaces.
class AllStatic {
public:
    void *operator new( std::size_t size ) = delete;

    void operator delete( void *p ) = delete;
};


// Base class for objects allocated by the C runtime.
class CHeapAllocatedObject {
public:
    void *operator new( std::size_t size );

    void operator delete( void *p );

    void *new_array( std::size_t size );

    virtual ~CHeapAllocatedObject() = default;

};


// Base class for objects with printing behavior allocated on the c-heap
class PrintableCHeapAllocatedObject : public CHeapAllocatedObject {
public:
    virtual void print() = 0;

    virtual void print_short();

    virtual ~PrintableCHeapAllocatedObject() = default;
};


// Base class for objects allocated only on the stack.
// Calling new or delete will result in fatal error.
class StackAllocatedObject {
public:
    void *operator new( std::size_t size );

    void operator delete( void *p );

    virtual ~StackAllocatedObject() = default;

};


// Base class for objects with printing behavior allocated only on the stack
class PrintableStackAllocatedObject : StackAllocatedObject {
public:
    virtual void print() = 0;

    virtual void print_short();

    virtual ~PrintableStackAllocatedObject() = default;

};
