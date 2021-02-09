
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// -----------------------------------------------------------------------------

#if __SIZE_WIDTH__ == 32

typedef std::uint32_t address_t; //

#endif


#if __SIZE_WIDTH__ == 64

typedef std::uint64_t  address_t;    //

#endif


// -----------------------------------------------------------------------------

typedef std::intptr_t small_int_t;        //

static_assert( sizeof( small_int_t ) == sizeof( address_t ) );


// -----------------------------------------------------------------------------

constexpr std::int32_t SIZEOF_FLOAT = sizeof( double );


constexpr std::int32_t st_log2( std::int32_t n ) {
    return ( ( n < 2 ) ? 1 : 1 + st_log2( n / 2 ) );
}


constexpr std::int32_t BITS_PER_BYTE       = 8;
constexpr std::int32_t LOG_2_BITS_PER_BYTE = st_log2( BITS_PER_BYTE );

constexpr std::int32_t BITS_PER_WORD        = __SIZE_WIDTH__;
constexpr std::int32_t BYTES_PER_WORD       = __SIZE_WIDTH__ / BITS_PER_BYTE;
constexpr std::int32_t LOG_2_BYTES_PER_WORD = st_log2( BYTES_PER_WORD );
constexpr std::int32_t LOG_2_BITS_PER_WORD  = st_log2( BITS_PER_BYTE ) + st_log2( BYTES_PER_WORD );
