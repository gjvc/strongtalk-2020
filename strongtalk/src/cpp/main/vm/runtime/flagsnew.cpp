//

#include <string>
#include <iostream>

#include "vm/system/platform.hpp"


// ----------------------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
class ConfigurationValue {
private:
    std::string _name;
    T           _value;
    std::string _description;
    T           _default_value;
public:
    explicit ConfigurationValue( const T &value ) : _name{ "" }, _value{ value } {

    }


    ConfigurationValue( const char *name, const T &value ) : _name{ name }, _value{ value } {

    }


    ConfigurationValue( const char *name, const T &value, const char *description ) : _name{ name }, _value{ value }, _description{ description } {

    }


    ConfigurationValue &operator=( const ConfigurationValue &other ) {
        if ( this == &other ) return *this;  // ignore self-assignment attempts

        _name        = other._name;
        _description = other._description;
        _value       = other._value;

        return *this;
    }


    const std::string &name() { return _name; }


    const std::string &description() { return _description; }


    const T value() { return _value; }


    const T default_value() { return _default_value; }


    operator T() const { return _value; }


};


template<typename T>
class ConfigurationOverride {
private:
    ConfigurationValue<T> &_cv;
    T                     _default_value;
    T                     _value;

public:
    ConfigurationOverride( ConfigurationValue<T> &configuration_value, const T &default_value ) : _cv{ configuration_value } {
        static_cast<void>(default_value); // unused
    }


    ~ConfigurationOverride() {
//        _cv._value = _default_value;
    }


    const T &default_value() { return _default_value; }


    const T &value() { return _value; }


    operator T() const { return _value; }


};


// ----------------------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
constexpr T _flag( const char *name, T default_value, const char *description ) {

    auto cv = ConfigurationValue<T>( name, default_value, description );

    if constexpr ( std::is_same<T, bool>::value ) {
        auto co = ConfigurationOverride<T>( cv, not default_value );
        spdlog::info( "bool [{}], default value [{}], overridden value [{}]", cv.name(), cv.value(), co.value() );
    }

    if constexpr ( std::is_same<T, std::int32_t>::value ) {
        auto co = ConfigurationOverride<T>( cv, default_value + 1 );
        spdlog::info( "std::int32_t [{}], default value [{}], overridden value [{}]", cv.name(), cv.value(), co.value() );
    }

    return T( default_value );
}


// ----------------------------------------------------------------------------------------------------------------------------------------------------------

auto _ActivationShowByteCodeIndex         = _flag<bool>( "ActivationShowByteCodeIndex", false, "Show current byteCodeIndex for activation" );
auto _ActivationShowCode                  = _flag<bool>( "ActivationShowCode", false, "Show pretty printed code for activation" );
auto _ActivationShowContext               = _flag<bool>( "ActivationShowContext", false, "Show context if any for activation" );
auto _ActivationShowExpressionStack       = _flag<bool>( "ActivationShowExpressionStack", false, "Show expression stack for activation" );
auto _ActivationShowFrame                 = _flag<bool>( "ActivationShowFrame", false, "Show frame for activation" );
auto _ActivationShowNameDescs             = _flag<bool>( "ActivationShowNameDescs", false, "Show name desc in the printed code" );
auto _AlwaysFlushVMMessages               = _flag<bool>( "AlwaysFlushVMMessages", true, "Flush VM message log after every line" );
auto _BlockArgAdditionalAllowedInlineCost = _flag<std::int32_t>( "BlockArgAdditionalAllowedInlineCost", 35, "additional allowed cost for each block arg" );
auto _BlockArgAdditionalInstrSize         = _flag<std::int32_t>( "BlockArgAdditionalInstrSize", 150, "extra allowance (in instr bytes) for each block arg" );
auto _BreakAtWarning                      = _flag<bool>( "BreakAtWarning", false, "Interrupt execution at warning?" );
auto _BruteForcePropagate                 = _flag<bool>( "BruteForcePropagate", false, "Perform brute-force global copy propagation (UNSAFE  -Urs 5/3/96)" );
auto _CodeForP6                           = _flag<bool>( "CodeForP6", false, "Minimize use of byte registers in code generation for P6" );
auto _CodeSize                            = _flag<std::int32_t>( "CodeSize", 20 * 1024, "size of code cache (in Kbytes)" );
auto _CodeSizeImpactsInlining             = _flag<bool>( "CodeSizeImpactsInlining", true, "code size is used as parameter to guide inlining" );
auto _CompiledCodeOnly                    = _flag<bool>( "CompiledCodeOnly", false, "Use compiled code only" );
auto _CompilerDebug                       = _flag<bool>( "CompilerDebug", false, "Make compiler debugging easier" );
auto _CompilerInstrsSize                  = _flag<std::int32_t>( "CompilerInstrsSize", 50 * 1024, "max. size of NativeMethod instrs" );
auto _CompilerPCsSize                     = _flag<std::int32_t>( "CompilerPCsSize", 15 * 1024, "max. size of relocation info info per NativeMethod" );
auto _CompilerScopesSize                  = _flag<std::int32_t>( "CompilerScopesSize", 50 * 1024, "max. size of debugging info per NativeMethod" );
auto _CompressProgramCounterDescriptors   = _flag<bool>( "CompressProgramCounterDescriptors", true, "ScopeDescriptorRecorder: Compress ProgramCounterDescriptors by ignoring multiple entries at same offset" );
auto _ConstantFoldPrims                   = _flag<bool>( "ConstantFoldPrims", true, "Constant-fold primitive calls" );
auto _CountBytecodes                      = _flag<bool>( "CountBytecodes", false, "Count number of bytecodes executed" );
auto _CountParentLinksAsOne               = _flag<bool>( "CountParentLinksAsOne", true, "Count going up to parent frame as 1 when checking MaxInterpretedSearchLength during recompilee search" );
auto _CounterHalfLifeTime                 = _flag<std::int32_t>( "CounterHalfLifeTime", 30, "time (in seconds) in which invocation counters decay by half" );
auto _CreateScopeDescInfo                 = _flag<bool>( "CreateScopeDescInfo", true, "Create ScopeDescriptor info for new backend code" );
auto _DebugPerformance                    = _flag<bool>( "DebugPerformance", false, "Print info useful for performance debugging" );
auto _DeferUncommonBranches               = _flag<bool>( "DeferUncommonBranches", true, "Don't generate code for uncommon cases" );
auto _EdenSize                            = _flag<std::int32_t>( "EdenSize", 512, "size of eden (in Kbytes)" );
auto _EliminateContexts                   = _flag<bool>( "EliminateContexts", true, "Eliminate context allocations" );
auto _EliminateJumpsToJumps               = _flag<bool>( "EliminateJumpsToJumps", true, "Eliminate jumps to jumps" );
auto _EliminateUnneededNodes              = _flag<bool>( "EliminateUnneededNodes", true, "Eliminate dead code" );
auto _EnableInt3                          = _flag<bool>( "EnableInt3", true, "Enables/disables code generation for int3 instructions" );
auto _EnableOptimizedCodeRecompilation    = _flag<bool>( "EnableOptimizedCodeRecompilation", true, "Enable recompilation of optimized code" );
auto _EnableProcessPreemption             = _flag<bool>( "EnableProcessPreemption", false, "Enables or disables preemption of running Smalltalk processes" );
auto _EnableTasks                         = _flag<bool>( "EnableTasks", true, "Enable periodic tasks to be performed" );
auto _EventLogLength                      = _flag<std::int32_t>( "EventLogLength", 1000, "Length of internal event log" );
auto _GenTraceCalls                       = _flag<bool>( "GenTraceCalls", false, "Generate code for TraceCalls" );
auto _GenerateFullDebugInfo               = _flag<bool>( "GenerateFullDebugInfo", false, "Generate debugging info for each byte code and not only for sends/traps" );
auto _GenerateHTML                        = _flag<bool>( "GenerateHTML", false, "Generate HTML output for documentation" );
auto _GenerateLiteScopeDescs              = _flag<bool>( "GenerateLiteScopeDescs", false, "generate lite scope descs" );
auto _GenerateSmalltalk                   = _flag<bool>( "GenerateSmalltalk", false, "Generate Smalltalk output for file_in" );
auto _GlobalCopyPropagate                 = _flag<bool>( "GlobalCopyPropagate", true, "Perform global copy propagation" );
auto _HeapSweeperInterval                 = _flag<std::int32_t>( "HeapSweeperInterval", 120, "Time interval (sec) between starting heap sweep" );
auto _Inline                              = _flag<bool>( "Inline", true, "Inline message sends" );
auto _InlinePrims                         = _flag<bool>( "InlinePrims", true, "Inline some primitive calls" );
auto _InliningDatabasePruningLimit        = _flag<std::int32_t>( "InliningDatabasePruningLimit", 3, "Min. number of nodes in inlining structure to qualify for database" );
auto _InvocationCounterLimit              = _flag<std::int32_t>( "InvocationCounterLimit", 10000, "max. number of method invocations before (re-)compiling" );
auto _JumpTableSize                       = _flag<std::int32_t>( "JumpTableSize", 8 * 1024, "size of jump table" );
auto _LRUDecayFactor                      = _flag<std::int32_t>( "LRUDecayFactor", 2, "LRUDecayFactor" );
auto _LocalCopyPropagate                  = _flag<bool>( "LocalCopyPropagate", true, "Perform local copy propagation" );
auto _LogVMMessages                       = _flag<bool>( "LogVMMessages", true, "Log all vm messages to a file" );
auto _LoopCounterLimit                    = _flag<std::int32_t>( "LoopCounterLimit", 10000, "max. number of loop iterations before (re-)compiling" );
auto _MakeBlockMethodZombies              = _flag<bool>( "MakeBlockMethodZombies", false, "Make block NativeMethod zombies if needed" );
auto _MaterializeEliminatedBlocks         = _flag<bool>( "MaterializeEliminatedBlocks", true, "Create fake blocks for eliminated blocks when printing stack" );
auto _MaxBlockInlineCost                  = _flag<std::int32_t>( "MaxBlockInlineCost", 70, "max. cost of block method" );
auto _MaxBlockInstrSize                   = _flag<std::int32_t>( "MaxBlockInstrSize", 450, "max. inline size (in instr bytes) of block method" );
auto _MaxCustomization                    = _flag<std::int32_t>( "MaxCustomization", 10, "max. number of customized method copies to create" );
auto _MaxElementPrintSize                 = _flag<std::int32_t>( "MaxElementPrintSize", 64, "Maximum number of elements to print" );
auto _MaxFnInlineCost                     = _flag<std::int32_t>( "MaxFnInlineCost", 40, "max. cost of normal inlined method" );
auto _MaxFnInstrSize                      = _flag<std::int32_t>( "MaxFnInstrSize", 300, "max. inline size (in instr bytes) of normal method" );
auto _MaxInterpretedSearchLength          = _flag<std::int32_t>( "MaxInterpretedSearchLength", 10, "max. number of intrepreted stack frames to traverse searching for recompilee" );
auto _MaxNmInstrSize                      = _flag<std::int32_t>( "MaxNmInstrSize", 12000, "max. desired size (in instr bytes) of an method" );
auto _MaxRecompilationSearchLength        = _flag<std::int32_t>( "MaxRecompilationSearchLength", 10, "max. number of real stack frames to traverse searching for recompilee" );
auto _MaxRecursionUnroll                  = _flag<std::int32_t>( "MaxRecursionUnroll", 2, "max. unrolling depth of recursive methods" );
auto _MaxTypeCaseSize                     = _flag<std::int32_t>( "MaxTypeCaseSize", 3, "max. number of types in typecase-based inlining" );
auto _MemoizeBlocks                       = _flag<bool>( "MemoizeBlocks", true, "memoize (delay creation of) blocks" );
auto _MinBlockCostFraction                = _flag<std::int32_t>( "MinBlockCostFraction", 50, "(in %) inline block if makes up more than this fraction of parent's cost" );
auto _MinInvocationsBeforeTrust           = _flag<std::int32_t>( "MinInvocationsBeforeTrust", 100, "min. number of invocations required before trusting NativeMethod's PICs" );
auto _MinSendsBeforeRecompile             = _flag<std::int32_t>( "MinSendsBeforeRecompile", 2000, "min number of sends a method must have performed before being recompiled" );
auto _NativeMethodAgeLimit                = _flag<std::int32_t>( "NativeMethodAgeLimit", 2, "min. number of sweeps before NativeMethod becomes old" );
auto _NumberOfBlockAllocations            = _flag<std::int32_t>( "NumberOfBlockAllocations", 0, "Number of allocated blocks" );
auto _NumberOfBytecodesExecuted           = _flag<std::int32_t>( "NumberOfBytecodesExecuted", 0, "Number of bytecodes executed by interpreter (if tracing)" );
auto _NumberOfContextAllocations          = _flag<std::int32_t>( "NumberOfContextAllocations", 0, "Number of allocated block contexts" );
auto _ObjectHeapExpandSize                = _flag<std::int32_t>( "ObjectHeapExpandSize", 512, "Chunk size (in Kbytes) by which the object heap grows" );
auto _OldSize                             = _flag<std::int32_t>( "OldSize", 3 * 1024, "initial size of oldspace (in Kbytes)" );
auto _OptimizeIntegerLoops                = _flag<bool>( "OptimizeIntegerLoops", true, "optimize integer loops" );
auto _OptimizeLoops                       = _flag<bool>( "OptimizeLoops", true, "optimize loops (hoist type tests" );
auto _PICSize                             = _flag<std::int32_t>( "PICSize", 128, "size of PolymorphicInlineCache cache (in Kbytes)" );
auto _PrintAssemblyCode                   = _flag<bool>( "PrintAssemblyCode", false, "Print assembly code" );
auto _PrintCode                           = _flag<bool>( "PrintCode", false, "Print intermediate code" );
auto _PrintCodeCompaction                 = _flag<bool>( "PrintCodeCompaction", false, "Print code compaction" );
auto _PrintCodeGeneration                 = _flag<bool>( "PrintCodeGeneration", false, "Print code generation with new backend" );
auto _PrintCodeReclamation                = _flag<bool>( "PrintCodeReclamation", false, "Print code reclamation" );
auto _PrintCodeSweep                      = _flag<bool>( "PrintCodeSweep", false, "Print sweeps through zone/methods" );
auto _PrintCompilation                    = _flag<bool>( "PrintCompilation", false, "Print each compilation" );
auto _PrintCompilerWarnings               = _flag<bool>( "PrintCompilerWarnings", true, "Print compiler warning?" );
auto _PrintCopyPropagation                = _flag<bool>( "PrintCopyPropagation", false, "Print info about copy propagation" );
auto _PrintDebugInfo                      = _flag<bool>( "PrintDebugInfo", false, "Print debugging info of ScopeDescs generated for NativeMethod" );
auto _PrintDebugInfoGeneration            = _flag<bool>( "PrintDebugInfoGeneration", false, "Print debugging info generation" );
auto _PrintEliminateContexts              = _flag<bool>( "PrintEliminateContexts", false, "Print info about eliminating context allocations" );
auto _PrintEliminateUnnededNodes          = _flag<bool>( "PrintEliminateUnnededNodes", false, "Print info about dead code elimination" );
auto _PrintEliminatedJumps                = _flag<bool>( "PrintEliminatedJumps", false, "Print eliminated jumps" );
auto _PrintExposed                        = _flag<bool>( "PrintExposed", false, "Print info about exposed-block analysis" );
auto _PrintGC                             = _flag<bool>( "PrintGC", true, "Print message at garbage collect" );
auto _PrintGlobalAllocation               = _flag<bool>( "PrintGlobalAllocation", false, "Print info about global register allocation" );
auto _PrintHeapAllocation                 = _flag<bool>( "PrintHeapAllocation", false, "Print heap allocation" );
auto _PrintHexAddresses                   = _flag<bool>( "PrintHexAddresses", true, "Print hex addresses in print outs (otherwise print 0)" );
auto _PrintInlineCacheInvalidation        = _flag<bool>( "PrintInlineCacheInvalidation", false, "Print inline cache invalidation" );
auto _PrintInlining                       = _flag<bool>( "PrintInlining", false, "Print info about inlining" );
auto _PrintInliningDatabaseCompilation    = _flag<bool>( "PrintInliningDatabaseCompilation", false, "Print inlining database compilations?" );
auto _PrintInterpreter                    = _flag<bool>( "PrintInterpreter", false, "Prints the generated interpreter's code" );
auto _PrintLRUSweep                       = _flag<bool>( "PrintLRUSweep", false, "Print least-recently used" );
auto _PrintLRUSweep2                      = _flag<bool>( "PrintLRUSweep2", false, "Print least-recently used" );
auto _PrintLocalAllocation                = _flag<bool>( "PrintLocalAllocation", false, "Print info about local register allocation" );
auto _PrintLongFrames                     = _flag<bool>( "PrintLongFrames", false, "Print tons of details in VM stack traces" );
auto _PrintLoopOpts                       = _flag<bool>( "PrintLoopOpts", false, "Print info about loop optimizations" );
auto _PrintMakeConformantCode             = _flag<bool>( "PrintMakeConformantCode", false, "Print code generated by makeConformant" );
auto _PrintMethodFlushing                 = _flag<bool>( "PrintMethodFlushing", false, "Print method flushing" );
auto _PrintObjectID                       = _flag<bool>( "PrintObjectID", true, "Always prepend object ID when printing" );
auto _PrintOopAddress                     = _flag<bool>( "PrintOopAddress", false, "Always print the location of the Oop" );
auto _PrintPseudoRegisterMapping          = _flag<bool>( "PrintPseudoRegisterMapping", false, "Print PseudoRegisterMapping during code generation" );
auto _PrintProgress                       = _flag<std::int32_t>( "PrintProgress", 0, "No. of compilations that cause a . to be printed out (0 means turned off)" );
auto _PrintRScopes                        = _flag<bool>( "PrintRScopes", false, "Print info about RScopes (type feedback sources)" );
auto _PrintRecompilation                  = _flag<bool>( "PrintRecompilation", false, "Print each recompilation" );
auto _PrintRecompilation2                 = _flag<bool>( "PrintRecompilation2", false, "Print details about each recompilation" );
auto _PrintRegAlloc                       = _flag<bool>( "PrintRegAlloc", false, "Print register allocation" );
auto _PrintRegTargeting                   = _flag<bool>( "PrintRegTargeting", false, "Print info about register targeting" );
auto _PrintResourceAllocation             = _flag<bool>( "PrintResourceAllocation", false, "Print each resource area allocation" );
auto _PrintResourceChunkAllocation        = _flag<bool>( "PrintResourceChunkAllocation", false, "Print each resource area chunk allocation" );
auto _PrintScavenge                       = _flag<bool>( "PrintScavenge", false, "Print message at scavenge" );
auto _PrintSplitting                      = _flag<bool>( "PrintSplitting", false, "Print info about boolean splitting" );
auto _PrintStackAfterUnpacking            = _flag<bool>( "PrintStackAfterUnpacking", false, "Print stack after unpacking deoptimized frames" );
auto _PrintStackAtScavenge                = _flag<bool>( "PrintStackAtScavenge", false, "Print stack at Scavenge" );
auto _PrintStubRoutines                   = _flag<bool>( "PrintStubRoutines", false, "Prints the stub routine's code" );
auto _PrintUncommonBranches               = _flag<bool>( "PrintUncommonBranches", false, "Print message upon encountering uncommon case" );
auto _PrintVMMessages                     = _flag<bool>( "PrintVMMessages", true, "Print vm messages on _console" );
auto _ProfilerNumberOfCompiledMethods     = _flag<std::int32_t>( "ProfilerNumberOfCompiledMethods", 10, "Max. number of compiled methods to print" );
auto _ProfilerNumberOfInterpreterMethods  = _flag<std::int32_t>( "ProfilerNumberOfInterpreterMethods", 10, "Max. number of interpreter methods to print" );
auto _ProfilerShowMethodHolder            = _flag<bool>( "ProfilerShowMethodHolder", true, "Show method holder for method" );
auto _ReorderBBs                          = _flag<bool>( "ReorderBBs", true, "Reorder basic blocks" );
auto _ReservedCodeSize                    = _flag<std::int32_t>( "ReservedCodeSize", 10 * 1024, "Maximum size of code cache (in Kbytes)" );
auto _ReservedHeapSize                    = _flag<std::int32_t>( "ReservedHeapSize", 50 * 1024, "Maximum size for object heap in Kbytes" );
auto _ReservedPICSize                     = _flag<std::int32_t>( "ReservedPICSize", 4 * 1024, "Maximum size of PolymorphicInlineCache cache (in Kbytes)" );
auto _ShowMessageBoxOnError               = _flag<bool>( "ShowMessageBoxOnError", false, "Show a message box on error" );
auto _Splitting                           = _flag<bool>( "Splitting", true, "Perform message splitting" );
auto _StackPrintLimit                     = _flag<std::int32_t>( "StackPrintLimit", 64, "Number of stack frames to print in VM-level stack dump" );
auto _StopInterpreterAt                   = _flag<std::int32_t>( "StopInterpreterAt", 0, "Stops interpreter execution at specified bytecode number" );
auto _SurvivorSize                        = _flag<std::int32_t>( "SurvivorSize", 64, "size of survivor spaces (in Kbytes)" );
auto _SweeperUseTimer                     = _flag<bool>( "SweeperUseTimer", true, "Tells whether the sweeper should use timer interrupts or compile events" );
auto _ThreadStackSize                     = _flag<std::int32_t>( "ThreadStackSize", 512, "Size (in 1024) of each thread's stack" );
auto _TraceAllocation                     = _flag<bool>( "TraceAllocation", false, "Trace allocation" );
auto _TraceApplyChange                    = _flag<bool>( "TraceApplyChange", false, "Trace reflective operation" );
auto _TraceBehaviorPrims                  = _flag<bool>( "TraceBehaviorPrims", false, "Trace behavior primitives" );
auto _TraceBlockPrims                     = _flag<bool>( "TraceBlockPrims", false, "Trace block primitives" );
auto _TraceBootstrap                      = _flag<bool>( "TraceBootstrap", false, "Trace the Bootstrap reading" );
auto _TraceByteArrayPrims                 = _flag<bool>( "TraceByteArrayPrims", false, "Trace ByteArray primitives" );
auto _TraceBytecodes                      = _flag<bool>( "TraceBytecodes", false, "Trace byte code execution" );
auto _TraceCallBackPrims                  = _flag<bool>( "TraceCallBackPrims", false, "Trace callBack primitives" );
auto _TraceCalls                          = _flag<bool>( "TraceCalls", false, "Print non-inlined calls/returns (must compile with VerifyCode or GenTraceCalls)" );
auto _TraceCanonicalContext               = _flag<bool>( "TraceCanonicalContext", false, "Trace canonical context construction" );
auto _TraceDLLCalls                       = _flag<bool>( "TraceDLLCalls", false, "Trace DLL function calls" );
auto _TraceDLLLookup                      = _flag<bool>( "TraceDLLLookup", false, "Trace DLL function lookup" );
auto _TraceDebugPrims                     = _flag<bool>( "TraceDebugPrims", false, "Trace debug primitives" );
auto _TraceDeoptimization                 = _flag<bool>( "TraceDeoptimization", false, "Trace deoptimizion" );
auto _TraceDoubleByteArrayPrims           = _flag<bool>( "TraceDoubleByteArrayPrims", false, "Trace DoubleByteArray primitives" );
auto _TraceDoublePrims                    = _flag<bool>( "TraceDoublePrims", false, "Trace Double primitives" );
auto _TraceDoubleValueArrayPrims          = _flag<bool>( "TraceDoubleValueArrayPrims", false, "Trace DoubleByteArray primitives" );
auto _TraceExpansion                      = _flag<bool>( "TraceExpansion", false, "Trace expansion of committed Space" );
auto _TraceGC                             = _flag<bool>( "TraceGC", true, "Trace Garbage Collection" );
auto _TraceInlineCacheMiss                = _flag<bool>( "TraceInlineCacheMiss", false, "Trace inline cache misses" );
auto _TraceInliningDatabase               = _flag<bool>( "TraceInliningDatabase", false, "Trace inlining database" );
auto _TraceInterpreterFramesAt            = _flag<std::int32_t>( "TraceInterpreterFramesAt", 0, "Trace interpreter frames at specified bytecode number" );
auto _TraceLookup                         = _flag<bool>( "TraceLookup", false, "Trace lookups" );
auto _TraceLookup2                        = _flag<bool>( "TraceLookup2", false, "Trace lookups in excruciating detail" );
auto _TraceLookupAtMiss                   = _flag<bool>( "TraceLookupAtMiss", false, "Trace lookups at lookup cache miss" );
auto _TraceMessageSend                    = _flag<bool>( "TraceMessageSend", false, "Trace all message sends" );
auto _TraceMethodPrims                    = _flag<bool>( "TraceMethodPrims", false, "Trace method prims" );
auto _TraceMixinPrims                     = _flag<bool>( "TraceMixinPrims", false, "Trace mixin prims" );
auto _TraceObjArrayPrims                  = _flag<bool>( "TraceObjArrayPrims", false, "Trace objArray primitives" );
auto _TraceOopPrims                       = _flag<bool>( "TraceOopPrims", false, "Trace Oop primitives" );
auto _TraceProcessEvents                  = _flag<bool>( "TraceProcessEvents", false, "Trace all process events" );
auto _TraceProcessPrims                   = _flag<bool>( "TraceProcessPrims", false, "Trace process primitives" );
auto _TraceProxyPrims                     = _flag<bool>( "TraceProxyPrims", false, "Trace Proxy primitives" );
auto _TraceResults                        = _flag<bool>( "TraceResults", false, "Trace NativeMethod results" );
auto _TraceSmiPrims                       = _flag<bool>( "TraceSmiPrims", false, "Trace SmallInteger primitives" );
auto _TraceSystemPrims                    = _flag<bool>( "TraceSystemPrims", false, "Trace system primitives" );
auto _TraceVMOperation                    = _flag<bool>( "TraceVMOperation", false, "Trace vm operations" );
auto _TraceVirtualFramePrims              = _flag<bool>( "TraceVirtualFramePrims", false, "Trace VirtualFrame primitives" );
auto _TraceZombieCreation                 = _flag<bool>( "TraceZombieCreation", false, "Trace NativeMethod zombie creation" );
auto _TryNewBackend                       = _flag<bool>( "TryNewBackend", false, "Use new backend & set additional flags as needed for compilation" );
auto _TypeFeedback                        = _flag<bool>( "TypeFeedback", true, "use type feedback data" );
auto _TypePredict                         = _flag<bool>( "TypePredict", true, "Predict smi_t/bool/array message sends" );
auto _TypePredictArrays                   = _flag<bool>( "TypePredictArrays", false, "Predict at:/at:Put: message sends" );
auto _UncommonAgeBackoffFactor            = _flag<std::int32_t>( "UncommonAgeBackoffFactor", 4, "for exponential back-off of UncommonAgeLimit based on NativeMethod version" );
auto _UncommonInvocationLimit             = _flag<std::int32_t>( "UncommonInvocationLimit", 10000, "min. number of invocations uncommon NativeMethod before recompiling it again" );
auto _UncommonRecompileLimit              = _flag<std::int32_t>( "UncommonRecompileLimit", 5, "min. number of uncommon traps before recompiling" );
auto _UseAccessMethods                    = _flag<bool>( "UseAccessMethods", true, "Use access methods" );
auto _UseFPUStack                         = _flag<bool>( "UseFPUStack", false, "Use FPU stack for floats (unsafe)" );
auto _UseGlobalFlatProfiling              = _flag<bool>( "UseGlobalFlatProfiling", true, "Include all processes when flat-profiling" );
auto _UseInlineCaching                    = _flag<bool>( "UseInlineCaching", true, "Use inline caching in compiled code" );
auto _UseInliningDatabase                 = _flag<bool>( "UseInliningDatabase", false, "Use the inlining database for recompilation" );
auto _UseInliningDatabaseEagerly          = _flag<bool>( "UseInliningDatabaseEagerly", false, "Use the inlining database eagerly at lookup" );
auto _UseLRUInterrupts                    = _flag<bool>( "UseLRUInterrupts", true, "User timers for zone LRU info" );
auto _UseMICs                             = _flag<bool>( "UseMICs", true, "Use MEGAMORPHIC PICs (MegamorphicInlineCache)" );
auto _UseNativeMethodAging                = _flag<bool>( "UseNativeMethodAging", true, "Age nativeMethods before recompiling them" );
auto _UseNewBackend                       = _flag<bool>( "UseNewBackend", false, "Use new backend" );
auto _UseNewMakeConformant                = _flag<bool>( "UseNewMakeConformant", true, "Use new makeConformant function" );
auto _UsePredictedMethods                 = _flag<bool>( "UsePredictedMethods", true, "Use predicted methods" );
auto _UsePrimitiveMethods                 = _flag<bool>( "UsePrimitiveMethods", false, "Use primitive methods" );
auto _UseRecompilation                    = _flag<bool>( "UseRecompilation", true, "Automatically (re-)compile frequently-used methods" );
auto _UseSlidingSystemAverage             = _flag<bool>( "UseSlidingSystemAverage", true, "Compute sliding system average on the fly" );
auto _UseTimers                           = _flag<bool>( "UseTimers", true, "Tells whether the VM should use timers (only used at startup)" );
auto _VerifyAfterGC                       = _flag<bool>( "VerifyAfterGC", false, "Verify system after garbage collect" );
auto _VerifyAfterScavenge                 = _flag<bool>( "VerifyAfterScavenge", false, "Verify system after scavenge" );
auto _VerifyBeforeGC                      = _flag<bool>( "VerifyBeforeGC", false, "Verify system before garbage collect" );
auto _VerifyBeforeScavenge                = _flag<bool>( "VerifyBeforeScavenge", false, "Verify system before scavenge" );
auto _VerifyCode                          = _flag<bool>( "VerifyCode", false, "Generates verification code in compiled code" );
auto _VerifyDebugInfo                     = _flag<bool>( "VerifyDebugInfo", false, "Verify compiled-code debug info at each call (very slow)" );
auto _VerifyZoneOften                     = _flag<bool>( "VerifyZoneOften", false, "Verify compiled-code zone often" );
auto _WizardMode                          = _flag<bool>( "WizardMode", false, "Wizard debugging mode" );
auto _ZapResourceArea                     = _flag<bool>( "ZapResourceArea", false, "Zap the resource area when deallocated" );
