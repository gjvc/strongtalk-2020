
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/memory/RememberedSet.hpp"
#include "vm/memory/Space.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/MarkOopDescriptor.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/MarkSweep.hpp"


// values of bytes in byte map: during normal operation (incl. scavenge),
// bytes are either -1 (clean) or 0 (dirty); i.e., the interpreter / compiled code
// clear a byte when storing into that card.
//
// More precisely, the card being marked corresponds to the object start, not
// necessarily to the word being updated (e.g., if the object starts towards the
// end of a card, the updated instance variable may be on the next card).  But for
// objArrays, the precise card is marked so that we don't need to scavenge the
// entire array.
//
// During GC, the bytes are used to remember object sizes, see comment further below
//

// To do list for the remembered set
//
// 1. Optimization suggested by Urs 9/30/95
//    Speed up card scanning by comparing words for frequent case
//    (A profile of Mark Sweep is necessary to make the call).
//
// 2. Handle objArrays in a more efficient way. Scavenge only parts of the objArray with dirty cards.
//    The current implementation is REALLY slow for huge tenured objArrays with few new pointers.

RememberedSet::RememberedSet() {
    _lowBoundary  = Universe::new_gen._lowBoundary;
    _highBoundary = Universe::old_gen._highBoundary;
    clear();
    Set_Byte_Map_Base( byte_for( nullptr ) );
    st_assert( byte_for( _lowBoundary ) == _byteMap, "Checking start of map" );
}


void *RememberedSet::operator new( std::size_t size ) {
    st_assert( ( std::int32_t( Universe::new_gen._lowBoundary ) & ( card_size - 1 ) ) == 0, "new must start at card boundary" );
    st_assert( ( std::int32_t( Universe::old_gen._lowBoundary ) & ( card_size - 1 ) ) == 0, "old must start at card boundary" );
    st_assert( ( std::int32_t( Universe::old_gen._highBoundary ) & ( card_size - 1 ) ) == 0, "old must end at card boundary" );
    st_assert( card_size >= 512, "card_size must be at least 512" );
    std::int32_t bmsize = ( Universe::old_gen._highBoundary - Universe::new_gen._lowBoundary ) / card_size;

    return AllocateHeap( size + bmsize, "RememberedSet" );
}


// copy the bits from an older, smaller bitmap, add area [start,end)
RememberedSet::RememberedSet( RememberedSet *old, const char *start, const char *end ) {
    static_cast<void>(old); // unused
    static_cast<void>(start); // unused
    static_cast<void>(end); // unused

    ShouldNotReachHere();
    /*
    low_boundary = Universe::new_gen.low_boundary;
    high_boundary = Universe::old_gen.high_boundary;
    char *old_low = old->low_boundary;
    char *old_high = old->high_boundary;
    Set_Byte_Map_Base( byte_for( nullptr ) );
    memcpy( byte_for( old_low ), old->byte_for( old_low ), old->byte_for( old_high ) - old->byte_for( old_low ) );
    clear( byte_for( start ), byte_for( end ) );
    delete old;
    */
}


char *RememberedSet::scavenge_contents( OldSpace *sp, char *begin, char *limit ) {

    // make sure we are staring with a dirty page
    st_assert( !*begin, "check for dirty page" );

    // Find object at page start
    Oop *s = oop_for( begin );

    // Return if we're at the end.
    if ( s >= sp->top() )
        return begin + 1;

    s = sp->object_start( s );
    char *end = begin + 1;

    Oop *object_end = nullptr;
    while ( !*end and end < limit ) {
        while ( !*end and end < limit )
            end++;

        // We now have a string of dirty pages [begin..end[
        Oop *e = min( oop_for( end ), (Oop *) sp->top() );

        if ( e < (Oop *) sp->top() ) {
            // Find the object crossing the last dirty page
            object_end = sp->object_start( e );
            if ( object_end not_eq e ) {
                // object starts on page boundary
                std::int32_t size = as_memOop( object_end )->size();
                object_end += size;
            }

            end = byte_for( object_end );
        }
    }

    // Clear the cards
    for ( char *i = begin; i < end; i++ )
        *i = -1;

    // Find the end
    Oop *e = min( oop_for( end ), (Oop *) sp->top() );

    while ( s < e ) {
        MemOop       m    = as_memOop( s );
        std::int32_t size = m->scavenge_tenured_contents();
        st_assert( size = m->size(), "just checking" );
        s += size;
    }

    return end;
}


void RememberedSet::scavenge_contents( OldSpace *sp ) {
    char *current_byte = byte_for( sp->bottom() );
    char *end_byte     = byte_for( sp->top() );
    // set sentinel for scan (dirty page)
    *( end_byte + 1 ) = 0;

    // scan over clean pages
    while ( *current_byte )
        current_byte++;

    while ( current_byte <= end_byte ) {
        // Pass the dirty page on to scavenge_contents
        current_byte = scavenge_contents( sp, current_byte, end_byte );

        // scan over clean pages
        while ( *current_byte )
            current_byte++;
    }
}


void RememberedSet::print_set_for_space( OldSpace *sp ) {
    char *current_byte = byte_for( sp->bottom() );
    char *end_byte     = byte_for( sp->top() );
    spdlog::info( "%s: [0x{0:x}, 0x{0:x}]", sp->name(), current_byte, end_byte );
    while ( current_byte <= end_byte ) {
        if ( *current_byte ) {
            spdlog::info( "_" );
        } else {
            spdlog::info( "*" );
        }
        current_byte++;
    }
    spdlog::info( "" );
}


std::int32_t RememberedSet::number_of_dirty_pages_in( OldSpace *sp ) {
    std::int32_t count         = 0;
    char         *current_byte = byte_for( sp->bottom() );
    char         *end_byte     = byte_for( sp->top() );
    while ( current_byte <= end_byte ) {
        if ( !*current_byte )
            count++;
        current_byte++;
    }
    return count;
}


class CheckDirtyClosure : public OopClosure {
public:
    bool is_dirty;


    void clear() {
        is_dirty = false;
    }


    void do_oop( Oop *o ) {
        if ( ( *o )->is_new() ) {
            is_dirty = true;
            /*
            { FlagSetting fs(PrintObjectID, false);
              _console->print("0x%lx ", o);
              (*o)->print_value();
              _console->cr();
            }
            */
        }
    }
};


bool RememberedSet::has_page_dirty_objects( OldSpace *sp, char *page ) {
    // Find object at page start

    Oop *s = sp->object_start( oop_for( page ) );
    // Find the end
    Oop *e = min( oop_for( page + 1 ), (Oop *) sp->top() );

    CheckDirtyClosure blk;

    while ( s < e ) {
        MemOop m = as_memOop( s );
        blk.clear();
        m->oop_iterate( &blk );
        if ( blk.is_dirty )
            return true;
        s += m->size();
    }
    return false;
}


std::int32_t RememberedSet::number_of_pages_with_dirty_objects_in( OldSpace *sp ) {
    std::int32_t count         = 0;
    char         *current_byte = byte_for( sp->bottom() );
    char         *end_byte     = byte_for( sp->top() );
    while ( current_byte <= end_byte ) {
        if ( has_page_dirty_objects( sp, current_byte ) )
            count++;
        current_byte++;
    }
    return count;
}


void RememberedSet::print_set_for_object( MemOop obj ) {
    _console->print( "Remember set for 0x%lx ", obj );
    obj->print_value();
    _console->cr();
    if ( obj->is_new() ) {
        spdlog::info( " object is in new Space!" );
    } else {
        _console->sp();
        char *current_byte = byte_for( obj->addr() );
        char *end_byte     = byte_for( obj->addr() + obj->size() );
        while ( current_byte <= end_byte ) {
            if ( *current_byte ) {
                _console->print( "_" );
            } else {
                _console->print( "*" );
            }
            current_byte++;
        }
        _console->cr();
    }
}


bool RememberedSet::is_object_dirty( MemOop obj ) {
    st_assert( not obj->is_new(), "just checking" );
    char *current_byte = byte_for( obj->addr() );
    char *end_byte     = byte_for( obj->addr() + obj->size() );
    while ( current_byte <= end_byte ) {
        if ( *current_byte == 0 )
            return true;
        current_byte++;
    }
    return false;
}


void RememberedSet::clear( const char *start, const char *end ) {
    std::int32_t *from = (std::int32_t *) start;
    std::int32_t count = (std::int32_t *) end - from;
    set_words( from, count, AllBitsSet );
}


bool RememberedSet::verify( bool postScavenge ) {
    static_cast<void>(postScavenge); // unused
    return true;
}

// Scheme for storing the size of objects during pointer-reversal phase of GC.
// (Can't get object size via klass because pointers are reversed! :-)
// size must be >= 128.
// 1. byte     Size            Size
// [0..128[ -> [128..256[
// 128      -> 1 extra byte   [256      .. 256   + 2^8 [
// 129      -> 2 extra bytes  [512      .. 512   + 2^16[
// 130      -> 4 extra bytes  [66048    ..         2^32[

constexpr std::int32_t lim_0 = MarkOopDescriptor::max_age;
constexpr std::int32_t lim_1 = ( 1 << 8 );
constexpr std::int32_t lim_2 = lim_1 + ( 1 << 8 );
constexpr std::int32_t lim_3 = lim_2 + ( 1 << 16 );


void RememberedSet::set_size( MemOop obj, std::int32_t size ) {
    std::uint8_t *p = (std::uint8_t *) byte_for( obj->addr() );
    st_assert( size >= lim_0, "size must be >= max_age" );

    if ( size < lim_1 ) {        // use 1 byte
        *p = (std::uint8_t) ( size - lim_0 );
    } else if ( size < lim_2 ) {    // use 1 + 1 bytes
        *p++ = lim_0 + 2;
        *p   = (std::uint8_t) ( size - lim_1 );
    } else if ( size < lim_3 ) {    // use 1 + 2 bytes
        *p++                 = lim_0 + 3;
        *(std::uint16_t *) p = (std::uint16_t) ( size - lim_2 );
    } else {            // use 1 + 4 bytes
        *p++                 = lim_0 + 4;
        *(std::uint32_t *) p = (std::uint32_t) ( size - lim_3 );
    }
}


std::int32_t RememberedSet::get_size( MemOop obj ) {
    std::uint8_t *p = (std::uint8_t *) byte_for( obj->addr() );
    std::uint8_t h  = *p++;
    if ( h <= lim_0 + 1 )
        return h + lim_0;
    if ( h == lim_0 + 2 )
        return ( *(std::uint8_t *) p ) + lim_1;
    if ( h == lim_0 + 3 )
        return ( *(std::uint16_t *) p ) + lim_2;
    if ( h == lim_0 + 4 )
        return ( *(std::uint32_t *) p ) + lim_3;
    ShouldNotReachHere();
    return 0;
}


// new old Space added; fix the cards
void RememberedSet::fixup( const char *start, const char *end ) {
    if ( end > _highBoundary ) {
        Universe::remembered_set = new RememberedSet( this, start, end );
    } else {
        clear( byte_for( start ), byte_for( end ) );
    }
}


char *RememberedSet::byte_map_end() const {
    return byte_for( Universe::old_gen._highBoundary );
}


void RememberedSet::clear() {
    clear( _byteMap, byte_map_end() );
}
