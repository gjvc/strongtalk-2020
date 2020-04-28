//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"


// -----------------------------------------------------------------------------

typedef class OopDescriptor * Oop;


class NativeInstruction : ValueObject {

        // The base class for different kinds of native instruction abstractions.
        // Provides the primitive operations to manipulate code relative to this.

    protected:
        char * addr_at( int offset ) const {
            return ( char * ) this + offset;
        }


        char char_at( int offset ) const {
            return *addr_at( offset );
        }


        int long_at( int offset ) const {
            return *( int * ) addr_at( offset );
        }


        Oop oop_at( int offset ) const {
            return *( Oop * ) addr_at( offset );
        }


        void set_char_at( int offset, char c ) {
            *addr_at( offset ) = c;
        }


        void set_long_at( int offset, int i ) {
            *( int * ) addr_at( offset ) = i;
        }


        void set_oop_at( int offset, Oop o ) {
            *( Oop * ) addr_at( offset ) = o;
        }

};


// An abstraction for accessing/manipulating native call imm32 instructions.
// (used to manipulate inline caches, primitive & dll calls, etc.)

class NativeCall : public NativeInstruction {

    public:
        enum Intel_specific_constants {
            instruction_code      = 0xE8,   //
            instruction_size      = 5,      //
            instruction_offset    = -5,     //
            displacement_offset   = -4,     //
            return_address_offset = 0,      //
        };


        char * instruction_address() const {
            return addr_at( instruction_offset );
        }


        char * next_instruction_address() const {
            return addr_at( return_address_offset );
        }


        int displacement() const {
            return long_at( displacement_offset );
        }


        char * return_address() const {
            return addr_at( return_address_offset );
        }


        char * destination() const {
            return return_address() + displacement();
        }


        void set_destination( const char * dest ) {
            set_long_at( displacement_offset, dest - return_address() );
        }


        void verify();

        void print();

        // Creation
        friend NativeCall * nativeCall_at( const char * address );

        friend NativeCall * nativeCall_from_return_address( const char * return_address );

        friend NativeCall * nativeCall_from_relocInfo( const char * displacement_address );

};


inline NativeCall * nativeCall_at( const char * address ) {
    NativeCall * call = ( NativeCall * ) ( address - NativeCall::instruction_offset );
    call->verify();
    return call;
}


inline NativeCall * nativeCall_from_return_address( const char * return_address ) {
    NativeCall * call = ( NativeCall * ) ( return_address - NativeCall::return_address_offset );
    call->verify();
    return call;
}


inline NativeCall * nativeCall_from_relocInfo( const char * displacement_address ) {
    NativeCall * call = ( NativeCall * ) ( displacement_address - NativeCall::displacement_offset );
    call->verify();
    return call;
}

// An abstraction for accessing/manipulating native mov reg, imm32 instructions.
// (used to manipulate inlined 32bit data dll calls, etc.)

class NativeMov : public NativeInstruction {

    public:
        enum Intel_specific_constants {
            instruction_code        = 0xB8, //
            instruction_size        = 5,    //
            instruction_offset      = 0,    //
            data_offset             = 1,    //
            next_instruction_offset = 5,    //
            register_mask           = 0x07, //
        };


        char * instruction_address() const {
            return addr_at( instruction_offset );
        }


        char * next_instruction_address() const {
            return addr_at( next_instruction_offset );
        }


        int data() const {
            return long_at( data_offset );
        }


        void set_data( int x ) {
            set_long_at( data_offset, x );
        }


        void verify();

        void print();

        // Creation
        friend NativeMov * nativeMov_at( const char * address );

};


inline NativeMov * nativeMov_at( const char * address ) {
    NativeMov * test = ( NativeMov * ) ( address - NativeMov::instruction_offset );
    test->verify();
    return test;
}


// An abstraction for accessing/manipulating native test eax, imm32 instructions.
// (used to manipulate inlined 32bit data for NonLocalReturns, dll calls, etc.)

class NativeTest : public NativeInstruction {

    public:
        enum Intel_specific_constants {
            instruction_code        = 0xA9,     //
            instruction_size        = 5,        //
            instruction_offset      = 0,        //
            data_offset             = 1,        //
            next_instruction_offset = 5,        //
        };


        char * instruction_address() const {
            return addr_at( instruction_offset );
        }


        char * next_instruction_address() const {
            return addr_at( next_instruction_offset );
        }


        int data() const {
            return long_at( data_offset );
        }


        void set_data( int x ) {
            set_long_at( data_offset, x );
        }


        void verify();

        void print();

        // Creation
        friend NativeTest * nativeTest_at( const char * address );

};


inline NativeTest * nativeTest_at( const char * address ) {
    NativeTest * test = ( NativeTest * ) ( address - NativeTest::instruction_offset );
    test->verify();
    return test;
}


// An abstraction for accessing/manipulating native test eax, imm32 instructions that serve as InlineCache info.

class InlineCacheInfo : public NativeTest {

    public:
        enum InlineCacheInfo_specific_constants {
            info_offset     = data_offset,                  //
            number_of_flags = 8,                            //
            flags_mask      = ( 1 << number_of_flags ) - 1, //
        };


        char * NonLocalReturn_target() const {
            return instruction_address() + ( data() >> number_of_flags );
        }


        int flags() const {
            return data() & flags_mask;
        }


        void set_flags( int flags ) {
            set_data( ( data() & ~flags_mask ) | ( flags & flags_mask ) );
        }


        // Creation
        friend InlineCacheInfo * ic_info_at( const char * address );

};


inline InlineCacheInfo * ic_info_at( const char * address ) {
    return ( InlineCacheInfo * ) nativeTest_at( address );
}
