//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for small integers

class smiOopPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <SmallInteger> primitiveLessThan: aNumber   <SmallInteger>
        //                           ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::lessThan' }
        //%
        static Oop __CALLING_CONVENTION lessThan( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveGreaterThan: aNumber   <SmallInteger>
        //                              ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::greaterThan' }
        //%
        static Oop __CALLING_CONVENTION greaterThan( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveLessThanOrEqual: aNumber   <SmallInteger>
        //                                  ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::lessThanOrEqual' }
        //%
        static Oop __CALLING_CONVENTION lessThanOrEqual( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveGreaterThanOrEqual: aNumber   <SmallInteger>
        //                                     ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::greaterThanOrEqual' }
        //%
        static Oop __CALLING_CONVENTION greaterThanOrEqual( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveSmallIntegerEqual: aNumber   <SmallInteger>
        //                                    ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::equal' }
        //%
        static Oop __CALLING_CONVENTION equal( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveSmallIntegerNotEqual: aNumber   <SmallInteger>
        //                                       ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { flags = #(Pure SmiCompare LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::notEqual' }
        //%
        static Oop __CALLING_CONVENTION notEqual( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveBitAnd: aNumber   <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::bitAnd' }
        //%
        static Oop __CALLING_CONVENTION bitAnd( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveBitOr: aNumber   <SmallInteger>
        //                        ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::bitOr' }
        //%
        static Oop __CALLING_CONVENTION bitOr( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveBitXor: aNumber   <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::bitXor' }
        //%
        static Oop __CALLING_CONVENTION bitXor( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveBitShift: aNumber   <SmallInteger>
        //                           ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::bitShift' }
        //%
        static Oop __CALLING_CONVENTION bitShift( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveRawBitShift: aNumber   <SmallInteger>
        //                              ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Pure SmiArith LastDeltaFrameNotNeeded)
        //              name  = 'smiOopPrimitives::rawBitShift' }
        //%
        static Oop __CALLING_CONVENTION rawBitShift( Oop receiver, Oop argument );

        //%prim
        // <SmallInteger> primitiveAsObjectIfFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #Function
        //              name  = 'smiOopPrimitives::asObject' }
        //%
        static Oop __CALLING_CONVENTION asObject( Oop receiver );

        // For debugging only
        //%prim
        // <SmallInteger> primitivePrintCharacterIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #Function
        //              name  = 'smiOopPrimitives::printCharacter' }
        //%
        static Oop __CALLING_CONVENTION printCharacter( Oop receiver );
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

