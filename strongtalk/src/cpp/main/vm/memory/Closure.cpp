
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/memory/Closure.hpp"


// -----------------------------------------------------------------------------

void ObjectClosure::begin_space( Space *s ) {
    st_unused( s ); // unused
}


void ObjectClosure::end_space( Space *s ) {
    st_unused( s ); // unused
}


void ObjectClosure::do_object( MemOop obj ) {
    st_unused( obj ); // unused
}


// -----------------------------------------------------------------------------

void FilteredObjectClosure::do_object( MemOop obj ) {
    if ( include_object( obj ) ) {
        do_filtered_object( obj );
    }
}


bool FilteredObjectClosure::include_object( MemOop obj ) {
    st_unused( obj ); // unused
    return true;
}


void FilteredObjectClosure::do_filtered_object( MemOop obj ) {
    st_unused( obj ); // unused
}


// -----------------------------------------------------------------------------

void ObjectLayoutClosure::do_oop( const char *title, Oop *o ) {
    st_unused( title ); // unused
    st_unused( o ); // unused
}


void ObjectLayoutClosure::do_mark( MarkOop *m ) {
    st_unused( m ); // unused
}


void ObjectLayoutClosure::do_byte( const char *title, std::uint8_t *b ) {
    st_unused( title ); // unused
    st_unused( b ); // unused
}


void ObjectLayoutClosure::do_long( const char *title, void **p ) {
    st_unused( title ); // unused
    st_unused( p ); // unused
}


void ObjectLayoutClosure::do_double( const char *title, double *d ) {
    st_unused( title ); // unused
    st_unused( d ); // unused
}


void ObjectLayoutClosure::begin_indexables() {
}


void ObjectLayoutClosure::end_indexables() {
}


void ObjectLayoutClosure::do_indexable_oop( std::int32_t index, Oop *o ) {
    st_unused( index ); // unused
    st_unused( o ); // unused
}


void ObjectLayoutClosure::do_indexable_byte( std::int32_t index, std::uint8_t *b ) {
    st_unused( index ); // unused
    st_unused( b ); // unused
}


void ObjectLayoutClosure::do_indexable_doubleByte( std::int32_t index, std::uint16_t *b ) {
    st_unused( index ); // unused
    st_unused( b ); // unused
}


void ObjectLayoutClosure::do_indexable_long( std::int32_t index, std::int32_t *l ) {
    st_unused( index ); // unused
    st_unused( l ); // unused
}


// -----------------------------------------------------------------------------

void FrameClosure::begin_process( Process *p ) {
    st_unused( p ); // unused
}


void FrameClosure::end_process( Process *p ) {
    st_unused( p ); // unused
}


void FrameClosure::do_frame( Frame *f ) {
    st_unused( f ); // unused
}


// -----------------------------------------------------------------------------

void FrameLayoutClosure::do_stack( std::int32_t index, Oop *o ) {
    st_unused( index ); // unused
    st_unused( o ); // unused
}


void FrameLayoutClosure::do_hp( std::uint8_t **hp ) {
    st_unused( hp ); // unused
}


void FrameLayoutClosure::do_receiver( Oop *o ) {
    st_unused( o ); // unused
}


void FrameLayoutClosure::do_link( std::int32_t **fp ) {
    st_unused( fp ); // unused
}


void FrameLayoutClosure::do_return_addr( const char **pc ) {
    st_unused( pc ); // unused
}


// -----------------------------------------------------------------------------

void ProcessClosure::do_process( DeltaProcess *p ) {
    st_unused( p ); // unused
}


void OopClosure::do_oop( Oop *o ) {
    st_unused( o ); // unused
}


void klassOopClosure::do_klass( KlassOop klass ) {
    st_unused( klass ); // unused
}
