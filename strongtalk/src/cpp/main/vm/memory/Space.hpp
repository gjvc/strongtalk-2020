
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "allocation.hpp"
#include "vm/memory/util.hpp"
#include "vm/memory/WaterMark.hpp"

class ObjectClosure;

class Space : public CHeapAllocatedObject {

private:
    // instance variable
    const char *_name;

public:
    // invariant: bottom() and end() are on page_size boundaries
    // and:       bottom() <= top() <= end()
    virtual Oop *bottom() = 0;

    virtual Oop *top() = 0;

    virtual Oop *end() = 0;


    const char *name() const {
        return _name;
    }


    void set_name( const char *name ) {
        _name = name;
    }


protected:
    virtual void set_bottom( Oop *value ) = 0;

    virtual void set_top( Oop *value ) = 0;

    virtual void set_end( Oop *value ) = 0;

public:
    void initialize( const char *name, Oop *bottom, Oop *end );

    void clear();

public:
    bool_t is_empty() {
        return used() == 0;
    }


    int capacity() {
        return byte_size( bottom(), end() );
    }


    int used() {
        return byte_size( bottom(), top() );
    }


    int free() {
        return byte_size( top(), end() );
    }


    // MarkSweep support
    // phase2
    void prepare_for_compaction( OldWaterMark *mark );

    // phase3
    void compact( OldWaterMark *mark );

protected:
    // operations
    virtual void verify() = 0;

    // for debugging & fun
//        void oops_do( oopsDoFn f );

public:
    void switch_pointers( Oop from, Oop to );

    void print();

    void object_iterate( ObjectClosure *blk );
};

class NewSpace : public Space {
public:
    NewSpace *next_space;

public:
    Oop *object_start( Oop *p );

    void verify();

    void object_iterate_from( NewWaterMark *mark, ObjectClosure *blk );


    NewWaterMark top_mark() {
        NewWaterMark m;
        m._point = top();
        return m;
    }
};


extern "C" Oop *eden_bottom;
extern "C" Oop *eden_top;
extern "C" Oop *eden_end;

class EdenSpace : public NewSpace {
public:
    Oop *bottom() {
        return eden_bottom;
    }


    Oop *top() {
        return eden_top;
    }


    Oop *end() {
        return eden_end;
    }


    bool_t contains( void *p ) {
        return (Oop *) p >= eden_bottom and (Oop *) p < eden_top;
    }


protected:
    void set_bottom( Oop *value ) {
        eden_bottom = value;
    }


    void set_top( Oop *value ) {
        eden_top = value;
    }


    void set_end( Oop *value ) {
        eden_end = value;
    }


public:
    EdenSpace();


    // allocation
    Oop *allocate( int size ) {
        Oop *oops     = eden_top;
        Oop *oops_end = oops + size;
        if ( oops_end <= eden_end ) {
            eden_top = oops_end;
            return oops;
        } else {
            return nullptr;
        }
    }
};

class SurvivorSpace : public NewSpace {
private:
    Oop *_bottom;
    Oop *_top;
    Oop *_end;

public:
    Oop *bottom() {
        return _bottom;
    }


    Oop *top() {
        return _top;
    }


    Oop *end() {
        return _end;
    }


    bool_t contains( void *p ) {
        return (Oop *) p >= _bottom and (Oop *) p < _top;
    }


protected:
    void set_bottom( Oop *value ) {
        _bottom = value;
    }


    void set_top( Oop *value ) {
        _top = value;
    }


    void set_end( Oop *value ) {
        _end = value;
    }


public:
    SurvivorSpace();


    // allocation
    Oop *allocate( int size ) {
        Oop *oops     = _top;
        Oop *oops_end = oops + size;
        if ( oops_end <= _end ) {
            _top = oops_end;
            return oops;
        } else {
            return nullptr;
        }
    }


    // allocation test
    bool_t would_fit( int size ) {
        return _top + size < _end;
    }


    // Scavenge support
    void scavenge_contents_from( NewWaterMark *mark );
};


class OldSpaceMark;

class OldSpace : public Space {
    friend class Space;

    friend class OldSpaceMark;

private:
    Oop *_bottom;
    Oop *_top;
    Oop *_end;

public:
    Oop *bottom() {
        return _bottom;
    }


    Oop *top() {
        return _top;
    }


    Oop *end() {
        return _end;
    }


    bool_t contains( void *p ) {
        return (Oop *) p >= _bottom and (Oop *) p < _top;
    }


protected:
    void set_bottom( Oop *value ) {
        _bottom = value;
    }


    void set_top( Oop *value ) {
        _top = value;
    }


    void set_end( Oop *value ) {
        _end = value;
    }


public:
    void initialize_threshold();


    inline void update_offsets( Oop *begin, Oop *end ) {
        if ( end >= _nextOffsetThreshold )
            update_offset_array( begin, end );
    }


public:
    OldSpace *_nextSpace;

    Oop *object_start( Oop *p );

    void update_offset_array( Oop *p, Oop *p_end );

    int expand( int size );

    Oop *expand_and_allocate( int size );

    int shrink( int size );

    // Keeps offset for retrieving object start given a card_page
    std::uint8_t *_offsetArray;
    Oop          *_nextOffsetThreshold;
    int _nextOffsetIndex;


    Oop *allocate( int size, bool_t allow_expansion = true ) {
        Oop *p  = _top;
        Oop *p1 = p + size;
        if ( p1 < _end ) {
            _top = p1;
            update_offsets( p, p1 );
            return p;
        } else {
            if ( not allow_expansion )
                return nullptr;
            return expand_and_allocate( size );
        }
    }


    // constructors
    // allocates object Space too; sets size to amount allocated, 0 if none
    OldSpace( const char *nm, int &size );

    void scavenge_contents_from( OldWaterMark *mark );

    void object_iterate_from( OldWaterMark *mark, ObjectClosure *blk );

    void scavenge_recorded_stores();

    void verify();


    OldWaterMark top_mark() {
        OldWaterMark m;
        m._space = this;
        m._point = _top;
        return m;
    }


    OldWaterMark bottom_mark() {
        OldWaterMark m;
        m._space = this;
        m._point = _bottom;
        return m;
    }
};


// This is primarily intended for testing to allow temporary allocations to be reset easily.
class OldSpaceMark : public ValueObject {

private:
    Oop *oldTop;

public:
    OldSpace *theSpace;


    OldSpaceMark( OldSpace *aSpace ) :
            theSpace( aSpace ) {
        oldTop = theSpace->top();
    }


    ~OldSpaceMark() {
        theSpace->set_top( oldTop );
    }
};
