//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"



// DispatchTable controls the dispatch of byte codes

class DispatchTable : AllStatic {

    private:
        enum class Mode {
            normal_mode = 0,    //
            step_mode = 1,      //
            next_mode = 2,      //
            return_mode = 3     //
        };

        static Mode mode;

        static void patch_with_sst_stub();

    public:
        // the dispatch table
        static uint8_t ** table();

        // initializes the dispatch table to the original state.
        static void reset();

        // intercepts all relevant entries to enable single step.
        static void intercept_for_step( int * fr );

        static void intercept_for_next( int * fr );

        static void intercept_for_return( int * fr );


        // answers whether the dispatch table is in single step mode.
        static bool_t in_normal_mode() {
            return mode == Mode::normal_mode;
        }


        static bool_t in_step_mode() {
            return mode == Mode::step_mode;
        }


        static bool_t in_next_mode() {
            return mode == Mode::next_mode;
        }


        static bool_t in_return_mode() {
            return mode == Mode::return_mode;
        }
};

