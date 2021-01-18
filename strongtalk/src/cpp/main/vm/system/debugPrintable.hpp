//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"


// these printing functions are conveniently called from the debugger

extern "C" {

void pp( void *p );
void urs_ps();

void pp_short( void *p );
void pr( Oop p );
void pm( Klass *p );

}
