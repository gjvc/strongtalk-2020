
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// -----------------------------------------------------------------------------

// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html


// -----------------------------------------------------------------------------

#if defined( _MSC_VER )

#define __CALLING_CONVENTION __stdcall

//    #include <ciso646>

#if _WIN32 || _WIN64

#if _MSC_VER >= 1600
#include <cstdint>
#else
    typedef __int8              std::int8_t;
    typedef __int16             std::int16_t;
    typedef __int32             std::int32_t;
    typedef __int64             std::int64_t;
    typedef unsigned __int8     std::uint8_t;
    typedef unsigned __int16    std::uint16_t;
    typedef unsigned __int32    std::uint32_t;
    typedef unsigned __int64    std::uint64_t;
#endif

#endif

#if _WIN32
#define __SIZE_WIDTH__ 32
#endif

#if _WIN64
#define __SIZE_WIDTH__ 64
#endif

#define __CALLING_CONVENTION __stdcall

#endif


// -----------------------------------------------------------------------------

#if defined( __MINGW32__ ) && defined( __GNUC__ )

#define __CALLING_CONVENTION __attribute__((__stdcall__))

#include <cstddef>

extern "C" {
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstdarg>
#include <cstdarg>
#include <cfloat>
#include <cmath>
}


// 32-bit word
#if __SIZE_WIDTH__ == 32

// generic word
typedef std::intptr_t smi_t;     //
typedef std::int32_t  bool_t;    //
typedef std::uint32_t address_t; //

#endif


// 64-bit word
#if __SIZE_WIDTH__ == 64

typedef std::intptr_t  smi_t;        //
typedef std::int64_t   bool_t;       //
typedef std::uint64_t  address_t;    //

#endif

static_assert( sizeof( intptr_t ) == sizeof( address_t ) );

#endif


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
