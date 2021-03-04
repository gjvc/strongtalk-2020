//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/primitive/primitive_declarations.hpp"
#include "vm/primitive/primitive_tracing.hpp"
// Primitives applying to all objects

class OopPrimitives : AllStatic {
private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static std::int32_t number_of_calls;

    //%prim
    // <Object> primitiveBecome: anObject  <Object>
    //                   ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(RecieverHasWrongType)
    //              name  = 'OopPrimitives::become' }
    //%
    static PRIM_DECL_2( become, Oop receiver, Oop argument );

    //%prim
    // <Object> primitiveInstVarAt: index     <SmallInteger>
    //                      ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(OutOfBounds)
    //              name  = 'OopPrimitives::instVarAt' }
    //%
    static PRIM_DECL_2( instVarAt, Oop receiver, Oop index );

    //%prim
    // <Reciever> primitiveInstVarNameFor: obj       <Object>
    //                                 at: index     <SmallInteger>
    //                             ifFail: failBlock <PrimFailBlock> ^<Symbol> =
    //   Internal { error = #(OutOfBounds)
    //              name  = 'OopPrimitives::instance_variable_name_at' }
    //%
    static PRIM_DECL_2( instance_variable_name_at, Oop obj, Oop index );

    //%prim
    // <NoReceiver> primitiveInstVarOf: obj       <Object>
    //                              at: index     <SmallInteger>
    //                             put: contents  <Object>
    //                          ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(OutOfBounds)
    //              name  = 'OopPrimitives::instVarAtPut' }
    //%
    static PRIM_DECL_3( instVarAtPut, Oop receiver, Oop index, Oop value );

    //%prim
    // <Object> primitiveHash ^<SmallInteger> =
    //   Internal { name = 'OopPrimitives::hash' }
    //%
    static PRIM_DECL_1( hash, Oop receiver );

    //%prim
    // <NoReceiver> primitiveHashOf: obj <Object> ^<SmallInteger> =
    //   Internal { name = 'OopPrimitives::hash_of' }
    //%
    static PRIM_DECL_1( hash_of, Oop obj );

    //%prim
    // <Object> primitiveShallowCopyIfFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(ReceiverHasWrongType)
    //              name  = 'OopPrimitives::shallowCopy' }
    //%
    static PRIM_DECL_1( shallowCopy, Oop receiver );

    //%prim
    // <Object> primitiveCopyTenuredIfFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(NotOops)
    //              name  = 'OopPrimitives::copy_tenured' }
    //%
    static PRIM_DECL_1( copy_tenured, Oop receiver );

    //%prim
    // <Object> primitiveEqual: anObject <Object> ^<Boolean> =
    //   Internal { flags = #Pure
    //		  name  = 'OopPrimitives::equal' }
    //%
    static PRIM_DECL_2( equal, Oop receiver, Oop argument );

    //%prim
    // <Object> primitiveNotEqual: anObject <Object> ^<Boolean> =
    //   Internal { flags = #Pure
    //		  name  = 'OopPrimitives::not_equal' }
    //%
    static PRIM_DECL_2( not_equal, Oop receiver, Oop argument );

    //%prim
    // <Object> primitiveOopSize ^<SmallInteger> =
    //   Internal {
    //     flags = #Pure
    //     name = 'OopPrimitives::oop_size'}
    //%
    static PRIM_DECL_1( oop_size, Oop receiver );

    //%prim
    // <Object> primitiveClass ^<Self class> =
    //   Internal {
    //     flags = #(Pure LastDeltaFrameNotNeeded)
    //     name = 'OopPrimitives::klass'}
    //%
    static PRIM_DECL_1( klass, Oop receiver );

    //%prim
    // <NoReceiver> primitiveClassOf: obj <Object> ^<Behavior> =
    //   Internal {
    //     flags = #(Pure LastDeltaFrameNotNeeded)
    //     name = 'OopPrimitives::klass_of'}
    //%
    static PRIM_DECL_1( klass_of, Oop obj );

    //%prim
    // <Object> primitivePrint ^<Self> =
    //   Internal { name = 'OopPrimitives::print'}
    //%
    static PRIM_DECL_1( print, Oop receiver );

    //%prim
    // <Object> primitivePrintValue ^<Self> =
    //   Internal { name = 'OopPrimitives::printValue'}
    //%
    static PRIM_DECL_1( printValue, Oop receiver );

    //%prim
    // <Object> primitiveAsObjectID ^<SmallInteger> =
    //   Internal { name = 'OopPrimitives::asObjectID'}
    //%
    static PRIM_DECL_1( asObjectID, Oop receiver );

    //%prim
    // <Object> primitivePerform: selector  <CompressedSymbol>
    //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'OopPrimitives::perform'
    //              error = #(SelectorHasWrongNumberOfArguments)
    //            }
    //%
    static PRIM_DECL_2( perform, Oop receiver, Oop selector );

    //%prim
    // <Object> primitivePerform: selector  <CompressedSymbol>
    //                      with: arg1      <Object>
    //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'OopPrimitives::performWith'
    //              error = #(SelectorHasWrongNumberOfArguments)
    //              flags = #(NonLocalReturn)
    //            }
    //%
    static PRIM_DECL_3( performWith, Oop receiver, Oop selector, Oop arg1 );

    //%prim
    // <Object> primitivePerform: selector  <CompressedSymbol>
    //                      with: arg1      <Object>
    //                      with: arg2      <Object>
    //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'OopPrimitives::performWithWith'
    //              error = #(SelectorHasWrongNumberOfArguments)
    //              flags = #(NonLocalReturn)
    //            }
    //%
    static PRIM_DECL_4( performWithWith, Oop receiver, Oop selector, Oop arg1, Oop arg2 );

    //%prim
    // <Object> primitivePerform: selector  <CompressedSymbol>
    //                      with: arg1      <Object>
    //                      with: arg2      <Object>
    //                      with: arg3      <Object>
    //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'OopPrimitives::performWithWithWith'
    //              error = #(SelectorHasWrongNumberOfArguments)
    //              flags = #(NonLocalReturn)
    //            }
    //%
    static PRIM_DECL_5( performWithWithWith, Oop receiver, Oop selector, Oop arg1, Oop arg2, Oop arg3 );

    //%prim
    // <Object> primitivePerform: selector  <CompressedSymbol>
    //                 arguments: args      <Array>
    //                    ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'OopPrimitives::performArguments'
    //              error = #(SelectorHasWrongNumberOfArguments)
    //              flags = #(NonLocalReturn)
    //            }
    //%
    static PRIM_DECL_3( performArguments, Oop receiver, Oop selector, Oop args );
};
