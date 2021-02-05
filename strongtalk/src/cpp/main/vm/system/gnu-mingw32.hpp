
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// -----------------------------------------------------------------------------

// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html


#if defined( __GNUC__ ) && defined( __MINGW32__ )

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


#endif
