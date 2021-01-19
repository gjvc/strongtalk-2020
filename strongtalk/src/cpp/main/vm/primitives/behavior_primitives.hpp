//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/system/platform.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"

// primitives for classes

class behaviorPrimitives : AllStatic {

private:
    static void inc_calls() {
        _numberOfCalls++;
    }


public:
    static int _numberOfCalls;

    //%prim
    // <NoReceiver> primitiveNew: class <Behavior>
    //              tenured: tenured <Boolean>
    //              ifFail: failBlock <PrimFailBlock> ^<Instance> =
    //   Internal { error = #(ReceiverIsIndexable)
    //              name  = 'behaviorPrimitives::allocate3'
    //              flags = #Allocate }
    //%
    static PRIM_DECL_2( allocate3, Oop receiver, Oop tenured );

    //%prim
    // <Behavior> primitiveNew2IfFail: failBlock <PrimFailBlock> ^<Instance> =
    //   Internal { error = #(ReceiverIsIndexable)
    //              name  = 'behaviorPrimitives::allocate2'
    //              flags = #Allocate }
    //%
    static PRIM_DECL_1( allocate2, Oop receiver );

    //%prim
    // <Behavior> primitiveNewIfFail: failBlock <PrimFailBlock> ^<Instance> =
    //   Internal { error = #(ReceiverIsIndexable)
    //              name  = 'behaviorPrimitives::allocate'
    //              flags = #Allocate }
    //%
    static PRIM_DECL_1( allocate, Oop receiver );

    // MIXIN

    //%prim
    // <NoReceiver> primitiveBehaviorMixinOf: behavior <Behavior>
    //                                ifFail: failBlock <PrimFailBlock> ^<Mixin> =
    //   Internal { name = 'behaviorPrimitives::mixinOf' }
    //%
    static PRIM_DECL_1( mixinOf, Oop behavior );

    //%prim
    // <NoReceiver> primitiveBehaviorHeaderSizeOf: behavior <Behavior>
    //                                     ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { name = 'behaviorPrimitives::headerSize' }
    //%
    static PRIM_DECL_1( headerSize, Oop behavior );

    //%prim
    // <NoReceiver> primitiveBehaviorNonIndexableSizeOf: behavior <Behavior>
    //                                           ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { name = 'behaviorPrimitives::nonIndexableSize' }
    //%
    static PRIM_DECL_1( nonIndexableSize, Oop behavior );

    //%prim
    // <NoReceiver> primitiveBehaviorIsSpecializedClass: behavior  <Behavior>
    //                                           ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { name  = 'behaviorPrimitives::is_specialized_class' }
    //%
    static PRIM_DECL_1( is_specialized_class, Oop behavior );

    //%prim
    // <NoReceiver> primitiveBehaviorCanBeSubclassed: behavior  <Behavior>
    //                                        ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { name  = 'behaviorPrimitives::can_be_subclassed' }
    //%
    static PRIM_DECL_1( can_be_subclassed, Oop behavior );

    //%prim
    // <NoReceiver> primitiveBehaviorCanHaveInstanceVariables: behavior <Behavior>
    //                                                 ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal { name  = 'behaviorPrimitives::can_have_instance_variables' }
    //%
    static PRIM_DECL_1( can_have_instance_variables, Oop behavior );

    //%prim
    // <Behavior> primitiveSuperclass ^<Behavior|Nil> =
    //   Internal { name  = 'behaviorPrimitives::superclass' }
    //%
    static PRIM_DECL_1( superclass, Oop receiver );

    //%prim
    // <NoReceiver> primitiveSuperclassOf: class <Behavior>
    //                             ifFail: failBlock <PrimFailBlock> ^<Behavior|Nil> =
    //   Internal { name  = 'behaviorPrimitives::superclass_of' }
    //%
    static PRIM_DECL_1( superclass_of, Oop klass );

    //%prim
    // <NoReceiver> primitiveSetSuperclassOf: behavior  <Behavior>
    //                               toClass: newSuper  <Behavior>
    //                                ifFail: failBlock <PrimFailBlock> ^<Behavior> =
    //   Internal { error = #(NotAClass)
    //              name  = 'behaviorPrimitives::setSuperclass' }
    //%
    static PRIM_DECL_2( setSuperclass, Oop receiver, Oop newSuper );

    // CLASS VARIABLES

    //%prim
    // <NoReceiver> primitiveBehavior: behavior  <Behavior>
    //                classVariableAt: index     <SmallInteger>
    //                         ifFail: failBlock <PrimFailBlock> ^<GlobalAssociation> =
    //   Internal { error = #(OutOfBounds)
    //              name  = 'behaviorPrimitives::classVariableAt' }
    //%
    static PRIM_DECL_2( classVariableAt, Oop behavior, Oop index );


    //%prim
    // <NoReceiver> primitiveBehavior: behavior  <Behavior>
    //           classVariablesIfFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
    //   Internal { error = #(OutOfBounds)
    //              name  = 'behaviorPrimitives::classVariables' }
    //%
    static PRIM_DECL_1( classVariables, Oop behavior );

    // METHODS

    //%prim
    // <Behavior> primitivePrintMethod: selector  <ByteArray>
    //                          ifFail: failBlock <PrimFailBlock> ^<Behavior> =
    //   Internal { name  = 'behaviorPrimitives::printMethod' }
    //%
    static PRIM_DECL_2( printMethod, Oop receiver, Oop name );

    //%prim
    // <Behavior> primitiveMethodFor: selector  <CompressedSymbol>
    //                        ifFail: failBlock <PrimFailBlock> ^<Method> =
    //   Internal { name = 'behaviorPrimitives::methodFor'
    //              error = #(NotFound)
    //            }
    //%
    static PRIM_DECL_2( methodFor, Oop receiver, Oop selector );

    //%prim
    // <NoReceiver> primitiveBehaviorFormat: behavior  <Behavior>
    //                               ifFail: failBlock <PrimFailBlock> ^<Symbol> =
    //   Internal { name  = 'behaviorPrimitives::format' }
    //%
    static PRIM_DECL_1( format, Oop behavior );

    //%prim
    // <NoReceiver> primitiveBehaviorVMType: behavior  <Behavior>
    //                               ifFail: failBlock <PrimFailBlock> ^<Symbol> =
    //   Internal {  doc   = 'Oops, SmallInteger, GlobalAssociation, Method, Float, Array, WeakArray, '
    //               doc   = 'DoubleByteArray, FloatValueArray, ByteArray, Symbol, '
    //               doc   = 'Block, Context, Process, Proxy, Activation, Mixin, and Class'
    //               name  = 'behaviorPrimitives::vm_type' }
    //%
    static PRIM_DECL_1( vm_type, Oop behavior );

    //%prim
    // <Behavior> primitiveBehaviorIsClassOf: obj  <Object> ^<Boolean> =
    //   Internal {
    //              flags = #(LastDeltaFrameNotNeeded)
    //              name  = 'behaviorPrimitives::is_class_of' }
    //%
    static PRIM_DECL_2( is_class_of, Oop receiver, Oop obj );
};

//%prim
// <NoReceiver> primitiveInlineAllocations: behavior <Behavior>
//                                   count: count <SmallInt> ^<Instance> =
//   Internal { flags = #(Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveInlineAllocations' }
//%
extern "C" Oop primitiveInlineAllocations( Oop receiver, Oop count );

//%prim
// <Behavior> primitiveNew0: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew0' }
//%
extern "C" Oop primitiveNew0( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew1: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew1' }
//%
extern "C" Oop primitiveNew1( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew2: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew2' }
//%
extern "C" Oop primitiveNew2( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew3: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew3' }
//%
extern "C" Oop primitiveNew3( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew4: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew4' }
//%
extern "C" Oop primitiveNew4( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew5: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew5' }
//%
extern "C" Oop primitiveNew5( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew6: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew6' }
//%
extern "C" Oop primitiveNew6( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew7: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew7' }
//%
extern "C" Oop primitiveNew7( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew8: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew8' }
//%
extern "C" Oop primitiveNew8( Oop receiver, Oop tenured );

//%prim
// <Behavior> primitiveNew9: tenured <Boolean> ifFail: failBlock <PrimFailBlock> ^<Instance> =
//   Internal { flags = #(Internal Allocate LastDeltaFrameNotNeeded)
//              name  = 'primitiveNew9' }
//%
extern "C" Oop primitiveNew9( Oop receiver, Oop tenured );
