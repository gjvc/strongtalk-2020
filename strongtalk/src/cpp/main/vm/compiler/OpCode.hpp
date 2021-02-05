
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <array>
#include "vm/system/platform.hpp"


enum class BranchOpCode {
    EQBranchOp,     // equal
    NEBranchOp,     // not equal
    LTBranchOp,     // less than
    LEBranchOp,     // less than or equal
    LTUBranchOp,    // less than unsigned
    LEUBranchOp,    // less than or equal unsigned
    GTBranchOp,     // greater than
    GEBranchOp,     // greater than or equal
    GTUBranchOp,    // greater than unsigned
    GEUBranchOp,    // greater than or equal unsigned
    VSBranchOp,     //
    VCBranchOp,     //
    // Overflow set/cleared
    LastBranchOp    //
};

extern std::array<const char *, static_cast<std::size_t >( BranchOpCode::LastBranchOp )> BranchOpName; // indexed by BranchOpCode


enum class ArithOpCode {
    NilArithOp,     //
    TestArithOp,    //

    // untagged operations
    AddArithOp,     // add
    SubArithOp,     // subtract
    MulArithOp,     // multiply
    DivArithOp,     // divide
    ModArithOp,     // modulo
    AndArithOp,     // and
    OrArithOp,      // or
    XOrArithOp,     // xor
    ShiftArithOp,   // shift
    CmpArithOp,     // compare

    // tagged operations
    tAddArithOp,    //
    tSubArithOp,    //
    tMulArithOp,    //
    tDivArithOp,    //
    tModArithOp,    //
    tAndArithOp,    //
    tOrArithOp,     //
    tXOrArithOp,    //
    tShiftArithOp,  //
    tCmpArithOp,    //

    // binary untagged float operations
    fAddArithOp,    //
    fSubArithOp,    //
    fMulArithOp,    //
    fDivArithOp,    //
    fModArithOp,    //
    fCmpArithOp,    //

    // unary untagged float operations
    fNegArithOp,    //
    fAbsArithOp,    //
    fSqrArithOp,    //
    f2OopArithOp,   //

    // unary tagged float operation
    f2FloatArithOp, //

    LastArithOp     //
};

extern std::array<const char *, static_cast<std::size_t >( ArithOpCode::LastArithOp )> ArithOpName;
extern std::array<bool, static_cast<std::size_t >( ArithOpCode::LastArithOp )>         ArithOpIsCommutative;
