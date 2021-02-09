//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"

// Primitives for debugging

class DebugPrimitives : AllStatic {
private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static std::int32_t number_of_calls;

    //%prim
    // <NoReceiver> primitiveBooleanFlagAt: name      <Symbol>
    //                              ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal {
    //      error = #(NotFound)
    //      name  = 'DebugPrimitives::boolAt' }
    //%
    static PRIM_DECL_1( boolAt, Oop name );

    //%prim
    // <NoReceiver> primitiveBooleanFlagAt: name      <Symbol>
    //                                 put: value     <Boolean>
    //                              ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal {
    //      error = #(NotFound)
    //      name  = 'DebugPrimitives::boolAtPut' }
    //%
    static PRIM_DECL_2( boolAtPut, Oop name, Oop value );

    //%prim
    // <NoReceiver> primitiveSmallIntegerFlagAt: name      <Symbol>
    //                                   ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal {
    //      error = #(NotFound)
    //      name  = 'DebugPrimitives::smiAt' }
    //%
    static PRIM_DECL_1( smiAt, Oop name );

    //%prim
    // <NoReceiver> primitiveSmallIntegerFlagAt: name      <Symbol>
    //                                      put: value     <Boolean>
    //                                   ifFail: failBlock <PrimFailBlock> ^<Boolean> =
    //   Internal {
    //      error = #(NotFound)
    //      name  = 'DebugPrimitives::smiAtPut' }
    //%
    static PRIM_DECL_2( smiAtPut, Oop name, Oop value );

    //%prim
    // <NoReceiver> primitiveClearLookupCache ^<Object> =
    //   Internal { name  = 'DebugPrimitives::clearLookupCache' }
    //%
    static PRIM_DECL_0( clearLookupCache );

    //%prim
    // <NoReceiver> primitiveClearLookupCacheStatistics ^<Object> =
    //   Internal { name  = 'DebugPrimitives::clearLookupCacheStatistics' }
    //%
    static PRIM_DECL_0( clearLookupCacheStatistics );

    //%prim
    // <NoReceiver> primitivePrintLookupCacheStatistics ^<Object> =
    //   Internal { name  = 'DebugPrimitives::printLookupCacheStatistics' }
    //%
    static PRIM_DECL_0( printLookupCacheStatistics );

    //%prim
    // <NoReceiver> primitivePrintLayout ^<Object> =
    //   Internal { name  = 'DebugPrimitives::printMemoryLayout' }
    //%
    static PRIM_DECL_0( printMemoryLayout );

    //%prim
    // <NoReceiver> primitiveDecodeAllMethods ^<Object> =
    //   Internal { name  = 'DebugPrimitives::decodeAllMethods' }
    //%
    static PRIM_DECL_0( decodeAllMethods );

    //%prim
    // <Object> primitivePrintMethodCodes: selector <Symbol>
    //                             ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(NotFound)
    //              name  = 'DebugPrimitives::printMethodCodes' }
    //%
    static PRIM_DECL_2( printMethodCodes, Oop receiver, Oop sel );

    //%prim
    // <Object> primitiveGenerateIR: selector <Symbol>
    //                       ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(NotFound)
    //              name  = 'DebugPrimitives::generateIR' }
    //%
    static PRIM_DECL_2( generateIR, Oop receiver, Oop sel );

    //%prim
    // <Object> primitiveOptimizeMethod: selector <Symbol>
    //                           ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(NotFound)
    //              name  = 'DebugPrimitives::optimizeMethod' }
    //%
    static PRIM_DECL_2( optimizeMethod, Oop receiver, Oop sel );

    //%prim
    // <Object> primitiveDecodeMethod: selector <Symbol>
    //                         ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { error = #(NotFound)
    //              name  = 'DebugPrimitives::decodeMethod' }
    //%
    static PRIM_DECL_2( decodeMethod, Oop receiver, Oop sel );

    //%prim
    // <NoReceiver> primitiveTimerStart ^<Object> =
    //   Internal { name  = 'DebugPrimitives::timerStart' }
    //%
    static PRIM_DECL_0( timerStart );

    //%prim
    // <NoReceiver> primitiveTimerStop ^<Object> =
    //   Internal { name  = 'DebugPrimitives::timerStop' }
    //%
    static PRIM_DECL_0( timerStop );

    //%prim
    // <NoReceiver> primitiveTimerPrintBuffer ^<Object> =
    //   Internal { name  = 'DebugPrimitives::timerPrintBuffer' }
    //%
    static PRIM_DECL_0( timerPrintBuffer );

    //%prim
    // <NoReceiver> primitiveInterpreterInvocationCounterLimit ^<SmallInteger> =
    //   Internal { name = 'DebugPrimitives::interpreterInvocationCounterLimit' }
    //%
    static PRIM_DECL_0( interpreterInvocationCounterLimit );

    //%prim
    // <NoReceiver> primitiveSetInterpreterInvocationCounterLimitTo: limit <SmallInteger>
    //                                                       ifFail: failBlock <PrimFailBlock> ^ <Object> =
    //   Internal { name = 'DebugPrimitives::setInterpreterInvocationCounterLimit' }
    //%
    static PRIM_DECL_1( setInterpreterInvocationCounterLimit, Oop limit );

    //%prim
    // <NoReceiver> primitiveClearInvocationCounters ^<Object> =
    //   Internal { name  = 'DebugPrimitives::clearInvocationCounters' }
    //%
    static PRIM_DECL_0( clearInvocationCounters );

    //%prim
    // <NoReceiver> primitivePrintInvocationCounterHistogram: size <SmallInteger>
    //                                                ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name  = 'DebugPrimitives::printInvocationCounterHistogram' }
    //%
    static PRIM_DECL_1( printInvocationCounterHistogram, Oop size );

    //%prim
    // <NoReceiver> primitivePrintObjectHistogram ^<Object> =
    //   Internal { name  = 'DebugPrimitives::printObjectHistogram' }
    //%
    static PRIM_DECL_0( printObjectHistogram );

    //%prim
    // <NoReceiver> primitiveClearInlineCaches ^<Object> =
    //   Internal {name  = 'DebugPrimitives::clearInlineCaches' }
    //%
    static PRIM_DECL_0( clearInlineCaches );

    //%prim
    // <NoReceiver> primitiveClearNativeMethodCounters ^<Object> =
    //   Internal { name  = 'DebugPrimitives::clearNativeMethodCounters' }
    //%
    static PRIM_DECL_0( clearNativeMethodCounters );

    //%prim
    // <NoReceiver> primitivePrintNativeMethodCounterHistogram: size <SmallInteger>
    //                                        ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name  = 'DebugPrimitives::printNativeMethodCounterHistogram' }
    //%
    static PRIM_DECL_1( printNativeMethodCounterHistogram, Oop size );

    //%prim
    // <NoReceiver> primitiveNumberOfMethodInvocations ^<SmallInteger> =
    //   Internal { name  = 'DebugPrimitives::numberOfMethodInvocations' }
    //%
    static PRIM_DECL_0( numberOfMethodInvocations );

    //%prim
    // <NoReceiver> primitiveNumberOfNativeMethodInvocations ^<SmallInteger> =
    //   Internal { name  = 'DebugPrimitives::numberOfNativeMethodInvocations' }
    //%
    static PRIM_DECL_0( numberOfNativeMethodInvocations );

    // Accessors to LookupCache statistics

    //%prim
    // <NoReceiver> primitiveNumberOfPrimaryLookupCacheHits ^<SmallInteger> =
    //   Internal { name  = 'DebugPrimitives::numberOfPrimaryLookupCacheHits' }
    //%
    static PRIM_DECL_0( numberOfPrimaryLookupCacheHits );

    //%prim
    // <NoReceiver> primitiveNumberOfSecondaryLookupCacheHits ^<SmallInteger> =
    //   Internal { name  = 'DebugPrimitives::numberOfSecondaryLookupCacheHits' }
    //%
    static PRIM_DECL_0( numberOfSecondaryLookupCacheHits );

    //%prim
    // <NoReceiver> primitiveNumberOfLookupCacheMisses ^<SmallInteger> =
    //   Internal { name  = 'DebugPrimitives::numberOfLookupCacheMisses' }
    //%
    static PRIM_DECL_0( numberOfLookupCacheMisses );

    //%prim
    // <NoReceiver> primitiveClearPrimitiveCounters ^<Object> =
    //   Internal { name  = 'DebugPrimitives::clearPrimitiveCounters' }
    //%
    static PRIM_DECL_0( clearPrimitiveCounters );

    //%prim
    // <NoReceiver> primitivePrintPrimitiveCounters ^<Object> =
    //   Internal { name  = 'DebugPrimitives::printPrimitiveCounters' }
    //%
    static PRIM_DECL_0( printPrimitiveCounters );

    //%prim
    // <NoReceiver> primitiveDeoptimizeStacks ^<Object> =
    //   Internal { doc   = 'Deoptimizes all stack to the canonical form'
    //              flags = #(NonLocalReturn)
    //              name  = 'DebugPrimitives::deoptimizeStacks' }
    //%
    static PRIM_DECL_0( deoptimizeStacks );

    //%prim
    // <NoReceiver> primitiveVerify ^<Object> =
    //   Internal { doc   = 'Verify the system'
    //              name  = 'DebugPrimitives::verify' }
    //%
    static PRIM_DECL_0( verify );
};
