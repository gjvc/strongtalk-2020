
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/memory/Space.hpp"


// The following classes are C++ "closures" for iterating over Objects, Roots, and Frames.
// A Closure is used for iterating over data structures; use it instead of function pointers.


template<typename T>
class Closure : StackAllocatedObject {

public:
    virtual void do_it( T t ) = 0;

    Closure() = default;
    virtual ~Closure() = default;
    auto operator<=>( const Closure & ) const = default;

    Closure( const Closure & ) = default;

    Closure &operator=( const Closure & ) = default;


    void operator delete( void *p ) {}

};


// ObjectClosure is used for iterating through object Space (see Universe::object_iterate).
class ObjectClosure : StackAllocatedObject {

public:
    ObjectClosure() = default;
    virtual ~ObjectClosure() = default;
    auto operator<=>( const ObjectClosure & ) const = default;

    ObjectClosure( const ObjectClosure & ) = default;

    ObjectClosure &operator=( const ObjectClosure & ) = default;


    void operator delete( void *p ) {}


    // Called when entering a Space.
    virtual void begin_space( Space *s );


    // Called when exiting a Space.
    virtual void end_space( Space *s );


    // Called for each object.
    virtual void do_object( MemOop obj );
};


// FilteredObjectClosure is an ObjectClosure with filtering.
class FilteredObjectClosure : public ObjectClosure {

private:
    void do_object( MemOop obj );

public:

    FilteredObjectClosure() = default;
    virtual ~FilteredObjectClosure() = default;
    auto operator<=>( const FilteredObjectClosure & ) const = default;

    FilteredObjectClosure( const FilteredObjectClosure & ) = default;

    FilteredObjectClosure &operator=( const FilteredObjectClosure & ) = default;

    // Called for each object and returns whether do_filtered_objects should be called.
    virtual bool include_object( MemOop obj );


    // Called for each object where include_object returns true.
    virtual void do_filtered_object( MemOop obj );
};


// ObjectLayoutClosure is a closure for iterating through the layout of a MemOop (see MemOop::layout_iterate).
class ObjectLayoutClosure : StackAllocatedObject {

public:

    ObjectLayoutClosure() = default;
    virtual ~ ObjectLayoutClosure() = default;
    auto operator<=>( const ObjectLayoutClosure & ) const = default;

    ObjectLayoutClosure( const ObjectLayoutClosure & ) = default;

    ObjectLayoutClosure &operator=( const ObjectLayoutClosure & ) = default;

    // NON-INDEXABLE PART
    // Called for the markOop
    virtual void do_mark( MarkOop *m );


    // Called for each Oop
    virtual void do_oop( const char *title, Oop *o );


    // Called for each byte
    virtual void do_byte( const char *title, std::uint8_t *b );


    // Called for each std::int32_t
    virtual void do_long( const char *title, void **p );


    // Called for each double
    virtual void do_double( const char *title, double *d );


    // INDEXABLE PART
    // Called before iterating through the indexable part.
    // ONLY called if the object has an indexable part.
    virtual void begin_indexables();


    // Called after iterating through the indexable part.
    // ONLY called if the object has an indexable part.
    virtual void end_indexables();


    // Called for each indexable Oop
    virtual void do_indexable_oop( std::int32_t index, Oop *o );


    // Called for each indexable byte
    virtual void do_indexable_byte( std::int32_t index, std::uint8_t *b );


    // Called for each indexable double byte
    virtual void do_indexable_doubleByte( std::int32_t index, std::uint16_t *b );


    // Called for each indexable std::int32_t
    virtual void do_indexable_long( std::int32_t index, std::int32_t *l );
};


class Process;

class Frame;

// A FrameClosure is used for iterating though frames
class FrameClosure : StackAllocatedObject {
public:
    FrameClosure() = default;
    virtual ~FrameClosure() = default;
    auto operator<=>( const FrameClosure & ) const = default;

    FrameClosure( const FrameClosure & ) = default;

    FrameClosure &operator=( const FrameClosure & ) = default;
    void operator delete( void *p ) {}


public:
    // Called before iterating through a process.
    virtual void begin_process( Process *p );


    // Called after iterating through a process.
    virtual void end_process( Process *p );


    // Called for each frame
    virtual void do_frame( Frame *f );
};


// A FrameLayoutClosure is used for iterating though the layout of frame
class FrameLayoutClosure : StackAllocatedObject {
public:
    FrameLayoutClosure() = default;
    virtual ~FrameLayoutClosure() = default;
    auto operator<=>( const FrameLayoutClosure & ) const = default;

    FrameLayoutClosure( const FrameLayoutClosure & ) = default;

    FrameLayoutClosure &operator=( const FrameLayoutClosure & ) = default;

    // Called for each Oop
    virtual void do_stack( std::int32_t index, Oop *o );


    // Called for the hcode pointer
    virtual void do_hp( std::uint8_t **hp );


    // Called for the receiver
    virtual void do_receiver( Oop *o );


    // Called for the link
    virtual void do_link( std::int32_t **fp );


    // Called for the return address
    virtual void do_return_addr( const char **pc );
};


class DeltaProcess;

// A ProcessClosure is used for iterating over Delta processes
class ProcessClosure : public StackAllocatedObject {
public:
    // Called for each process
    virtual void do_process( DeltaProcess *p );
};


// A OopClosure is used for iterating through oops (see MemOop::oop_iterate, Universe::root_iterate).
class OopClosure : StackAllocatedObject {
public:
    OopClosure() = default;
    virtual ~OopClosure() = default;
    auto operator<=>( const OopClosure & ) const = default;

    OopClosure( const OopClosure & ) = default;

    OopClosure &operator=( const OopClosure & ) = default;


    void operator delete( void *p ) {}


    // Called for each Oop
    virtual void do_oop( Oop *o );
};


// A klassOopClosure is used for iterating through klassOops (see MemOop::oop_iterate, Universe::root_iterate).
class klassOopClosure : StackAllocatedObject {
public:
    klassOopClosure() = default;
    virtual ~klassOopClosure() = default;
    auto operator<=>( const klassOopClosure & ) const = default;

    klassOopClosure( const klassOopClosure & ) = default;

    klassOopClosure &operator=( const klassOopClosure & ) = default;


    void operator delete( void *p ) {}


    // Called for each Oop
    virtual void do_klass( KlassOop klass );
};
