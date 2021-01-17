//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Generation.hpp"
#include "vm/memory/NewGeneration.hpp"
#include "vm/memory/OldGeneration.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/memory/WaterMark.hpp"


void Generation::print() {
    _console->print( " total %6dK, %d%% used ", capacity() / 1024, ( 100 * used() ) / capacity() );
    _console->print_cr( " [%#x, %#x[", _lowBoundary, _highBoundary );
}


void NewGeneration::swap_spaces() {
    eden()->clear();
    from()->clear();
    {
        SurvivorSpace * s = from();
        _fromSpace = to();
        _toSpace   = s;
    }
    eden()->next_space = from();

    from()->set_name( "from" );
    from()->next_space = to();

    to()->set_name( "to" );
    to()->next_space = nullptr;
}


void NewGeneration::initialize( ReservedSpace rs, int eden_size, int surv_size ) {
    int new_size = eden_size + surv_size + surv_size;

    _virtualSpace.initialize( rs, rs.size() );

    const char * eden_start = _virtualSpace.low();
    const char * from_start = eden_start + eden_size;
    const char * to_start   = from_start + surv_size;
    const char * to_end     = to_start + surv_size;

    _fromSpace = new SurvivorSpace();
    _toSpace   = new SurvivorSpace();

    eden()->initialize( "eden", ( Oop * ) eden_start, ( Oop * ) from_start );
    from()->initialize( "from", ( Oop * ) from_start, ( Oop * ) to_start );
    to()->initialize( "to", ( Oop * ) to_start, ( Oop * ) to_end );

    eden()->next_space = from();
    from()->next_space = to();
    to()->next_space   = nullptr;

    _lowBoundary  = _virtualSpace.low_boundary();
    _highBoundary = _virtualSpace.high_boundary();
}


void NewGeneration::prepare_for_compaction( OldWaterMark * mark ) {
    // %note same order as in compact
    from()->prepare_for_compaction( mark );
    eden()->prepare_for_compaction( mark );
}


void NewGeneration::compact( OldWaterMark * mark ) {
    // %note same order as in prepare_for_compaction
    from()->compact( mark );
    from()->clear();
    eden()->compact( mark );
    eden()->clear();
}


Oop * NewGeneration::object_start( Oop * p ) {
    if ( eden()->contains( p ) )
        return eden()->object_start( p );
    return from()->object_start( p );
}


int NewGeneration::capacity() {
    return eden()->capacity() + from()->capacity() + to()->capacity();
}


int NewGeneration::used() {
    return eden()->used() + from()->used() + to()->used();
}


int NewGeneration::free() {
    return eden()->free() + from()->free() + to()->free();
}


void NewGeneration::switch_pointers( Oop f, Oop t ) {
    eden()->switch_pointers( f, t );
    from()->switch_pointers( f, t );
    to()->switch_pointers( f, t );
}


void NewGeneration::print() {
    if ( WizardMode ) {
        _console->print_cr( " New generation" );
        Generation::print();
    }
    eden()->print();
    from()->print();
    if ( WizardMode ) {
        to()->print();
    }
}


void NewGeneration::object_iterate( ObjectClosure * blk ) {
    eden()->object_iterate( blk );
    from()->object_iterate( blk );
}


void NewGeneration::verify() {

    if ( eden()->next_space not_eq _fromSpace or from()->next_space not_eq _toSpace or to()->next_space not_eq nullptr )
        error( "misconnnected spaces in new gen" );

    eden()->verify();
    from()->verify();
    to()->verify();
}


#undef FOR_EACH_OLD_SPACE
// this version used with old_gen
// ensure that you surround the call with {} to prevent s leaking out!
#define FOR_EACH_OLD_SPACE( s ) \
  for ( OldSpace *s = _firstSpace; s not_eq nullptr; s = s->_nextSpace )


void OldGeneration::initialize( ReservedSpace rs, int initial_size ) {

    _virtualSpace.initialize( rs, initial_size );

    _firstSpace = new OldSpace( "old", initial_size );
    _oldSpace   = _currentSpace = _firstSpace;

    _lowBoundary  = _virtualSpace.low_boundary();
    _highBoundary = _virtualSpace.high_boundary();
}


bool_t OldGeneration::contains( void * p ) {
    FOR_EACH_OLD_SPACE( s ) {
        if ( s->contains( p ) )
            return true;
    }
    return false;
}


int OldGeneration::capacity() {
    int sum = 0;
    FOR_EACH_OLD_SPACE( s )sum += s->capacity();
    return sum;
}


int OldGeneration::used() {
    int sum = 0;
    FOR_EACH_OLD_SPACE( s )sum += s->used();
    return sum;
}


int OldGeneration::free() {
    int sum = 0;
    FOR_EACH_OLD_SPACE( s )sum += s->free();
    return sum;
}


void OldGeneration::scavenge_contents_from( OldWaterMark * mark ) {
    mark->_space->scavenge_contents_from( mark );
    while ( mark->_space not_eq _currentSpace ) {
        *mark = mark->_space->_nextSpace->bottom_mark();
        mark->_space->scavenge_contents_from( mark );
    }
}


void OldGeneration::switch_pointers( Oop from, Oop to ) {
    FOR_EACH_OLD_SPACE( space )space->switch_pointers( from, to );
}


int OldGeneration::expand( int size ) {
    return _currentSpace->expand( size );
}


int OldGeneration::shrink( int size ) {
    return _currentSpace->shrink( size );
}


void OldGeneration::prepare_for_compaction( OldWaterMark * mark ) {
    // %note same order as in compact
    FOR_EACH_OLD_SPACE( s )s->prepare_for_compaction( mark );
}


void OldGeneration::compact( OldWaterMark * mark ) {
    // %note same order as in prepare_for_compaction
    FOR_EACH_OLD_SPACE( s )s->compact( mark );
}


void OldGeneration::append_space( OldSpace * last ) {
    _oldSpace->_nextSpace = last;
    _oldSpace = last;
    last->_nextSpace = nullptr;
}


Oop * OldGeneration::allocate_in_next_space( int size ) {
    // Scavenge breaks the there is more than one old Space chunks
    // Fix this with VirtualSpace
    // 4/5/96 Lars
    warning( "Second old Space chunk allocated, this could mean trouble" );
    if ( _currentSpace == _oldSpace ) {
        int space_size = _currentSpace->capacity();
        OldSpace * s = new OldSpace( "old", space_size );

        if ( ( const char * ) s->bottom() < Universe::new_gen._highBoundary ) st_fatal( "allocation of old Space before new Space" );

        append_space( s );
    }
    _currentSpace = _currentSpace->_nextSpace;

    const char * sStart = ( const char * ) _currentSpace->bottom();
    const char * sEnd   = ( const char * ) _currentSpace->end();
    if ( sStart < _lowBoundary )
        _lowBoundary  = sStart;
    if ( sEnd > _highBoundary )
        _highBoundary = sEnd;

    Universe::remembered_set->fixup( sStart, sEnd );
    Universe::current_sizes._old_size = capacity();

    return allocate( size );
}


void OldGeneration::print() {
    if ( WizardMode ) {
        _console->print_cr( " Old generation" );
        Generation::print();
    }
    FOR_EACH_OLD_SPACE( s )s->print();
}


void OldGeneration::print_remembered_set() {
    lprintf( "Remembered set\n" );
    FOR_EACH_OLD_SPACE( s )Universe::remembered_set->print_set_for_space( s );
}


int OldGeneration::number_of_dirty_pages() {
    int count = 0;
    FOR_EACH_OLD_SPACE( s ) {
        count += Universe::remembered_set->number_of_dirty_pages_in( s );
    }
    return count;
}


int OldGeneration::number_of_pages_with_dirty_objects() {
    int count = 0;
    FOR_EACH_OLD_SPACE( s ) {
        count += Universe::remembered_set->number_of_pages_with_dirty_objects_in( s );
    }
    return count;
}


void OldGeneration::object_iterate( ObjectClosure * blk ) {
    FOR_EACH_OLD_SPACE( s )s->object_iterate( blk );
}


void OldGeneration::object_iterate_from( OldWaterMark * mark, ObjectClosure * blk ) {
    mark->_space->object_iterate_from( mark, blk );
    for ( OldSpace * s = mark->_space->_nextSpace; s not_eq nullptr; s = s->_nextSpace ) {
        *mark = s->bottom_mark();
        s->object_iterate_from( mark, blk );
    }
}


void OldGeneration::verify() {
    int n = 0;
    OldSpace * p;
    FOR_EACH_OLD_SPACE( s ) {
        n++;
        p = s;
    }
    if ( p not_eq _oldSpace )
        error( "Wrong last_space in old gen" );
    APPLY_TO_OLD_SPACES( SPACE_VERIFY_TEMPLATE );
}


static int addr_cmp( OldSpace ** s1, OldSpace ** s2 ) {
    const char * s1start = ( const char * ) ( *s1 )->bottom();
    const char * s2start = ( const char * ) ( *s2 )->bottom();
    if ( s1start < s2start )
        return -1;
    else if ( s1start > s2start )
        return 1;
    else
        return 0;
}


Oop * OldGeneration::object_start( Oop * p ) {
    FOR_EACH_OLD_SPACE( s ) {
        if ( s->contains( p ) )
            return s->object_start( p );
    }
    return nullptr;
}
