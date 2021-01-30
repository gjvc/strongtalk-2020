
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


class StringOutputStream : public ConsoleOutputStream {

protected:
    std::string  _string;
    char         *buffer;
    int32_t buffer_pos;
    int32_t buffer_length;

public:
    StringOutputStream( const int32_t initial_size = 1 * 1024 );

    void put( char c );

    char *as_string();

    // Conversion into Delta object
    struct ByteArrayOopDescriptor *as_byteArray();
};
