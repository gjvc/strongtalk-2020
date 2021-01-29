
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// -----------------------------------------------------------------------------

#if __SIZE_WIDTH__ == 32

typedef std::intptr_t smi_t;     //
typedef std::uint32_t address_t; //

#endif


#if __SIZE_WIDTH__ == 64

typedef std::intptr_t  smi_t;        //
typedef std::uint64_t  address_t;    //

#endif


// -----------------------------------------------------------------------------

static_assert( sizeof( smi_t ) == sizeof( address_t ) );


// -----------------------------------------------------------------------------

constexpr std::int32_t SIZEOF_FLOAT = sizeof( double );


constexpr std::int32_t st_log2( std::int32_t n ) {
    return ( ( n < 2 ) ? 1 : 1 + st_log2( n / 2 ) );
}


constexpr std::int32_t BitsPerByte    = 8;
constexpr std::int32_t LogBitsPerByte = st_log2( BitsPerByte );

constexpr std::int32_t BitsPerWord     = __SIZE_WIDTH__;
constexpr std::int32_t BytesPerWord    = __SIZE_WIDTH__ / BitsPerByte;
constexpr std::int32_t LogBytesPerWord = st_log2( BytesPerWord );
constexpr std::int32_t LogBitsPerWord  = st_log2( BitsPerByte ) + st_log2( BytesPerWord );
