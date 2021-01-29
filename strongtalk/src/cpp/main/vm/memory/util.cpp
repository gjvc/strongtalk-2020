
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/memory/util.hpp"
#include "allocation.hpp"

#include "vm/runtime/flags.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/os.hpp"

#include <cstring>

std::int32_t Indent = 0;


void printIndent() {
    for ( std::int32_t i = 0; i < Indent; i++ )
        spdlog::info( "  " );
}


#define LOOP_UNROLL( count, body )                                            \
    {                                                                         \
    st_assert(count >= 0, "cannot have negative count in loop unroll");       \
    std::int32_t __c1__ = count;                                                       \
    for (std::int32_t __c__ = __c1__ >> 3; __c__; __c__ --) {                          \
    body;       body;                                                         \
    body;       body;                                                         \
    body;       body;                                                         \
    body;       body;                                                         \
  }                                                                           \
    switch (maskBits(__c1__, nthMask(3))) {                                   \
   case 7:      body;                                                         \
   case 6:      body;                                                         \
   case 5:      body;                                                         \
   case 4:      body;                                                         \
   case 3:      body;                                                         \
   case 2:      body;                                                         \
   case 1:      body;                                                         \
   case 0:      ;                                                             \
  } }

#define DO_UP( from ) LOOP_UNROLL(count, *to++ = from)
#define DO_DOWN( from ) LOOP_UNROLL(count, *--to = from)


void copy_oops_up( Oop *from, Oop *to, std::int32_t count ) {
    st_assert( maskBits( std::int32_t( from ), TAG_SIZE ) == 0, "not word aligned" );
    st_assert( maskBits( std::int32_t( to ), TAG_SIZE ) == 0, "not word aligned" );
    st_assert( count >= 0, "negative count" );

    // block_step was determined by profiling the scavenger.
    //  11/14-95 (Robert and Lars)
    constexpr std::int32_t block_step = 4;

    while ( count >= block_step ) {
        *( to + 0 ) = *( from + 0 );
        *( to + 1 ) = *( from + 1 );
        *( to + 2 ) = *( from + 2 );
        *( to + 3 ) = *( from + 3 );
        to += block_step;
        from += block_step;
        count -= block_step;
    }

    if ( count > 0 ) {
        *( to + 0 ) = *( from + 0 );
        if ( count > 1 ) {
            *( to + 1 ) = *( from + 1 );
            if ( count > 2 ) {
                *( to + 2 ) = *( from + 2 );
            }
        }
    }
}


void copy_oops_down( Oop *from, Oop *to, std::int32_t count ) {
    st_assert( maskBits( std::int32_t( from ), TAG_SIZE ) == 0, "not word aligned" );
    st_assert( maskBits( std::int32_t( to ), TAG_SIZE ) == 0, "not word aligned" );
    st_assert( count >= 0, "negative count" );
    DO_DOWN( *--from )
}


void set_oops( Oop *to, std::int32_t count, Oop value ) {
    st_assert( maskBits( std::int32_t( to ), TAG_SIZE ) == 0, "not word aligned" );
    st_assert( count >= 0, "negative count" );

    constexpr std::int32_t block_step = 4;

    while ( count >= block_step ) {
        *( to + 0 ) = value;
        *( to + 1 ) = value;
        *( to + 2 ) = value;
        *( to + 3 ) = value;
        to += block_step;
        count -= block_step;
    }

    if ( count > 0 ) {
        *( to + 0 ) = value;
        if ( count > 1 ) {
            *( to + 1 ) = value;
            if ( count > 2 ) {
                *( to + 2 ) = value;
            }
        }
    }
}


char *copy_string( const char *s ) {
    std::int32_t len  = strlen( s ) + 1;
    char         *str = new_resource_array<char>( len );
    strcpy( str, s );
    return str;
}


char *copy_c_heap_string( const char *s ) {
    std::int32_t len  = strlen( s ) + 1;
    char         *str = new_c_heap_array<char>( len );
    strcpy( str, s );
    return str;
}


char *copy_string( const char *s, smi_t len ) {
    char *str = new_resource_array<char>( len + 1 );
    memcpy( str, s, len + 1 );
    str[ len ] = '\0';
    return str;
}


void copy_oops( Oop *from, Oop *to, std::int32_t count ) {
    copy_oops_up( from, to, count );
}


void copy_oops_overlapping( Oop *from, Oop *to, std::int32_t count ) {
    if ( from < to )
        copy_oops_down( from + count, to + count, count );
    else if ( from > to )
        copy_oops_up( from, to, count );
}


void copy_words( std::int32_t *from, std::int32_t *to, std::int32_t count ) {
    copy_oops( (Oop *) from, (Oop *) to, count );
}


void set_words( std::int32_t *from, std::int32_t count, std::int32_t value ) {
    set_oops( (Oop *) from, count, (Oop) value );
}


std::int32_t min( std::int32_t a, std::int32_t b ) {
    return a < b ? a : b;
}


std::int32_t max( std::int32_t a, std::int32_t b ) {
    return a > b ? a : b;
}


std::int32_t min( std::int32_t a, std::int32_t b, std::int32_t c ) {
    return a < b ? min( a, c ) : min( b, c );
}


std::int32_t max( std::int32_t a, std::int32_t b, std::int32_t c ) {
    return a > b ? max( a, c ) : max( b, c );
}


void *align( void *p, std::int32_t alignment ) {
    std::int32_t number = (std::int32_t) p;
    std::int32_t adjust = alignment - ( number % alignment ) % alignment;
    return (void *) ( number + adjust );
}


std::int32_t byte_size( void *from, void *to ) {
    return (const char *) to - (const char *) from;
}


Oop catchThisOne;


void breakpoint() {
    if ( BreakAtWarning )
        error_breakpoint();
}


void error_breakpoint() {
    //flush_logFile();
    os::breakpoint();
}
