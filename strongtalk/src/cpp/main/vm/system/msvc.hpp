
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// -----------------------------------------------------------------------------

#if defined( _MSC_VER )

#define __CALLING_CONVENTION __stdcall

//    #include <ciso646>

#if _WIN32 || _WIN64

#if _MSC_VER >= 1600
#include <cstdint>
#else
    typedef __int8              std::int8_t;
    typedef __int16             std::std::int16_t;
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

// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
