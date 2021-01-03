//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
#include "vm/memory/SymbolTable.hpp"


Oop simplified( ByteArrayOop result );


// Primitives for byte arrays

class byteArrayPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <IndexedByteInstanceVariables class>
        //   primitiveIndexedByteNew: size      <SmallInteger>
        //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NegativeSize)
        //              flags = #(Allocate)
        //              name  = 'byteArrayPrimitives::allocateSize' }
        //%
        static PRIM_DECL_2( allocateSize, Oop receiver, Oop argument );

        //%prim
        // <NoReceiver>
        //   primitiveIndexedByteNew:  class     <IndexedByteInstanceVariables class>
        //                    size:    size      <SmallInteger>
        //                    tenured: tenured   <Boolean>
        //                    ifFail:  failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NegativeSize)
        //              flags = #(Allocate)
        //              name  = 'byteArrayPrimitives::allocateSize2' }
        //%
        static PRIM_DECL_3( allocateSize2, Oop receiver, Oop argument, Oop tenured );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteSize ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedByte)
        //              name  = 'byteArrayPrimitives::size' }
        //%
        static PRIM_DECL_1( size, Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveSymbolNumberOfArguments ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedByte)
        //              name  = 'byteArrayPrimitives::numberOfArguments' }
        //%
        static PRIM_DECL_1( numberOfArguments, Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteAt: index <SmallInteger>
        //                   ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::at' }
        //%
        static PRIM_DECL_2( at, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteAt: index     <SmallInteger>
        //                      put: c         <SmallInteger>
        //                   ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds ValueOutOfBounds)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::atPut' }
        //%
        static PRIM_DECL_3( atPut, Oop receiver, Oop index, Oop value );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteCompare: index <String>
        //                        ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { name = 'byteArrayPrimitives::compare' }
        //%
        static PRIM_DECL_2( compare, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteInternIfFail: failBlock <PrimFailBlock> ^<CompressedSymbol> =
        //   Internal { error = #(ValueOutOfBounds)
        //              name  = 'byteArrayPrimitives::intern' }
        //%
        static PRIM_DECL_1( intern, Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteCharacterAt: index <SmallInteger>
        //                            ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::characterAt' }
        //%
        static PRIM_DECL_2( characterAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteAtAllPut: c <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { name  = 'byteArrayPrimitives::at_all_put' }
        //%
        static PRIM_DECL_2( at_all_put, Oop receiver, Oop c );


        // SUPPORT FOR LARGE INTEGER

        //%prim
        // <IndexedByteInstanceVariables class>
        //   primitiveIndexedByteLargeIntegerFromSmallInteger: number  <SmallInteger>
        //                                             ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags = #(Function)
        //              name  = 'byteArrayPrimitives::largeIntegerFromSmallInteger' }
        //%
        static PRIM_DECL_2( largeIntegerFromSmallInteger, Oop receiver, Oop number );

        //%prim
        // <IndexedByteInstanceVariables class>
        //   primitiveIndexedByteLargeIntegerFromFloat: number  <Float>
        //                                      ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags = #(Function)
        //              name  = 'byteArrayPrimitives::largeIntegerFromDouble' }
        //%
        static PRIM_DECL_2( largeIntegerFromDouble, Oop receiver, Oop number );

        //%prim
        // <IndexedByteInstanceVariables class>
        //   primitiveIndexedByteLargeIntegerFromString: argument  <String>
        //                                         base: base      <Integer>
        //                                       ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { error = #(ConversionFailed)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerFromString' }
        //%
        static PRIM_DECL_3( largeIntegerFromString, Oop receiver, Oop argument, Oop base );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerAdd: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerAdd' }
        //%
        static PRIM_DECL_2( largeIntegerAdd, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerSubtract: argument <IndexedByteInstanceVariables>
        //                                     ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerSubtract' }
        //%
        static PRIM_DECL_2( largeIntegerSubtract, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerMultiply: argument <IndexedByteInstanceVariables>
        //                                     ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerMultiply' }
        //%
        static PRIM_DECL_2( largeIntegerMultiply, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerQuo: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerQuo' }
        //%
        static PRIM_DECL_2( largeIntegerQuo, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerDiv: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerDiv' }
        //%
        static PRIM_DECL_2( largeIntegerDiv, Oop receiver, Oop argument );


        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerMod: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerMod' }
        //%
        static PRIM_DECL_2( largeIntegerMod, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerRem: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerRem' }
        //%
        static PRIM_DECL_2( largeIntegerRem, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerAnd: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerAnd' }
        //%
        static PRIM_DECL_2( largeIntegerAnd, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerXor: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerXor' }
        //%
        static PRIM_DECL_2( largeIntegerXor, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerOr: argument <IndexedByteInstanceVariables>
        //                               ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerOr' }
        //%
        static PRIM_DECL_2( largeIntegerOr, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerShift: argument <SmallInt>
        //                                  ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerShift' }
        //%
        static PRIM_DECL_2( largeIntegerShift, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerCompare: argument <IndexedByteInstanceVariables>
        //                                    ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerCompare' }
        //%
        static PRIM_DECL_2( largeIntegerCompare, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerAsFloatIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerToFloat' }
        //%
        static PRIM_DECL_1( largeIntegerToFloat, Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerToStringBase: base      <SmallInteger>
        //                                         ifFail: failBlock <PrimFailBlock> ^<String> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerToString' }
        //%
        static PRIM_DECL_2( largeIntegerToString, Oop receiver, Oop base );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteHash ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedByte)
        //              name  = 'byteArrayPrimitives::hash' }
        //%
        static PRIM_DECL_1( hash, Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveLargeIntegerHash ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerHash' }
        //%
        static PRIM_DECL_1( largeIntegerHash, Oop receiver );

        // Aliens primitives

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSizeIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienGetSize' }
        //%
        static PRIM_DECL_1( alienGetSize, Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSize: size <SmallInteger>
        //               ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSetSize' }
        //%
        static PRIM_DECL_2( alienSetSize, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienAddressIfFail: failBlock <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienGetAddress' }
        //%
        static PRIM_DECL_1( alienGetAddress, Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienAddress: address <Integer>
        //                  ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSetAddress' }
        //%
        static PRIM_DECL_2( alienSetAddress, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedByteAt: index  <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedByteAt' }
        //%
        static PRIM_DECL_2( alienUnsignedByteAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedByteAt: index  <SmallInteger>
        //                            put: value  <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedByteAtPut' }
        //%
        static PRIM_DECL_3( alienUnsignedByteAtPut, Oop receiver, Oop index, Oop value );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedByteAt: index  <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedByteAt' }
        //%
        static PRIM_DECL_2( alienSignedByteAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedByteAt: index  <SmallInteger>
        //                          put: value  <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedByteAtPut' }
        //%
        static PRIM_DECL_3( alienSignedByteAtPut, Oop receiver, Oop index, Oop value );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedShortAt: index  <SmallInteger>
        //                          ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedShortAt' }
        //%
        static PRIM_DECL_2( alienUnsignedShortAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedShortAt: index  <SmallInteger>
        //                             put: value  <SmallInteger>
        //                          ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedShortAtPut' }
        //%
        static PRIM_DECL_3( alienUnsignedShortAtPut, Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedShortAt: index  <SmallInteger>
        //                        ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedShortAt' }
        //%
        static PRIM_DECL_2( alienSignedShortAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedShortAt: index  <SmallInteger>
        //                           put: value  <SmallInteger>
        //                        ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedShortAtPut' }
        //%
        static PRIM_DECL_3( alienSignedShortAtPut, Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedLongAt: index  <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger|LargeInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedLongAt' }
        //%
        static PRIM_DECL_2( alienUnsignedLongAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedLongAt: index  <SmallInteger>
        //                            put: value  <SmallInteger|LargeInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger|LargeInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedLongAtPut' }
        //%
        static PRIM_DECL_3( alienUnsignedLongAtPut, Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedLongAt: index  <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger|LargeInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedLongAt' }
        //%
        static PRIM_DECL_2( alienSignedLongAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedLongAt: index  <SmallInteger>
        //                          put: value  <SmallInteger|LargeInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger|LargeInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedLongAtPut' }
        //%
        static PRIM_DECL_3( alienSignedLongAtPut, Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienDoubleAt: index  <SmallInteger>
        //                   ifFail: failBlock <PrimFailBlock> ^<Double> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienDoubleAt' }
        //%
        static PRIM_DECL_2( alienDoubleAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienDoubleAt: index  <SmallInteger>
        //                      put: value  <Double>
        //                   ifFail: failBlock <PrimFailBlock> ^<Double> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienDoubleAtPut' }
        //%
        static PRIM_DECL_3( alienDoubleAtPut, Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienFloatAt: index  <SmallInteger>
        //                  ifFail: failBlock <PrimFailBlock> ^<Double> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienFloatAt' }
        //%
        static PRIM_DECL_2( alienFloatAt, Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienFloatAt: index  <SmallInteger>
        //                     put: value  <Double>
        //                  ifFail: failBlock <PrimFailBlock> ^<Double> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienFloatAtPut' }
        //%
        static PRIM_DECL_3( alienFloatAtPut, Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result  <IndexedByteInstanceVariables>
        //                     ifFail: failBlock <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult0' }
        //%
        static PRIM_DECL_2( alienCallResult0, Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result    <IndexedByteInstanceVariables>
        //                       with: argument  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult1' }
        //%
        static PRIM_DECL_3( alienCallResult1, Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //                       with: argument1  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument2  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult2' }
        //%
        static PRIM_DECL_4( alienCallResult2, Oop receiver, Oop argument1, Oop argument2, Oop argument3 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //                       with: argument1  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument2  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument3  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult3' }
        //%
        static PRIM_DECL_5( alienCallResult3, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //                       with: argument1  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument2  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument3  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument4  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult4' }
        //%
        static PRIM_DECL_6( alienCallResult4, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //                       with: argument1  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument2  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument3  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument4  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument5  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult5' }
        //%
        static PRIM_DECL_7( alienCallResult5, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //                       with: argument1  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument2  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument3  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument4  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument5  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument6  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult6' }
        //%
        static PRIM_DECL_8( alienCallResult6, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6, Oop argument7 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //                       with: argument1  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument2  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument3  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument4  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument5  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument6  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument7  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult7' }
        //%
        static PRIM_DECL_9( alienCallResult7, Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6, Oop argument7, Oop argument8 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //              withArguments: argument1  <IndexedInstanceVariables>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResultWithArguments' }
        //%
        static PRIM_DECL_3( alienCallResultWithArguments, Oop receiver, Oop argument1, Oop argument2 );
};

