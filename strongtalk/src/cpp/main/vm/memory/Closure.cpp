//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/memory/Closure.hpp"


// -----------------------------------------------------------------------------

void ObjectClosure::begin_space( Space * s ) {
}


void ObjectClosure::end_space( Space * s ) {
}


void ObjectClosure::do_object( MemOop obj ) {
}


// -----------------------------------------------------------------------------

void FilteredObjectClosure::do_object( MemOop obj ) {
    if ( include_object( obj ) )
        do_filtered_object( obj );
}


bool_t FilteredObjectClosure::include_object( MemOop obj ) {
    return true;
}


void FilteredObjectClosure::do_filtered_object( MemOop obj ) {
}


// -----------------------------------------------------------------------------

void ObjectLayoutClosure::do_oop( const char * title, Oop * o ) {
}


void ObjectLayoutClosure::do_mark( MarkOop * m ) {
}


void ObjectLayoutClosure::do_byte( const char * title, uint8_t * b ) {
}


void ObjectLayoutClosure::do_long( const char * title, void ** p ) {
}


void ObjectLayoutClosure::do_double( const char * title, double * d ) {
}


void ObjectLayoutClosure::begin_indexables() {
}


void ObjectLayoutClosure::end_indexables() {
}


void ObjectLayoutClosure::do_indexable_oop( int index, Oop * o ) {
}


void ObjectLayoutClosure::do_indexable_byte( int index, uint8_t * b ) {
}


void ObjectLayoutClosure::do_indexable_doubleByte( int index, uint16_t * b ) {
}


void ObjectLayoutClosure::do_indexable_long( int index, int32_t * l ) {
}


// -----------------------------------------------------------------------------

void FrameClosure::begin_process( Process * p ) {
}


void FrameClosure::end_process( Process * p ) {
}


void FrameClosure::do_frame( Frame * f ) {
}


// -----------------------------------------------------------------------------

void FrameLayoutClosure::do_stack( int index, Oop * o ) {
}


void FrameLayoutClosure::do_hp( uint8_t ** hp ) {
}


void FrameLayoutClosure::do_receiver( Oop * o ) {
}


void FrameLayoutClosure::do_link( int ** fp ) {
}


void FrameLayoutClosure::do_return_addr( const char ** pc ) {
}


// -----------------------------------------------------------------------------

void ProcessClosure::do_process( DeltaProcess * p ) {
}


void OopClosure::do_oop( Oop * o ) {
}


void klassOopClosure::do_klass( KlassOop klass ) {
}
