
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <chrono>
#include <ctime>
#include <string>


std::string format_time_point( std::chrono::system_clock::time_point &point );

void log_line( const std::string &line );
