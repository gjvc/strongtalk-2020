//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"

// The evaluator provides a read-eval-loop for simple vm debugging
// See print_help() for documentation.

// -----------------------------------------------------------------------------

class TokenStream;

typedef class OopDescriptor * Oop;


class evaluator : AllStatic {

    protected:
        static bool_t get_line( char * line );

        static bool_t process_line( const char * line );

        static void eval_message( TokenStream * stream );

        static bool_t get_oop( TokenStream * stream, Oop * addr );

        static void print_mini_help();

        static void print_help();

        static void top_command( TokenStream * stream );

        static void show_command( TokenStream * stream );

        static void change_debug_flag( TokenStream * stream, bool_t value );

        static void print_status();

    public:
        static void read_eval_loop();

        static void single_step( int * fr );
};

