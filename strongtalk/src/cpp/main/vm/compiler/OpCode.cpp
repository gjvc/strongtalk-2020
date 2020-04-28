//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/OpCode.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"


const char * BranchOpName[] = {
    "B=", "B#", "B<", "B<=", "Bu<", "Bu<=", "B>", "B>=", "Bu>", "Bu>=", "Bovfl", "Bnofvl",

    "last (INVALID)"
};


const char * ArithOpName[] = {
    "nil (INVALID)", "test",

    // untagged operations
    "+", "-", "*", "div", "mod", "and", "or", "xor", "shift", "cmp",

    // tagged operations
    "t+", "t-", "t*", "tdiv", "tmod", "tand", "tor", "txor", "tshift", "tcmp",

    // untagged float operations
    "f+", "f-", "f*", "fdiv", "fmod", "fcmp", "fneg", "fabs", "f^2", "f2oop",

    // tagged float operation
    "f2float",

    "last (INVALID)"
};


bool_t ArithOpIsCommutative[] = {
    false, true,

    // untagged operations
    true, false, true, false, false, true, true, true, false, false,

    // tagged operations
    true, false, true, false, false, true, true, true, false, false,

    // untagged float operations
    true, false, true, false, false, false, false, false, false, false,

    // tagged float operation
    false,

    false
};


void opcode_init() {
    _console->print_cr( "%%system-init:  opcode_init" );

    if ( sizeof( BranchOpName ) / sizeof( const char * ) not_eq LastBranchOp + 1 ) {
        fatal( "forgot to change BranchOpName after changing BranchOpCode" );
    }

    if ( sizeof( ArithOpName ) / sizeof( const char * ) not_eq LastArithOp + 1 ) {
        fatal( "forgot to change ArithOpName after changing ArithOpCode" );
    }

    if ( sizeof( ArithOpIsCommutative ) / sizeof( bool_t ) not_eq LastArithOp + 1 ) {
        fatal( "forgot to change ArithOpIsCommutative after changing ArithOpCode" );
    }

}
