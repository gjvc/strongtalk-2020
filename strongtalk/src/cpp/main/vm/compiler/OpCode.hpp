
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <array>
#include "vm/system/platform.hpp"


// opcodes used by the Compiler

enum BranchOpCode {
    EQBranchOp, //
    NEBranchOp, //
    LTBranchOp, //
    LEBranchOp, //
    LTUBranchOp, //
    LEUBranchOp, //
    GTBranchOp, //
    GEBranchOp, //
    GTUBranchOp, //
    GEUBranchOp, //
    VSBranchOp, //
    VCBranchOp, //
    // Overflow set/cleared

    LastBranchOp
};


enum ArithOpCode {
    NilArithOp, //
    TestArithOp,

    // untagged operations
    AddArithOp, //
    SubArithOp, //
    MulArithOp, //
    DivArithOp, //
    ModArithOp, //
    AndArithOp, //
    OrArithOp, //
    XOrArithOp, //
    ShiftArithOp, //
    CmpArithOp, //

    // tagged operations
    tAddArithOp, //
    tSubArithOp, //
    tMulArithOp, //
    tDivArithOp, //
    tModArithOp, //
    tAndArithOp, //
    tOrArithOp, //
    tXOrArithOp, //
    tShiftArithOp, //
    tCmpArithOp,

    // binary untagged float operations
    fAddArithOp, //
    fSubArithOp, //
    fMulArithOp, //
    fDivArithOp, //
    fModArithOp, //
    fCmpArithOp, //

    // unary untagged float operations
    fNegArithOp, //
    fAbsArithOp, //
    fSqrArithOp, //
    f2OopArithOp, //

    // unary tagged float operation
    f2FloatArithOp,

    LastArithOp
};

extern std::array<const char *, 13>BranchOpName; // indexed by BranchOpCode
extern std::array<const char *, 34>ArithOpName; // indexed by ArithOpCode
extern std::array<bool_t, 34>ArithOpIsCommutative; // indexed by ArithOpCode
