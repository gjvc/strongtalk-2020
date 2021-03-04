//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/Generation.hpp"
#include "vm/memory/Space.hpp"
#include "vm/oop/OopDescriptor.hpp"
#include "vm/memory/WaterMark.hpp"

class NewGeneration : public Generation {

    friend class RememberedSet;

    friend class Universe;

    friend class MarkSweep;

    friend class OopNativeCode;

private:
    EdenSpace     _edenSpace;
    SurvivorSpace *_fromSpace;
    SurvivorSpace *_toSpace;

public:

    NewGeneration() :
        Generation(),
        _edenSpace{},
        _fromSpace{ nullptr },
        _toSpace{ nullptr } {

    }


    auto operator<=>( const NewGeneration & ) const = default;

    NewGeneration( const NewGeneration & ) = default;

    NewGeneration &operator=( const NewGeneration & ) = default;

    virtual ~NewGeneration() = default;


    static void *operator new( std::size_t size ) {
        static_cast<void>(size); // unused

        ShouldNotCallThis();
        return static_cast<void *>(nilObject); // dummy to silence compiler warning
    }


    static void operator delete( void *p ) {
        static_cast<void>(p); // unused

        ShouldNotCallThis();
    }


    EdenSpace *eden() {
        return &_edenSpace;
    }


    SurvivorSpace *from() {
        return _fromSpace;
    }


    SurvivorSpace *to() {
        return _toSpace;
    }


    // Space enquiries
    std::int32_t capacity();

    std::int32_t used();

    std::int32_t free();

    void print();

    void object_iterate( ObjectClosure *blk );

    void verify();


    bool would_fit( std::int32_t size ) {
        return _toSpace->would_fit( size );
    }


    void swap_spaces();


    bool contains( void *p ) {
        return (const char *) p >= _lowBoundary and (const char *) p < _highBoundary;
    }


    Oop *object_start( Oop *p );


    Oop *allocate( std::int32_t size ) {
        return eden()->allocate( size );
    }


    Oop *allocate_in_survivor_space( std::int32_t size ) {
        return _toSpace->allocate( size );
    }


    const char *boundary() {
        return _highBoundary;
    }


protected:
//        inline bool is_new( MemOop p, char *boundary ); // inlined in generation.dcl.h
//        inline bool is_new( Oop p, char *boundary ); // ditto

    inline bool is_new( MemOop p, char *boundary ) {
        return (const char *) p < boundary;
    }


    inline bool is_new( Oop p, char *boundary ) {
        return p->isMemOop() and is_new( MemOop( p ), boundary );
    }


private:
    // called by Universe
    void initialize( ReservedSpace rs, std::int32_t eden_size, std::int32_t surv_size );

    // phase2 of mark sweep
    void prepare_for_compaction( OldWaterMark *mark );

    // phase3 of mark sweep
    void compact( OldWaterMark *mark );

    void switch_pointers( Oop from, Oop to );
};
