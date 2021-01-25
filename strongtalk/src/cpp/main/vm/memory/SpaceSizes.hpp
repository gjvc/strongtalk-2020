
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"


// default sizes for heaps & zone
class SpaceSizes : ValueObject {

public:
    // Objects
    std::int32_t _reserved_object_size;   // reserved space for all objects
    std::int32_t _eden_size;              // size of eden
    std::int32_t _surv_size;              // size of from & to spaces
    std::int32_t _old_size;               // size of old Space

    // compiled code
    std::int32_t         _reserved_codes_size;    // reserved Space for NativeMethod zone
    std::int32_t _code_size;              // size of NativeMethod zone
    std::int32_t         _reserved_pic_heap_size; // reserved Space for PolymorphicInlineCache zone
    std::int32_t         _pic_heap_size;          // size of pic_heap
    std::int32_t         _jump_table_size;        // size of jump table

    // Reads debug variables for initial settings.
    void initialize();
};
