//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for obj arrays

class objArrayPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <NoReceiver>
        //   primitiveIndexedObjectNew: class <IndexedInstanceVariables class>
        //                        size: size <SmallInteger>
        //                     tenured: tenured <Boolean>
        //                      ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NegativeSize)
        //              flags = #(Allocate)
        //              name  = 'objArrayPrimitives::allocateSize2' }
        //%
        static Oop __CALLING_CONVENTION allocateSize2( Oop receiver, Oop argument, Oop tenured );

        //%prim
        // <IndexedInstanceVariables class>
        //   primitiveIndexedObjectNew: size <SmallInteger>
        //                      ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NegativeSize)
        //              flags = #(Allocate)
        //              name  = 'objArrayPrimitives::allocateSize' }
        //%
        static Oop __CALLING_CONVENTION allocateSize( Oop receiver, Oop argument );

        //%prim
        // <IndexedInstanceVariables>
        //   primitiveIndexedObjectSize =
        //   Internal { flags = #(Pure IndexedObject)
        //              name  = 'objArrayPrimitives::size' }
        //%
        static Oop __CALLING_CONVENTION size( Oop receiver );

        //%prim
        // <IndexedInstanceVariables>
        //   primitiveIndexedObjectAt: index     <SmallInteger>
        //                     ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(Function IndexedObject)
        //              name  = 'objArrayPrimitives::at' }
        //%
        static Oop __CALLING_CONVENTION at( Oop receiver, Oop index );

        //%prim
        // <IndexedInstanceVariables>
        //   primitiveIndexedObjectAt: index     <SmallInteger>
        //                        put: c         <Object>
        //                     ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(Function IndexedObject)
        //              name  = 'objArrayPrimitives::atPut' }
        //%
        static Oop __CALLING_CONVENTION atPut( Oop receiver, Oop index, Oop value );

        //%prim
        // <IndexedInstanceVariables>
        //   primitiveIndexedObjectAtAllPut: obj <Object> ^<Self> =
        //   Internal { flags = #(Function IndexedObject)
        //              name  = 'objArrayPrimitives::at_all_put' }
        //%
        static Oop __CALLING_CONVENTION at_all_put( Oop receiver, Oop obj );

        //%prim
        // <IndexedInstanceVariables>
        //   primitiveIndexedObjectReplaceFrom: from      <SmallInteger>
        //                                  to: to        <SmallInteger>
        //                                with: source    <IndexedInstanceVariables>
        //                          startingAt: start     <SmallInteger>
        //                              ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { error = #(OutOfBounds)
        //              flags = #(IndexedObject)
        //              name  = 'objArrayPrimitives::replace_from_to' }
        //%
        static Oop __CALLING_CONVENTION replace_from_to( Oop receiver, Oop from, Oop to, Oop source, Oop start );

        //%prim
        // <IndexedInstanceVariables>
        //   primitiveIndexedObjectCopyFrom: from      <SmallInteger>
        //                       startingAt: start     <SmallInteger>
        //                             size: size      <SmallInteger>
        //                           ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { error = #(OutOfBounds NegativeSize)
        //              flags = #(IndexedObject)
        //              name  = 'objArrayPrimitives::copy_size' }
        //%
        static Oop __CALLING_CONVENTION copy_size( Oop receiver, Oop from, Oop start, Oop size );

};


