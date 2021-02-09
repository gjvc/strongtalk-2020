
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Space.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/runtime/flags.hpp"


extern "C" {
Oop *eden_bottom = nullptr;
Oop *eden_top    = nullptr;
Oop *eden_end    = nullptr;
}


void Space::clear() {
    set_top( bottom() );
    // to detect scavenging bugs
    set_oops( bottom(), capacity() / OOP_SIZE, Oop( 1 ) );
}


void Space::switch_pointers( Oop from, Oop to ) {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    // FIX LATER
    st_fatal( "not implemented yet" );
}


void Space::initialize( const char *name, Oop *bottom, Oop *end ) {
    st_assert( Universe::on_page_boundary( bottom ) and Universe::on_page_boundary( end ), "invalid Space boundaries" );

    set_name( name );
    set_bottom( bottom );
    set_top( bottom );
    set_end( end );
}


void Space::prepare_for_compaction( OldWaterMark *mark ) {
    //
    // compute the new addresses for the live objects and update all
    // pointers to these objects.
    // Used by Universe::mark_sweep_phase2()
    // %profiling note:
    //    the garbage collector spends 55% of its time in this function
    //
    Oop    *q         = bottom();
    Oop    *t         = top();
    Oop    *new_top   = mark->_point;
    MemOop first_free = nullptr;
    while ( q < t ) {
        MemOop m = as_memOop( q );
        if ( m->is_gc_marked() ) {
            if ( first_free ) {
                first_free->set_mark( q );
                SPDLOG_INFO( "first_free [0x{0:x}] = first_free->mark() [0x{0:x}], q [0x{0:x}]", static_cast<const void *>(first_free), static_cast<const void *>(first_free->mark()), static_cast<const void *>(q) );
                first_free = nullptr;
            }

            // Reverse the list with the mark at the end
            Oop *root_or_mark = (Oop *) m->mark();
            while ( is_oop_root( root_or_mark ) ) {
                Oop *next = (Oop *) *root_or_mark;
                *root_or_mark = (Oop) as_memOop( new_top );
                root_or_mark = next;
            }
            m->set_mark( MarkOop( root_or_mark ) );

            std::int32_t size = m->gc_retrieve_size(); // The mark has to be restored before
            // the size is retrieved
            new_top += size;
            q += size;
        } else {
            if ( not first_free ) {
                first_free = m;
                SPDLOG_INFO( "First free 0x{0:x}", static_cast<const void *>(q) );
            }
            q += m->size();
        }
    }
    if ( first_free ) {
        first_free->set_mark( q );
        SPDLOG_INFO( "[0x{0:x}] = 0x{0:x}, 0x{0:x}", static_cast<const void *>(first_free), static_cast<const void *>(first_free->mark()), static_cast<const void *>(q) );
    }
    mark->_point = new_top;
}


void Space::compact( OldWaterMark *mark ) {
    // compute the new addresses for the live objects
    // Used by Universe::mark_sweep_phase3()
    // %profiling note:
    //    the garbage collectior spends 23% of its time in this function
    Oop *q       = bottom();
    Oop *t       = top();
    Oop *new_top = mark->_point;

    while ( q < t ) {
        MemOop m = as_memOop( q );
        if ( m->mark()->is_smi() ) {
//            SPDLOG_INFO( "Space::compact()  expand [0x{0:x}] -> [0x{0:x}]", q, *q );
            q = (Oop *) *q;
        } else {
            std::int32_t size = m->gc_retrieve_size();
            // make sure we don't run out of old Space!
            if ( size > mark->_space->end() - new_top )
                mark->_space->expand( size * OOP_SIZE );

            if ( q not_eq new_top ) {
                copy_oops( q, new_top, size );
//                SPDLOG_INFO( "Space::compact()  copy [0x{0:x}] -> [0x{0:x}] (%d)", q, new_top, size );
                st_assert( ( *new_top )->is_mark(), "should be header" );
            }
            mark->_space->update_offsets( new_top, new_top + size );
            q += size;
            new_top += size;
        }
    }
    mark->_point = new_top;
    mark->_space->set_top( new_top );
    set_top( new_top );
}


Oop *NewSpace::object_start( Oop *p ) {
    st_assert ( bottom() <= p and p < top(), "p must be in Space" );
    Oop *q = bottom();
    Oop *t = top();
    while ( q < t ) {
        Oop *prev = q;
        q += as_memOop( q )->size();
        if ( q > p )
            return prev;
    }
    st_fatal( "should never reach this point" );
    return nullptr;
}


void NewSpace::object_iterate_from( NewWaterMark *mark, ObjectClosure *blk ) {
    blk->begin_space( this );
    Oop *p = mark->_point;
    Oop *t = top();
    while ( p < t ) {
        MemOop m = as_memOop( p );
        blk->do_object( m );
        p += m->size();
    }
    mark->_point = p;
    blk->end_space( this );
}


EdenSpace::EdenSpace() {
}


SurvivorSpace::SurvivorSpace() :
    _bottom{ nullptr },
    _end{ nullptr },
    _top{ nullptr } {
}


void SurvivorSpace::scavenge_contents_from( NewWaterMark *mark ) {

#ifdef VERBOSE_SCAVENGING
    SPDLOG_INFO("{scavenge_contents [ 0x{0:x} <= 0x{0:x} <= 0x{0:x}]}", bottom(), mark->_point, top());
#endif

    if ( top() == mark->_point )
        return;

    st_assert( mark->_point < top(), "scavenging past top" );

    Oop *p = mark->_point; // for performance
    Oop *t = top();

    do {
        MemOop m = as_memOop( p );

#ifdef VERBOSE_SCAVENGING
        SPDLOG_INFO("{scavenge 0x{0:x} (0x{0:x})} ", p, m->klass());
        SPDLOG_INFO("{}", m->klass()->name());
        Oop *prev = p;
#endif

        st_assert( ( *p )->is_mark(), "Header should be mark" );
        p += m->scavenge_contents();

#ifdef VERBOSE_SCAVENGING
        if (p - prev not_eq m->size()) {
            fatal("scavenge_contents is not returning the right size");
        }
#endif

    } while ( p < t );
    mark->_point = p;
}


void OldSpace::initialize_threshold() {
    _nextOffsetIndex = 0;
    _offsetArray[ _nextOffsetIndex++ ] = 0;
    _nextOffsetThreshold = _bottom + card_size_in_oops;
}


OldSpace::OldSpace( const char *name, std::int32_t &size ) :
    _end{ nullptr },
    _top{ nullptr },
    _bottom{ nullptr },
    _nextOffsetIndex{ 0 },
    _nextOffsetThreshold{ nullptr },
    _nextSpace{ nullptr },
    _offsetArray{ nullptr } {

    //
    static_cast<void>(size); // unused

    _offsetArray = new_c_heap_array<std::uint8_t>( Universe::old_gen._virtualSpace.reserved_size() / card_size );
    set_name( name );
    set_bottom( (Oop *) Universe::old_gen._virtualSpace.low() );
    set_top( (Oop *) Universe::old_gen._virtualSpace.low() );
    set_end( (Oop *) Universe::old_gen._virtualSpace.high() );
    initialize_threshold();
}


void OldSpace::update_offset_array( Oop *p, Oop *p_end ) {
    st_assert( p_end >= _nextOffsetThreshold, "should be past threshold" );
    //  [    ][    ][   ]       "card pages"
    //    ^p  ^t     ^p_end
    st_assert( ( _nextOffsetThreshold - p ) <= card_size_in_oops, "Offset should be <= card_size_in_oops" );
    _offsetArray[ _nextOffsetIndex++ ] = _nextOffsetThreshold - p;
    _nextOffsetThreshold += card_size_in_oops;
    while ( _nextOffsetThreshold < p_end ) {
        _offsetArray[ _nextOffsetIndex++ ] = card_size_in_oops;
        _nextOffsetThreshold += card_size_in_oops;
    }
}


extern "C" {
std::int32_t expansion_count = 0;
}


std::int32_t OldSpace::expand( std::int32_t size ) {
    std::int32_t min_size    = ReservedSpace::page_align_size( size );
    std::int32_t expand_size = ReservedSpace::align_size( min_size, ObjectHeapExpandSize * 1024 );
    Universe::old_gen._virtualSpace.expand( expand_size );
    set_end( (Oop *) Universe::old_gen._virtualSpace.high() );
    expansion_count++;
    return expand_size;
}


std::int32_t OldSpace::shrink( std::int32_t size ) {
    std::int32_t shrink_size = ReservedSpace::align_size( size, ObjectHeapExpandSize * 1024 );
    if ( shrink_size > free() )
        return 0;
    Universe::old_gen._virtualSpace.shrink( shrink_size );
    set_end( (Oop *) Universe::old_gen._virtualSpace.high() );
    return shrink_size;
}


Oop *OldSpace::expand_and_allocate( std::int32_t size ) {
    expand( size * OOP_SIZE );
    return allocate( size );
}


void OldSpace::scavenge_recorded_stores() {
    Universe::remembered_set->scavenge_contents( this );
}


void OldSpace::scavenge_contents_from( OldWaterMark *mark ) {
    st_assert( this == mark->_space, "Match does not match Space" );
    Oop *p = mark->_point;
    while ( p < _top ) {
        st_assert( Oop(*p)->is_mark(), "must be mark" );
        MemOop x = as_memOop( p );
        p += x->scavenge_tenured_contents();
    }
    st_assert( p == _top, "p should be top" );
    mark->_point = _top;
}


void OldSpace::object_iterate_from( OldWaterMark *mark, ObjectClosure *blk ) {
    blk->begin_space( this );
    Oop *p = mark->_point;
    Oop *t = top();
    while ( p < t ) {
        MemOop m = as_memOop( p );
        blk->do_object( m );
        p += m->size();
    }
    mark->_point = p;
    blk->end_space( this );
}


//void Space::oops_do( oopsDoFn f ) {
//    Oop * p = bottom();
//    Oop * t = top();
//    while ( p < t ) {
//        MemOop m = as_memOop( p );
//        f( ( Oop * ) &m );
//        p += m->size();
//    }
//}


void Space::print() {
    _console->print( "space [%5s] capacity [%6dK] percentage used [%d%%]", name(), capacity() / 1024, used() * 100 / capacity() );
    if ( WizardMode ) {
        _console->print( "from [%#-6lx -> %#-6lx[", bottom(), top() );
    }
    _console->cr();
}


void Space::object_iterate( ObjectClosure *blk ) {
    if ( is_empty() )
        return;
    blk->begin_space( this );
    Oop *p = bottom();
    Oop *t = top();
    while ( p < t ) {
        MemOop m = as_memOop( p );
        blk->do_object( m );
        p += m->size();
    }
    blk->end_space( this );
}


void NewSpace::verify() {
    SPDLOG_INFO( "{}, ", name() );
    Oop *p = bottom();
    Oop *t = top();

    MemOop m;
    while ( p < t ) {
        st_assert( Oop(*p)->is_mark(), "First word must be mark" );
        m = as_memOop( p );
        m->verify();
        p += m->size();
    }
    st_assert( p == top(), "end of last object must match end of Space" );
}


class VerifyOldOopClosure : public OopClosure {

public:
    MemOop _the_obj;


    VerifyOldOopClosure() : _the_obj{} {}
    virtual ~VerifyOldOopClosure() = default;
    VerifyOldOopClosure( const VerifyOldOopClosure & ) = default;
    VerifyOldOopClosure &operator=( const VerifyOldOopClosure & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    void do_oop( Oop *o ) {

        Oop obj = *o;
        if ( !obj->is_new() ) return;

        // Make sure the the_obj is in the remembered set
        if ( Universe::remembered_set->is_object_dirty( _the_obj ) != 0 ) return;

        _console->cr();
        SPDLOG_INFO( "New obj reference found in non dirty page." );
        SPDLOG_INFO( "- object containing the reference:" );
        _the_obj->print();
        SPDLOG_INFO( "- the referred object:" );
        _console->print( "[0x%lx]: 0x%lx = ", o, obj );
        obj->print_value();
        _console->cr();

        Universe::remembered_set->print_set_for_object( _the_obj );
        SPDLOG_WARN( "gc problem" );
    }
};


void OldSpace::verify() {
    //
    SPDLOG_INFO( "{} ", name() );
    Oop                 *p = _bottom;
    MemOop              m;
    VerifyOldOopClosure blk;

    //
    while ( p < _top ) {
        st_assert( Oop(*p)->is_mark(), "First word must be mark" );
        m = as_memOop( p );

        std::int32_t size = m->size();
        st_assert( m == as_memOop( Universe::object_start( p + ( size / 2 ) ) ), "check offset computation" );

        m->verify();
        blk._the_obj = m;
        m->oop_iterate( &blk );
        p += m->size();
    }
    st_assert( p == _top, "end of last object must match end of Space" );
}


Oop *OldSpace::object_start( Oop *p ) {
    // Find the page start
    Oop          *q = p;
    std::int32_t b  = (std::int32_t) q;
    clearBits( b, nthMask( card_shift ) );
    q = (Oop *) b;
    st_assert( contains( q ), "q must be in this Space" );
    std::int32_t index = ( q - bottom() ) / card_size_in_oops;

    std::int32_t offset = _offsetArray[ index-- ];
    while ( offset == card_size_in_oops ) {
        q -= card_size_in_oops;
        offset = _offsetArray[ index-- ];
    }
    q -= offset;
    Oop *n = q;
    st_assert( ( *n )->is_mark(), "check for header" );
    while ( n <= p ) {
        q = n;
        n += as_memOop( n )->size();
    }
    st_assert( as_memOop( q )->mark()->is_mark(), "Must be mark" );
    return q;
}
