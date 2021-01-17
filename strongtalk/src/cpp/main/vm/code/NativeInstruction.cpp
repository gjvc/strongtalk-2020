//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/code/NativeInstruction.hpp"



// Implementation of NativeCall

void NativeCall::verify() {
    // make sure code pattern is actually a call imm32 instruction
    if ( *( uint8_t * ) instruction_address() not_eq instruction_code ) st_fatal( "not a call imm32" );
}


void NativeCall::print() {
    _console->print_cr( "0x%x: call 0x%x", instruction_address(), destination() );
}


// Implementation of NativeMov
void NativeMov::verify() {
    // make sure code pattern is actually a mov reg, imm32 instruction
    if ( ( *( uint8_t * ) instruction_address() & ~register_mask ) not_eq instruction_code ) st_fatal( "not a mov reg, imm32" );
}


void NativeMov::print() {
    _console->print_cr( "0x%x: mov reg, 0x%x", instruction_address(), data() );
}


// Implementation of NativeTest
void NativeTest::verify() {
    // make sure code pattern is actually a test eax, imm32 instruction
    if ( *( uint8_t * ) instruction_address() not_eq instruction_code ) st_fatal( "not a test eax, imm32" );
}


void NativeTest::print() {
    _console->print_cr( "0x%x: test eax, 0x%x", instruction_address(), data() );
}
