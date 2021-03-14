//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "allocation.hpp"


// remembered set for GC, implemented as a card-marking byte array with one byte per card
// Card size is 512 bytes
// NB: Card size must be >= 512 because of the offset_array in oldspace

const std::int32_t card_shift        = 9;            // wired in to scavenge_contents
const std::int32_t card_size         = 1 << card_shift;
const std::int32_t card_size_in_oops = card_size / OOP_SIZE;

class RememberedSet : public CHeapAllocatedObject {
    friend class OldSpace;

    friend class OldGeneration;

    friend class SetOopClosure;

private:
    const char *_lowBoundary;       // duplicate of old_gen var so byte_for can be inlined
    const char *_highBoundary;      //
    char       _byteMap[1];         // size is a lie XXX XXX

    // friend void OldSpace::switch_pointers_by_card(Oop, Oop);
    char *byte_for( const void *p ) const {
        return (char *) &_byteMap[ ( ( (std::uint32_t) p ) >> card_shift ) - ( ( (std::uint32_t) _lowBoundary ) >> card_shift ) ];
    }


    //  char* byte_for(void *p) const { return (char*)&byte_map[std::int32_t((char*)p - low_boundary) >> card_shift]; }
    Oop *oop_for( const char *p ) const {
        return (Oop *) ( _lowBoundary + ( ( p - _byteMap ) << card_shift ) );
    }


    friend Oop *card_for( Oop *p ) {
        return (Oop *) ( std::int32_t( p ) & ~( card_size - 1 ) );
    }


    char *byte_map_end() const;

public:
    std::int32_t byte_map_size() const {
        return ( _highBoundary - _lowBoundary ) / card_size;
    }

    RememberedSet();
    RememberedSet( const RememberedSet & ) = default;
    RememberedSet &operator=( const RememberedSet & ) = default;


    void *operator new( std::size_t size );

    void clear();


    char *byte_map_base() const {
        return byte_for( nullptr );
    }


    void record_store( void *p ) {
        *byte_for( p ) = 0;
    }


    bool is_dirty( void *p ) const {
        return *byte_for( p ) == 0;
    }


    // Tells is any card for obj is dirty
    bool is_object_dirty( MemOop obj );

    void scavenge_contents( OldSpace *s );

    char *scavenge_contents( OldSpace *s, char *begin, char *limit );

    bool verify( bool postScavenge );

    void print_set_for_space( OldSpace *sp );

    void print_set_for_object( MemOop obj );

    // Returns the number of dirty pages in an old segment
    std::int32_t number_of_dirty_pages_in( OldSpace *sp );

    // Return the number of pages with dirty objects.
    std::int32_t number_of_pages_with_dirty_objects_in( OldSpace *sp );

    // Operations used during garbage collection
    void set_size( MemOop obj, std::int32_t size );

    std::int32_t get_size( MemOop obj );

private:
    void fixup( const char *start, const char *end );

    void clear( const char *start, const char *end );

    RememberedSet( RememberedSet *old, const char *start, const char *end );

    bool has_page_dirty_objects( OldSpace *sp, char *page );
};
