//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for debugging

class debugPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <NoReceiver> primitiveBooleanFlagAt: name      <Symbol>
        //                              ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal {
        //      error = #(NotFound)
        //      name  = 'debugPrimitives::boolAt' }
        //%
        static Oop __CALLING_CONVENTION boolAt( Oop name );

        //%prim
        // <NoReceiver> primitiveBooleanFlagAt: name      <Symbol>
        //                                 put: value     <Boolean>
        //                              ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal {
        //      error = #(NotFound)
        //      name  = 'debugPrimitives::boolAtPut' }
        //%
        static Oop __CALLING_CONVENTION boolAtPut( Oop name, Oop value );

        //%prim
        // <NoReceiver> primitiveSmallIntegerFlagAt: name      <Symbol>
        //                                   ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal {
        //      error = #(NotFound)
        //      name  = 'debugPrimitives::smiAt' }
        //%
        static Oop __CALLING_CONVENTION smiAt( Oop name );

        //%prim
        // <NoReceiver> primitiveSmallIntegerFlagAt: name      <Symbol>
        //                                      put: value     <Boolean>
        //                                   ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal {
        //      error = #(NotFound)
        //      name  = 'debugPrimitives::smiAtPut' }
        //%
        static Oop __CALLING_CONVENTION smiAtPut( Oop name, Oop value );

        //%prim
        // <NoReceiver> primitiveClearLookupCache ^<Object> =
        //   Internal { name  = 'debugPrimitives::clearLookupCache' }
        //%
        static Oop __CALLING_CONVENTION clearLookupCache();

        //%prim
        // <NoReceiver> primitiveClearLookupCacheStatistics ^<Object> =
        //   Internal { name  = 'debugPrimitives::clearLookupCacheStatistics' }
        //%
        static Oop __CALLING_CONVENTION clearLookupCacheStatistics();

        //%prim
        // <NoReceiver> primitivePrintLookupCacheStatistics ^<Object> =
        //   Internal { name  = 'debugPrimitives::printLookupCacheStatistics' }
        //%
        static Oop __CALLING_CONVENTION printLookupCacheStatistics();

        //%prim
        // <NoReceiver> primitivePrintLayout ^<Object> =
        //   Internal { name  = 'debugPrimitives::printMemoryLayout' }
        //%
        static Oop __CALLING_CONVENTION printMemoryLayout();

        //%prim
        // <NoReceiver> primitiveDecodeAllMethods ^<Object> =
        //   Internal { name  = 'debugPrimitives::decodeAllMethods' }
        //%
        static Oop __CALLING_CONVENTION decodeAllMethods();

        //%prim
        // <Object> primitivePrintMethodCodes: selector <Symbol>
        //                             ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NotFound)
        //              name  = 'debugPrimitives::printMethodCodes' }
        //%
        static Oop __CALLING_CONVENTION printMethodCodes( Oop receiver, Oop sel );

        //%prim
        // <Object> primitiveGenerateIR: selector <Symbol>
        //                       ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NotFound)
        //              name  = 'debugPrimitives::generateIR' }
        //%
        static Oop __CALLING_CONVENTION generateIR( Oop receiver, Oop sel );

        //%prim
        // <Object> primitiveOptimizeMethod: selector <Symbol>
        //                           ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NotFound)
        //              name  = 'debugPrimitives::optimizeMethod' }
        //%
        static Oop __CALLING_CONVENTION optimizeMethod( Oop receiver, Oop sel );

        //%prim
        // <Object> primitiveDecodeMethod: selector <Symbol>
        //                         ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { error = #(NotFound)
        //              name  = 'debugPrimitives::decodeMethod' }
        //%
        static Oop __CALLING_CONVENTION decodeMethod( Oop receiver, Oop sel );

        //%prim
        // <NoReceiver> primitiveTimerStart ^<Object> =
        //   Internal { name  = 'debugPrimitives::timerStart' }
        //%
        static Oop __CALLING_CONVENTION timerStart();

        //%prim
        // <NoReceiver> primitiveTimerStop ^<Object> =
        //   Internal { name  = 'debugPrimitives::timerStop' }
        //%
        static Oop __CALLING_CONVENTION timerStop();

        //%prim
        // <NoReceiver> primitiveTimerPrintBuffer ^<Object> =
        //   Internal { name  = 'debugPrimitives::timerPrintBuffer' }
        //%
        static Oop __CALLING_CONVENTION timerPrintBuffer();

        //%prim
        // <NoReceiver> primitiveInterpreterInvocationCounterLimit ^<SmallInteger> =
        //   Internal { name = 'debugPrimitives::interpreterInvocationCounterLimit' }
        //%
        static Oop __CALLING_CONVENTION interpreterInvocationCounterLimit();

        //%prim
        // <NoReceiver> primitiveSetInterpreterInvocationCounterLimitTo: limit <SmallInteger>
        //                                                       ifFail: failBlock <PrimFailBlock> ^ <Object> =
        //   Internal { name = 'debugPrimitives::setInterpreterInvocationCounterLimit' }
        //%
        static Oop __CALLING_CONVENTION setInterpreterInvocationCounterLimit( Oop limit );

        //%prim
        // <NoReceiver> primitiveClearInvocationCounters ^<Object> =
        //   Internal { name  = 'debugPrimitives::clearInvocationCounters' }
        //%
        static Oop __CALLING_CONVENTION clearInvocationCounters();

        //%prim
        // <NoReceiver> primitivePrintInvocationCounterHistogram: size <SmallInteger>
        //                                                ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name  = 'debugPrimitives::printInvocationCounterHistogram' }
        //%
        static Oop __CALLING_CONVENTION printInvocationCounterHistogram( Oop size );

        //%prim
        // <NoReceiver> primitivePrintObjectHistogram ^<Object> =
        //   Internal { name  = 'debugPrimitives::printObjectHistogram' }
        //%
        static Oop __CALLING_CONVENTION printObjectHistogram();

        //%prim
        // <NoReceiver> primitiveClearInlineCaches ^<Object> =
        //   Internal {name  = 'debugPrimitives::clearInlineCaches' }
        //%
        static Oop __CALLING_CONVENTION clearInlineCaches();

        //%prim
        // <NoReceiver> primitiveClearNativeMethodCounters ^<Object> =
        //   Internal { name  = 'debugPrimitives::clearNativeMethodCounters' }
        //%
        static Oop __CALLING_CONVENTION clearNativeMethodCounters();

        //%prim
        // <NoReceiver> primitivePrintNativeMethodCounterHistogram: size <SmallInteger>
        //                                        ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name  = 'debugPrimitives::printNativeMethodCounterHistogram' }
        //%
        static Oop __CALLING_CONVENTION printNativeMethodCounterHistogram( Oop size );

        //%prim
        // <NoReceiver> primitiveNumberOfMethodInvocations ^<SmallInteger> =
        //   Internal { name  = 'debugPrimitives::numberOfMethodInvocations' }
        //%
        static Oop __CALLING_CONVENTION numberOfMethodInvocations();

        //%prim
        // <NoReceiver> primitiveNumberOfNativeMethodInvocations ^<SmallInteger> =
        //   Internal { name  = 'debugPrimitives::numberOfNativeMethodInvocations' }
        //%
        static Oop __CALLING_CONVENTION numberOfNativeMethodInvocations();

        // Accessors to LookupCache statistics

        //%prim
        // <NoReceiver> primitiveNumberOfPrimaryLookupCacheHits ^<SmallInteger> =
        //   Internal { name  = 'debugPrimitives::numberOfPrimaryLookupCacheHits' }
        //%
        static Oop __CALLING_CONVENTION numberOfPrimaryLookupCacheHits();

        //%prim
        // <NoReceiver> primitiveNumberOfSecondaryLookupCacheHits ^<SmallInteger> =
        //   Internal { name  = 'debugPrimitives::numberOfSecondaryLookupCacheHits' }
        //%
        static Oop __CALLING_CONVENTION numberOfSecondaryLookupCacheHits();

        //%prim
        // <NoReceiver> primitiveNumberOfLookupCacheMisses ^<SmallInteger> =
        //   Internal { name  = 'debugPrimitives::numberOfLookupCacheMisses' }
        //%
        static Oop __CALLING_CONVENTION numberOfLookupCacheMisses();

        //%prim
        // <NoReceiver> primitiveClearPrimitiveCounters ^<Object> =
        //   Internal { name  = 'debugPrimitives::clearPrimitiveCounters' }
        //%
        static Oop __CALLING_CONVENTION clearPrimitiveCounters();

        //%prim
        // <NoReceiver> primitivePrintPrimitiveCounters ^<Object> =
        //   Internal { name  = 'debugPrimitives::printPrimitiveCounters' }
        //%
        static Oop __CALLING_CONVENTION printPrimitiveCounters();

        //%prim
        // <NoReceiver> primitiveDeoptimizeStacks ^<Object> =
        //   Internal { doc   = 'Deoptimizes all stack to the canonical form'
        //              flags = #(NonLocalReturn)
        //              name  = 'debugPrimitives::deoptimizeStacks' }
        //%
        static Oop __CALLING_CONVENTION deoptimizeStacks();

        //%prim
        // <NoReceiver> primitiveVerify ^<Object> =
        //   Internal { doc   = 'Verify the system'
        //              name  = 'debugPrimitives::verify' }
        //%
        static Oop __CALLING_CONVENTION verify();
};

