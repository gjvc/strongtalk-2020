//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for blocks

// The following 9 primitives are implemented in the interpreter
// but we still need the interface!

//%prim
// <BlockWithoutArguments> primitiveValue ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue0' }
//%
extern "C" Oop primitiveValue0( Oop blk );

//%prim
// <BlockWithOneArgument> primitiveValue: arg1 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue1' }
//%
extern "C" Oop primitiveValue1( Oop blk, Oop arg1 );

//%prim
// <BlockWithTwoArguments> primitiveValue: arg1 <Object> value: arg2 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue2' }
//%
extern "C" Oop primitiveValue2( Oop blk, Oop arg1, Oop arg2 );

//%prim
// <BlockWithThreeArguments> primitiveValue: arg1 <Object> value: arg2 <Object> value: arg3 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue3' }
//%
extern "C" Oop primitiveValue3( Oop blk, Oop arg1, Oop arg2, Oop arg3 );

//%prim
// <BlockWithFourArguments> primitiveValue: arg1 <Object> value: arg2 <Object> value: arg3 <Object>
//                                   value: arg4 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue4' }
//%
extern "C" Oop primitiveValue4( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4 );

//%prim
// <BlockWithFiveArguments> primitiveValue: arg1 <Object> value: arg2 <Object> value: arg3 <Object>
//                                   value: arg4 <Object> value: arg5 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue5' }
//%
extern "C" Oop primitiveValue5( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5 );

//%prim
// <BlockWithSixArguments> primitiveValue: arg1 <Object> value: arg2 <Object> value: arg3 <Object>
//                                  value: arg4 <Object> value: arg5 <Object> value: arg6 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue6' }
//%
extern "C" Oop primitiveValue6( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop arg6 );

//%prim
// <BlockWithSevenArguments> primitiveValue: arg1 <Object> value: arg2 <Object> value: arg3 <Object>
//                                    value: arg4 <Object> value: arg5 <Object> value: arg6 <Object>
//                                    value: arg7 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue7' }
//%
extern "C" Oop primitiveValue7( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop arg6, Oop arg7 );

//%prim
// <BlockWithEightArguments> primitiveValue: arg1 <Object> value: arg2 <Object> value: arg3 <Object>
//                                    value: arg4 <Object> value: arg5 <Object> value: arg6 <Object>
//                                    value: arg7 <Object> value: arg8 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue8' }
//%
extern "C" Oop primitiveValue8( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop arg6, Oop arg7, Oop arg8 );

//%prim
// <BlockWithNineArguments> primitiveValue: arg1 <Object> value: arg2 <Object> value: arg3 <Object>
//                                   value: arg4 <Object> value: arg5 <Object> value: arg6 <Object>
//                                   value: arg7 <Object> value: arg8 <Object> value: arg9 <Object> ^<Object> =
//   Internal {
//     flags = #(NonLocalReturn Block LastDeltaFrameNotNeeded)
//     name  = 'primitiveValue9' }
//%
extern "C" Oop primitiveValue9( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop arg6, Oop arg7, Oop arg8, Oop arg9 );

// Instead we should come up with a
// generic solution for up to 255 arguments at some point. (gri)

// The following primitives are called directly by the interpreter.

extern "C" BlockClosureOop allocateBlock( SMIOop nofArgs );
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate: size <SmallInteger> ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate)
//     name  = 'allocateBlock' }
//%
extern "C" BlockClosureOop allocateTenuredBlock( SMIOop nofArgs );  // for compiler


extern "C" BlockClosureOop allocateBlock0();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate0 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock0' }
//%

extern "C" BlockClosureOop allocateBlock1();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate1 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock1' }
//%

extern "C" BlockClosureOop allocateBlock2();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate2 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock2' }
//%

extern "C" BlockClosureOop allocateBlock3();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate3 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock3' }
//%

extern "C" BlockClosureOop allocateBlock4();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate4 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock4' }
//%

extern "C" BlockClosureOop allocateBlock5();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate5 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock5' }
//%

extern "C" BlockClosureOop allocateBlock6();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate6 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock6' }
//%

extern "C" BlockClosureOop allocateBlock7();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate7 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock7' }
//%

extern "C" BlockClosureOop allocateBlock8();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate8 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock8' }
//%

extern "C" BlockClosureOop allocateBlock9();
//%prim
// <NoReceiver> primitiveCompiledBlockAllocate9 ^<Block> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateBlock9' }
//%

extern "C" ContextOop allocateContext( SMIOop nofVars );
//%prim
// <NoReceiver> primitiveCompiledContextAllocate: size <SmallInteger> ^<Object> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateContext' }
//%

extern "C" ContextOop allocateContext0();
//%prim
// <NoReceiver> primitiveCompiledContextAllocate0 ^<Object> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateContext0' }
//%
extern "C" ContextOop allocateContext1();
//%prim
// <NoReceiver> primitiveCompiledContextAllocate1 ^<Object> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateContext1' }
//%
extern "C" ContextOop allocateContext2();
//%prim
// <NoReceiver> primitiveCompiledContextAllocate2 ^<Object> =
//   Internal {
//     flags = #(Internal Block Allocate LastDeltaFrameNotNeeded)
//     name  = 'allocateContext2' }
//%

PRIM_DECL_2( unwindprotect, Oop receiver, Oop protectBlock );
//%prim
// <BlockWithoutArguments> primitiveUnwindProtect: protect   <BlockWithoutArguments>
//                                         ifFail: failBlock <PrimFailBlock> ^<Object> =
//   Internal {
//     doc   = 'Evaluates the receiver block and if it returns via a non-local-return'
//     doc   = 'the protect block is invoked.'
//     doc   = 'The original non-local-return continues after evaluation of the protect block.'
//     flags = #(NonLocalReturn)
//     name  = 'unwindprotect' }
//%

PRIM_DECL_1( blockRepeat, Oop receiver );
//%prim
// <BlockWithoutArguments> primitiveRepeat ^<BottomType> =
//   Internal {
//     doc   = 'Repeats evaluating the receiver block'
//     flags = #(NonLocalReturn)
//     name  = 'blockRepeat' }
//%

PRIM_DECL_1( block_method, Oop receiver );
//%prim
// <Block> primitiveBlockMethod ^<Method> =
//   Internal {
//     doc   = 'Returns the block method'
//     name  = 'block_method' }
//%

PRIM_DECL_1( block_is_optimized, Oop receiver );
//%prim
// <Block> primitiveBlockIsOptimized ^<Boolean> =
//   Internal {
//     doc   = 'Tells whether the block has optimized code'
//     name  = 'block_is_optimized' }
//%
