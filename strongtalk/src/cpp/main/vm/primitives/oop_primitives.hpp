//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives applying to all objects

class oopPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <Object> primitiveBecome: anObject  <Object>
        //                   ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(RecieverHasWrongType)
        //              name  = 'oopPrimitives::become' }
        //%
        static Oop __CALLING_CONVENTION become( Oop receiver, Oop argument );

        //%prim
        // <Object> primitiveInstVarAt: index     <SmallInteger>
        //                      ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(OutOfBounds)
        //              name  = 'oopPrimitives::instVarAt' }
        //%
        static Oop __CALLING_CONVENTION instVarAt( Oop receiver, Oop index );

        //%prim
        // <Reciever> primitiveInstVarNameFor: obj       <Object>
        //                                 at: index     <SmallInteger>
        //                             ifFail: failBlock <PrimFailBlock> ^<Symbol> =
        //   Internal { error = #(OutOfBounds)
        //              name  = 'oopPrimitives::instance_variable_name_at' }
        //%
        static Oop __CALLING_CONVENTION instance_variable_name_at( Oop obj, Oop index );

        //%prim
        // <NoReceiver> primitiveInstVarOf: obj       <Object>
        //                              at: index     <SmallInteger>
        //                             put: contents  <Object>
        //                          ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(OutOfBounds)
        //              name  = 'oopPrimitives::instVarAtPut' }
        //%
        static Oop __CALLING_CONVENTION instVarAtPut( Oop receiver, Oop index, Oop value );

        //%prim
        // <Object> primitiveHash ^<SmallInteger> =
        //   Internal { name = 'oopPrimitives::hash' }
        //%
        static Oop __CALLING_CONVENTION hash( Oop receiver );

        //%prim
        // <NoReceiver> primitiveHashOf: obj <Object> ^<SmallInteger> =
        //   Internal { name = 'oopPrimitives::hash_of' }
        //%
        static Oop __CALLING_CONVENTION hash_of( Oop obj );

        //%prim
        // <Object> primitiveShallowCopyIfFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(ReceiverHasWrongType)
        //              name  = 'oopPrimitives::shallowCopy' }
        //%
        static Oop __CALLING_CONVENTION shallowCopy( Oop receiver );

        //%prim
        // <Object> primitiveCopyTenuredIfFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NotOops)
        //              name  = 'oopPrimitives::copy_tenured' }
        //%
        static Oop __CALLING_CONVENTION copy_tenured( Oop receiver );

        //%prim
        // <Object> primitiveEqual: anObject <Object> ^<Boolean> =
        //   Internal { flags = #Pure
        //		  name  = 'oopPrimitives::equal' }
        //%
        static Oop __CALLING_CONVENTION equal( Oop receiver, Oop argument );

        //%prim
        // <Object> primitiveNotEqual: anObject <Object> ^<Boolean> =
        //   Internal { flags = #Pure
        //		  name  = 'oopPrimitives::not_equal' }
        //%
        static Oop __CALLING_CONVENTION not_equal( Oop receiver, Oop argument );

        //%prim
        // <Object> primitiveOopSize ^<SmallInteger> =
        //   Internal {
        //     flags = #Pure
        //     name = 'oopPrimitives::oop_size'}
        //%
        static Oop __CALLING_CONVENTION oop_size( Oop receiver );

        //%prim
        // <Object> primitiveClass ^<Self class> =
        //   Internal {
        //     flags = #(Pure LastDeltaFrameNotNeeded)
        //     name = 'oopPrimitives::klass'}
        //%
        static Oop __CALLING_CONVENTION klass( Oop receiver );

        //%prim
        // <NoReceiver> primitiveClassOf: obj <Object> ^<Behavior> =
        //   Internal {
        //     flags = #(Pure LastDeltaFrameNotNeeded)
        //     name = 'oopPrimitives::klass_of'}
        //%
        static Oop __CALLING_CONVENTION klass_of( Oop obj );

        //%prim
        // <Object> primitivePrint ^<Self> =
        //   Internal { name = 'oopPrimitives::print'}
        //%
        static Oop __CALLING_CONVENTION print( Oop receiver );

        //%prim
        // <Object> primitivePrintValue ^<Self> =
        //   Internal { name = 'oopPrimitives::printValue'}
        //%
        static Oop __CALLING_CONVENTION printValue( Oop receiver );

        //%prim
        // <Object> primitiveAsObjectID ^<SmallInteger> =
        //   Internal { name = 'oopPrimitives::asObjectID'}
        //%
        static Oop __CALLING_CONVENTION asObjectID( Oop receiver );

        //%prim
        // <Object> primitivePerform: selector  <CompressedSymbol>
        //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'oopPrimitives::perform'
        //              error = #(SelectorHasWrongNumberOfArguments)
        //            }
        //%
        static Oop __CALLING_CONVENTION perform( Oop receiver, Oop selector );

        //%prim
        // <Object> primitivePerform: selector  <CompressedSymbol>
        //                      with: arg1      <Object>
        //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'oopPrimitives::performWith'
        //              error = #(SelectorHasWrongNumberOfArguments)
        //              flags = #(NonLocalReturn)
        //            }
        //%
        static Oop __CALLING_CONVENTION performWith( Oop receiver, Oop selector, Oop arg1 );

        //%prim
        // <Object> primitivePerform: selector  <CompressedSymbol>
        //                      with: arg1      <Object>
        //                      with: arg2      <Object>
        //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'oopPrimitives::performWithWith'
        //              error = #(SelectorHasWrongNumberOfArguments)
        //              flags = #(NonLocalReturn)
        //            }
        //%
        static Oop __CALLING_CONVENTION performWithWith( Oop receiver, Oop selector, Oop arg1, Oop arg2 );

        //%prim
        // <Object> primitivePerform: selector  <CompressedSymbol>
        //                      with: arg1      <Object>
        //                      with: arg2      <Object>
        //                      with: arg3      <Object>
        //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'oopPrimitives::performWithWithWith'
        //              error = #(SelectorHasWrongNumberOfArguments)
        //              flags = #(NonLocalReturn)
        //            }
        //%
        static Oop __CALLING_CONVENTION performWithWithWith( Oop receiver, Oop selector, Oop arg1, Oop arg2, Oop arg3 );

        //%prim
        // <Object> primitivePerform: selector  <CompressedSymbol>
        //                 arguments: args      <Array>
        //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'oopPrimitives::performArguments'
        //              error = #(SelectorHasWrongNumberOfArguments)
        //              flags = #(NonLocalReturn)
        //            }
        //%
        static Oop __CALLING_CONVENTION performArguments( Oop receiver, Oop selector, Oop args );
};

