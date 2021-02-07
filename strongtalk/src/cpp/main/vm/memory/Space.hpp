
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/memory/WaterMark.hpp"
#include "vm/memory/util.hpp"


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


    Space() : _name{ nullptr } {
    }


    virtual ~Space() {}


    auto operator<=>( const Space & ) const = default;

    Space( const Space & ) = default;

    Space &operator=( const Space & ) = default;

protected:
    virtual void set_bottom( Oop *value ) = 0;

    virtual void set_top( Oop *value ) = 0;

    virtual void set_end( Oop *value ) = 0;

public:
    void initialize( const char *name, Oop *bottom, Oop *end );

    void clear();

public:
    bool is_empty() {
        return used() == 0;
    }


    std::int32_t capacity() {
        return byte_size( bottom(), end() );
    }


    std::int32_t used() {
        return byte_size( bottom(), top() );
    }


    std::int32_t free() {
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


    NewSpace() : Space(), next_space{ nullptr } {}


    auto operator<=>( const NewSpace & ) const = default;

    NewSpace( const NewSpace & ) = default;

    NewSpace &operator=( const NewSpace & ) = default;


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


    bool contains( void *p ) {
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

    EdenSpace( const EdenSpace & ) = default;


    // allocation
    Oop *allocate( std::int32_t size ) {
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


    bool contains( void *p ) {
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

    auto operator<=>( const SurvivorSpace & ) const = default;

    SurvivorSpace( const SurvivorSpace & ) = default;

    SurvivorSpace &operator=( const SurvivorSpace & ) = default;


    // allocation
    Oop *allocate( std::int32_t size ) {
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
    bool would_fit( std::int32_t size ) {
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


    bool contains( void *p ) {
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

    std::int32_t expand( std::int32_t size );

    Oop *expand_and_allocate( std::int32_t size );

    std::int32_t shrink( std::int32_t size );

    // Keeps offset for retrieving object start given a card_page
    std::uint8_t *_offsetArray;
    Oop          *_nextOffsetThreshold;
    std::int32_t _nextOffsetIndex;


    Oop *allocate( std::int32_t size, bool allow_expansion = true ) {
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
    OldSpace( const char *nm, std::int32_t &size );
    auto operator<=>( const OldSpace & ) const = default;

    OldSpace( const OldSpace & ) = default;

    OldSpace &operator=( const OldSpace & ) = default;

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
    Oop *_oldTop;

public:
    OldSpace *_theSpace;


    OldSpaceMark( OldSpace *oldSpace ) :
        _oldTop{ nullptr },
        _theSpace{ oldSpace } {
        _oldTop = _theSpace->top();
    }


    ~OldSpaceMark() {
        _theSpace->set_top( _oldTop );
    }


    auto operator<=>( const OldSpaceMark & ) const = default;

    OldSpaceMark( const OldSpaceMark & ) = default;

    OldSpaceMark &operator=( const OldSpaceMark & ) = default;

};
