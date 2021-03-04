//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/primitive/primitive_declarations.hpp"
#include "vm/primitive/primitive_tracing.hpp"


// Primitives for doubles
//
//  alias PrimFailBlock : <[Symbol, ^BottomType]>

class DoubleOopPrimitives : AllStatic {
private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static std::int32_t number_of_calls;

    //%prim
    // <Float> primitiveFloatLessThan:  aNumber   <Float>
    //                         ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { doc   = 'Returns whether the receiver is less than the argument'
    //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
    //              name  = 'DoubleOopPrimitives::lessThan' }
    //%
    static PRIM_DECL_2( lessThan, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatGreaterThan: aNumber   <Float>
    //                            ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { doc   = 'Returns whether the receiver is greater than the argument'
    //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
    //              name  = 'DoubleOopPrimitives::greaterThan' }
    //%
    static PRIM_DECL_2( greaterThan, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatLessThanOrEqual: aNumber   <Float>
    //                                ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { doc   = 'Returns whether the receiver is less than or equal to the argument'
    //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
    //              name  = 'DoubleOopPrimitives::lessThanOrEqual' }
    //%
    static PRIM_DECL_2( lessThanOrEqual, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatGreaterThanOrEqual: aNumber   <Float>
    //                                   ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { doc   = 'Returns whether the receiver is greater than or equal to the argument'
    //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
    //              name  = 'DoubleOopPrimitives::greaterThanOrEqual' }
    //%
    static PRIM_DECL_2( greaterThanOrEqual, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatEqual: aNumber   <Float>
    //                      ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { doc   = 'Returns whether the receiver is equal to the argument'
    //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
    //              name  = 'DoubleOopPrimitives::equal' }
    //%
    static PRIM_DECL_2( equal, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatNotEqual: aNumber   <Float>
    //                         ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { doc   = 'Returns whether the receiver is not equal to the argument'
    //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
    //              name  = 'DoubleOopPrimitives::notEqual' }
    //%
    static PRIM_DECL_2( notEqual, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatMod: aNumber   <Float>
    //                    ifFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the result of dividing the receiver by the argument'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::mod' }
    //%
    static PRIM_DECL_2( mod, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatCosine ^<Float> =
    //   Internal { doc   = 'Returns the cosine of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::cosine' }
    //%
    static PRIM_DECL_1( cosine, Oop receiver );

    //%prim
    // <Float> primitiveFloatSine ^<Float> =
    //   Internal { doc   = 'Returns the sine of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::sine' }
    //%
    static PRIM_DECL_1( sine, Oop receiver );

    //%prim
    // <Float> primitiveFloatTangentIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the tangent of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::tangent' }
    //%
    static PRIM_DECL_1( tangent, Oop receiver );

    //%prim
    // <Float> primitiveFloatArcCosineIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the arc-cosine of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::arcCosine' }
    //%
    static PRIM_DECL_1( arcCosine, Oop receiver );

    //%prim
    // <Float> primitiveFloatArcSineIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the arc-sine of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::arcSine' }
    //%
    static PRIM_DECL_1( arcSine, Oop receiver );

    //%prim
    // <Float> primitiveFloatArcTangentIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the arc-tangent of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::arcTangent' }
    //%
    static PRIM_DECL_1( arcTangent, Oop receiver );

    //%prim
    // <Float> primitiveFloatHyperbolicCosineIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the hyperbolic-cosine of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::hyperbolicCosine' }
    //%
    static PRIM_DECL_1( hyperbolicCosine, Oop receiver );

    //%prim
    // <Float> primitiveFloatHyperbolicSineIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the hyperbolic-sine of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::hyperbolicSine' }
    //%
    static PRIM_DECL_1( hyperbolicSine, Oop receiver );

    //%prim
    // <Float> primitiveFloatHyperbolicTangentIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the hyperbolic-tangent of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::hyperbolicTangent' }
    //%
    static PRIM_DECL_1( hyperbolicTangent, Oop receiver );

    //%prim
    // <Float> primitiveFloatSqrtIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the square root of the receiver'
    //              error = #(ReceiverNegative)
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::sqrt' }
    //%
    static PRIM_DECL_1( sqrt, Oop receiver );

    //%prim
    // <Float> primitiveFloatSquared ^<Float> =
    //   Internal { doc   = 'Returns the result of multiplying the receiver by it self'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::squared' }
    //%
    static PRIM_DECL_1( squared, Oop receiver );

    //%prim
    // <Float> primitiveFloatLnIfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the logarithm of the receiver'
    //              error = #(ReceiverNotStrictlyPositive)
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::ln' }
    //%
    static PRIM_DECL_1( ln, Oop receiver );

    //%prim
    // <Float> primitiveFloatExp ^<Float> =
    //   Internal { doc   = 'Returns the exponential value of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::exp' }
    //%
    static PRIM_DECL_1( exp, Oop receiver );

    //%prim
    // <Float> primitiveFloatLog10IfFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the base 10 logarithm of the receiver'
    //              error = #(ReceiverNotStrictlyPositive)
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::log10' }
    //%
    static PRIM_DECL_1( log10, Oop receiver );

    //%prim
    // <Float> primitiveFloatIsNan ^<Boolean> =
    //   Internal { doc   = 'Returns whether the receiver is NaN (Not a Number)'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::isNan' }
    //%
    static PRIM_DECL_1( isNan, Oop receiver );

    //%prim
    // <Float> primitiveFloatIsFinite ^<Boolean> =
    //   Internal { doc   = 'Returns whether the receiver is finite (not NaN)'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::isFinite' }
    //%
    static PRIM_DECL_1( isFinite, Oop receiver );

    //%prim
    // <Float> primitiveFloatFloor ^<Float> =
    //   Internal { doc   = 'Returns the largest integral Float that is less than or equal to the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::floor' }
    //%
    static PRIM_DECL_1( floor, Oop receiver );

    //%prim
    // <Float> primitiveFloatSmallIntegerFloorIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { doc   = 'Returns the largest SmallInteger that is less than or equal to the receiver'
    //              flags = #(Pure DoubleArith LastDeltaFrameNotNeeded)
    //              error = #(ConversionFailed)
    //              name  = 'DoubleOopPrimitives::smi_floor' }
    //%
    static PRIM_DECL_1( smi_floor, Oop receiver );

    //%prim
    // <Float> primitiveFloatCeiling ^<Float> =
    //   Internal { doc   = 'Returns the smallest integral Float that is greater than or equal to the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::ceiling' }
    //%
    static PRIM_DECL_1( ceiling, Oop receiver );

    //%prim
    // <Float> primitiveFloatExponent ^<SmallInteger> =
    //   Internal { doc   = 'Returns the exponent part of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::exponent' }
    //%
    static PRIM_DECL_1( exponent, Oop receiver );

    //%prim
    // <Float> primitiveFloatMantissa ^<Float> =
    //   Internal { doc   = 'Returns the mantissa part of the receiver'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::mantissa' }
    //%
    static PRIM_DECL_1( mantissa, Oop receiver );

    //%prim
    // <Float> primitiveFloatTruncated  ^<Float> =
    //   Internal { doc   = 'Returns the receiver truncated'
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::truncated' }
    //%
    static PRIM_DECL_1( truncated, Oop receiver );

    //%prim
    // <Float> primitiveFloatTimesTwoPower: aNumber   <SmallInteger>
    //                              ifFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { doc   = 'Returns the receiver multiplied with 2 to the power of aNumber'
    //              error = #(RangeError)
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::timesTwoPower' }
    //%
    static PRIM_DECL_2( timesTwoPower, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatRoundedAsSmallIntegerIfFail: failBlock <PrimFailBlock>  ^<SmallInteger> =
    //   Internal { doc   = 'Returns the receiver converted to a SmallInteger'
    //              error = #(SmallIntegerConversionFailed)
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::roundedAsSmallInteger' }
    //%
    static PRIM_DECL_1( roundedAsSmallInteger, Oop receiver );

    //%prim
    // <Float> primitiveFloatAsSmallIntegerIfFail: failBlock <PrimFailBlock>  ^<SmallInteger> =
    //   Internal { doc   = 'Returns the receiver as a SmallInteger'
    //              error = #(SmallIntegerConversionFailed)
    //              flags = #(Pure DoubleArith)
    //              name  = 'DoubleOopPrimitives::asSmallInteger' }
    //%
    static PRIM_DECL_1( asSmallInteger, Oop receiver );

    //%prim
    // <Float> primitiveFloatPrintFormat: format    <IndexedByteInstanceVariables>
    //                            ifFail: failBlock <PrimFailBlock> ^<Self> =
    //   Internal { doc   = 'Prints the receiver using the format and returns the recever'
    //              flags = #Function
    //              name  = 'DoubleOopPrimitives::printFormat' }
    //%
    static PRIM_DECL_2( printFormat, Oop receiver, Oop argument );

    //%prim
    // <Float> primitiveFloatPrintString ^<IndexedByteInstanceVariables> =
    //   Internal { doc   = 'Returns the print string for the receiver'
    //              flags = #Function
    //              name  = 'DoubleOopPrimitives::printString' }
    //%
    static PRIM_DECL_1( printString, Oop receiver );

    //%prim
    // <NoReceiver> primitiveFloatMaxValue ^<Float> =
    //   Internal { doc   = 'Returns the maximum Float value'
    //              flags = #(Pure)
    //              name  = 'DoubleOopPrimitives::min_positive_value' }
    //%
    static PRIM_DECL_0( max_value );

    //%prim
    // <NoReceiver> primitiveFloatMinPositiveValue ^<Float> =
    //   Internal { doc   = 'Returns the minimum positive Float value'
    //              flags = #(Pure)
    //              name  = 'DoubleOopPrimitives::min_positive_value' }
    //%
    static PRIM_DECL_0( min_positive_value );

    //%prim
    // <Float> primitiveFloatStoreString ^<ByteArray> =
    //   Internal { flags = #(Function)
    //              name  = 'DoubleOopPrimitives::store_string' }
    //%
    static PRIM_DECL_1( store_string, Oop receiver );

    //%prim
    // <NoReceiver> primitiveMandelbrotAtRe: re        <Float>
    //                                   im: im        <Float>
    //                              iterate: n         <SmallInteger>
    //                               ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { doc   = 'Returns no. of iterations used for Mandelbrot value at (re, im)'
    //              flags = #(Pure LastDeltaFrameNotNeeded)
    //              name  = 'DoubleOopPrimitives::mandelbrot' }
    //%
    static PRIM_DECL_3( mandelbrot, Oop re, Oop im, Oop n );
};

//%prim
// <Float> primitiveFloatSubtract: aNumber   <Float>
//                         ifFail: failBlock <PrimFailBlock> ^<Float> =
//   Internal { doc   = 'Returns the result of subtracting the argument from the receiver'
//              flags = #(Pure DoubleArith LastDeltaFrameNotNeeded)
//              name  = 'double_subtract' }
//%
extern "C" Oop __CALLING_CONVENTION double_subtract( Oop receiver, Oop argument );

//%prim
// <Float> primitiveFloatDivide: aNumber   <Float>
//                       ifFail: failBlock <PrimFailBlock> ^<Float> =
//   Internal { doc   = 'Returns the modulus of the receiver by the argument'
//              flags = #(Pure DoubleArith LastDeltaFrameNotNeeded)
//              name  = 'double_divide' }
//%
extern "C" Oop __CALLING_CONVENTION double_divide( Oop receiver, Oop argument );

//%prim
// <Float> primitiveFloatAdd: aNumber   <Float>
//                    ifFail: failBlock <PrimFailBlock> ^<Float> =
//   Internal { doc   = 'Returns the sum of the receiver and the argument'
//              flags = #(Pure DoubleArith LastDeltaFrameNotNeeded)
//              name  = 'double_add' }
//%
extern "C" Oop __CALLING_CONVENTION double_add( Oop receiver, Oop argument );

//%prim
// <Float> primitiveFloatMultiply: aNumber   <Float>
//                         ifFail: failBlock <PrimFailBlock> ^<Float> =
//   Internal { doc   = 'Returns the multiply of the receiver and the argument'
//              flags = #(Pure DoubleArith LastDeltaFrameNotNeeded)
//              name  = 'double_multiply' }
//%
extern "C" Oop __CALLING_CONVENTION double_multiply( Oop receiver, Oop argument );
