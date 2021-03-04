//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"
#include "vm/primitive/primitive_declarations.hpp"
#include "vm/primitive/primitive_tracing.hpp"
// Primitives for small integers

class SmallIntegerOopPrimitives : AllStatic {
private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static std::int32_t number_of_calls;

    //%prim
    // <SmallInteger> primitiveLessThan: aNumber   <SmallInteger>
    //                           ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::lessThan' }
    //%
    static PRIM_DECL_2( lessThan, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveGreaterThan: aNumber   <SmallInteger>
    //                              ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::greaterThan' }
    //%
    static PRIM_DECL_2( greaterThan, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveLessThanOrEqual: aNumber   <SmallInteger>
    //                                  ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::lessThanOrEqual' }
    //%
    static PRIM_DECL_2( lessThanOrEqual, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveGreaterThanOrEqual: aNumber   <SmallInteger>
    //                                     ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::greaterThanOrEqual' }
    //%
    static PRIM_DECL_2( greaterThanOrEqual, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveSmallIntegerEqual: aNumber   <SmallInteger>
    //                                    ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::equal' }
    //%
    static PRIM_DECL_2( equal, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveSmallIntegerNotEqual: aNumber   <SmallInteger>
    //                                       ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::notEqual' }
    //%
    static PRIM_DECL_2( notEqual, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveBitAnd: aNumber   <SmallInteger>
    //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::bitAnd' }
    //%
    static PRIM_DECL_2( bitAnd, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveBitOr: aNumber   <SmallInteger>
    //                        ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::bitOr' }
    //%
    static PRIM_DECL_2( bitOr, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveBitXor: aNumber   <SmallInteger>
    //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::bitXor' }
    //%
    static PRIM_DECL_2( bitXor, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveBitShift: aNumber   <SmallInteger>
    //                           ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::bitShift' }
    //%
    static PRIM_DECL_2( bitShift, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveRawBitShift: aNumber   <SmallInteger>
    //                              ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
    //              name  = 'SmallIntegerOopPrimitives::rawBitShift' }
    //%
    static PRIM_DECL_2( rawBitShift, Oop receiver, Oop argument );

    //%prim
    // <SmallInteger> primitiveAsObjectIfFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(OutOfBounds)
    //              flags = #Function
    //              name  = 'SmallIntegerOopPrimitives::asObject' }
    //%
    static PRIM_DECL_1( asObject, Oop receiver );

    // For debugging only
    //%prim
    // <SmallInteger> primitivePrintCharacterIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { error = #(OutOfBounds)
    //              flags = #Function
    //              name  = 'SmallIntegerOopPrimitives::printCharacter' }
    //%
    static PRIM_DECL_1( printCharacter, Oop receiver );
};

// Assembler optimized primitives

//%prim
// <SmallInteger> primitiveAdd: aNumber   <SmallInteger>
//                      ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
//   Internal { error = #(Overflow)
//              flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
//              name  = 'smiOopPrimitives_add' }
//%
extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_add( Oop receiver, Oop argument );

//%prim
// <SmallInteger> primitiveSubtract: aNumber   <SmallInteger>
//                           ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
//   Internal { error = #(Overflow)
//              flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
//              name  = 'smiOopPrimitives_subtract' }
//%
extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_subtract( Oop receiver, Oop argument );

//%prim
// <SmallInteger> primitiveMultiply: aNumber   <SmallInteger>
//                           ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
//   Internal { error = #(Overflow)
//              flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
//              name  = 'smiOopPrimitives_multiply' }
//%
extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_multiply( Oop receiver, Oop argument );

//%prim
// <SmallInteger> primitiveMod: aNumber   <SmallInteger>
//                      ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
//   Internal { error = #(Overflow DivisionByZero)
//              flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
//              name  = 'smiOopPrimitives_mod' }
//%
extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_mod( Oop receiver, Oop argument );

//%prim
// <SmallInteger> primitiveDiv: aNumber   <SmallInteger>
//                      ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
//   Internal { error = #(Overflow DivisionByZero)
//              flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
//              name  = 'smiOopPrimitives_div' }
//%
extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_div( Oop receiver, Oop argument );

//%prim
// <SmallInteger> primitiveQuo: aNumber   <SmallInteger>
//                      ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
//   Internal { error = #(NotImplementedYet)
//            flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
//              name  = 'smiOopPrimitives_quo' }
//%
extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_quo( Oop receiver, Oop argument );

//%prim
// <SmallInteger> primitiveRemainder: aNumber   <SmallInteger>
//                            ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
//   Internal { error = #(DivisionByZero)
//              flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
//              name  = 'smiOopPrimitives_remainder' }
//%
extern "C" Oop __CALLING_CONVENTION smiOopPrimitives_remainder( Oop receiver, Oop argument );

//%prim
// <SmallInteger> primitiveAsFloat ^<Float> =
//   Internal { flags = #(Pure SmiArith)
//              name  = 'double_from_smi' }
//%
extern "C" Oop __CALLING_CONVENTION double_from_smi( Oop receiver );
