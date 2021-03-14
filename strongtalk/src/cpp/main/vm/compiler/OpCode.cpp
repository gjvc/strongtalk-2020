
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/OpCode.hpp"
#include "vm/system/asserts.hpp"

#include <array>


std::array<const char *, static_cast<std::size_t >( BranchOpCode::LastBranchOp )> BranchOpName = {
    "B=",       //
    "B#",       //
    "B<",       //
    "B<=",      //
    "Bu<",      //
    "Bu<=",     //
    "B>",       //
    "B>=",      //
    "Bu>",      //
    "Bu>=",     //
    "Bovfl",   //
    "Bnofvl",  //
};


std::array<const char *, static_cast<std::size_t >( ArithOpCode::LastArithOp )> ArithOpName = {

    "nil (INVALID)", //
    "test", //

    "+", //
    "-", //
    "*", //
    "div", //
    "mod", //
    "and", //
    "or", //
    "xor", //
    "shift", //
    "cmp", //
// untagged operations
    "t+", //
    "t-", //
    "t*", //
    "tdiv", //
    "tmod", //
    "tand", //
    "tor", //
    "txor", //
    "tshift", //
    "tcmp", //
// tagged operations
    "f+", //
    "f-", //
    "f*", //
    "fdiv", //
    "fmod", //
    "fcmp", //
    "fneg", //
    "fabs", //
    "f^2", //
    "f2oop", //
// untagged float operations

    "f2float", //
// tagged float operation

};


std::array<bool, static_cast<std::size_t >( ArithOpCode::LastArithOp )> ArithOpIsCommutative = {


    false, //
    true,   //

    true, // untagged operation
    false, // untagged operation
    true, // untagged operation
    false, // untagged operation
    false, // untagged operation
    true, // untagged operation
    true, // untagged operation
    true, // untagged operation
    false, // untagged operation
    false, // untagged operation

    true, // tagged operation
    false, // tagged operation
    true, // tagged operation
    false, // tagged operation
    false, // tagged operation
    true, // tagged operation
    true, // tagged operation
    true, // tagged operation
    false, // tagged operation
    false, // tagged operation

    true, // untagged float operation
    false, // untagged float operation
    true, // untagged float operation
    false, // untagged float operation
    false, // untagged float operation
    false, // untagged float operation
    false, // untagged float operation
    false, // untagged float operation
    false, // untagged float operation
    false, // untagged float operation

    false, // tagged float operation

};


void opcode_init() {
    SPDLOG_INFO( "system-init:  opcode_init" );

    // check sizes
    std::numeric_limits<typename std::underlying_type<BranchOpCode>::type>::max();

    if ( sizeof( BranchOpName ) / sizeof( const char * ) not_eq static_cast<std::int32_t>(BranchOpCode::LastBranchOp) ) {
        st_fatal( "forgot to change BranchOpName after changing BranchOpCode" );
    }

    if ( sizeof( ArithOpName ) / sizeof( const char * ) not_eq static_cast<std::int32_t>(ArithOpCode::LastArithOp) ) {
        st_fatal( "forgot to change ArithOpName after changing ArithOpCode" );
    }

    if ( sizeof( ArithOpIsCommutative ) / sizeof( bool ) not_eq static_cast<std::int32_t>(ArithOpCode::LastArithOp) ) {
        st_fatal( "forgot to change ArithOpIsCommutative after changing ArithOpCode" );
    }

}
