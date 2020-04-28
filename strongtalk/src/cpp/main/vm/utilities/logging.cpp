
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/utilities/logging.hpp"

#include <chrono>
#include <ctime>
#include <iostream>


std::string format_time_point( std::chrono::system_clock::time_point & point ) {

    static_assert( std::chrono::system_clock::time_point::period::den == 1E9 );

    std::string result( 29, '0' );
    char * buf = &result[ 0 ];
    std::time_t now_c = std::chrono::system_clock::to_time_t( point );
    std::strftime( buf, 21, "%Y-%m-%dT%H:%M:%S.", std::localtime( &now_c ) );
    sprintf( buf + 20, "%09ld", point.time_since_epoch().count() % static_cast<int>(1E9) );

    return result;
}


void log_line( const std::string & line ) {

//    std::chrono::time_point now = std::chrono::system_clock::now();
//
//    std::cout << format_time_point( now ) << "  " << line << std::endl;

}
