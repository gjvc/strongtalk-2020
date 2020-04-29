//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/utilities/objectIDTable.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for byte arrays

class doubleByteArrayPrimitives : AllStatic {

    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <IndexedDoubleByteInstanceVariables class>
        //   primitiveIndexedDoubleByteNew: size      <SmallInteger>
        //                          ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NegativeSize)
        //              flags = #(Allocate)
        //              name  = 'doubleByteArrayPrimitives::allocateSize' }
        //%
        static Oop __CALLING_CONVENTION allocateSize( Oop receiver, Oop argument );

        //%prim
        // <NoReceiver>
        //   primitiveIndexedDoubleByteNew: behaviour <IndexedDoubleByteInstanceVariables class>
        //                            size: size      <SmallInteger>
        //                         tenured: tenured   <Boolean>
        //                          ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NegativeSize)
        //              flags = #(Allocate)
        //              name  = 'doubleByteArrayPrimitives::allocateSize2' }
        //%
        static Oop __CALLING_CONVENTION allocateSize2( Oop receiver, Oop argument, Oop tenured );

        //%prim
        // <IndexedDoubleByteInstanceVariables>
        //   primitiveIndexedDoubleByteSize ^<SmallInteger> =
        //   Internal { flags = #(Pure IndexedDoubleByte)
        //              name  = 'doubleByteArrayPrimitives::size' }
        //%
        static Oop __CALLING_CONVENTION size( Oop receiver );

        //%prim
        // <IndexedDoubleByteInstanceVariables>
        //   primitiveIndexedDoubleByteAt: index     <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(Function IndexedDoubleByte)
        //              name  = 'doubleByteArrayPrimitives::at' }
        //%
        static Oop __CALLING_CONVENTION at( Oop receiver, Oop index );

        //%prim
        // <IndexedDoubleByteInstanceVariables>
        //   primitiveIndexedDoubleByteAt: index     <SmallInteger>
        //                            put: c         <SmallInteger>
        //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds ValueOutOfBounds)
        //              flags = #(Function IndexedDoubleByte)
        //              name  = 'doubleByteArrayPrimitives::atPut' }
        //%
        static Oop __CALLING_CONVENTION atPut( Oop receiver, Oop index, Oop value );

        //%prim
        // <IndexedDoubleByteInstanceVariables>
        //   primitiveIndexedDoubleByteCompare: str       <String>
        //                              ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #Function
        //              name  = 'doubleByteArrayPrimitives::compare' }
        //%
        static Oop __CALLING_CONVENTION compare( Oop receiver, Oop argumentw );

        //%prim
        // <IndexedDoubleByteInstanceVariables>
        //   primitiveIndexedDoubleByteInternIfFail: failBlock <PrimFailBlock> ^<CompressedSymbol> =
        //   Internal { error = #(ValueOutOfBounds)
        //              name  = 'doubleByteArrayPrimitives::intern' }
        //%
        static Oop __CALLING_CONVENTION intern( Oop receiver );

        //%prim
        // <IndexedDoubleByteInstanceVariables>
        //   primitiveIndexedDoubleByteCharacterAt: index <SmallInteger>
        //                                  ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(Function IndexedDoubleByte)
        //              name  = 'doubleByteArrayPrimitives::characterAt' }
        //%
        static Oop __CALLING_CONVENTION characterAt( Oop receiver, Oop index );

        //%prim
        // <IndexedDoubleByteInstanceVariables>
        //   primitiveIndexedDoubleByteHash ^<SmallInteger> =
        //   Internal { name  = 'doubleByteArrayPrimitives::hash' }
        //%
        static Oop __CALLING_CONVENTION hash( Oop receiver );
};


