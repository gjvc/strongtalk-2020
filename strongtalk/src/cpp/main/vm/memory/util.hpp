//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/OopDescriptor.hpp"

// conveniences

extern std::int32_t Indent;

void printIndent();

extern Oop catchThisOne;


// utilities

extern "C" void copy_oops_up( Oop *from, Oop *to, std::int32_t count );
extern "C" void set_oops( Oop *to, std::int32_t count, Oop value = nullptr );

char *copy_string( const char *s );

char *copy_string( const char *s, smi_t len );

char *copy_c_heap_string( const char *s );

// copying oops must be accompanied by record_multistores for remembered set
void copy_oops_down( Oop *from, Oop *to, std::int32_t count );


void copy_oops( Oop *from, Oop *to, std::int32_t count );


void copy_oops_overlapping( Oop *from, Oop *to, std::int32_t count );


void copy_words( std::int32_t *from, std::int32_t *to, std::int32_t count );


void set_words( std::int32_t *from, std::int32_t count, std::int32_t value = 0 );


std::int32_t min( std::int32_t a, std::int32_t b );


std::int32_t max( std::int32_t a, std::int32_t b );


std::int32_t min( std::int32_t a, std::int32_t b, std::int32_t c );


std::int32_t max( std::int32_t a, std::int32_t b, std::int32_t c );


//#define between( p, low, high ) ((void *)(p) >= (void *)(low) and (void *)(p) < (void *)(high))


void *align( void *p, std::int32_t alignment );


std::int32_t byte_size( void *from, void *to );
