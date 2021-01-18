//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"


// Primitives for double value arrays

class doubleValueArrayPrimitives : AllStatic {
private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static int number_of_calls;

    //%prim
    // <IndexedFloatValueInstanceVariables class>
    //   primitiveIndexedFloatValueNew: size      <SmallInteger>
    //                          ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(NegativeSize)
    //              flags = #(Allocate)
    //              name  = 'doubleValueArrayPrimitives::allocateSize' }
    //%
    static PRIM_DECL_2( allocateSize, Oop receiver, Oop argument );

    //%prim
    // <IndexedFloatValueInstanceVariables>
    //   primitiveIndexedFloatValueSize ^<SmallInteger> =
    //   Internal { flags = #(Pure IndexedFloatValue)
    //              name  = 'doubleValueArrayPrimitives::size' }
    //%
    static PRIM_DECL_1( size, Oop receiver );

    //%prim
    // <IndexedFloatValueInstanceVariables>
    //   primitiveIndexedFloatValueAt: index     <SmallInteger>
    //                         ifFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { error = #(OutOfBounds)
    //              flags = #(Function IndexedFloatValue)
    //              name  = 'doubleValueArrayPrimitives::at' }
    //%
    static PRIM_DECL_2( at, Oop receiver, Oop index );

    //%prim
    // <IndexedFloatValueInstanceVariables>
    //   primitiveIndexedFloatValueAt: index     <SmallInteger>
    //                            put: value     <Float>
    //                         ifFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { error = #(OutOfBounds ValueOutOfBounds)
    //              flags = #(Function IndexedFloatValue)
    //              name  = 'doubleValueArrayPrimitives::atPut' }
    //%
    static PRIM_DECL_3( atPut, Oop receiver, Oop index, Oop value );

};
