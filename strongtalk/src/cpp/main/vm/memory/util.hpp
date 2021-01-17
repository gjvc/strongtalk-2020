//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/OopDescriptor.hpp"

// conveniences

extern int Indent;

void printIndent();

extern Oop catchThisOne;


// utilities

extern "C" void copy_oops_up( Oop * from, Oop * to, int count );
extern "C" void set_oops( Oop * to, int count, Oop value = nullptr );

char * copy_string( const char * s );

char * copy_string( const char * s, smi_t len );

char * copy_c_heap_string( const char * s );

// copying oops must be accompanied by record_multistores for remembered set
void copy_oops_down( Oop * from, Oop * to, int count );


void copy_oops( Oop * from, Oop * to, int count );


void copy_oops_overlapping( Oop * from, Oop * to, int count );


void copy_words( int * from, int * to, int count );


void set_words( int * from, int count, int value = 0 );


int min( int a, int b );


int max( int a, int b );


int min( int a, int b, int c );


int max( int a, int b, int c );


//#define between( p, low, high ) ((void *)(p) >= (void *)(low) and (void *)(p) < (void *)(high))


void * align( void * p, int alignment );


int byte_size( void * from, void * to );
