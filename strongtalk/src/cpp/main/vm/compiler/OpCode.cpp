//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/OpCode.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"

#include <array>

std::array<const char *, 13> BranchOpName = {
        "B=",  //
        "B#",  //
        "B<",  //
        "B<=",  //
        "Bu<",  //
        "Bu<=",  //
        "B>",  //
        "B>=",  //
        "Bu>",  //
        "Bu>=",  //
        "Bovfl",  //
        "Bnofvl",

        "last (INVALID)"
};

std::array<const char *, 34> ArithOpName = {
        "nil (INVALID)", "test",

        "+", "-", "*", "div", "mod", "and", "or", "xor", "shift", "cmp", // untagged operations
        "t+", "t-", "t*", "tdiv", "tmod", "tand", "tor", "txor", "tshift", "tcmp", // tagged operations
        "f+", "f-", "f*", "fdiv", "fmod", "fcmp", "fneg", "fabs", "f^2", "f2oop", // untagged float operations

        "f2float", // tagged float operation

        "last (INVALID)"
};


std::array<bool_t, 34> ArithOpIsCommutative = {
        false, true,

        true, false, true, false, false, true, true, true, false, false, // untagged operations

        true, false, true, false, false, true, true, true, false, false, // tagged operations

        true, false, true, false, false, false, false, false, false, false, // untagged float operations

        false, // tagged float operation

        false
};


void opcode_init() {
    _console->print_cr( "%%system-init:  opcode_init" );

    // check sizes

    //    std::numeric_limits<typename std::underlying_type<BranchOpCode>::type>::max()

    if ( sizeof( BranchOpName ) / sizeof( const char * ) not_eq static_cast<int>(BranchOpCode::LastBranchOp) + 1 ) {
        st_fatal( "forgot to change BranchOpName after changing BranchOpCode" );
    }

    if ( sizeof( ArithOpName ) / sizeof( const char * ) not_eq static_cast<int>(ArithOpCode::LastArithOp) + 1 ) {
        st_fatal( "forgot to change ArithOpName after changing ArithOpCode" );
    }

    if ( sizeof( ArithOpIsCommutative ) / sizeof( bool_t ) not_eq static_cast<int>(ArithOpCode::LastArithOp) + 1 ) {
        st_fatal( "forgot to change ArithOpIsCommutative after changing ArithOpCode" );
    }

}
