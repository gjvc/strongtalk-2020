//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for doubles
//
//  alias PrimFailBlock : <[Symbol, ^BottomType]>

class doubleOopPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <Float> primitiveFloatLessThan:  aNumber   <Float>
        //                         ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { doc   = 'Returns whether the receiver is less than the argument'
        //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
        //              name  = 'doubleOopPrimitives::lessThan' }
        //%
        static Oop __CALLING_CONVENTION lessThan( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatGreaterThan: aNumber   <Float>
        //                            ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { doc   = 'Returns whether the receiver is greater than the argument'
        //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
        //              name  = 'doubleOopPrimitives::greaterThan' }
        //%
        static Oop __CALLING_CONVENTION greaterThan( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatLessThanOrEqual: aNumber   <Float>
        //                                ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { doc   = 'Returns whether the receiver is less than or equal to the argument'
        //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
        //              name  = 'doubleOopPrimitives::lessThanOrEqual' }
        //%
        static Oop __CALLING_CONVENTION lessThanOrEqual( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatGreaterThanOrEqual: aNumber   <Float>
        //                                   ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { doc   = 'Returns whether the receiver is greater than or equal to the argument'
        //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
        //              name  = 'doubleOopPrimitives::greaterThanOrEqual' }
        //%
        static Oop __CALLING_CONVENTION greaterThanOrEqual( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatEqual: aNumber   <Float>
        //                      ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { doc   = 'Returns whether the receiver is equal to the argument'
        //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
        //              name  = 'doubleOopPrimitives::equal' }
        //%
        static Oop __CALLING_CONVENTION equal( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatNotEqual: aNumber   <Float>
        //                         ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { doc   = 'Returns whether the receiver is not equal to the argument'
        //              flags = #(Pure DoubleCompare LastDeltaFrameNotNeeded)
        //              name  = 'doubleOopPrimitives::notEqual' }
        //%
        static Oop __CALLING_CONVENTION notEqual( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatMod: aNumber   <Float>
        //                    ifFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the result of dividing the receiver by the argument'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::mod' }
        //%
        static Oop __CALLING_CONVENTION mod( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatCosine ^<Float> =
        //   Internal { doc   = 'Returns the cosine of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::cosine' }
        //%
        static Oop __CALLING_CONVENTION cosine( Oop receiver );

        //%prim
        // <Float> primitiveFloatSine ^<Float> =
        //   Internal { doc   = 'Returns the sine of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::sine' }
        //%
        static Oop __CALLING_CONVENTION sine( Oop receiver );

        //%prim
        // <Float> primitiveFloatTangentIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the tangent of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::tangent' }
        //%
        static Oop __CALLING_CONVENTION tangent( Oop receiver );

        //%prim
        // <Float> primitiveFloatArcCosineIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the arc-cosine of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::arcCosine' }
        //%
        static Oop __CALLING_CONVENTION arcCosine( Oop receiver );

        //%prim
        // <Float> primitiveFloatArcSineIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the arc-sine of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::arcSine' }
        //%
        static Oop __CALLING_CONVENTION arcSine( Oop receiver );

        //%prim
        // <Float> primitiveFloatArcTangentIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the arc-tangent of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::arcTangent' }
        //%
        static Oop __CALLING_CONVENTION arcTangent( Oop receiver );

        //%prim
        // <Float> primitiveFloatHyperbolicCosineIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the hyperbolic-cosine of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::hyperbolicCosine' }
        //%
        static Oop __CALLING_CONVENTION hyperbolicCosine( Oop receiver );

        //%prim
        // <Float> primitiveFloatHyperbolicSineIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the hyperbolic-sine of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::hyperbolicSine' }
        //%
        static Oop __CALLING_CONVENTION hyperbolicSine( Oop receiver );

        //%prim
        // <Float> primitiveFloatHyperbolicTangentIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the hyperbolic-tangent of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::hyperbolicTangent' }
        //%
        static Oop __CALLING_CONVENTION hyperbolicTangent( Oop receiver );

        //%prim
        // <Float> primitiveFloatSqrtIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the square root of the receiver'
        //              error = #(ReceiverNegative)
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::sqrt' }
        //%
        static Oop __CALLING_CONVENTION sqrt( Oop receiver );

        //%prim
        // <Float> primitiveFloatSquared ^<Float> =
        //   Internal { doc   = 'Returns the result of multiplying the receiver by it self'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::squared' }
        //%
        static Oop __CALLING_CONVENTION squared( Oop receiver );

        //%prim
        // <Float> primitiveFloatLnIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the logarithm of the receiver'
        //              error = #(ReceiverNotStrictlyPositive)
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::ln' }
        //%
        static Oop __CALLING_CONVENTION ln( Oop receiver );

        //%prim
        // <Float> primitiveFloatExp ^<Float> =
        //   Internal { doc   = 'Returns the exponential value of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::exp' }
        //%
        static Oop __CALLING_CONVENTION exp( Oop receiver );

        //%prim
        // <Float> primitiveFloatLog10IfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the base 10 logarithm of the receiver'
        //              error = #(ReceiverNotStrictlyPositive)
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::log10' }
        //%
        static Oop __CALLING_CONVENTION log10( Oop receiver );

        //%prim
        // <Float> primitiveFloatIsNan ^<Boolean> =
        //   Internal { doc   = 'Returns whether the receiver is NaN (Not a Number)'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::isNan' }
        //%
        static Oop __CALLING_CONVENTION isNan( Oop receiver );

        //%prim
        // <Float> primitiveFloatIsFinite ^<Boolean> =
        //   Internal { doc   = 'Returns whether the receiver is finite (not NaN)'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::isFinite' }
        //%
        static Oop __CALLING_CONVENTION isFinite( Oop receiver );

        //%prim
        // <Float> primitiveFloatFloor ^<Float> =
        //   Internal { doc   = 'Returns the largest integral Float that is less than or equal to the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::floor' }
        //%
        static Oop __CALLING_CONVENTION floor( Oop receiver );

        //%prim
        // <Float> primitiveFloatSmallIntegerFloorIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { doc   = 'Returns the largest SmallInteger that is less than or equal to the receiver'
        //              flags = #(Pure DoubleArith LastDeltaFrameNotNeeded)
        //              error = #(ConversionFailed)
        //              name  = 'doubleOopPrimitives::smi_floor' }
        //%
        static Oop __CALLING_CONVENTION smi_floor( Oop receiver );

        //%prim
        // <Float> primitiveFloatCeiling ^<Float> =
        //   Internal { doc   = 'Returns the smallest integral Float that is greater than or equal to the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::ceiling' }
        //%
        static Oop __CALLING_CONVENTION ceiling( Oop receiver );

        //%prim
        // <Float> primitiveFloatExponent ^<SmallInteger> =
        //   Internal { doc   = 'Returns the exponent part of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::exponent' }
        //%
        static Oop __CALLING_CONVENTION exponent( Oop receiver );

        //%prim
        // <Float> primitiveFloatMantissa ^<Float> =
        //   Internal { doc   = 'Returns the mantissa part of the receiver'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::mantissa' }
        //%
        static Oop __CALLING_CONVENTION mantissa( Oop receiver );

        //%prim
        // <Float> primitiveFloatTruncated  ^<Float> =
        //   Internal { doc   = 'Returns the receiver truncated'
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::truncated' }
        //%
        static Oop __CALLING_CONVENTION truncated( Oop receiver );

        //%prim
        // <Float> primitiveFloatTimesTwoPower: aNumber   <SmallInteger>
        //                              ifFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc   = 'Returns the receiver multiplied with 2 to the power of aNumber'
        //              error = #(RangeError)
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::timesTwoPower' }
        //%
        static Oop __CALLING_CONVENTION timesTwoPower( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatRoundedAsSmallIntegerIfFail: failBlock <PrimFailBlock>  ^<SmallInteger> =
        //   Internal { doc   = 'Returns the receiver converted to a SmallInteger'
        //              error = #(SmallIntegerConversionFailed)
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::roundedAsSmallInteger' }
        //%
        static Oop __CALLING_CONVENTION roundedAsSmallInteger( Oop receiver );

        //%prim
        // <Float> primitiveFloatAsSmallIntegerIfFail: failBlock <PrimFailBlock>  ^<SmallInteger> =
        //   Internal { doc   = 'Returns the receiver as a SmallInteger'
        //              error = #(SmallIntegerConversionFailed)
        //              flags = #(Pure DoubleArith)
        //              name  = 'doubleOopPrimitives::asSmallInteger' }
        //%
        static Oop __CALLING_CONVENTION asSmallInteger( Oop receiver );

        //%prim
        // <Float> primitiveFloatPrintFormat: format    <IndexedByteInstanceVariables>
        //                            ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { doc   = 'Prints the receiver using the format and returns the recever'
        //              flags = #Function
        //              name  = 'doubleOopPrimitives::printFormat' }
        //%
        static Oop __CALLING_CONVENTION printFormat( Oop receiver, Oop argument );

        //%prim
        // <Float> primitiveFloatPrintString ^<IndexedByteInstanceVariables> =
        //   Internal { doc   = 'Returns the print string for the receiver'
        //              flags = #Function
        //              name  = 'doubleOopPrimitives::printString' }
        //%
        static Oop __CALLING_CONVENTION printString( Oop receiver );

        //%prim
        // <NoReceiver> primitiveFloatMaxValue ^<Float> =
        //   Internal { doc   = 'Returns the maximum Float value'
        //              flags = #(Pure)
        //              name  = 'doubleOopPrimitives::min_positive_value' }
        //%
        static Oop __CALLING_CONVENTION max_value();

        //%prim
        // <NoReceiver> primitiveFloatMinPositiveValue ^<Float> =
        //   Internal { doc   = 'Returns the minimum positive Float value'
        //              flags = #(Pure)
        //              name  = 'doubleOopPrimitives::min_positive_value' }
        //%
        static Oop __CALLING_CONVENTION min_positive_value();

        //%prim
        // <Float> primitiveFloatStoreString ^<ByteArray> =
        //   Internal { flags = #(Function)
        //              name  = 'doubleOopPrimitives::store_string' }
        //%
        static Oop __CALLING_CONVENTION store_string( Oop receiver );

        //%prim
        // <NoReceiver> primitiveMandelbrotAtRe: re        <Float>
        //                                   im: im        <Float>
        //                              iterate: n         <SmallInteger>
        //                               ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { doc   = 'Returns no. of iterations used for Mandelbrot value at (re, im)'
        //              flags = #(Pure LastDeltaFrameNotNeeded)
        //              name  = 'doubleOopPrimitives::mandelbrot' }
        //%
        static Oop __CALLING_CONVENTION mandelbrot( Oop re, Oop im, Oop n );
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


