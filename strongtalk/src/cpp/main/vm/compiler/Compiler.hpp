//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/compiler/PerformanceDebugger.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/compiler/CodeGenerator.hpp"
#include "vm/runtime/ResourceObject.hpp"

extern std::int32_t compilationCount;
extern Compiler     *theCompiler;

class CodeGenerator;

class NonLocalReturnTestNode;


// A Compiler holds all the global state required for a single compilation.

class Compiler : public PrintableResourceObject {

private:
    GrowableArray<InlinedScope *> _scopeStack;                         // to keep track of current scope
    std::int32_t                  _totalNofBytes;                      // total no. of bytes compiled (for statistics)
    std::int32_t                  _special_handler_call_offset;        // call to recompilation stub or zombie handler (offset in bytes)
    std::int32_t                  _entry_point_offset;                 // NativeMethod entry point if receiver is unknown (offset in bytes)
    std::int32_t                  _verified_entry_point_offset;        // NativeMethod entry point if receiver is known (offset in bytes)
    std::int32_t                  _totalNofFloatTemporaries;           // the number of floatTemporaries for this NativeMethod
    std::int32_t                  _float_section_size;                 // the size of the float section on the stack in oops
    std::int32_t                  _float_section_start_offset;         // the offset of the float section on the stack relative to ebp in oops
    CodeBuffer                    *_code;                              // the buffer used for code generation
    std::int32_t                  _nextLevel;                          // optimization level for NativeMethod being created
    bool                          _hasInlinableSendsRemaining;         // no inlinable sends remaining?
    bool                          _uses_inlining_database;             // tells whether the compilation is base on inlinine database information.

public:
    LookupKey                               *key;
    CompiledInlineCache                     *ic;                       // sending inline cache
    NativeMethod                            *parentNativeMethod;       // NativeMethod of lexical parent (or nullptr if not compiling a block method)
    MethodOop                               method;                    // top-level method being compiled
    NonInlinedBlockScopeDescriptor          *blockScope;               // or nullptr if not compiling a block method
    RecompilationScope                      *recompileeRScope;         // recompilee's rscope (or nullptr)
    std::int32_t                            countID;                   // recompile counter ID
    JumpTableID                             main_jumpTable_id;         // jump table id
    JumpTableID                             promoted_jumpTable_id;     // promoted jump table entry for block method only
    bool                                    useUncommonTraps;          // ok to use uncommon traps?
    ScopeDescriptorRecorder                 *rec;                      //
    InlinedScope                            *topScope;                 // top scope
    BasicBlock                              *firstBasicBlock;          // first basic block
    GrowableArray<NonLocalReturnTestNode *> *nlrTestPoints;            // needed to fix up NonLocalReturn points after node generation
    GrowableArray<InlinedScope *>           *scopes;                   // list of all scopes (indexed by scope ID)
    GrowableArray<InlinedScope *>           *contextList;              // list of all scopes with run-time contexts
    GrowableArray<BlockPseudoRegister *>    *blockClosures;            // list of all block literals created so far
    Node                                    *firstNode;                // the very first node of the intermediate representation
    PerformanceDebugger                     *reporter;                 // for reporting performance info
    StringOutputStream                      *messages;                 // debug messages

    std::array<std::int32_t, static_cast<std::int32_t>(InlineLimitType::LastLimit)> inlineLimit; // limits for current compilation

private:
    void initialize( RecompilationScope *remote_scope = nullptr );

    void initTopScope();

    void initLimits();

    void buildBBs();

    void fixupNonLocalReturnTestPoints();

    void computeBlockInfo();

public:
    Compiler( LookupKey *k, MethodOop m, CompiledInlineCache *ic = nullptr );   // normal entry point (method lookups)
    Compiler( BlockClosureOop blk, NonInlinedBlockScopeDescriptor *scope );     // for block methods
    Compiler( RecompilationScope *scope );                                      // for inlining database
    ~Compiler() {
        finalize();
    }


    CodeBuffer *code() const;

    ScopeDescriptorRecorder *scopeDescRecorder();

    NativeMethod *compile();

    void finalize();

    std::int32_t level() const;      // optimization level of new NativeMethod
    std::int32_t version() const;    // version ("nth recompilation") of new NativeMethod
    std::int32_t special_handler_call_offset() const {
        return _special_handler_call_offset;
    }


    std::int32_t entry_point_offset() const {
        return _entry_point_offset;
    }


    std::int32_t verified_entry_point_offset() const {
        return _verified_entry_point_offset;
    }


    std::int32_t get_invocation_counter_limit() const;        // invocation limit for NativeMethod being created

    void set_special_handler_call_offset( std::int32_t offset );

    void set_entry_point_offset( std::int32_t offset );

    void set_verified_entry_point_offset( std::int32_t offset );


    std::int32_t totalNofFloatTemporaries() const {
        st_assert( _totalNofFloatTemporaries >= 0, "not yet determined" );
        return _totalNofFloatTemporaries;
    }


    bool has_float_temporaries() const {
        return totalNofFloatTemporaries() > 0;
    }


    std::int32_t float_section_size() const {
        return has_float_temporaries() ? _float_section_size : 0;
    }


    std::int32_t float_section_start_offset() const {
        return has_float_temporaries() ? _float_section_start_offset : 0;
    }


    void set_float_section_size( std::int32_t size );

    void set_float_section_start_offset( std::int32_t offset );


    bool is_block_compile() const {
        return parentNativeMethod not_eq nullptr;
    }


    bool is_method_compile() const {
        return not is_block_compile();
    }


    bool is_uncommon_compile() const;            // recompiling because of uncommon trap?
    bool is_database_compile() const {
        return _uses_inlining_database;
    }


    std::int32_t number_of_noninlined_blocks() const;                // no. of noninlined blocks in NativeMethod (used for jump entry alloc.)
    void copy_noninlined_block_info( NativeMethod *nm );    // copy the noninlined block info to the NativeMethod.
    void nofBytesCompiled( std::int32_t n ) {
        _totalNofBytes += n;
    }


    std::int32_t totalNofBytes() const {
        return _totalNofBytes;
    }


    std::int32_t estimatedSize() const;

    InlinedScope *currentScope() const;            // scope currently being compiled
    void enterScope( InlinedScope *s );

    void exitScope( InlinedScope *s );

    void allocateArgs( std::int32_t nargs, bool isPrimCall );

    bool registerUninlinable( Inliner *inliner );

    void print();

    void print_short();

    void print_key( ConsoleOutputStream *s );

    void print_code( bool suppressTrivial );
};


// cout: output stream for compiler messages
// Intended usage pattern: if (CompilerDebug) cout(PrintXXX)->print("....").
// This will print the message to _console if the PrintXXX flag is set or to the compiler's string buffer otherwise.
ConsoleOutputStream *cout( bool flag );    // for compiler debugging; returns stdout or compiler-internal string stream depending on flag
void print_cout();        // prints hidden messages of current compilation
