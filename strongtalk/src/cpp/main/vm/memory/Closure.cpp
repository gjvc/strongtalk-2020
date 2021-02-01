
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/memory/Closure.hpp"


// -----------------------------------------------------------------------------

void ObjectClosure::begin_space( Space *s ) {
    static_cast<void>(s); // unused
}


void ObjectClosure::end_space( Space *s ) {
    static_cast<void>(s); // unused
}


void ObjectClosure::do_object( MemOop obj ) {
    static_cast<void>(obj); // unused
}


// -----------------------------------------------------------------------------

void FilteredObjectClosure::do_object( MemOop obj ) {
    if ( include_object( obj ) ) {
        do_filtered_object( obj );
    }
}


bool FilteredObjectClosure::include_object( MemOop obj ) {
    static_cast<void>(obj); // unused
    return true;
}


void FilteredObjectClosure::do_filtered_object( MemOop obj ) {
    static_cast<void>(obj); // unused
}


// -----------------------------------------------------------------------------

void ObjectLayoutClosure::do_oop( const char *title, Oop *o ) {
    static_cast<void>(title); // unused
    static_cast<void>(o); // unused
}


void ObjectLayoutClosure::do_mark( MarkOop *m ) {
    static_cast<void>(m); // unused
}


void ObjectLayoutClosure::do_byte( const char *title, std::uint8_t *b ) {
    static_cast<void>(title); // unused
    static_cast<void>(b); // unused
}


void ObjectLayoutClosure::do_long( const char *title, void **p ) {
    static_cast<void>(title); // unused
    static_cast<void>(p); // unused
}


void ObjectLayoutClosure::do_double( const char *title, double *d ) {
    static_cast<void>(title); // unused
    static_cast<void>(d); // unused
}


void ObjectLayoutClosure::begin_indexables() {
}


void ObjectLayoutClosure::end_indexables() {
}


void ObjectLayoutClosure::do_indexable_oop( std::int32_t index, Oop *o ) {
    static_cast<void>(index); // unused
    static_cast<void>(o); // unused
}


void ObjectLayoutClosure::do_indexable_byte( std::int32_t index, std::uint8_t *b ) {
    static_cast<void>(index); // unused
    static_cast<void>(b); // unused
}


void ObjectLayoutClosure::do_indexable_doubleByte( std::int32_t index, std::uint16_t *b ) {
    static_cast<void>(index); // unused
    static_cast<void>(b); // unused
}


void ObjectLayoutClosure::do_indexable_long( std::int32_t index, std::int32_t *l ) {
    static_cast<void>(index); // unused
    static_cast<void>(l); // unused
}


// -----------------------------------------------------------------------------

void FrameClosure::begin_process( Process *p ) {
    static_cast<void>(p); // unused
}


void FrameClosure::end_process( Process *p ) {
    static_cast<void>(p); // unused
}


void FrameClosure::do_frame( Frame *f ) {
    static_cast<void>(f); // unused
}


// -----------------------------------------------------------------------------

void FrameLayoutClosure::do_stack( std::int32_t index, Oop *o ) {
    static_cast<void>(index); // unused
    static_cast<void>(o); // unused
}


void FrameLayoutClosure::do_hp( std::uint8_t **hp ) {
    static_cast<void>(hp); // unused
}


void FrameLayoutClosure::do_receiver( Oop *o ) {
    static_cast<void>(o); // unused
}


void FrameLayoutClosure::do_link( std::int32_t **fp ) {
    static_cast<void>(fp); // unused
}


void FrameLayoutClosure::do_return_addr( const char **pc ) {
    static_cast<void>(pc); // unused
}


// -----------------------------------------------------------------------------

void ProcessClosure::do_process( DeltaProcess *p ) {
    static_cast<void>(p); // unused
}


void OopClosure::do_oop( Oop *o ) {
    static_cast<void>(o); // unused
}


void klassOopClosure::do_klass( KlassOop klass ) {
    static_cast<void>(klass); // unused
}
