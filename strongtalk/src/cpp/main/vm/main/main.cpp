
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/os.hpp"
#include "vm/runtime/vmOperations.hpp"


#include <iostream>
#include <cstdlib>


void atexit_handler_1() {
    std::cout << "at exit #1\n";
}


void atexit_handler_2() {
    std::cout << "at exit #2\n";
}


int main( int argc, char * argv[] ) {
    const int result_1 = std::atexit( atexit_handler_1 );
    const int result_2 = std::atexit( atexit_handler_2 );

    if ( ( result_1 != 0 ) || ( result_2 != 0 ) ) {
        std::cerr << "Registration failed\n";
        return EXIT_FAILURE;
    }

    os::set_args( argc, argv );
    int status = vm_main( argc, argv );

    std::cout << "returning from main\n";
    return EXIT_SUCCESS;
}
