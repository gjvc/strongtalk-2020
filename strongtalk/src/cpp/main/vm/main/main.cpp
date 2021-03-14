
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/VMOperation.hpp"

#include <iostream>
#include <cstdlib>


void atexit_handler_1() {
    std::cout << "atexit_handler_1()\n";
}


void atexit_handler_2() {
    std::cout << "atexit_handler_2()\n";
}


int main( std::int32_t argc, char *argv[] ) {

    const std::int32_t result_1 = std::atexit( atexit_handler_1 );
    const std::int32_t result_2 = std::atexit( atexit_handler_2 );

    if ( ( result_1 != 0 ) || ( result_2 != 0 ) ) {
        std::cerr << "Registration failed\n";
        return EXIT_FAILURE;
    }

    std::int32_t status = vm_main( argc, argv );

    return status;
}
