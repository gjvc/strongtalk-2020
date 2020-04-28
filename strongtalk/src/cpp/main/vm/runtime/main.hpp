//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#pragma once


int vm_main( int argc, char * argv[] );
int createVMProcess();
int vmProcessMain( void * ignored );
extern "C" void load_image();
