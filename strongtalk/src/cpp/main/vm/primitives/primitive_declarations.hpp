
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/utilities/lprintf.hpp"


// -----------------------------------------------------------------------------

typedef class OopDescriptor * Oop;


// -----------------------------------------------------------------------------
// Macros for the primitive implementation

#define PRIM_DECL_0( name )                                           Oop __CALLING_CONVENTION name()
#define PRIM_DECL_1( name, a1 )                                       Oop __CALLING_CONVENTION name( a1 )
#define PRIM_DECL_2( name, a1, a2 )                                   Oop __CALLING_CONVENTION name( a2, a1 )
#define PRIM_DECL_3( name, a1, a2, a3 )                               Oop __CALLING_CONVENTION name( a3, a2, a1 )
#define PRIM_DECL_4( name, a1, a2, a3, a4 )                           Oop __CALLING_CONVENTION name( a4, a3, a2, a1 )
#define PRIM_DECL_5( name, a1, a2, a3, a4, a5 )                       Oop __CALLING_CONVENTION name( a5, a4, a3, a2, a1 )
#define PRIM_DECL_6( name, a1, a2, a3, a4, a5, a6 )                   Oop __CALLING_CONVENTION name( a6, a5, a4, a3, a2, a1 )
#define PRIM_DECL_7( name, a1, a2, a3, a4, a5, a6, a7 )               Oop __CALLING_CONVENTION name( a7, a6, a5, a4, a3, a2, a1 )
#define PRIM_DECL_8( name, a1, a2, a3, a4, a5, a6, a7, a8 )           Oop __CALLING_CONVENTION name( a8, a7, a6, a5, a4, a3, a2, a1 )
#define PRIM_DECL_9( name, a1, a2, a3, a4, a5, a6, a7, a8, a9 )       Oop __CALLING_CONVENTION name( a9, a8, a7, a6, a5, a4, a3, a2, a1 )
#define PRIM_DECL_10( name, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 ) Oop __CALLING_CONVENTION name(a10, a9, a8, a7, a6, a5, a4, a3, a2, a1 )
