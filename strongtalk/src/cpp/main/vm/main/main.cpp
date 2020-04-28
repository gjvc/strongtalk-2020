
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/os.hpp"
#include "vm/runtime/vmOperations.hpp"


int main( int argc, char *argv[] ) {
    os::set_args( argc, argv );
    return vm_main( argc, argv );
}
