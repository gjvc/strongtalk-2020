
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// -----------------------------------------------------------------------------

#if __SIZE_WIDTH__ == 32

typedef std::intptr_t smi_t;     //
typedef std::int32_t  bool_t;    //
typedef std::uint32_t address_t; //

#endif


#if __SIZE_WIDTH__ == 64

typedef std::intptr_t  smi_t;        //
typedef std::int64_t   bool_t;       //
typedef std::uint64_t  address_t;    //

#endif


// -----------------------------------------------------------------------------

static_assert( sizeof( smi_t ) == sizeof( address_t ) );


// -----------------------------------------------------------------------------

constexpr int SIZEOF_FLOAT = sizeof( double );

constexpr std::size_t st_log2( std::size_t n ) {
    return ( ( n < 2 ) ? 1 : 1 + st_log2( n / 2 ) );
}

constexpr int BitsPerByte    = 8;
constexpr int LogBitsPerByte = st_log2( BitsPerByte );

constexpr int BitsPerWord     = __SIZE_WIDTH__;
constexpr int BytesPerWord    = __SIZE_WIDTH__ / BitsPerByte;
constexpr int LogBytesPerWord = st_log2( BytesPerWord );
constexpr int LogBitsPerWord  = st_log2( BitsPerByte ) + st_log2( BytesPerWord );
