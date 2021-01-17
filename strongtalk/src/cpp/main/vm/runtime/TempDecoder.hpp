//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
// TempDecoder decodes the method annotation describing
// the names of parameters and temporaries.

class TempDecoder {
    private:
        int _num_of_params;

        bool_t is_heap_parameter( ByteArrayOop name, ObjectArrayOop tempInfo );

    public:
        virtual void decode( MethodOop method, int byteCodeIndex = 0 );


        // arguments are numbered from 1 to n
        virtual void parameter( ByteArrayOop name, int index ) {
        }


        virtual void stack_temp( ByteArrayOop name, int no ) {
        }


        virtual void stack_float_temp( ByteArrayOop name, int fno ) {
        }


        virtual void heap_temp( ByteArrayOop name, int no ) {
        }


        virtual void heap_parameter( ByteArrayOop name, int no ) {
        }


        virtual void no_debug_info() {
        }
};

class TempPrinter : public TempDecoder {
    private:

    public:
        void decode( MethodOop method, int byteCodeIndex = 0 );

        // arguments are numbered from 1 to n
        void parameter( ByteArrayOop name, int index );

        void stack_temp( ByteArrayOop name, int no );

        void stack_float_temp( ByteArrayOop name, int fno );

        void heap_temp( ByteArrayOop name, int no );

        void heap_parameter( ByteArrayOop name, int no );

        void no_debug_info();
};

// Returns the name of parameter number if found, nullptr otherwise.
ByteArrayOop find_parameter_name( MethodOop method, int no );

// Returns the name of temp at offset if found, nullptr otherwise.
ByteArrayOop find_stack_temp( MethodOop method, int byteCodeIndex, int no );

ByteArrayOop find_heap_temp( MethodOop method, int byteCodeIndex, int no );

ByteArrayOop find_stack_float_temp( MethodOop method, int byteCodeIndex, int fno );
