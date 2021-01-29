//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "allocation.hpp"
#include "vm/memory/Generation.hpp"
#include "vm/memory/Space.hpp"
#include "vm/memory/WaterMark.hpp"

class OldGeneration : public Generation {
    friend class RememberedSet;

    friend class Universe;

    friend class MarkSweep;

    friend class OldSpace;

    friend class symbolKlass;

    Oop *allocate_in_next_space( std::int32_t size );

private:
    // OldGeneration consists of a linked lists of spaces.
    // [ ] -> [ ] -> [ ] -> [ ] -> [ ]
    //  ^             ^             ^
    // first         current       last

    OldSpace *_firstSpace;
    OldSpace *_currentSpace;
    OldSpace *_oldSpace;

public:
    std::int32_t expand( std::int32_t size );

    std::int32_t shrink( std::int32_t size );


    Oop *allocate( std::int32_t size, bool allow_expansion = true ) {
        return _currentSpace->allocate( size, allow_expansion );
    }


    // called by Universe
    void initialize( ReservedSpace rs, std::int32_t initial_size );

public:
    // Space enquiries
    std::int32_t capacity();

    std::int32_t used();

    std::int32_t free();

    void print();

    void print_remembered_set();

    // Returns the number of dirty pages in old Space.
    // ie. # of pages marked as dirty
    std::int32_t number_of_dirty_pages();

    // Returns the number of pages with dirty objects
    // ie. # of pages with object pointing to new objects.
    std::int32_t number_of_pages_with_dirty_objects();

    void object_iterate( ObjectClosure *blk );

    void object_iterate_from( OldWaterMark *mark, ObjectClosure *blk );

    void verify();

    bool contains( void *p );

    Oop *object_start( Oop *p );


    OldWaterMark top_mark() {
        return _currentSpace->top_mark();
    }


    OldWaterMark bottom_mark() {
        return _firstSpace->bottom_mark();
    }


    OldSpaceMark memo() {
        return OldSpaceMark( _currentSpace );
    }


private:
    void scavenge_contents_from( OldWaterMark *mark );

    void switch_pointers( Oop from, Oop to );

    void switch_pointers_by_card( Oop from, Oop to );

    void sorted_space_list( OldSpace *sp[], std::int32_t (*cmp)( OldSpace **, OldSpace ** ) );

    // phase2 of mark sweep
    void prepare_for_compaction( OldWaterMark *mark );

    // phase3 of mark sweep
    void compact( OldWaterMark *mark );

    void append_space( OldSpace *last );
};
