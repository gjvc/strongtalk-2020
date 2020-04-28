//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/oops/OopDescriptor.hpp"


char * byte_map_base;
char * MaxSP;

// verifyMethod: called by interpreter to verify some value is a methodOop
extern "C" void verifyMethod( Oop method ) {
    if ( not method->is_method() ) fatal( "not a method" );
    method->verify();
}
