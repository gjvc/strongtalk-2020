
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
    int _reserved_object_size;   // reserved space for all objects
    int _eden_size;              // size of eden
    int _surv_size;              // size of from & to spaces
    int _old_size;               // size of old Space

    // compiled code
    int         _reserved_codes_size;    // reserved Space for NativeMethod zone
    std::size_t _code_size;              // size of NativeMethod zone
    int         _reserved_pic_heap_size; // reserved Space for PolymorphicInlineCache zone
    int         _pic_heap_size;          // size of pic_heap
    int         _jump_table_size;        // size of jump table

    // Reads debug variables for initial settings.
    void initialize();
};
