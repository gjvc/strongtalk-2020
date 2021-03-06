//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/platform/platform.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/utility/StringOutputStream.hpp"

class Node;

// helper functions / data structures for compiler support; e.g. compiler parameters (inlining limits etc.)

// kinds of inlining limits
enum class InlineLimitType {
    NormalFnLimit,          // "size" of normal method (see msgCost)
    BlockArgFnLimit,        // size of method with block args
    BlockFnLimit,           // size of block method (value, value: etc)
    SplitCostLimit,         // max total cost of copied nodes
    NormalFnInstrLimit,     // size (instructions) of normal method
    BlockArgFnInstrLimit,   // ditto for method with block args
    BlockFnInstrLimit,      // ditto for block method
    NmInstrLimit,           // desired max. NativeMethod size
    LastLimit               // sentinel
};

typedef bool (*checkLocalSendFn)( SymbolOop sel );

enum class InlineFnType {
    NormalFn,               // normal method
    BlockArgFn,             // method with one or more block args
    BlockFn                 // block method (value, value: etc)
};

// parameters of cost function that computes Space cost of inlining a particular method
struct CostParam {
    std::int32_t localCost;          // local variable access / assign
    std::int32_t cheapSendCost;      // "cheap" send (see isCheapMessage)
    std::int32_t sendCost;           // arbitrary send
    std::int32_t blockArgPenalty;    // penalty if non-cheap send has block arg(s)
    std::int32_t primCallCost;       // primitive call

    CostParam( std::int32_t l, std::int32_t c, std::int32_t s, std::int32_t b, std::int32_t p ) :
        localCost{ l },
        cheapSendCost{ c },
        sendCost{ s },
        blockArgPenalty{ b },
        primCallCost{ p } {
    }

};


// The PerformanceDebugger reports info useful for finding performance bugs (e.g., contexts and blocks that can't be eliminated and the reasons why).

class PrimitiveDescriptor;

class Compiler;

class BlockPseudoRegister;

class InlinedScope;


class PerformanceDebugger : public ResourceObject {

private:
    Compiler                             *_compiler;
    bool                                 _compileAlreadyReported;          // have we already reported something for this compile?
    GrowableArray<BlockPseudoRegister *> *_blockPseudoRegisters;
    GrowableArray<char *>                *_reports;                                // list of reports already printed (to avoid duplicates)
    StringOutputStream                   *_stringStream;
    GrowableArray<InlinedScope *>        *_notInlinedBecauseNativeMethodTooBig;

public:
    PerformanceDebugger( Compiler *c );

    PerformanceDebugger() = default;
    virtual ~PerformanceDebugger() = default;
    PerformanceDebugger( const PerformanceDebugger & ) = default;
    PerformanceDebugger &operator=( const PerformanceDebugger & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    void report_context( InlinedScope *s );                                     // couldn't eliminate scope's context
    void report_block( Node *s, BlockPseudoRegister *blk, const char *what );   // couldn't eliminate block
    void report_toobig( InlinedScope *s );                                      // NativeMethod getting too big
    void report_uncommon( bool reoptimizing );                                // uncommon recompile
    void report_primitive_failure( PrimitiveDescriptor *pd );                   // failure not uncommon
    void finish_reporting();

private:
    void report_compile();

    void start_report();

    void stop_report();

    friend class Reporter;
};
