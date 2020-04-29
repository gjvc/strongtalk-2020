//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
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
        static Oop __CALLING_CONVENTION allocateSize( Oop receiver, Oop argument );

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
        static Oop __CALLING_CONVENTION allocateSize2( Oop receiver, Oop argument, Oop tenured );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteSize ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedByte)
        //              name  = 'byteArrayPrimitives::size' }
        //%
        static Oop __CALLING_CONVENTION size( Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveSymbolNumberOfArguments ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedByte)
        //              name  = 'byteArrayPrimitives::numberOfArguments' }
        //%
        static Oop __CALLING_CONVENTION numberOfArguments( Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteAt: index <SmallInteger>
        //                   ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::at' }
        //%
        static Oop __CALLING_CONVENTION at( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteAt: index     <SmallInteger>
        //                      put: c         <SmallInteger>
        //                   ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds ValueOutOfBounds)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::atPut' }
        //%
        static Oop __CALLING_CONVENTION atPut( Oop receiver, Oop index, Oop value );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteCompare: index <String>
        //                        ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { name = 'byteArrayPrimitives::compare' }
        //%
        static Oop __CALLING_CONVENTION compare( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteInternIfFail: failBlock <PrimFailBlock> ^<CompressedSymbol> =
        //   Internal { error = #(ValueOutOfBounds)
        //              name  = 'byteArrayPrimitives::intern' }
        //%
        static Oop __CALLING_CONVENTION intern( Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteCharacterAt: index <SmallInteger>
        //                            ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::characterAt' }
        //%
        static Oop __CALLING_CONVENTION characterAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteAtAllPut: c <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { name  = 'byteArrayPrimitives::at_all_put' }
        //%
        static Oop __CALLING_CONVENTION at_all_put( Oop receiver, Oop c );


        // SUPPORT FOR LARGE INTEGER

        //%prim
        // <IndexedByteInstanceVariables class>
        //   primitiveIndexedByteLargeIntegerFromSmallInteger: number  <SmallInteger>
        //                                             ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags = #(Function)
        //              name  = 'byteArrayPrimitives::largeIntegerFromSmallInteger' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerFromSmallInteger( Oop receiver, Oop number );

        //%prim
        // <IndexedByteInstanceVariables class>
        //   primitiveIndexedByteLargeIntegerFromFloat: number  <Float>
        //                                      ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags = #(Function)
        //              name  = 'byteArrayPrimitives::largeIntegerFromDouble' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerFromDouble( Oop receiver, Oop number );

        //%prim
        // <IndexedByteInstanceVariables class>
        //   primitiveIndexedByteLargeIntegerFromString: argument  <String>
        //                                         base: base      <Integer>
        //                                       ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { error = #(ConversionFailed)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerFromString' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerFromString( Oop receiver, Oop argument, Oop base );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerAdd: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerAdd' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerAdd( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerSubtract: argument <IndexedByteInstanceVariables>
        //                                     ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerSubtract' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerSubtract( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerMultiply: argument <IndexedByteInstanceVariables>
        //                                     ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerMultiply' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerMultiply( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerQuo: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerQuo' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerQuo( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerDiv: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerDiv' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerDiv( Oop receiver, Oop argument );


        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerMod: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerMod' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerMod( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerRem: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerRem' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerRem( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerAnd: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerAnd' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerAnd( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerXor: argument <IndexedByteInstanceVariables>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerXor' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerXor( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerOr: argument <IndexedByteInstanceVariables>
        //                               ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerOr' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerOr( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerShift: argument <SmallInt>
        //                                  ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables|SmallInteger> =
        //   Internal { error = #(ArgumentIsInvalid DivisionByZero)
        //              flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerShift' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerShift( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerCompare: argument <IndexedByteInstanceVariables>
        //                                    ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerCompare' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerCompare( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerAsFloatIfFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerToFloat' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerToFloat( Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteLargeIntegerToStringBase: base      <SmallInteger>
        //                                         ifFail: failBlock <PrimFailBlock> ^<String> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerToString' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerToString( Oop receiver, Oop base );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveIndexedByteHash ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedByte)
        //              name  = 'byteArrayPrimitives::hash' }
        //%
        static Oop __CALLING_CONVENTION hash( Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveLargeIntegerHash ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedByte)
        //              name  = 'byteArrayPrimitives::largeIntegerHash' }
        //%
        static Oop __CALLING_CONVENTION largeIntegerHash( Oop receiver );

        // Aliens primitives

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSizeIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienGetSize' }
        //%
        static Oop __CALLING_CONVENTION alienGetSize( Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSize: size <SmallInteger>
        //               ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSetSize' }
        //%
        static Oop __CALLING_CONVENTION alienSetSize( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienAddressIfFail: failBlock <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienGetAddress' }
        //%
        static Oop __CALLING_CONVENTION alienGetAddress( Oop receiver );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienAddress: address <Integer>
        //                  ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSetAddress' }
        //%
        static Oop __CALLING_CONVENTION alienSetAddress( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedByteAt: index  <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedByteAt' }
        //%
        static Oop __CALLING_CONVENTION alienUnsignedByteAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedByteAt: index  <SmallInteger>
        //                            put: value  <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedByteAtPut' }
        //%
        static Oop __CALLING_CONVENTION alienUnsignedByteAtPut( Oop receiver, Oop index, Oop value );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedByteAt: index  <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedByteAt' }
        //%
        static Oop __CALLING_CONVENTION alienSignedByteAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedByteAt: index  <SmallInteger>
        //                          put: value  <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedByteAtPut' }
        //%
        static Oop __CALLING_CONVENTION alienSignedByteAtPut( Oop receiver, Oop index, Oop value );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedShortAt: index  <SmallInteger>
        //                          ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedShortAt' }
        //%
        static Oop __CALLING_CONVENTION alienUnsignedShortAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedShortAt: index  <SmallInteger>
        //                             put: value  <SmallInteger>
        //                          ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedShortAtPut' }
        //%
        static Oop __CALLING_CONVENTION alienUnsignedShortAtPut( Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedShortAt: index  <SmallInteger>
        //                        ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedShortAt' }
        //%
        static Oop __CALLING_CONVENTION alienSignedShortAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedShortAt: index  <SmallInteger>
        //                           put: value  <SmallInteger>
        //                        ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedShortAtPut' }
        //%
        static Oop __CALLING_CONVENTION alienSignedShortAtPut( Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedLongAt: index  <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger|LargeInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedLongAt' }
        //%
        static Oop __CALLING_CONVENTION alienUnsignedLongAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienUnsignedLongAt: index  <SmallInteger>
        //                            put: value  <SmallInteger|LargeInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger|LargeInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienUnsignedLongAtPut' }
        //%
        static Oop __CALLING_CONVENTION alienUnsignedLongAtPut( Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedLongAt: index  <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger|LargeInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedLongAt' }
        //%
        static Oop __CALLING_CONVENTION alienSignedLongAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienSignedLongAt: index  <SmallInteger>
        //                          put: value  <SmallInteger|LargeInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger|LargeInteger> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienSignedLongAtPut' }
        //%
        static Oop __CALLING_CONVENTION alienSignedLongAtPut( Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienDoubleAt: index  <SmallInteger>
        //                   ifFail: failBlock <PrimFailBlock> ^<Double> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienDoubleAt' }
        //%
        static Oop __CALLING_CONVENTION alienDoubleAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienDoubleAt: index  <SmallInteger>
        //                      put: value  <Double>
        //                   ifFail: failBlock <PrimFailBlock> ^<Double> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienDoubleAtPut' }
        //%
        static Oop __CALLING_CONVENTION alienDoubleAtPut( Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienFloatAt: index  <SmallInteger>
        //                  ifFail: failBlock <PrimFailBlock> ^<Double> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienFloatAt' }
        //%
        static Oop __CALLING_CONVENTION alienFloatAt( Oop receiver, Oop index );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienFloatAt: index  <SmallInteger>
        //                     put: value  <Double>
        //                  ifFail: failBlock <PrimFailBlock> ^<Double> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienFloatAtPut' }
        //%
        static Oop __CALLING_CONVENTION alienFloatAtPut( Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result  <IndexedByteInstanceVariables>
        //                     ifFail: failBlock <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult0' }
        //%
        static Oop __CALLING_CONVENTION alienCallResult0( Oop receiver, Oop argument );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result    <IndexedByteInstanceVariables>
        //                       with: argument  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult1' }
        //%
        static Oop __CALLING_CONVENTION alienCallResult1( Oop receiver, Oop argument1, Oop argument2 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //                       with: argument1  <IndexedByteInstanceVariables|SmallInteger>
        //                       with: argument2  <IndexedByteInstanceVariables|SmallInteger>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResult2' }
        //%
        static Oop __CALLING_CONVENTION alienCallResult2( Oop receiver, Oop argument1, Oop argument2, Oop argument3 );

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
        static Oop __CALLING_CONVENTION alienCallResult3( Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4 );

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
        static Oop __CALLING_CONVENTION alienCallResult4( Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5 );

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
        static Oop __CALLING_CONVENTION alienCallResult5( Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6 );

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
        static Oop __CALLING_CONVENTION alienCallResult6( Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6, Oop argument7 );

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
        static Oop __CALLING_CONVENTION alienCallResult7( Oop receiver, Oop argument1, Oop argument2, Oop argument3, Oop argument4, Oop argument5, Oop argument6, Oop argument7, Oop argument8 );

        //%prim
        // <IndexedByteInstanceVariables>
        //   primitiveAlienCallResult: result     <IndexedByteInstanceVariables>
        //              withArguments: argument1  <IndexedInstanceVariables>
        //                     ifFail: failBlock  <PrimFailBlock> ^<Integer> =
        //   Internal { flags = #(Function IndexedByte)
        //              name  = 'byteArrayPrimitives::alienCallResultWithArguments' }
        //%
        static Oop __CALLING_CONVENTION alienCallResultWithArguments( Oop receiver, Oop argument1, Oop argument2 );
};

