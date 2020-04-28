//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/utilities/OutputStream.hpp"

#include <cstring>

// Debug flags control various aspects of the VM and are accessible by Delta programs.
// use FlagSetting to temporarily change some debug flag.
// When the FlagSetting instance goes out of scope the original value is restored by the destructor.

class FlagSetting {

    private:
        bool_t _value;
        bool_t * _flag;

    public:
        FlagSetting( bool_t & fl, bool_t newValue ) {
            _flag  = &fl;
            _value = fl;
            fl     = newValue;
        }


        ~FlagSetting() {
            *_flag = _value;
        }
};


class debugFlags {

    public:
        static bool_t boolAt( const char * name, int len, bool_t * value );


        static bool_t boolAt( const char * name, bool_t * value ) {
            return boolAt( name, strlen( name ), value );
        }


        static bool_t boolAtPut( const char * name, int len, bool_t * value );


        static bool_t boolAtPut( const char * name, bool_t * value ) {
            return boolAtPut( name, strlen( name ), value );
        }


        static bool_t intAt( const char * name, int len, int * value );


        static bool_t intAt( const char * name, int * value ) {
            return intAt( name, strlen( name ), value );
        }


        static bool_t intAtPut( const char * name, int len, int * value );


        static bool_t intAtPut( const char * name, int * value ) {
            return intAtPut( name, strlen( name ), value );
        }


        static void printFlags();

        static void print_on( ConsoleOutputStream * stream );

        static void print_diff_on( ConsoleOutputStream * stream );
};



//
//           name                              default   description
//
#define APPLY_TO_BOOLEAN_FLAGS( develop ) \
    develop( ZapResourceArea,                     false, "Zap the resource area when deallocated"                                      ) \
    develop( PrintResourceAllocation,             false, "Print each resource area allocation"                                         ) \
    develop( PrintResourceChunkAllocation,        false, "Print each resource area chunk allocation"                                   ) \
    develop( PrintHeapAllocation,                 false, "Print heap alloction"                                                        ) \
    develop( PrintOopAddress,                     false, "Always print the location of the Oop"                                        ) \
    develop( PrintObjectID,                        true, "Always prepend object ID when printing"                                      ) \
    develop( PrintLongFrames,                     false, "Print tons of details in VM stack traces"                                    ) \
    develop( LogVMMessages,                        true, "Log all vm messages to a file"                                               ) \
    develop( AlwaysFlushVMMessages,                true, "Flush VM message log after every line"                                       ) \
    develop( VerifyBeforeScavenge,                false, "Verify system before scavenge"                                               ) \
    develop( VerifyAfterScavenge,                 false, "Verify system after scavenge"                                                ) \
    develop( PrintScavenge,                       false, "Print message at scavenge"                                                   ) \
    develop( PrintGC,                              true, "Print message at garbage collect"                                            ) \
    develop( WizardMode,                          false, "Wizard debugging mode"                                                       ) \
    develop( VerifyBeforeGC,                      false, "Verify system before garbage collect"                                        ) \
    develop( VerifyAfterGC,                       false, "Verify system after garbage collect"                                         ) \
    develop( VerifyZoneOften,                     false, "Verify compiled-code zone often"                                             ) \
    develop( PrintVMMessages,                      true, "Print vm messages on _console"                                               ) \
    develop( CompiledCodeOnly,                    false, "Use compiled code only"                                                      ) \
    develop( UseRecompilation,                     true, "Automatically (re-)compile frequently-used methods"                          ) \
    develop( UseNativeMethodAging,                 true, "Age nativeMethods before recompiling them"                                   ) \
    develop( UseInlineCaching,                     true, "Use inline caching in compiled code"                                         ) \
    develop( EnableTasks,                          true, "Enable periodic tasks to be performed"                                       ) \
    develop( CompressProgramCounterDescriptors,    true, "ScopeDescriptorRecorder: Compress ProgramCounterDescriptors by ignoring multiple entries at same offset" ) \
    develop( UseAccessMethods,                     true, "Use access methods"                                                          ) \
    develop( UsePredictedMethods,                  true, "Use predicted methods"                                                       ) \
    develop( UsePrimitiveMethods,                 false, "Use primitive methods"                                                       ) \
    develop( PrintStackAtScavenge,                false, "Print stack at Scavenge"                                                     ) \
    develop( PrintInterpreter,                    false, "Prints the generated interpreter's code"                                     ) \
    develop( PrintStubRoutines,                   false, "Prints the stub routine's code"                                              ) \
    develop( UseInliningDatabase,                 false, "Use the inlining database for recompilation"                                 ) \
    develop( UseInliningDatabaseEagerly,          false, "Use the inlining database eagerly at lookup"                                 ) \
    develop( UseSlidingSystemAverage,              true, "Compute sliding system average on the fly"                                   ) \
    develop( UseGlobalFlatProfiling,               true, "Include all processes when flat-profiling"                                   ) \
    develop( EnableOptimizedCodeRecompilation,     true, "Enable recompilation of optimized code"                                      ) \
    develop( CountParentLinksAsOne,                true, "Count going up to parent frame as 1 when checking MaxInterpretedSearchLength during recompilee search" ) \
    develop( GenerateSmalltalk,                   false, "Generate Smalltalk output for file_in"                                       ) \
    develop( GenerateHTML,                        false, "Generate HTML output for documentation"                                      ) \
\
    develop( UseTimers,                            true, "Tells whether the VM should use timers (only used at startup)"               ) \
    develop( SweeperUseTimer,                      true, "Tells whether the sweeper should use timer interrupts or compile events"     ) \
    develop( EnableProcessPreemption,             false, "Enables or disables preemption of running Smalltalk processes"               ) \
\
\
    /* tracing */       \
    develop( GenTraceCalls,                       false, "Generate code for TraceCalls"                                                ) \
    develop( TraceOopPrims,                       false, "Trace Oop primitives"                                                        ) \
    develop( TraceDoublePrims,                    false, "Trace Double primitives"                                                     ) \
    develop( TraceByteArrayPrims,                 false, "Trace ByteArray primitives"                                                  ) \
    develop( TraceDoubleByteArrayPrims,           false, "Trace DoubleByteArray primitives"                                            ) \
    develop( TraceDoubleValueArrayPrims,          false, "Trace DoubleByteArray primitives"                                            ) \
    develop( TraceObjArrayPrims,                  false, "Trace objArray primitives"                                                   ) \
    develop( TraceSmiPrims,                       false, "Trace SmallInteger primitives"                                               ) \
    develop( TraceProxyPrims,                     false, "Trace Proxy primitives"                                                      ) \
    develop( TraceBehaviorPrims,                  false, "Trace behavior primitives"                                                   ) \
    develop( TraceBlockPrims,                     false, "Trace block primitives"                                                      ) \
    develop( TraceCalls,                          false, "Print non-inlined calls/returns (must compile with VerifyCode or GenTraceCalls)" ) \
    develop( TraceDebugPrims,                     false, "Trace debug primitives"                                                      ) \
    develop( TraceSystemPrims,                    false, "Trace system primitives"                                                     ) \
    develop( TraceProcessPrims,                   false, "Trace process primitives"                                                    ) \
    develop( TraceVirtualFramePrims,              false, "Trace VirtualFrame primitives"                                               ) \
    develop( TraceCallBackPrims,                  false, "Trace callBack primitives"                                                   ) \
    develop( TraceLookup,                         false, "Trace lookups"                                                               ) \
    develop( TraceLookup2,                        false, "Trace lookups in excruciating detail"                                        ) \
    develop( TraceLookupAtMiss,                   false, "Trace lookups at lookup cache miss"                                          ) \
    develop( TraceBytecodes,                      false, "Trace byte code execution"                                                   ) \
    develop( TraceAllocation,                     false, "Trace allocation"                                                            ) \
    develop( TraceExpansion,                      false, "Trace expansion of committed Space"                                          ) \
    develop( TraceBootstrap,                      false, "Trace the Bootstrap reading"                                                 ) \
    develop( TraceMethodPrims,                    false, "Trace method prims"                                                          ) \
    develop( TraceMixinPrims,                     false, "Trace mixin prims"                                                           ) \
    develop( TraceVMOperation,                    false, "Trace vm operations"                                                         ) \
    develop( TraceDLLLookup,                      false, "Trace DLL function lookup"                                                   ) \
    develop( TraceDLLCalls,                       false, "Trace DLL function calls"                                                    ) \
    develop( TraceGC,                              true, "Trace Garbage Collection"                                                    ) \
    develop( TraceMessageSend,                    false, "Trace all message sends"                                                     ) \
    develop( TraceInlineCacheMiss,                false, "Trace inline cache misses"                                                   ) \
    develop( TraceProcessEvents,                  false, "Trace all process events"                                                    ) \
    develop( TraceDeoptimization,                 false, "Trace deoptimizion"                                                          ) \
    develop( TraceZombieCreation,                 false, "Trace NativeMethod zombie creation"                                          ) \
    develop( TraceResults,                        false, "Trace NativeMethod results"                                                  ) \
    develop( TraceApplyChange,                    false, "Trace reflective operation"                                                  ) \
    develop( TraceInliningDatabase,               false, "Trace inlining database"                                                     ) \
    develop( TraceCanonicalContext,               false, "Trace canonical context construction"                                        ) \
\
    develop( ActivationShowExpressionStack,       false, "Show expression stack for activation"                                        ) \
    develop( ActivationShowByteCodeIndex,         false, "Show current byteCodeIndex for activation"                                   ) \
    develop( ActivationShowFrame,                 false, "Show frame for activation"                                                   ) \
    develop( ActivationShowContext,               false, "Show context if any for activation"                                          ) \
    develop( ActivationShowCode,                  false, "Show pretty printed code for activation"                                     ) \
    develop( ActivationShowNameDescs,             false, "Show name desc in the printed code"                                          ) \
\
    develop( ShowMessageBoxOnError,               false, "Show a message box on error"                                                 ) \
    develop( BreakAtWarning,                      false, "Interrupt execution at warning?"                                             ) \
    develop( PrintCompilerWarnings,                true, "Print compiler warning?"                                                     ) \
    develop( PrintInliningDatabaseCompilation,    false, "Print inlining database compilations?"                                       ) \
\
    develop( CountBytecodes,                      false, "Count number of bytecodes executed"                                          ) \
\
    develop( ProfilerShowMethodHolder,             true, "Show method holder for method"                                               ) \
\
    develop( UseMICs,                              true, "Use megamorphic PICs (MegamorphicInlineCache)"                               ) \
    develop( UseLRUInterrupts,                     true, "User timers for zone LRU info"                                               ) \
    develop( UseNewBackend,                       false, "Use new backend"                                                             ) \
    develop( TryNewBackend,                       false, "Use new backend & set additional flags as needed for compilation"            ) \
    develop( UseFPUStack,                         false, "Use FPU stack for floats (unsafe)"                                           ) \
    develop( ReorderBBs,                           true, "Reorder basic blocks"                                                        ) \
    develop( CodeForP6,                           false, "Minimize use of byte registers in code generation for P6"                    ) \
    develop( PrintInlineCacheInvalidation,        false, "Print inline cache invalidation"                                             ) \
    develop( PrintCodeReclamation,                false, "Print code reclamation"                                                      ) \
    develop( PrintCodeSweep,                      false, "Print sweeps through zone/methods"                                           ) \
    develop( PrintCodeCompaction,                 false, "Print code compaction"                                                       ) \
    develop( PrintMethodFlushing,                 false, "Print method flushing"                                                       ) \
    develop( MakeBlockMethodZombies,              false, "Make block NativeMethod zombies if needed"                                   ) \
\
    develop( CompilerDebug,                       false, "Make compiler debugging easier"                                              ) \
    develop( EnableInt3,                           true, "Enables/disables code generation for int3 instructions"                      ) \
    develop( VerifyCode,                          false, "Generates verification code in compiled code"                                ) \
    develop( VerifyDebugInfo,                     false, "Verify compiled-code debug info at each call (very slow)"                    ) \
    develop( MaterializeEliminatedBlocks,          true, "Create fake blocks for eliminated blocks when printing stack"                ) \
    develop( Inline,                               true, "Inline message sends"                                                        ) \
    develop( InlinePrims,                          true, "Inline some primitive calls"                                                 ) \
    develop( ConstantFoldPrims,                    true, "Constant-fold primitive calls"                                               ) \
    develop( TypePredict,                          true, "Predict smi_t/bool_t/array message sends"                                    ) \
    develop( TypePredictArrays,                   false, "Predict at:/at:Put: message sends"                                           ) \
    develop( TypeFeedback,                         true, "use type feedback data"                                                      ) \
    develop( CodeSizeImpactsInlining,              true, "code size is used as parameter to guide inlining"                            ) \
    develop( OptimizeIntegerLoops,                 true, "optimize integer loops"                                                      ) \
    develop( OptimizeLoops,                        true, "optimize loops (hoist type tests"                                            ) \
    develop( EliminateJumpsToJumps,                true, "Eliminate jumps to jumps"                                                    ) \
    develop( EliminateContexts,                    true, "Eliminate context allocations"                                               ) \
    develop( LocalCopyPropagate,                   true, "Perform local copy propagation"                                              ) \
    develop( GlobalCopyPropagate,                  true, "Perform global copy propagation"                                             ) \
    develop( BruteForcePropagate,                 false, "Perform brute-force global copy propagation (UNSAFE  -Urs 5/3/96)"           ) \
    develop( Splitting,                            true, "Perform message splitting"                                                   ) \
    develop( EliminateUnneededNodes,               true, "Eliminate dead code"                                                         ) \
    develop( DeferUncommonBranches,                true, "Don't generate code for uncommon cases"                                      ) \
    develop( MemoizeBlocks,                        true, "memoize (delay creation of) blocks"                                          ) \
    develop( DebugPerformance,                    false, "Print info useful for performance debugging"                                 ) \
    develop( PrintInlining,                       false, "Print info about inlining"                                                   ) \
    develop( PrintSplitting,                      false, "Print info about boolean splitting"                                          ) \
    develop( PrintLocalAllocation,                false, "Print info about local register allocation"                                  ) \
    develop( PrintGlobalAllocation,               false, "Print info about global register allocation"                                 ) \
    develop( PrintEliminateContexts,              false, "Print info about eliminating context allocations"                            ) \
    develop( PrintCompilation,                    false, "Print each compilation"                                                      ) \
    develop( PrintRecompilation,                  false, "Print each recompilation"                                                    ) \
    develop( PrintRecompilation2,                 false, "Print details about each recompilation"                                      ) \
    develop( PrintCode,                           false, "Print intermediate code"                                                     ) \
    develop( PrintAssemblyCode,                   false, "Print assembly code"                                                         ) \
    develop( PrintEliminatedJumps,                false, "Print eliminated jumps"                                                      ) \
    develop( PrintRegAlloc,                       false, "Print register allocation"                                                   ) \
    develop( PrintCopyPropagation,                false, "Print info about copy propagation"                                           ) \
    develop( PrintUncommonBranches,               false, "Print message upon encountering uncommon case"                               ) \
    develop( PrintRegTargeting,                   false, "Print info about register targeting"                                         ) \
    develop( PrintExposed,                        false, "Print info about exposed-block analysis"                                     ) \
    develop( PrintEliminateUnnededNodes,          false, "Print info about dead code elimination"                                      ) \
    develop( PrintHexAddresses,                    true, "Print hex addresses in print outs (otherwise print 0)"                       ) \
    develop( GenerateLiteScopeDescs,              false, "generate lite scope descs"                                                   ) \
    develop( PrintRScopes,                        false, "Print info about RScopes (type feedback sources)"                            ) \
    develop( PrintLoopOpts,                       false, "Print info about loop optimizations"                                         ) \
    develop( PrintStackAfterUnpacking,            false, "Print stack after unpacking deoptimized frames"                              ) \
    develop( PrintDebugInfo,                      false, "Print debugging info of ScopeDescs generated for NativeMethod"               ) \
    develop( PrintDebugInfoGeneration,            false, "Print debugging info generation"                                             ) \
\
    develop( PrintCodeGeneration,                 false, "Print code generation with new backend"                                      ) \
    develop( PrintPRegMapping,                    false, "Print PseudoRegisterMapping during code generation"                          ) \
    develop( PrintMakeConformantCode,             false, "Print code generated by makeConformant"                                      ) \
    develop( CreateScopeDescInfo,                  true, "Create ScopeDescriptor info for new backend code"                            ) \
    develop( GenerateFullDebugInfo,               false, "Generate debugging info for each byte code and not only for sends/traps"     ) \
    develop( UseNewMakeConformant,                 true, "Use new makeConformant function"                                             ) \


#define APPLY_TO_INTEGER_FLAGS( develop ) \
\
    develop( EventLogLength,                       1000, "Length of internal event log"                                                ) \
    develop( StackPrintLimit,                        64, "Number of stack frames to print in VM-level stack dump"                      ) \
    develop( MaxElementPrintSize,                    64, "Maximum number of elements to print"                                         ) \
\
    develop( ReservedHeapSize,                  50*1024, "Maximum size for object heap in Kbytes"                                      ) \
    develop( ObjectHeapExpandSize,                  512, "Chunk size (in Kbytes) by which the object heap grows"                       ) \
    develop( EdenSize,                              512, "size of eden (in Kbytes)"                                                    ) \
    develop( SurvivorSize,                           64, "size of survivor spaces (in Kbytes)"                                         ) \
    develop( OldSize,                            3*1024, "initial size of oldspace (in Kbytes)"                                        ) \
    develop( ReservedCodeSize,                  10*1024, "Maximum size of code cache (in Kbytes)"                                      ) \
    develop( CodeSize,                          20*1024, "size of code cache (in Kbytes)"                                              ) \
    develop( ReservedPICSize,                    4*1024, "Maximum size of PolymorphicInlineCache cache (in Kbytes)"                    ) \
    develop( PICSize,                               128, "size of PolymorphicInlineCache cache (in Kbytes)"                            ) \
    develop( JumpTableSize,                      8*1024, "size of jump table"                                                          ) \
    develop( ThreadStackSize,                       512, "Size (in 1024) of each thread's stack"                                       ) \
\
    develop( CompilerInstrsSize,                50*1024, "max. size of NativeMethod instrs"                                            ) \
    develop( CompilerScopesSize,                50*1024, "max. size of debugging info per NativeMethod"                                ) \
    develop( CompilerPCsSize,                   15*1024, "max. size of relocation info info per NativeMethod"                          ) \
\
    develop( MaxFnInlineCost,                        40, "max. cost of normal inlined method"                                          ) \
    develop( MaxBlockInlineCost,                     70, "max. cost of block method"                                                   ) \
    develop( MinBlockCostFraction,                   50, "(in %) inline block if makes up more than this fraction of parent's cost"    ) \
    develop( BlockArgAdditionalAllowedInlineCost,    35, "additional allowed cost for each block arg"                                  ) \
\
    develop( InvocationCounterLimit,              10000, "max. number of method invocations before (re-)compiling"                     ) \
    develop( LoopCounterLimit,                    10000, "max. number of loop iterations before (re-)compiling"                        ) \
\
    develop( MaxNmInstrSize,                      12000, "max. desired size (in instr bytes) of an method"                             ) \
    develop( MinSendsBeforeRecompile,              2000, "min number of sends a method must have performed before being recompiled"            ) \
    develop( MaxFnInstrSize,                        300, "max. inline size (in instr bytes) of normal method"                          ) \
    develop( BlockArgAdditionalInstrSize,           150, "extra allowance (in instr bytes) for each block arg"                         ) \
    develop( MaxBlockInstrSize,                     450, "max. inline size (in instr bytes) of block method"                           ) \
    develop( MaxRecursionUnroll,                      2, "max. unrolling depth of recursive methods"                                   ) \
    develop( MaxTypeCaseSize,                         3, "max. number of types in typecase-based inlining"                             ) \
    develop( UncommonRecompileLimit,                  5, "min. number of uncommon traps before recompiling"                            ) \
    develop( UncommonInvocationLimit,             10000, "min. number of invocations uncommon NativeMethod before recompiling it again"      ) \
    develop( UncommonAgeBackoffFactor,                4, "for exponential back-off of UncommonAgeLimit based on NativeMethod version"  ) \
    develop( MinInvocationsBeforeTrust,             100, "min. number of invocations required before trusting NativeMethod's PICs"             ) \
    develop( NativeMethodAgeLimit,                    2, "min. number of sweeps before NativeMethod becomes old"                              ) \
    develop( MaxRecompilationSearchLength,           10, "max. number of real stack frames to traverse searching for recompilee"            ) \
    develop( MaxInterpretedSearchLength,             10, "max. number of intrepreted stack frames to traverse searching for recompilee"     ) \
    develop( CounterHalfLifeTime,                    30, "time (in seconds) in which invocation counters decay by half"                ) \
    develop( MaxCustomization,                       10, "max. number of customized method copies to create"                                   ) \
\
    develop( StopInterpreterAt,                       0, "Stops interpreter execution at specified bytecode number"                    ) \
    develop( TraceInterpreterFramesAt,                0, "Trace interpreter frames at specified bytecode number"                       ) \
\
    develop( NumberOfContextAllocations,              0, "Number of allocated block contexts"                                          ) \
    develop( NumberOfBlockAllocations,                0, "Number of allocated blocks"                                                  ) \
    develop( NumberOfBytecodesExecuted,               0, "Number of bytecodes executed by interpreter (if tracing)"                    ) \
\
    develop( ProfilerNumberOfInterpreterMethods,     10, "Max. number of interpreter methods to print"                                 ) \
    develop( ProfilerNumberOfCompiledMethods,        10, "Max. number of compiled methods to print"                                    ) \
\
    develop( HeapSweeperInterval,                   120, "Time interval (sec) between starting heap sweep"                             ) \
    develop( PrintProgress,                           0, "No. of compilations that cause a . to be printed out (0 means turned off)"   ) \
\
    develop( InliningDatabasePruningLimit,            3, "Min. number of nodes in inlining structure to qualify for database"          ) \
\

// declaration of boolean flags
#define DECLARE_BOOLEAN_FLAG( name, value, doc ) \
  extern "C" bool_t name;

APPLY_TO_BOOLEAN_FLAGS( DECLARE_BOOLEAN_FLAG )


// declaration of integer flags
#define DECLARE_INTEGER_FLAG( name, value, doc ) \
  extern "C" int name;

APPLY_TO_INTEGER_FLAGS( DECLARE_INTEGER_FLAG )


// debug() is intended as a "start debugging" hook to be called from the C++ debugger.  It sets up everything for debugging.
extern "C" void debug();

