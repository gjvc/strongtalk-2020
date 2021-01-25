//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


extern const char *image_basename;
extern const char *rc_basename;

void parse_arguments( std::int32_t argc, char *argv[] );

void process_settings_file( const char *file_name, bool_t quiet = false );
