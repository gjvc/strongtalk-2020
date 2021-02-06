//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"

// TempDecoder decodes the method annotation describing the names of parameters and temporaries.

class TempDecoder {
private:
    std::int32_t _num_of_params;

    bool is_heap_parameter( ByteArrayOop name, ObjectArrayOop tempInfo );

public:
    TempDecoder() :
        _num_of_params{ 0 } {

    }


    virtual void decode( MethodOop method, std::int32_t byteCodeIndex = 0 );


    // arguments are numbered from 1 to n
    virtual void parameter( ByteArrayOop name, std::int32_t index ) {
        static_cast<void>(name); // unused
        static_cast<void>(index); // unused
    }


    virtual void stack_temp( ByteArrayOop name, std::int32_t no ) {
        static_cast<void>(name); // unused
        static_cast<void>(no); // unused
    }


    virtual void stack_float_temp( ByteArrayOop name, std::int32_t fno ) {
        static_cast<void>(name); // unused
        static_cast<void>(fno); // unused
    }


    virtual void heap_temp( ByteArrayOop name, std::int32_t no ) {
        static_cast<void>(name); // unused
        static_cast<void>(no); // unused
    }


    virtual void heap_parameter( ByteArrayOop name, std::int32_t no ) {
        static_cast<void>(name); // unused
        static_cast<void>(no); // unused
    }


    virtual void no_debug_info() {
    }
};


class TempPrinter : public TempDecoder {
private:

public:
    void decode( MethodOop method, std::int32_t byteCodeIndex = 0 );

    // arguments are numbered from 1 to n
    void parameter( ByteArrayOop name, std::int32_t index );

    void stack_temp( ByteArrayOop name, std::int32_t no );

    void stack_float_temp( ByteArrayOop name, std::int32_t fno );

    void heap_temp( ByteArrayOop name, std::int32_t no );

    void heap_parameter( ByteArrayOop name, std::int32_t no );

    void no_debug_info();
};

// Returns the name of parameter number if found, nullptr otherwise.
ByteArrayOop find_parameter_name( MethodOop method, std::int32_t no );

// Returns the name of temp at offset if found, nullptr otherwise.
ByteArrayOop find_stack_temp( MethodOop method, std::int32_t byteCodeIndex, std::int32_t no );

ByteArrayOop find_heap_temp( MethodOop method, std::int32_t byteCodeIndex, std::int32_t no );

ByteArrayOop find_stack_float_temp( MethodOop method, std::int32_t byteCodeIndex, std::int32_t fno );
