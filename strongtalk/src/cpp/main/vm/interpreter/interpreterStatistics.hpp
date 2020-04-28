//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/interpreter/ByteCodes.hpp"




// Collects statistical information on the interpreter.

class InterpreterStatistics : AllStatic {

    private:

        static bool_t   _is_initialized;            // true if InterpreterStatistics has been initialized
        static uint32_t _bytecode_counters[];
        static int      _bytecode_generation_order[];

        static void reset_bytecode_counters();

        static void reset_bytecode_generation_order();

    public:

        static bool_t is_initialized() {
            return _is_initialized;
        }


        static uint32_t * bytecode_counters() {
            return _bytecode_counters;
        }


        static ByteCodes::Code ith_bytecode_to_generate( int i );

        static void initialize();
};

