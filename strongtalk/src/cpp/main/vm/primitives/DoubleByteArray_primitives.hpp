//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/utilities/ObjectIDTable.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for byte arrays

class DoubleByteArrayPrimitives : AllStatic {

private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static std::int32_t number_of_calls;

    //%prim
    // <IndexedDoubleByteInstanceVariables class>
    //   primitiveIndexedDoubleByteNew: size      <SmallInteger>
    //                          ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(NegativeSize)
    //              flags = #(Allocate)
    //              name  = 'DoubleByteArrayPrimitives::allocateSize' }
    //%
    static PRIM_DECL_2( allocateSize, Oop receiver, Oop argument );

    //%prim
    // <NoReceiver>
    //   primitiveIndexedDoubleByteNew: behaviour <IndexedDoubleByteInstanceVariables class>
    //                            size: size      <SmallInteger>
    //                         tenured: tenured   <Boolean>
    //                          ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(NegativeSize)
    //              flags = #(Allocate)
    //              name  = 'DoubleByteArrayPrimitives::allocateSize2' }
    //%
    static PRIM_DECL_3( allocateSize2, Oop receiver, Oop argument, Oop tenured );

    //%prim
    // <IndexedDoubleByteInstanceVariables>
    //   primitiveIndexedDoubleByteSize ^<SmallInteger> =
    //   Internal { flags = #(Pure IndexedDoubleByte)
    //              name  = 'DoubleByteArrayPrimitives::size' }
    //%
    static PRIM_DECL_1( size, Oop receiver );

    //%prim
    // <IndexedDoubleByteInstanceVariables>
    //   primitiveIndexedDoubleByteAt: index     <SmallInteger>
    //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { error = #(OutOfBounds)
    //              flags = #(Function IndexedDoubleByte)
    //              name  = 'DoubleByteArrayPrimitives::at' }
    //%
    static PRIM_DECL_2( at, Oop receiver, Oop index );

    //%prim
    // <IndexedDoubleByteInstanceVariables>
    //   primitiveIndexedDoubleByteAt: index     <SmallInteger>
    //                            put: c         <SmallInteger>
    //                         ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { error = #(OutOfBounds ValueOutOfBounds)
    //              flags = #(Function IndexedDoubleByte)
    //              name  = 'DoubleByteArrayPrimitives::atPut' }
    //%
    static PRIM_DECL_3( atPut, Oop receiver, Oop index, Oop value );

    //%prim
    // <IndexedDoubleByteInstanceVariables>
    //   primitiveIndexedDoubleByteCompare: str       <String>
    //                              ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { flags = #Function
    //              name  = 'DoubleByteArrayPrimitives::compare' }
    //%
    static PRIM_DECL_2( compare, Oop receiver, Oop argumentw );

    //%prim
    // <IndexedDoubleByteInstanceVariables>
    //   primitiveIndexedDoubleByteInternIfFail: failBlock <PrimFailBlock> ^<CompressedSymbol> =
    //   Internal { error = #(ValueOutOfBounds)
    //              name  = 'DoubleByteArrayPrimitives::intern' }
    //%
    static PRIM_DECL_1( intern, Oop receiver );

    //%prim
    // <IndexedDoubleByteInstanceVariables>
    //   primitiveIndexedDoubleByteCharacterAt: index <SmallInteger>
    //                                  ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { error = #(OutOfBounds)
    //              flags = #(Function IndexedDoubleByte)
    //              name  = 'DoubleByteArrayPrimitives::characterAt' }
    //%
    static PRIM_DECL_2( characterAt, Oop receiver, Oop index );

    //%prim
    // <IndexedDoubleByteInstanceVariables>
    //   primitiveIndexedDoubleByteHash ^<SmallInteger> =
    //   Internal { name  = 'DoubleByteArrayPrimitives::hash' }
    //%
    static PRIM_DECL_1( hash, Oop receiver );
};
