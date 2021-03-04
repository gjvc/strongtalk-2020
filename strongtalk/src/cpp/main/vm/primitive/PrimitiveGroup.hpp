//
//  (C) 1994 - 2021, The Strongtalk authors and contributors



//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// The file defines the interface to the internal primitives.

// Primitives are divided into four categories:
//                                                        (canScavenge, NonLocalReturn,   canBeConstantFolded)
// 1) Allocation primitives.                              (true,        false, false)
// 2) Pure primitives.                                    (false,       false, true)
// 3) Function primitives                                 (false,       false, false)
// 4) Primitives who might scavenge, gc, or call delta    (true,        false, false)
//    .


// WARNING: do not change the element order of enum PrimitiveGroup
// without adjusting the Smalltalk DeltaPrimitiveGenerator code to match!

enum class PrimitiveGroup {
    NormalPrimitive,            //

    IntComparisonPrimitive,     // Integer comparison primitive
    IntArithmeticPrimitive,     // Integer arithmetic

    FloatComparisonPrimitive,   // FP comparison primitive
    FloatArithmeticPrimitive,   // FP arithmetic

    ObjectArrayPrimitive,          // access/size
    ByteArrayPrimitive,         // access/size
    DoubleByteArrayPrimitive,   // access/size

    BlockPrimitive              // block-related primitives (creation, invocation, contexts)
};

// WARNING: do not change the element order of enum PrimitiveGroup
// without adjusting the Smalltalk DeltaPrimitiveGenerator code to match!
