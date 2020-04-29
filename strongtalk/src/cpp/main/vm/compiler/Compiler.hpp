//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/compiler/PerformanceDebugger.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/compiler/CodeGenerator.hpp"
#include "vm/runtime/ResourceObject.hpp"

extern int compilationCount;
extern Compiler * theCompiler;

class CodeGenerator;

class NonLocalReturnTestNode;


// A Compiler holds all the global state required for a single compilation.

class Compiler : public PrintableResourceObject {

    private:
        GrowableArray <InlinedScope *> _scopeStack;                         // to keep track of current scope
        int                            _totalNofBytes;                      // total no. of bytes compiled (for statistics)
        int                            _special_handler_call_offset;        // call to recompilation stub or zombie handler (offset in bytes)
        int                            _entry_point_offset;                 // NativeMethod entry point if receiver is unknown (offset in bytes)
        int                            _verified_entry_point_offset;        // NativeMethod entry point if receiver is known (offset in bytes)
        int                            _totalNofFloatTemporaries;           // the number of floatTemporaries for this NativeMethod
        int                            _float_section_size;                 // the size of the float section on the stack in oops
        int                            _float_section_start_offset;         // the offset of the float section on the stack relative to ebp in oops
        CodeBuffer * _code;                              // the buffer used for code generation
        int    _nextLevel;                          // optimization level for NativeMethod being created
        bool_t _hasInlinableSendsRemaining;         // no inlinable sends remaining?
        bool_t _uses_inlining_database;             // tells whether the compilation is base on inlinine database information.

    public:
        LookupKey           * key;
        CompiledInlineCache * ic;                       // sending inline cache
        NativeMethod        * parentNativeMethod;       // NativeMethod of lexical parent (or nullptr if not compiling a block method)
        MethodOop method;                    // top-level method being compiled
        NonInlinedBlockScopeDescriptor * blockScope;               // or nullptr if not compiling a block method
        RecompilationScope             * recompileeRScope;         // recompilee's rscope (or nullptr)
        int         countID;                   // recompile counter ID
        JumpTableID main_jumpTable_id;         // jump table id
        JumpTableID promoted_jumpTable_id;     // promoted jump table entry for block method only
        bool_t      useUncommonTraps;          // ok to use uncommon traps?
        ScopeDescriptorRecorder * rec;                      //
        InlinedScope            * topScope;                 // top scope
        BasicBlock              * firstBasicBlock;          // first basic block
        GrowableArray <NonLocalReturnTestNode *> * nlrTestPoints;            // needed to fix up NonLocalReturn points after node generation
        GrowableArray <InlinedScope *>           * scopes;                   // list of all scopes (indexed by scope ID)
        GrowableArray <InlinedScope *>           * contextList;              // list of all scopes with run-time contexts
        GrowableArray <BlockPseudoRegister *>    * blockClosures;            // list of all block literals created so far
        Node                * firstNode;                // the very first node of the intermediate representation
        PerformanceDebugger * reporter;                 // for reporting performance info
        StringOutputStream  * messages;                 // debug messages
        std::array <int, static_cast<int>(InlineLimitType::LastLimit)> inlineLimit;                               // limits for current compilation

    private:
        void initialize( RecompilationScope * remote_scope = nullptr );

        void initTopScope();

        void initLimits();

        void buildBBs();

        void fixupNonLocalReturnTestPoints();

        void computeBlockInfo();

    public:
        Compiler( LookupKey * k, MethodOop m, CompiledInlineCache * ic = nullptr );   // normal entry point (method lookups)
        Compiler( BlockClosureOop blk, NonInlinedBlockScopeDescriptor * scope );     // for block methods
        Compiler( RecompilationScope * scope );                                      // for inlining database
        ~Compiler() {
            finalize();
        }


        CodeBuffer * code() const;

        ScopeDescriptorRecorder * scopeDescRecorder();

        NativeMethod * compile();

        void finalize();

        int level() const;      // optimization level of new NativeMethod
        int version() const;    // version ("nth recompilation") of new NativeMethod
        int special_handler_call_offset() const {
            return _special_handler_call_offset;
        }


        int entry_point_offset() const {
            return _entry_point_offset;
        }


        int verified_entry_point_offset() const {
            return _verified_entry_point_offset;
        }


        int get_invocation_counter_limit() const;        // invocation limit for NativeMethod being created

        void set_special_handler_call_offset( int offset );

        void set_entry_point_offset( int offset );

        void set_verified_entry_point_offset( int offset );


        int totalNofFloatTemporaries() const {
            st_assert( _totalNofFloatTemporaries >= 0, "not yet determined" );
            return _totalNofFloatTemporaries;
        }


        bool_t has_float_temporaries() const {
            return totalNofFloatTemporaries() > 0;
        }


        int float_section_size() const {
            return has_float_temporaries() ? _float_section_size : 0;
        }


        int float_section_start_offset() const {
            return has_float_temporaries() ? _float_section_start_offset : 0;
        }


        void set_float_section_size( int size );

        void set_float_section_start_offset( int offset );


        bool_t is_block_compile() const {
            return parentNativeMethod not_eq nullptr;
        }


        bool_t is_method_compile() const {
            return not is_block_compile();
        }


        bool_t is_uncommon_compile() const;            // recompiling because of uncommon trap?
        bool_t is_database_compile() const {
            return _uses_inlining_database;
        }


        int number_of_noninlined_blocks() const;                // no. of noninlined blocks in NativeMethod (used for jump entry alloc.)
        void copy_noninlined_block_info( NativeMethod * nm );    // copy the noninlined block info to the NativeMethod.
        void nofBytesCompiled( int n ) {
            _totalNofBytes += n;
        }


        int totalNofBytes() const {
            return _totalNofBytes;
        }


        int estimatedSize() const;

        InlinedScope * currentScope() const;            // scope currently being compiled
        void enterScope( InlinedScope * s );

        void exitScope( InlinedScope * s );

        void allocateArgs( int nargs, bool_t isPrimCall );

        bool_t registerUninlinable( Inliner * inliner );

        void print();

        void print_short();

        void print_key( ConsoleOutputStream * s );

        void print_code( bool_t suppressTrivial );
};


// cout: output stream for compiler messages
// Intended usage pattern: if (CompilerDebug) cout(PrintXXX)->print("....").
// This will print the message to _console if the PrintXXX flag is set or to the compiler's string buffer otherwise.
ConsoleOutputStream * cout( bool_t flag );    // for compiler debugging; returns stdout or compiler-internal string stream depending on flag
void print_cout();        // prints hidden messages of current compilation


