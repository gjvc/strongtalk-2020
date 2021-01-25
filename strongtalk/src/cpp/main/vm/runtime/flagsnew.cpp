//

#include <string>
#include <iostream>

#include "vm/system/platform.hpp"


// ----------------------------------------------------------------------------------------------------------------------------------------------------------

template<typename T>
class ConfigurationValue {
private:
    std::string _name;
    std::string _description;
    T           _value;
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
        std::cout << "bool [" << cv.name() << "] default value is [" << cv.value() << "], overridden value is [" << co.value() << "]" << std::endl;
    }

    if constexpr ( std::is_same<T, std::int32_t>::value ) {
        auto co = ConfigurationOverride<T>( cv, default_value + 1 );
        std::cout << " std::int32_t [" << cv.name() << "] default value is [" << cv.value() << "], overridden value is [" << co.value() << "]" << std::endl;
    }

    return T( default_value );
}


// ----------------------------------------------------------------------------------------------------------------------------------------------------------

auto _ActivationShowByteCodeIndex         = _flag<bool_t>( "ActivationShowByteCodeIndex", false, "Show current byteCodeIndex for activation" );
auto _ActivationShowCode                  = _flag<bool_t>( "ActivationShowCode", false, "Show pretty printed code for activation" );
auto _ActivationShowContext               = _flag<bool_t>( "ActivationShowContext", false, "Show context if any for activation" );
auto _ActivationShowExpressionStack       = _flag<bool_t>( "ActivationShowExpressionStack", false, "Show expression stack for activation" );
auto _ActivationShowFrame                 = _flag<bool_t>( "ActivationShowFrame", false, "Show frame for activation" );
auto _ActivationShowNameDescs             = _flag<bool_t>( "ActivationShowNameDescs", false, "Show name desc in the printed code" );
auto _AlwaysFlushVMMessages               = _flag<bool_t>( "AlwaysFlushVMMessages", true, "Flush VM message log after every line" );
auto _BlockArgAdditionalAllowedInlineCost = _flag<std::int32_t>( "BlockArgAdditionalAllowedInlineCost", 35, "additional allowed cost for each block arg" );
auto _BlockArgAdditionalInstrSize         = _flag<std::int32_t>( "BlockArgAdditionalInstrSize", 150, "extra allowance (in instr bytes) for each block arg" );
auto _BreakAtWarning                      = _flag<bool_t>( "BreakAtWarning", false, "Interrupt execution at warning?" );
auto _BruteForcePropagate                 = _flag<bool_t>( "BruteForcePropagate", false, "Perform brute-force global copy propagation (UNSAFE  -Urs 5/3/96)" );
auto _CodeForP6                           = _flag<bool_t>( "CodeForP6", false, "Minimize use of byte registers in code generation for P6" );
auto _CodeSize                            = _flag<std::int32_t>( "CodeSize", 20 * 1024, "size of code cache (in Kbytes)" );
auto _CodeSizeImpactsInlining             = _flag<bool_t>( "CodeSizeImpactsInlining", true, "code size is used as parameter to guide inlining" );
auto _CompiledCodeOnly                    = _flag<bool_t>( "CompiledCodeOnly", false, "Use compiled code only" );
auto _CompilerDebug                       = _flag<bool_t>( "CompilerDebug", false, "Make compiler debugging easier" );
auto _CompilerInstrsSize                  = _flag<std::int32_t>( "CompilerInstrsSize", 50 * 1024, "max. size of NativeMethod instrs" );
auto _CompilerPCsSize                     = _flag<std::int32_t>( "CompilerPCsSize", 15 * 1024, "max. size of relocation info info per NativeMethod" );
auto _CompilerScopesSize                  = _flag<std::int32_t>( "CompilerScopesSize", 50 * 1024, "max. size of debugging info per NativeMethod" );
auto _CompressProgramCounterDescriptors   = _flag<bool_t>( "CompressProgramCounterDescriptors", true, "ScopeDescriptorRecorder: Compress ProgramCounterDescriptors by ignoring multiple entries at same offset" );
auto _ConstantFoldPrims                   = _flag<bool_t>( "ConstantFoldPrims", true, "Constant-fold primitive calls" );
auto _CountBytecodes                      = _flag<bool_t>( "CountBytecodes", false, "Count number of bytecodes executed" );
auto _CountParentLinksAsOne               = _flag<bool_t>( "CountParentLinksAsOne", true, "Count going up to parent frame as 1 when checking MaxInterpretedSearchLength during recompilee search" );
auto _CounterHalfLifeTime                 = _flag<std::int32_t>( "CounterHalfLifeTime", 30, "time (in seconds) in which invocation counters decay by half" );
auto _CreateScopeDescInfo                 = _flag<bool_t>( "CreateScopeDescInfo", true, "Create ScopeDescriptor info for new backend code" );
auto _DebugPerformance                    = _flag<bool_t>( "DebugPerformance", false, "Print info useful for performance debugging" );
auto _DeferUncommonBranches               = _flag<bool_t>( "DeferUncommonBranches", true, "Don't generate code for uncommon cases" );
auto _EdenSize                            = _flag<std::int32_t>( "EdenSize", 512, "size of eden (in Kbytes)" );
auto _EliminateContexts                   = _flag<bool_t>( "EliminateContexts", true, "Eliminate context allocations" );
auto _EliminateJumpsToJumps               = _flag<bool_t>( "EliminateJumpsToJumps", true, "Eliminate jumps to jumps" );
auto _EliminateUnneededNodes              = _flag<bool_t>( "EliminateUnneededNodes", true, "Eliminate dead code" );
auto _EnableInt3                          = _flag<bool_t>( "EnableInt3", true, "Enables/disables code generation for int3 instructions" );
auto _EnableOptimizedCodeRecompilation    = _flag<bool_t>( "EnableOptimizedCodeRecompilation", true, "Enable recompilation of optimized code" );
auto _EnableProcessPreemption             = _flag<bool_t>( "EnableProcessPreemption", false, "Enables or disables preemption of running Smalltalk processes" );
auto _EnableTasks                         = _flag<bool_t>( "EnableTasks", true, "Enable periodic tasks to be performed" );
auto _EventLogLength                      = _flag<std::int32_t>( "EventLogLength", 1000, "Length of internal event log" );
auto _GenTraceCalls                       = _flag<bool_t>( "GenTraceCalls", false, "Generate code for TraceCalls" );
auto _GenerateFullDebugInfo               = _flag<bool_t>( "GenerateFullDebugInfo", false, "Generate debugging info for each byte code and not only for sends/traps" );
auto _GenerateHTML                        = _flag<bool_t>( "GenerateHTML", false, "Generate HTML output for documentation" );
auto _GenerateLiteScopeDescs              = _flag<bool_t>( "GenerateLiteScopeDescs", false, "generate lite scope descs" );
auto _GenerateSmalltalk                   = _flag<bool_t>( "GenerateSmalltalk", false, "Generate Smalltalk output for file_in" );
auto _GlobalCopyPropagate                 = _flag<bool_t>( "GlobalCopyPropagate", true, "Perform global copy propagation" );
auto _HeapSweeperInterval                 = _flag<std::int32_t>( "HeapSweeperInterval", 120, "Time interval (sec) between starting heap sweep" );
auto _Inline                              = _flag<bool_t>( "Inline", true, "Inline message sends" );
auto _InlinePrims                         = _flag<bool_t>( "InlinePrims", true, "Inline some primitive calls" );
auto _InliningDatabasePruningLimit        = _flag<std::int32_t>( "InliningDatabasePruningLimit", 3, "Min. number of nodes in inlining structure to qualify for database" );
auto _InvocationCounterLimit              = _flag<std::int32_t>( "InvocationCounterLimit", 10000, "max. number of method invocations before (re-)compiling" );
auto _JumpTableSize                       = _flag<std::int32_t>( "JumpTableSize", 8 * 1024, "size of jump table" );
auto _LRUDecayFactor                      = _flag<std::int32_t>( "LRUDecayFactor", 2, "LRUDecayFactor" );
auto _LocalCopyPropagate                  = _flag<bool_t>( "LocalCopyPropagate", true, "Perform local copy propagation" );
auto _LogVMMessages                       = _flag<bool_t>( "LogVMMessages", true, "Log all vm messages to a file" );
auto _LoopCounterLimit                    = _flag<std::int32_t>( "LoopCounterLimit", 10000, "max. number of loop iterations before (re-)compiling" );
auto _MakeBlockMethodZombies              = _flag<bool_t>( "MakeBlockMethodZombies", false, "Make block NativeMethod zombies if needed" );
auto _MaterializeEliminatedBlocks         = _flag<bool_t>( "MaterializeEliminatedBlocks", true, "Create fake blocks for eliminated blocks when printing stack" );
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
auto _MemoizeBlocks                       = _flag<bool_t>( "MemoizeBlocks", true, "memoize (delay creation of) blocks" );
auto _MinBlockCostFraction                = _flag<std::int32_t>( "MinBlockCostFraction", 50, "(in %) inline block if makes up more than this fraction of parent's cost" );
auto _MinInvocationsBeforeTrust           = _flag<std::int32_t>( "MinInvocationsBeforeTrust", 100, "min. number of invocations required before trusting NativeMethod's PICs" );
auto _MinSendsBeforeRecompile             = _flag<std::int32_t>( "MinSendsBeforeRecompile", 2000, "min number of sends a method must have performed before being recompiled" );
auto _NativeMethodAgeLimit                = _flag<std::int32_t>( "NativeMethodAgeLimit", 2, "min. number of sweeps before NativeMethod becomes old" );
auto _NumberOfBlockAllocations            = _flag<std::int32_t>( "NumberOfBlockAllocations", 0, "Number of allocated blocks" );
auto _NumberOfBytecodesExecuted           = _flag<std::int32_t>( "NumberOfBytecodesExecuted", 0, "Number of bytecodes executed by interpreter (if tracing)" );
auto _NumberOfContextAllocations          = _flag<std::int32_t>( "NumberOfContextAllocations", 0, "Number of allocated block contexts" );
auto _ObjectHeapExpandSize                = _flag<std::int32_t>( "ObjectHeapExpandSize", 512, "Chunk size (in Kbytes) by which the object heap grows" );
auto _OldSize                             = _flag<std::int32_t>( "OldSize", 3 * 1024, "initial size of oldspace (in Kbytes)" );
auto _OptimizeIntegerLoops                = _flag<bool_t>( "OptimizeIntegerLoops", true, "optimize integer loops" );
auto _OptimizeLoops                       = _flag<bool_t>( "OptimizeLoops", true, "optimize loops (hoist type tests" );
auto _PICSize                             = _flag<std::int32_t>( "PICSize", 128, "size of PolymorphicInlineCache cache (in Kbytes)" );
auto _PrintAssemblyCode                   = _flag<bool_t>( "PrintAssemblyCode", false, "Print assembly code" );
auto _PrintCode                           = _flag<bool_t>( "PrintCode", false, "Print intermediate code" );
auto _PrintCodeCompaction                 = _flag<bool_t>( "PrintCodeCompaction", false, "Print code compaction" );
auto _PrintCodeGeneration                 = _flag<bool_t>( "PrintCodeGeneration", false, "Print code generation with new backend" );
auto _PrintCodeReclamation                = _flag<bool_t>( "PrintCodeReclamation", false, "Print code reclamation" );
auto _PrintCodeSweep                      = _flag<bool_t>( "PrintCodeSweep", false, "Print sweeps through zone/methods" );
auto _PrintCompilation                    = _flag<bool_t>( "PrintCompilation", false, "Print each compilation" );
auto _PrintCompilerWarnings               = _flag<bool_t>( "PrintCompilerWarnings", true, "Print compiler warning?" );
auto _PrintCopyPropagation                = _flag<bool_t>( "PrintCopyPropagation", false, "Print info about copy propagation" );
auto _PrintDebugInfo                      = _flag<bool_t>( "PrintDebugInfo", false, "Print debugging info of ScopeDescs generated for NativeMethod" );
auto _PrintDebugInfoGeneration            = _flag<bool_t>( "PrintDebugInfoGeneration", false, "Print debugging info generation" );
auto _PrintEliminateContexts              = _flag<bool_t>( "PrintEliminateContexts", false, "Print info about eliminating context allocations" );
auto _PrintEliminateUnnededNodes          = _flag<bool_t>( "PrintEliminateUnnededNodes", false, "Print info about dead code elimination" );
auto _PrintEliminatedJumps                = _flag<bool_t>( "PrintEliminatedJumps", false, "Print eliminated jumps" );
auto _PrintExposed                        = _flag<bool_t>( "PrintExposed", false, "Print info about exposed-block analysis" );
auto _PrintGC                             = _flag<bool_t>( "PrintGC", true, "Print message at garbage collect" );
auto _PrintGlobalAllocation               = _flag<bool_t>( "PrintGlobalAllocation", false, "Print info about global register allocation" );
auto _PrintHeapAllocation                 = _flag<bool_t>( "PrintHeapAllocation", false, "Print heap allocation" );
auto _PrintHexAddresses                   = _flag<bool_t>( "PrintHexAddresses", true, "Print hex addresses in print outs (otherwise print 0)" );
auto _PrintInlineCacheInvalidation        = _flag<bool_t>( "PrintInlineCacheInvalidation", false, "Print inline cache invalidation" );
auto _PrintInlining                       = _flag<bool_t>( "PrintInlining", false, "Print info about inlining" );
auto _PrintInliningDatabaseCompilation    = _flag<bool_t>( "PrintInliningDatabaseCompilation", false, "Print inlining database compilations?" );
auto _PrintInterpreter                    = _flag<bool_t>( "PrintInterpreter", false, "Prints the generated interpreter's code" );
auto _PrintLRUSweep                       = _flag<bool_t>( "PrintLRUSweep", false, "Print least-recently used" );
auto _PrintLRUSweep2                      = _flag<bool_t>( "PrintLRUSweep2", false, "Print least-recently used" );
auto _PrintLocalAllocation                = _flag<bool_t>( "PrintLocalAllocation", false, "Print info about local register allocation" );
auto _PrintLongFrames                     = _flag<bool_t>( "PrintLongFrames", false, "Print tons of details in VM stack traces" );
auto _PrintLoopOpts                       = _flag<bool_t>( "PrintLoopOpts", false, "Print info about loop optimizations" );
auto _PrintMakeConformantCode             = _flag<bool_t>( "PrintMakeConformantCode", false, "Print code generated by makeConformant" );
auto _PrintMethodFlushing                 = _flag<bool_t>( "PrintMethodFlushing", false, "Print method flushing" );
auto _PrintObjectID                       = _flag<bool_t>( "PrintObjectID", true, "Always prepend object ID when printing" );
auto _PrintOopAddress                     = _flag<bool_t>( "PrintOopAddress", false, "Always print the location of the Oop" );
auto _PrintPRegMapping                    = _flag<bool_t>( "PrintPRegMapping", false, "Print PseudoRegisterMapping during code generation" );
auto _PrintProgress                       = _flag<std::int32_t>( "PrintProgress", 0, "No. of compilations that cause a . to be printed out (0 means turned off)" );
auto _PrintRScopes                        = _flag<bool_t>( "PrintRScopes", false, "Print info about RScopes (type feedback sources)" );
auto _PrintRecompilation                  = _flag<bool_t>( "PrintRecompilation", false, "Print each recompilation" );
auto _PrintRecompilation2                 = _flag<bool_t>( "PrintRecompilation2", false, "Print details about each recompilation" );
auto _PrintRegAlloc                       = _flag<bool_t>( "PrintRegAlloc", false, "Print register allocation" );
auto _PrintRegTargeting                   = _flag<bool_t>( "PrintRegTargeting", false, "Print info about register targeting" );
auto _PrintResourceAllocation             = _flag<bool_t>( "PrintResourceAllocation", false, "Print each resource area allocation" );
auto _PrintResourceChunkAllocation        = _flag<bool_t>( "PrintResourceChunkAllocation", false, "Print each resource area chunk allocation" );
auto _PrintScavenge                       = _flag<bool_t>( "PrintScavenge", false, "Print message at scavenge" );
auto _PrintSplitting                      = _flag<bool_t>( "PrintSplitting", false, "Print info about boolean splitting" );
auto _PrintStackAfterUnpacking            = _flag<bool_t>( "PrintStackAfterUnpacking", false, "Print stack after unpacking deoptimized frames" );
auto _PrintStackAtScavenge                = _flag<bool_t>( "PrintStackAtScavenge", false, "Print stack at Scavenge" );
auto _PrintStubRoutines                   = _flag<bool_t>( "PrintStubRoutines", false, "Prints the stub routine's code" );
auto _PrintUncommonBranches               = _flag<bool_t>( "PrintUncommonBranches", false, "Print message upon encountering uncommon case" );
auto _PrintVMMessages                     = _flag<bool_t>( "PrintVMMessages", true, "Print vm messages on _console" );
auto _ProfilerNumberOfCompiledMethods     = _flag<std::int32_t>( "ProfilerNumberOfCompiledMethods", 10, "Max. number of compiled methods to print" );
auto _ProfilerNumberOfInterpreterMethods  = _flag<std::int32_t>( "ProfilerNumberOfInterpreterMethods", 10, "Max. number of interpreter methods to print" );
auto _ProfilerShowMethodHolder            = _flag<bool_t>( "ProfilerShowMethodHolder", true, "Show method holder for method" );
auto _ReorderBBs                          = _flag<bool_t>( "ReorderBBs", true, "Reorder basic blocks" );
auto _ReservedCodeSize                    = _flag<std::int32_t>( "ReservedCodeSize", 10 * 1024, "Maximum size of code cache (in Kbytes)" );
auto _ReservedHeapSize                    = _flag<std::int32_t>( "ReservedHeapSize", 50 * 1024, "Maximum size for object heap in Kbytes" );
auto _ReservedPICSize                     = _flag<std::int32_t>( "ReservedPICSize", 4 * 1024, "Maximum size of PolymorphicInlineCache cache (in Kbytes)" );
auto _ShowMessageBoxOnError               = _flag<bool_t>( "ShowMessageBoxOnError", false, "Show a message box on error" );
auto _Splitting                           = _flag<bool_t>( "Splitting", true, "Perform message splitting" );
auto _StackPrintLimit                     = _flag<std::int32_t>( "StackPrintLimit", 64, "Number of stack frames to print in VM-level stack dump" );
auto _StopInterpreterAt                   = _flag<std::int32_t>( "StopInterpreterAt", 0, "Stops interpreter execution at specified bytecode number" );
auto _SurvivorSize                        = _flag<std::int32_t>( "SurvivorSize", 64, "size of survivor spaces (in Kbytes)" );
auto _SweeperUseTimer                     = _flag<bool_t>( "SweeperUseTimer", true, "Tells whether the sweeper should use timer interrupts or compile events" );
auto _ThreadStackSize                     = _flag<std::int32_t>( "ThreadStackSize", 512, "Size (in 1024) of each thread's stack" );
auto _TraceAllocation                     = _flag<bool_t>( "TraceAllocation", false, "Trace allocation" );
auto _TraceApplyChange                    = _flag<bool_t>( "TraceApplyChange", false, "Trace reflective operation" );
auto _TraceBehaviorPrims                  = _flag<bool_t>( "TraceBehaviorPrims", false, "Trace behavior primitives" );
auto _TraceBlockPrims                     = _flag<bool_t>( "TraceBlockPrims", false, "Trace block primitives" );
auto _TraceBootstrap                      = _flag<bool_t>( "TraceBootstrap", false, "Trace the Bootstrap reading" );
auto _TraceByteArrayPrims                 = _flag<bool_t>( "TraceByteArrayPrims", false, "Trace ByteArray primitives" );
auto _TraceBytecodes                      = _flag<bool_t>( "TraceBytecodes", false, "Trace byte code execution" );
auto _TraceCallBackPrims                  = _flag<bool_t>( "TraceCallBackPrims", false, "Trace callBack primitives" );
auto _TraceCalls                          = _flag<bool_t>( "TraceCalls", false, "Print non-inlined calls/returns (must compile with VerifyCode or GenTraceCalls)" );
auto _TraceCanonicalContext               = _flag<bool_t>( "TraceCanonicalContext", false, "Trace canonical context construction" );
auto _TraceDLLCalls                       = _flag<bool_t>( "TraceDLLCalls", false, "Trace DLL function calls" );
auto _TraceDLLLookup                      = _flag<bool_t>( "TraceDLLLookup", false, "Trace DLL function lookup" );
auto _TraceDebugPrims                     = _flag<bool_t>( "TraceDebugPrims", false, "Trace debug primitives" );
auto _TraceDeoptimization                 = _flag<bool_t>( "TraceDeoptimization", false, "Trace deoptimizion" );
auto _TraceDoubleByteArrayPrims           = _flag<bool_t>( "TraceDoubleByteArrayPrims", false, "Trace DoubleByteArray primitives" );
auto _TraceDoublePrims                    = _flag<bool_t>( "TraceDoublePrims", false, "Trace Double primitives" );
auto _TraceDoubleValueArrayPrims          = _flag<bool_t>( "TraceDoubleValueArrayPrims", false, "Trace DoubleByteArray primitives" );
auto _TraceExpansion                      = _flag<bool_t>( "TraceExpansion", false, "Trace expansion of committed Space" );
auto _TraceGC                             = _flag<bool_t>( "TraceGC", true, "Trace Garbage Collection" );
auto _TraceInlineCacheMiss                = _flag<bool_t>( "TraceInlineCacheMiss", false, "Trace inline cache misses" );
auto _TraceInliningDatabase               = _flag<bool_t>( "TraceInliningDatabase", false, "Trace inlining database" );
auto _TraceInterpreterFramesAt            = _flag<std::int32_t>( "TraceInterpreterFramesAt", 0, "Trace interpreter frames at specified bytecode number" );
auto _TraceLookup                         = _flag<bool_t>( "TraceLookup", false, "Trace lookups" );
auto _TraceLookup2                        = _flag<bool_t>( "TraceLookup2", false, "Trace lookups in excruciating detail" );
auto _TraceLookupAtMiss                   = _flag<bool_t>( "TraceLookupAtMiss", false, "Trace lookups at lookup cache miss" );
auto _TraceMessageSend                    = _flag<bool_t>( "TraceMessageSend", false, "Trace all message sends" );
auto _TraceMethodPrims                    = _flag<bool_t>( "TraceMethodPrims", false, "Trace method prims" );
auto _TraceMixinPrims                     = _flag<bool_t>( "TraceMixinPrims", false, "Trace mixin prims" );
auto _TraceObjArrayPrims                  = _flag<bool_t>( "TraceObjArrayPrims", false, "Trace objArray primitives" );
auto _TraceOopPrims                       = _flag<bool_t>( "TraceOopPrims", false, "Trace Oop primitives" );
auto _TraceProcessEvents                  = _flag<bool_t>( "TraceProcessEvents", false, "Trace all process events" );
auto _TraceProcessPrims                   = _flag<bool_t>( "TraceProcessPrims", false, "Trace process primitives" );
auto _TraceProxyPrims                     = _flag<bool_t>( "TraceProxyPrims", false, "Trace Proxy primitives" );
auto _TraceResults                        = _flag<bool_t>( "TraceResults", false, "Trace NativeMethod results" );
auto _TraceSmiPrims                       = _flag<bool_t>( "TraceSmiPrims", false, "Trace SmallInteger primitives" );
auto _TraceSystemPrims                    = _flag<bool_t>( "TraceSystemPrims", false, "Trace system primitives" );
auto _TraceVMOperation                    = _flag<bool_t>( "TraceVMOperation", false, "Trace vm operations" );
auto _TraceVirtualFramePrims              = _flag<bool_t>( "TraceVirtualFramePrims", false, "Trace VirtualFrame primitives" );
auto _TraceZombieCreation                 = _flag<bool_t>( "TraceZombieCreation", false, "Trace NativeMethod zombie creation" );
auto _TryNewBackend                       = _flag<bool_t>( "TryNewBackend", false, "Use new backend & set additional flags as needed for compilation" );
auto _TypeFeedback                        = _flag<bool_t>( "TypeFeedback", true, "use type feedback data" );
auto _TypePredict                         = _flag<bool_t>( "TypePredict", true, "Predict smi_t/bool_t/array message sends" );
auto _TypePredictArrays                   = _flag<bool_t>( "TypePredictArrays", false, "Predict at:/at:Put: message sends" );
auto _UncommonAgeBackoffFactor            = _flag<std::int32_t>( "UncommonAgeBackoffFactor", 4, "for exponential back-off of UncommonAgeLimit based on NativeMethod version" );
auto _UncommonInvocationLimit             = _flag<std::int32_t>( "UncommonInvocationLimit", 10000, "min. number of invocations uncommon NativeMethod before recompiling it again" );
auto _UncommonRecompileLimit              = _flag<std::int32_t>( "UncommonRecompileLimit", 5, "min. number of uncommon traps before recompiling" );
auto _UseAccessMethods                    = _flag<bool_t>( "UseAccessMethods", true, "Use access methods" );
auto _UseFPUStack                         = _flag<bool_t>( "UseFPUStack", false, "Use FPU stack for floats (unsafe)" );
auto _UseGlobalFlatProfiling              = _flag<bool_t>( "UseGlobalFlatProfiling", true, "Include all processes when flat-profiling" );
auto _UseInlineCaching                    = _flag<bool_t>( "UseInlineCaching", true, "Use inline caching in compiled code" );
auto _UseInliningDatabase                 = _flag<bool_t>( "UseInliningDatabase", false, "Use the inlining database for recompilation" );
auto _UseInliningDatabaseEagerly          = _flag<bool_t>( "UseInliningDatabaseEagerly", false, "Use the inlining database eagerly at lookup" );
auto _UseLRUInterrupts                    = _flag<bool_t>( "UseLRUInterrupts", true, "User timers for zone LRU info" );
auto _UseMICs                             = _flag<bool_t>( "UseMICs", true, "Use megamorphic PICs (MegamorphicInlineCache)" );
auto _UseNativeMethodAging                = _flag<bool_t>( "UseNativeMethodAging", true, "Age nativeMethods before recompiling them" );
auto _UseNewBackend                       = _flag<bool_t>( "UseNewBackend", false, "Use new backend" );
auto _UseNewMakeConformant                = _flag<bool_t>( "UseNewMakeConformant", true, "Use new makeConformant function" );
auto _UsePredictedMethods                 = _flag<bool_t>( "UsePredictedMethods", true, "Use predicted methods" );
auto _UsePrimitiveMethods                 = _flag<bool_t>( "UsePrimitiveMethods", false, "Use primitive methods" );
auto _UseRecompilation                    = _flag<bool_t>( "UseRecompilation", true, "Automatically (re-)compile frequently-used methods" );
auto _UseSlidingSystemAverage             = _flag<bool_t>( "UseSlidingSystemAverage", true, "Compute sliding system average on the fly" );
auto _UseTimers                           = _flag<bool_t>( "UseTimers", true, "Tells whether the VM should use timers (only used at startup)" );
auto _VerifyAfterGC                       = _flag<bool_t>( "VerifyAfterGC", false, "Verify system after garbage collect" );
auto _VerifyAfterScavenge                 = _flag<bool_t>( "VerifyAfterScavenge", false, "Verify system after scavenge" );
auto _VerifyBeforeGC                      = _flag<bool_t>( "VerifyBeforeGC", false, "Verify system before garbage collect" );
auto _VerifyBeforeScavenge                = _flag<bool_t>( "VerifyBeforeScavenge", false, "Verify system before scavenge" );
auto _VerifyCode                          = _flag<bool_t>( "VerifyCode", false, "Generates verification code in compiled code" );
auto _VerifyDebugInfo                     = _flag<bool_t>( "VerifyDebugInfo", false, "Verify compiled-code debug info at each call (very slow)" );
auto _VerifyZoneOften                     = _flag<bool_t>( "VerifyZoneOften", false, "Verify compiled-code zone often" );
auto _WizardMode                          = _flag<bool_t>( "WizardMode", false, "Wizard debugging mode" );
auto _ZapResourceArea                     = _flag<bool_t>( "ZapResourceArea", false, "Zap the resource area when deallocated" );
