//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <array>
#include "vm/system/platform.hpp"
#include "vm/oops/MemOopDescriptor.hpp"


// Keeps the sliding average of where the system spends the time
class SlidingSystemAverage : AllStatic {
    public:

        enum {
            // initial value
            nowhere = 0, //

            // ticks without last_delta_fp set
            in_compiled_code    = 1, //
            in_interpreted_code = 2, //
            in_pic_code         = 3, //
            in_stub_code        = 4, //

            // ticks with last_delta_fp set
            in_compiler        = 5, //
            in_garbage_collect = 6, // sacvenge or GC
            is_idle            = 7, // hanging in primitiveProcessSchedulerWait:
            in_vm              = 8, // somewhere else in the vm (primitive / dll)
            number_of_cases    = 9, //

            buffer_size = 256 //
        };


        // Resets the buffer
        static void reset();

        // Add tick to buffer
        static void add( char type );

        // Returns the update statistics array
        static std::array <uint32_t, SlidingSystemAverage::number_of_cases> update();

    private:
        static std::array <char, buffer_size>         _buffer;
        static std::array <uint32_t, number_of_cases> _stat;
        static uint32_t                               _position;                    // Current pos in buffer
};
