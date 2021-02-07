
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/Compiler.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/recompiler/RecompilationPolicy.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/utilities/disassembler.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/compiler/oldCodeGenerator.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/compiler/Inliner.hpp"
#include "vm/compiler/RegisterAllocator.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/compiler/NodeFactory.hpp"
#include "vm/utilities/StringOutputStream.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"
#include "vm/runtime/DeltaProcess.hpp"


bool verifyOften = false;

std::int32_t       compilationCount = 0;
Compiler           *theCompiler     = nullptr;
Compiler           *lastCompiler    = nullptr;        // for debugging
BasicBlockIterator *last_bbIterator;


void compiler_init() {
    spdlog::info( "%system-init:  compiler_init" );

    CompilerDebug = true;
}


ScopeDescriptorRecorder *Compiler::scopeDescRecorder() {
    return rec;
}


CodeBuffer *Compiler::code() const {
    return _code;
}


Compiler::Compiler( LookupKey *k, MethodOop m, CompiledInlineCache *i ) :
    key{ k },
    method{ m },
    ic{ i },
    parentNativeMethod{ nullptr },
    _scopeStack{},
    _totalNofBytes{},
    blockScope{ nullptr },
    main_jumpTable_id{ JumpTableID() },
    promoted_jumpTable_id{ JumpTableID() },
    _special_handler_call_offset{},
    _entry_point_offset{},
    _verified_entry_point_offset{},
    _totalNofFloatTemporaries{},
    _float_section_size{},
    _float_section_start_offset{},
    _code{},
    _nextLevel{},
    _hasInlinableSendsRemaining{},
    _uses_inlining_database{},
    recompileeRScope{},
    countID{},
    useUncommonTraps{},
    rec{},
    topScope{},
    firstBasicBlock{},
    nlrTestPoints{},
    scopes{},
    contextList{},
    blockClosures{},
    firstNode{},
    reporter{},
    messages{},
    inlineLimit{} {
    initialize();
}


Compiler::Compiler( RecompilationScope *scope ) :
    key{ scope->key() },
    method{ scope->method() },
    ic{ nullptr },
    parentNativeMethod{ nullptr },
    blockScope{ nullptr },
    main_jumpTable_id{ JumpTableID() },
    promoted_jumpTable_id{ JumpTableID() },
    _scopeStack{},
    _special_handler_call_offset{},
    _entry_point_offset{},
    _verified_entry_point_offset{},
    _totalNofFloatTemporaries{},
    _float_section_size{},
    _float_section_start_offset{},
    _code{},
    _nextLevel{},
    _hasInlinableSendsRemaining{},
    _uses_inlining_database{},
    _totalNofBytes{},
    recompileeRScope{},
    countID{},
    useUncommonTraps{},
    rec{},
    topScope{},
    firstBasicBlock{},
    nlrTestPoints{},
    scopes{},
    contextList{},
    blockClosures{},
    firstNode{},
    reporter{},
    messages{},
    inlineLimit{} {
    st_assert( scope not_eq nullptr, "scope must exist" );

    initialize( scope );
}


Compiler::Compiler( BlockClosureOop blk, NonInlinedBlockScopeDescriptor *scope ) :
    _special_handler_call_offset{},
    _entry_point_offset{},
    _verified_entry_point_offset{},
    _totalNofFloatTemporaries{},
    _float_section_size{},
    _float_section_start_offset{},
    _code{},
    _nextLevel{},
    _hasInlinableSendsRemaining{},
    _uses_inlining_database{},
    key{},
    ic{},
    parentNativeMethod{},
    method{},
    blockScope{},
    recompileeRScope{},
    countID{},
    main_jumpTable_id{},
    promoted_jumpTable_id{},
    _totalNofBytes{},
    useUncommonTraps{},
    rec{},
    topScope{},
    firstBasicBlock{},
    nlrTestPoints{},
    scopes{},
    contextList{},
    blockClosures{},
    firstNode{},
    reporter{},
    messages{},
    inlineLimit{},
    _scopeStack( 10 ) {
    // Create a valid key for the compiled method.
    // {receiver class, block method} see key.hpp
    key = LookupKey::allocate( scope->parent()->selfKlass(), scope->method() );

    st_assert( blk->isCompiledBlock(), "must be compiled block" );
    JumpTableEntry *e = blk->jump_table_entry();
    std::int32_t   sub_index;
    parentNativeMethod = e->parent_nativeMethod( sub_index );

    std::int16_t main_index = parentNativeMethod->_mainId.is_block() ? parentNativeMethod->_promotedId.major() : parentNativeMethod->_mainId.major();

    main_jumpTable_id     = JumpTableID( main_index, sub_index );
    promoted_jumpTable_id = JumpTableID();

    blockScope = scope;
    method     = scope->method();
    ic         = nullptr;

    // Check if the inlining database is active
    RecompilationScope *rs = nullptr;
    if ( UseInliningDatabase ) {
        LookupKey *outer = &parentNativeMethod->outermost()->_lookupKey;
        rs = InliningDatabase::lookup_and_remove( outer, key );
        if ( rs and TraceInliningDatabase ) {
            _console->print( "ID block compile: " );
            key->print();
            _console->cr();
        }
    }
    initialize( rs );
}


void Compiler::finalize() {
    st_assert( theMacroAssembler == nullptr, "shouldn't have an assembler anymore" );
    _code           = nullptr;
    last_bbIterator = bbIterator;
    bbIterator      = nullptr;
    theCompiler     = nullptr;
}


std::int32_t Compiler::level() const {
    return _hasInlinableSendsRemaining ? MAX_RECOMPILATION_LEVELS - 1 : _nextLevel;
}


std::int32_t Compiler::version() const {
    if ( recompilee ) {
        // don't increment version number when uncommon-recompiling
        // (otherwise limit is reached too quickly)
        return recompilee->version() + ( is_uncommon_compile() ? 0 : 1 );
    } else {
        return 0;
    }
}


std::int32_t Compiler::estimatedSize() const {
    // estimated target NativeMethod size (bytes)
    return NodeFactory::_cumulativeCost;
}


InlinedScope *Compiler::currentScope() const {
    return _scopeStack.top();
}


void Compiler::enterScope( InlinedScope *s ) {
    _scopeStack.push( s );
}


void Compiler::exitScope( InlinedScope *s ) {
    st_assert( s == _scopeStack.top(), "bad nesting" );
    _scopeStack.pop();
}


void Compiler::initialize( RecompilationScope *remote_scope ) {
    st_assert( VMProcess::vm_operation() not_eq nullptr, "must be in vmProcess to compile" );

    if ( VMProcess::vm_operation() == nullptr ) {
        spdlog::warn( "should be in vmProcess to compile" ); // softened to a warning to support testing
    }

    compilationCount++;
    messages = new StringOutputStream( 250 * 1024 );
    if ( remote_scope ) {
        _uses_inlining_database = true;
        recompileeRScope        = remote_scope;
    } else {
        _uses_inlining_database = false;
    }

    recompileeRScope = remote_scope;
    st_assert( theCompiler == nullptr, "shouldn't have but one compiler at a time" );
    st_assert( theMacroAssembler == nullptr, "shouldn't have an assembler yet" );

    PseudoRegister::initPseudoRegisters();    // must come early (before any PseudoRegister allocation)
    initNodes();        // same here (before creating nodes)
    initLimits();

    theCompiler  = lastCompiler = this;
    _code        = new CodeBuffer( CompilerInstrsSize, CompilerInstrsSize / 2 );
    countID      = -1;
    topScope     = nullptr;
    bbIterator   = new BasicBlockIterator;
    theAllocator = new RegisterAllocator();
    st_assert( method, "must have method" );

    Scope::initialize();
    _totalNofBytes               = 0;
    _special_handler_call_offset = -1;
    _entry_point_offset          = -1;
    _verified_entry_point_offset = -1;
    _totalNofFloatTemporaries    = -1;
    _float_section_size          = 0;
    _float_section_start_offset  = 0;

    rec = new ScopeDescriptorRecorder( CompilerScopesSize, CompilerPCsSize );
    // Save dependency information in the scopeDesc recorder.
    rec->add_dependent( key );

    nlrTestPoints = new GrowableArray<NonLocalReturnTestNode *>( 50 );
    contextList   = nullptr;
    scopes        = new GrowableArray<InlinedScope *>( 50 );
    blockClosures = new GrowableArray<BlockPseudoRegister *>( 50 );
    firstNode     = nullptr;
    reporter      = new PerformanceDebugger( this );
    initTopScope();
}


void Compiler::initLimits() {
    if ( recompileeRScope ) {
        // We're compiling from the inlining data base
        _nextLevel = MAX_RECOMPILATION_LEVELS - 1;
    } else if ( recompilee ) {
        if ( DeltaProcess::active()->isUncommon() ) {
            // when recompiling because of an uncommon trap, reset level
            _nextLevel = 0;
        } else {
            _nextLevel = recompilee->level() + 1;
            if ( _nextLevel >= MAX_RECOMPILATION_LEVELS ) {
                spdlog::warn( "recompilation level too high -- should not happen" );
                _nextLevel = MAX_RECOMPILATION_LEVELS;
            }
        }
    } else {
        // new NativeMethod
        _nextLevel = 0;
    }
    _hasInlinableSendsRemaining = true;

#if 0
    inlineLimit[ InlineLimitType::NormalFnLimit ]        = getLimit( limits[ InlineLimitType::NormalFnLimit ], level );
    inlineLimit[ InlineLimitType::BlockFnLimit ]         = getLimit( limits[ InlineLimitType::BlockFnLimit ], level );
    inlineLimit[ InlineLimitType::BlockArgFnLimit ]      = getLimit( limits[ InlineLimitType::BlockArgFnLimit ], level );
    inlineLimit[ InlineLimitType::NormalFnInstrLimit ]   = getLimit( limits[ InlineLimitType::NormalFnInstrLimit ], level );
    inlineLimit[ InlineLimitType::BlockFnInstrLimit ]    = getLimit( limits[ InlineLimitType::BlockFnInstrLimit ], level );
    inlineLimit[ InlineLimitType::BlockArgFnInstrLimit ] = getLimit( limits[ InlineLimitType::BlockArgFnInstrLimit ], level );
    inlineLimit[ InlineLimitType::SplitCostLimit ]       = getLimit( limits[ InlineLimitType::SplitCostLimit ], level );
    inlineLimit[ InlineLimitType::NmInstrLimit ]         = getLimit( limits[ InlineLimitType::NmInstrLimit ], level );

    if ( CompilerAdjustLimits ) {
        // adjust InlineLimitType::NmInstrLimit if top-level method is large
        std::int32_t cost = sicCost( (MethodKlass *) method->klass(), topScope, costP );
        if ( cost > NormalMethodLen ) {
            float l = (float) cost / NormalMethodLen * inlineLimit[ InlineLimitType::NmInstrLimit ];
            inlineLimit[ InlineLimitType::NmInstrLimit ] = min( std::int32_t( l ), CompilerInstructionsSize / 3 );
        }
    }
#endif
}


bool Compiler::registerUninlinable( Inliner *inliner ) {

    // All sends that aren't inlined for some reason are registered here
    // to determine the minimum optimization level needed for recompilation
    // (i.e. if the send wouldn't be inlined even at the highest optimization
    // level there's no point in recompiling).
    // At the end of compilation, _nextLevel will contain the lowest
    // optimization level that will generate better code than the current level.
    // Return true if the send is considered non-inlinable.


    if ( not Inline ) {
        return true;  // no point point recompiling
    }

    SendInfo *info = inliner->info();
    if ( is_database_compile() ) {
        info->_counting   = false;  //
        info->uninlinable = true;   // for now, never inline if not inlined in DB (would need to change DB format to allow counting and uninlinable sends)
    }

    if ( not UseRecompilation ) {
        // make sure we're not using counting sends
        info->_counting = false;
    }
    if ( info->uninlinable ) {
        info->_counting = false;
        return true;                // won't be inlined, ever
    }
    if ( is_uncommon_compile() ) {
        info->_counting = true;            // make sure the uncommon NativeMethod is recompiled eventually
    }
    if ( inliner->msg() == nullptr ) {
        info->_counting = true;            // for now
        _hasInlinableSendsRemaining = false;            // unknown receiver (?)
        return false;
    } else {
        st_assert( not info->_receiver->isUnknownExpression(), "oops" );
        return true;
    }
}


bool Compiler::is_uncommon_compile() const {
    return DeltaProcess::active()->isUncommon();
}


// NewBackendGuard is used only to set the right flags to enable the
// new backend (enabled via TryNewBackend) instead of setting them
// all manually. At some point all the bugs should be fixed and this
// class and its use can simply be removed.
//
// This class basically simplifies Dave's (or whoever's) life since
// only one flag (TryNewBackend) needs to be set and everything else
// is setup automatically. Eventually UseNewBackend should do the job.
//
// gri 10/2/96

class NewBackendGuard : StackAllocatedObject {
private:
    static bool _first_use;

    bool _UseNewBackend;
    bool _LocalCopyPropagate;
    bool _OptimizeLoops;
    bool _OptimizeIntegerLoops;

public:
    NewBackendGuard() :
        _UseNewBackend{ UseNewBackend },
        _LocalCopyPropagate{ LocalCopyPropagate },
        _OptimizeLoops{ OptimizeLoops },
        _OptimizeIntegerLoops{ OptimizeIntegerLoops } {

        if ( TryNewBackend ) {
            // print out a warning if this class is used
            if ( _first_use ) {
                spdlog::warn( "TryNewBackend automatically changes some flags for compilation - for temporary use only" );
                _first_use = false;
            }

            // switch to right settings
            UseNewBackend        = true;
            LocalCopyPropagate   = false;
            OptimizeLoops        = false;
            OptimizeIntegerLoops = false;
        }
    }


    ~NewBackendGuard() {
        // restore original settings in any case
        UseNewBackend        = _UseNewBackend;
        LocalCopyPropagate   = _LocalCopyPropagate;
        OptimizeLoops        = _OptimizeLoops;
        OptimizeIntegerLoops = _OptimizeIntegerLoops;
    }
};

bool NewBackendGuard::_first_use = true;


NativeMethod *Compiler::compile() {
    NewBackendGuard guard;

    if ( ( PrintProgress > 0 ) and ( compilationCount % PrintProgress == 0 ) )
        _console->print( "." );

    const char *compiling;
    if ( DeltaProcess::active()->isUncommon() ) {
        compiling = recompilee ? "Uncommon-Recompiling " : "Uncommon-Compiling ";
    } else {
        if ( _uses_inlining_database ) {
            compiling = recompilee ? "Recompiling (database)" : "Compiling (database)";
        } else {
            compiling = recompilee ? "Recompiling " : "Compiling ";
        }
    }
    EventMarker em( "%s0x{0:x} 0x{0:x}", compiling, key->selector(), nullptr );

    // don't use uncommon traps when recompiling because of trap
    useUncommonTraps = DeferUncommonBranches and not is_uncommon_compile();
    if ( is_uncommon_compile() )
        reporter->report_uncommon( false );

    if ( recompilee and recompilee->isUncommonRecompiled() )
        reporter->report_uncommon( true );

    // don't use counters when compiling from DB
    FlagSetting fs( UseRecompilation, UseRecompilation and not is_database_compile() );

    bool      should_trace = _uses_inlining_database ? PrintInliningDatabaseCompilation : PrintCompilation;
    TraceTime t( compiling, should_trace );

    if ( should_trace or PrintCode ) {
        print_key( _console );
        if ( PrintCode or PrintInlining ) {
            spdlog::info( "" );
        }
    }

    topScope->genCode();
    fixupNonLocalReturnTestPoints();
    buildBBs();

    if ( PrintCode )
        print_code( false );
    if ( verifyOften )
        bbIterator->verify();

    // compute escaping blocks and up-level accessed vars
    bbIterator->computeEscapingBlocks();
    bbIterator->computeUplevelAccesses();
    if ( verifyOften )
        bbIterator->verify();
    if ( PrintCode )
        print_code( false );

    // construct def & use information
    bbIterator->makeUses();
    if ( verifyOften )
        bbIterator->verify();
    if ( PrintCode )
        print_code( false );

    if ( LocalCopyPropagate ) {
        bbIterator->localCopyPropagate();
        if ( verifyOften )
            bbIterator->verify();
    }
    if ( PrintCode )
        print_code( false );
    if ( GlobalCopyPropagate ) {
        bbIterator->globalCopyPropagate();
        if ( verifyOften )
            bbIterator->verify();
    }
    if ( PrintCode )
        print_code( false );
    if ( BruteForcePropagate ) {
        bbIterator->bruteForceCopyPropagate();
        if ( verifyOften )
            bbIterator->verify();
    }
    if ( PrintCode )
        print_code( false );
    if ( EliminateUnneededNodes ) {
        bbIterator->eliminateUnneededResults();
        if ( verifyOften )
            bbIterator->verify();
    }
    if ( PrintCode )
        print_code( false );
    if ( OptimizeIntegerLoops ) {
        // run after copy propagation so that loop increment is easier to recognize
        // also run after eliminateUnneededResults so that cpInfo is set for eliminated PseudoRegisters
        topScope->optimizeLoops();
        if ( verifyOften )
            bbIterator->verify();
    }
    if ( PrintCode )
        print_code( false );

    // compute existence & format of run-time context objects and blocks
    computeBlockInfo();

    // allocate floats
    _totalNofFloatTemporaries = topScope->allocateFloatTemporaries( 0 );

    // HACK: Fix preallocation
    // Necessary because a few primitives (allocateContext/Closure) need self or
    // the previous context after calling a primitive; i.e., self or the previous
    // context should not be allocated to a register. Currently not working correctly
    // -> allocated to stack as a temporary fix for the problem.
    theAllocator->preAllocate( topScope->self()->pseudoRegister() );
    bbIterator->localAlloc();        // allocate regs within basic blocks
    theAllocator->allocate( bbIterator->globals );

    if ( PrintCode )
        print_code( false );
    bbIterator->verify();

    if ( PrintDebugInfoGeneration ) {
        _console->cr();
        _console->cr();
        spdlog::info( "Start of debugging info." );
    }
    topScope->generateDebugInfo();    // must come before gen to set getScopeInfo
    topScope->generateDebugInfoForNonInlinedBlocks();

    // generate machine code
    theMacroAssembler = new MacroAssembler( _code );
    if ( UseNewBackend ) {
        PseudoRegisterMapping *mapping = new PseudoRegisterMapping( theMacroAssembler, topScope->nofArguments(), 6, topScope->nofTemporaries() );
        CodeGenerator         *cgen    = new CodeGenerator( theMacroAssembler, mapping );
        cgen->initialize( topScope );
        bbIterator->apply( cgen );
        cgen->finalize( topScope );
    } else {
        // use a node visitor to generate code
        OldCodeGenerator *cgen = new OldCodeGenerator();
        bbIterator->apply( cgen );
    }
    theMacroAssembler->finalize();
    theMacroAssembler = nullptr;

    if ( verifyOften ) {
        bool ok = bbIterator->verifyLabels();
        if ( not ok )
            print_code( false );
    }

    rec->generate();            // write debugging info
    NativeMethod *nm = new_nativeMethod( this );    // construct new NativeMethod
    em.event.args[ 1 ] = nm;

    if ( PrintAssemblyCode )
        Disassembler::decode( nm );

    reporter->finish_reporting();
    if ( should_trace ) {
        spdlog::info( ": 0x{0:x} (%d bytes; level %ld v%d)", static_cast<void *>( nm ), nm->instructionsLength(), nm->level(), nm->version() );
        //flush_logFile();
    }

    if ( verifyOften )
        nm->verify();

    if ( PrintDebugInfo )
        nm->print_inlining( _console, true );

    return nm;
}


void Compiler::buildBBs() {        // build the basic block graph
    bbIterator->build( firstNode );
}


void Compiler::fixupNonLocalReturnTestPoints() {
    // the NonLocalReturnTest nodes didn't get their correct successors during node generation because
    // their sender scopes' nlrTestPoints may not yet have been created; fix them up now
    std::int32_t i = nlrTestPoints->length();
    while ( i-- > 0 )
        nlrTestPoints->at( i )->fixup();
}


void Compiler::computeBlockInfo() {
    FlagSetting( EliminateUnneededNodes, true );  // unused context nodes must be eliminated
    GrowableArray<InlinedScope *> *allContexts = new GrowableArray<InlinedScope *>( 25 );
    topScope->collectContextInfo( allContexts );
    // for now, just allocate all contexts as in interpreter
    // fix this later: collect all uplevel-accessed PseudoRegisters at same loop depth, form physical
    // contexts for these
    // also, if uplevel-read and single def --> could copy into context and keep
    // stack/register copy


    // remove all unused contexts
    // need to iterate because removing a nested context may enable removal of a parent context
    // (could avoid iteration with topo sort, but there are few contexts anyway)
    bool changed = EliminateContexts;
    while ( changed ) {
        changed              = false;
        for ( std::int32_t i = allContexts->length() - 1; i >= 0; i-- ) {
            InlinedScope *s = allContexts->at( i );
            if ( s == nullptr )
                continue;
            PseudoRegister *contextPR = s->context();
            st_assert( contextPR->isSinglyAssigned(), "should have exactly one def" );

            GrowableArray<Expression *> *temps = s->contextTemporaries();

            bool noUplevelAccesses = true;

            // check if all context temps can be stack-allocated
            for ( std::int32_t j = temps->length() - 1; j >= 0; j-- ) {

                PseudoRegister *r = temps->at( j )->pseudoRegister();

                if ( r->uplevelR() or r->uplevelW() ) { // this temp is still uplevel-accessed, so can't eliminate context
                    noUplevelAccesses = false;
                    break;
                }

                if ( r->isBlockPseudoRegister() and not r->isUnused() ) { // this block still forces a context
                    noUplevelAccesses = false;
                    break;
                }

            }

            // TO DO: check if context is needed for NonLocalReturns (noUplevelAccesses alone does not allow elimination)
            static_cast<void>( noUplevelAccesses );
//            if ( noUplevelAccesses or contextPR->isSinglyUsed() ) {
            if ( contextPR->isSinglyUsed() ) {
                // can eliminate context -- no uplevel-accessed vars (single use is context initializer)
                if ( CompilerDebug )
                    cout( PrintEliminateContexts )->print( "%*s*eliminating context %s\n", s->depth, "", contextPR->safeName() );

                contextPR->scope()->gen()->removeContextCreation();
                allContexts->at_put( i, nullptr );      // make code generator break if it tries to access this context
                changed = true;
            }
        }
    }

    // now collect all remaining contexts
    std::int32_t i = allContexts->length();
    contextList = new GrowableArray<InlinedScope *>( i, i, nullptr );
    while ( i-- > 0 ) {
        // should merge several contexts into one physical context if possible
        // fix this later
        InlinedScope *s = allContexts->at( i );
        if ( s == nullptr )
            continue;

        PseudoRegister *contextPR = s->context();
        if ( CompilerDebug ) {
            cout( PrintEliminateContexts )->print( "%*s*could not eliminate context %s in scope %s\n", s->depth, "", contextPR->safeName(), s->key()->toString() );
        }

        reporter->report_context( s );
        contextList->at_put( i, s );
        ContextCreateNode *c = s->contextInitializer()->creator();
        c->set_contextNo( i );
        GrowableArray<Expression *> *temps = s->contextTemporaries();

        // allocate the temps in this context (but only if they're used)
        std::int32_t ntemps = temps->length();
        std::int32_t size   = 0;

        for ( std::int32_t j = 0; j < ntemps; j++ ) {
            PseudoRegister *p = temps->at( j )->pseudoRegister();

            // should be:
            //     if (p->isUsed() and (p->uplevelR() or p->uplevelW())) {
            // but doesn't work yet (probably must fix set_self_via_context etc.)
            // -Urs 6/96

            if ( p->isUsed() ) {
                // allocate p to context temp
                st_assert( p->scope() == s or p->isBlockPseudoRegister(), "oops" );
                Location loc = Mapping::contextTemporary( i, size, s->scopeID() );
                if ( p->isBlockPseudoRegister() ) {
                    // Blocks aren't actually assigned (at the PseudoRegister level) so that the inlining info isn't lost.
                    // Thus we need to create a fake destination here if the context exists.
                    SinglyAssignedPseudoRegister *dest = new SinglyAssignedPseudoRegister( s, loc, true, true, PrologueByteCodeIndex, EpilogueByteCodeIndex );
                    Expression                   *e    = new UnknownExpression( dest, nullptr );
                    //contextPR->scope()->contextInitializer()->initialize(j, init);
                    temps->at_put( j, e );
                } else {
                    p->allocateTo( loc );
                }
                size++;
            }
        }
        c->set_sizeOfContext( size );
        if ( size < ntemps and c->scope()->number_of_noninlined_blocks() > 0 ) {
            // this hasn't been exercised much
            compiler_warning( "while compiling %s: eliminated some context temps", key->toString() );
        }
    }

    // Compute the number of noninlined blocks for the NativeMethod and allocate
    const std::int32_t nblocks = topScope->number_of_noninlined_blocks();

    if ( is_method_compile() or nblocks > 0 ) {
        // allocate nblocks+1 JumpTable entries
        const JumpTableID id = Universe::code->jump_table()->allocate( nblocks + 1 );

        if ( is_method_compile() ) {
            main_jumpTable_id = id;
        } else {
            promoted_jumpTable_id = id;
        }

        // first is for NativeMethod itself
        std::int32_t block_index = 1;

        for ( std::int32_t i = bbIterator->exposedBlks->length() - 1; i >= 0; i-- ) {

            BlockPseudoRegister *blk = bbIterator->exposedBlks->at( i );
            if ( blk->isUsed() ) {
                st_assert( block_index <= nblocks, "nblocks too small" );
                blk->closure()->set_id( id.sub( block_index++ ) );
            }
        }
        st_assert( nblocks + 1 == block_index, "just checking" );
    }
}


void Compiler::initTopScope() {
    if ( recompileeRScope == nullptr ) {
        if ( TypeFeedback ) {
            recompileeRScope = recompilee ? (RecompilationScope *) NonDummyRecompilationScope::constructRScopes( recompilee ) : (RecompilationScope *) new InterpretedRecompilationScope( nullptr, -1, key, method, 0, true );
        } else {
            recompileeRScope = new NullRecompilationScope;
        }
    }
    if ( PrintRScopes )
        recompileeRScope->printTree( 0, 0 );

    countID = Universe::code->nextNativeMethodID();
    Scope        *parentScope = nullptr;
    SendInfo     *info        = new SendInfo( nullptr, key, nullptr );
    InlinedScope *sender      = nullptr;    // no sender -- top scope in NativeMethod

    if ( is_block_compile() ) {
        // block method
        st_assert( parentNativeMethod not_eq nullptr, "parentNativeMethod must be set for block compile" );
        st_assert( blockScope->parent() not_eq nullptr, "must know parent" );
        parentScope = new_OutlinedScope( parentNativeMethod, blockScope->parent() );
        topScope    = BlockScope::new_BlockScope( method, parentScope->methodHolder(), parentScope, sender, recompileeRScope, info );
    } else {
        // normal method
        KlassOop methodHolder = key->klass()->klass_part()->lookup_method_holder_for( method );
        topScope = MethodScope::new_MethodScope( method, methodHolder, sender, recompileeRScope, info );
    }
    // make sure home exists always
    st_assert( topScope->home() not_eq nullptr, "no home" );
}


void Compiler::print() {
    print_short();
    spdlog::info( ":" );
    key->print();
    spdlog::info( "\tmethod: %s", method->toString() );
    spdlog::info( "\tp ((Compiler*)0x{0:x})->print_code()", static_cast<void *>( this ) );
}


void Compiler::print_short() {
    spdlog::info( "(Compiler*) 0x{0:x}", static_cast<void *>( this ) );
}


void Compiler::print_key( ConsoleOutputStream *stream ) {
    key->print_on( stream );
    if ( topScope == nullptr )
        return; // print_key may be used during fatals where the compiler isn't set up yet

    stream->print( " (no. %d, method 0x{0:x}", compilationCount, method );
    // print the parent scope offset for block compiles.
    if ( blockScope ) {
        _console->print( ", parent offset %d", blockScope->parent()->offset() );
    }
    stream->print( ")..." );
}


void Compiler::print_code( bool suppressTrivial ) {
    if ( theCompiler == nullptr ) {
        // This will not work, another indication that firstNode should be stored with the BasicBlockIterator
        // anyway, not fixed for now (gri 6/6/96)
        last_bbIterator->print_code( suppressTrivial );
        last_bbIterator->print();
    } else {
        bool hadBBs = bbIterator not_eq nullptr;
        if ( not hadBBs ) {
            // need BBs for printing
            bbIterator = new BasicBlockIterator;
            buildBBs();
        }
        bbIterator->print_code( suppressTrivial );
        bbIterator->print();
        if ( not hadBBs ) {
            bbIterator->clear();
            bbIterator = nullptr;
        }
    }
    spdlog::info( "" );
}


std::int32_t Compiler::get_invocation_counter_limit() const {
    if ( is_uncommon_compile() ) {
        return RecompilationPolicy::uncommonNativeMethodInvocationLimit( version() );
    } else {
        return Interpreter::get_invocation_counter_limit();
    }
}


void Compiler::set_special_handler_call_offset( std::int32_t offset ) {
    // doesn't need to be aligned since called rarely and from within the NativeMethod only
    _special_handler_call_offset = offset;
}


void Compiler::set_entry_point_offset( std::int32_t offset ) {
    st_assert( offset % OOP_SIZE == 0, "entry point must be aligned" );
    _entry_point_offset = offset;
}


void Compiler::set_verified_entry_point_offset( std::int32_t offset ) {
    st_assert( offset % OOP_SIZE == 0, "verified entry point must be aligned" );
    _verified_entry_point_offset = offset;
}


void Compiler::set_float_section_size( std::int32_t size ) {
    st_assert( size >= 0, "size cannot be negative" );
    _float_section_size = size;
}


void Compiler::set_float_section_start_offset( std::int32_t offset ) {
    _float_section_start_offset = offset;
}


std::int32_t Compiler::number_of_noninlined_blocks() const {
    return topScope->number_of_noninlined_blocks();
}


void Compiler::copy_noninlined_block_info( NativeMethod *nm ) {
    topScope->copy_noninlined_block_info( nm );
}


ConsoleOutputStream *cout( bool flag ) {
    return ( flag or theCompiler == nullptr ) ? _console : theCompiler->messages;
}


void print_cout() {
    ResourceMark resourceMark;
    lputs( theCompiler->messages->as_string() );
}
