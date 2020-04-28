
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/Scope.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/assembler/x86_mapping.hpp"

#include <cstring>


smi_t Scope::_currentScopeID;

// SendInfo implementation

SendInfo::SendInfo( InlinedScope * senderScope, LookupKey * lookupKey, Expression * receiver ) {
    _senderScope = senderScope;
    _receiver    = receiver;
    _selector    = lookupKey->selector();
    _lookupKey   = lookupKey;
    init();
}


void SendInfo::computeNSends( RecompilationScope * rscope, int byteCodeIndex ) {
    GrowableArray <RecompilationScope *> * lst = rscope->subScopes( byteCodeIndex );
    _sendCount = 0;
    for ( int i = lst->length() - 1; i >= 0; i-- ) {
        _sendCount += lst->at( i )->_invocationCount;
    }
}


void SendInfo::init() {
    _resultRegister     = nullptr;
    _needRealSend       = false;
    _counting           = false;
    _sendCount          = -1;
    _receiverStatic     = false;
    _predicted          = false;
    uninlinable         = false;
    _inPrimitiveFailure = _senderScope and _senderScope->gen()->in_primitive_failure_block();
}



// Scopes
// NB: constructors are protected to avoid stupid "call-of-virtual-in-constructor" bugs

InlinedScope::InlinedScope() {
}


void InlinedScope::initialize( MethodOop method, KlassOop methodHolder, InlinedScope * sender, RecompilationScope * rs, SendInfo * info ) {
    _scopeID = currentScopeID();
    theCompiler->scopes->append( this );
    st_assert( theCompiler->scopes->at( _scopeID ) == this, "bad list" );
    _sender    = sender;
    _scopeInfo = nullptr;
    if ( sender ) {
        _senderByteCodeIndex = sender->byteCodeIndex();
        sender->addSubScope( this );
        depth     = _sender->depth + 1;
        loopDepth = _sender->loopDepth;
    } else {
        _senderByteCodeIndex = IllegalByteCodeIndex;
        depth                = loopDepth = 0;
    }
    result     = nullptr;
    nlrResult  = nullptr;    // these are set during compilation
    if ( info and info->_resultRegister ) {
        resultPR = info->_resultRegister;
    } else {
        // potential bug: live range of resultPR is bogus
        st_assert( isTop(), "should have resReg for inlined scope" );
        resultPR = new SinglyAssignedPseudoRegister( this, resultLoc, false, false, PrologueByteCodeIndex, EpilogueByteCodeIndex );
    }
    rscope     = rs;
    rs->extend();

    predicted = info ? info->_predicted : false;

    st_assert( info->_lookupKey->klass(), "must have klass" );
    _key                = info->_lookupKey;
    _method             = method;
    _methodHolder       = methodHolder;    // NB: can be nullptr if method is in Object
    _nofSends           = 0;
    _nofInterruptPoints = 0;
    _primFailure        = sender ? sender->_primFailure : false;
    _endsDead           = false;
    _self               = nullptr;        // initialized by createTemps or by sender scope
    _gen.initialize( this );

    _temporaries        = nullptr;        // allocated by createTemporaries
    _floatTemporaries   = nullptr;        // allocated by createFloatTemporaries
    _contextTemporaries = nullptr;        // allocated by createContextTemporaries
    _context            = nullptr;        // set for blocks and used/set by createContextTemporaries
    _exprStackElems     = new GrowableArray <Expression *>( nofBytes() );
    _subScopes          = new GrowableArray <InlinedScope *>( 5 );
    _loops              = new GrowableArray <CompiledLoop *>( 5 );
    _typeTests          = new GrowableArray <NonTrivialNode *>( 10 );

    _pregsBegSorted   = new GrowableArray <PseudoRegister *>( 5 );
    _pregsEndSorted   = new GrowableArray <PseudoRegister *>( 5 );
    _firstFloatIndex  = -1;        // set during float allocation
    _hasBeenGenerated = false;

    theCompiler->nofBytesCompiled( nofBytes() );
    if ( not rs->isNullScope() and rs->method() not_eq method ) {
        // wrong rscope (could happen after programming change)
        rs = new NullRecompilationScope;
    }
}


bool_t InlinedScope::isLite() const {
    // A scope is light (doesn't need its locals/expression stack described) if it has no interrupt
    // points, i.e., if the program can never stop while the PC is in this scope.
    // The top scope of an NativeMethod can't be light (at least the receiver is needed).
    return GenerateLiteScopeDescs and ( sender() not_eq nullptr ) and ( _nofInterruptPoints == 0 );
}


void MethodScope::initialize( MethodOop method, KlassOop methodHolder, InlinedScope * sen, RecompilationScope * rs, SendInfo * info ) {
    InlinedScope::initialize( method, methodHolder, sen, rs, info );
}


MethodScope::MethodScope() {
}


BlockScope::BlockScope() {
}


void BlockScope::initialize( MethodOop method, KlassOop methodHolder, Scope * p, InlinedScope * s, RecompilationScope * rs, SendInfo * info ) {
    InlinedScope::initialize( method, methodHolder, s, rs, info );
    _parent              = p;
    _self_is_initialized = false;
    if ( s == nullptr ) {
        // top scope: create a context (currently always initialized for blocks)
        // (context is set up by the prologue node)
        _context = new SinglyAssignedPseudoRegister( this, PrologueByteCodeIndex, EpilogueByteCodeIndex );
    } else {
        // set up for context passed in by caller
        // (_context may be changed later if this scope allocates its own context)
        switch ( method->block_info() ) {
            case MethodOopDescriptor::expects_nil:        // no context needed
                _context = nullptr;
                break;
            case MethodOopDescriptor::expects_self:
                _context = self()->preg();
                fatal( "self not known yet -- fix this" );
                break;
            case MethodOopDescriptor::expects_parameter:    // fix this -- should find which
            Unimplemented();
                break;
            case MethodOopDescriptor::expects_context:
                if ( p->isInlinedScope() ) {
                    _context = ( ( InlinedScope * ) p )->context();
                } else {
                    fatal( "shouldn't inline" );    // shouldn't inline block unless parent was inlined, too
                }
                break;
            default: fatal( "unexpected incoming info" );
        }
    }
}


MethodScope * MethodScope::new_MethodScope( MethodOop method, KlassOop methodHolder, InlinedScope * sen, RecompilationScope * rs, SendInfo * info ) {
    MethodScope * new_scope = new MethodScope;
    new_scope->initialize( method, methodHolder, sen, rs, info );
    new_scope->initializeArguments();
    return new_scope;
}


BlockScope * BlockScope::new_BlockScope( MethodOop method, KlassOop methodHolder, Scope * p, InlinedScope * s, RecompilationScope * rs, SendInfo * info ) {
    BlockScope * new_scope = new BlockScope;
    new_scope->initialize( method, methodHolder, p, s, rs, info );
    new_scope->initializeArguments();
    return new_scope;
}


void InlinedScope::addSubScope( InlinedScope * s ) {
    // assert(_subScopes->isEmpty() or _subScopes->last()->senderByteCodeIndex() <= s->senderByteCodeIndex(),
    //	 "scopes not ordered by byteCodeIndex");
    // NB: subScopes are not in byteCodeIndex order when generating while loops -- condition is generated
    // first but has higher byteCodeIndexs than body.  Ugh.   -Urs 10/95
    _subScopes->append( s );
}


void InlinedScope::subScopesDo( Closure <InlinedScope *> * f ) {
    f->do_it( this );
    _subScopes->apply( f );
}


MergeNode * InlinedScope::nlrTestPoint() {
    /* Generate code handling NonLocalReturns coming from below.
       There are several factors influencing what happens:

       1. NonLocalReturn-Target:  A scope is a potential NonLocalReturn target if the scope is a method scope and
          there is at least one block with a NonLocalReturn in the scope.
      NonLocalReturn targets must test the target of the current NonLocalReturn and do a normal method return
      if the NonLocalReturn comes from one of their blocks.

       2. Context: does this scope have a context object?  If so, it must be zapped before
          going further (whether the scope was the NonLocalReturn-target or not).

       3. Top scope:  If the scope is the top scope of an NativeMethod, it must continue with a
          MethodReturn or NonLocalReturneturn rather than an InlinedMethodReturn/InlinedNonLocalReturneturn.

       Unfortunately, at this point in the compilation we can't answer questions 1 and 2 yet,
       because we don't know yet which blocks (and context objects) have been optimized away.
       (We could know, if we kept track of things during the node generation.  However, even
       then we'd be wrong sometimes because some optimizations can eliminate blocks later.)
       On the other hand, we have to generate *something* here to correctly model the control
       flow.  Thus we insert a NonLocalReturnTestNode now and defer the exact decision about what it
       does to the machine code generation phase.
    */
    if ( not _nlrTestPoint ) {
        int end_byteCodeIndex = nofBytes();
        // Note: src has incorrect def because it is implicitly assigned to via a call
        // (in sender scopes (when inlining), there will be an assignment when fixing up the connections).
        //SinglyAssignedPseudoRegister* src = new SinglyAssignedPseudoRegister(this, NonLocalReturnResultLoc, false, true, end_byteCodeIndex, end_byteCodeIndex);
        //SinglyAssignedPseudoRegister* dst = new SinglyAssignedPseudoRegister(this, NonLocalReturnResultLoc, false, false, end_byteCodeIndex, end_byteCodeIndex);
        _nlrTestPoint = NodeFactory::MergeNode( end_byteCodeIndex );
        _nlrTestPoint->append( NodeFactory::NonLocalReturnTestNode( end_byteCodeIndex ) );
    }
    return _nlrTestPoint;
}


void InlinedScope::addResult( Expression * e ) {
    // e is a possible return value; add it to our return expression to keep track of all possible return types
    st_assert( e->preg() == resultPR or resultPR == nullptr or e->isNoResultExpression(), "bad result PseudoRegister" );
    if ( result == nullptr ) {
        result = e;
    } else {
        result = result->mergeWith( e, result->node() );
    }
}


void InlinedScope::initializeArguments() {
    const int nofArgs = _method->number_of_arguments();
    _arguments = new GrowableArray <Expression *>( nofArgs, nofArgs, nullptr );
    if ( isTop() ) {
        // create expr for self but do not allocate a location yet
        // (self is setup by the prologue node)
        _self = new KlassExpression( KlassOop( selfKlass() ), new SinglyAssignedPseudoRegister( this, unAllocated, false, false, PrologueByteCodeIndex, EpilogueByteCodeIndex ), nullptr );
        // preallocate incoming arguments, i.e., create their expressions
        // using SAPRegs that are already allocated
        for ( int i = 0; i < nofArgs; i++ ) {
            SinglyAssignedPseudoRegister * arg = new SinglyAssignedPseudoRegister( this, Mapping::incomingArg( i, nofArgs ), false, false, PrologueByteCodeIndex, EpilogueByteCodeIndex );
            _arguments->at_put( i, new UnknownExpression( arg ) );
        }
    } else {
        _self = nullptr;    // will be initialized by sender
        // get args from sender's expression stack; top of expr stack = last arg, etc.
        const int top = sender()->exprStack()->length();
        for ( int i   = 0; i < nofArgs; i++ ) {
            _arguments->at_put( i, sender()->exprStack()->at( top - nofArgs + i ) );
        }
    }
}


void BlockScope::initializeSelf() {
    // for non-inlined top-level scope, make sure self is initialized
    // NB: bytecode compiler generates set_self_via_context, but only if self is needed in this
    // particular block and block expects a context (e.g., can have nested blocks with
    // set_self_via_context but not have one here).
    // Thus, load self in top-level block and ignore all nested set_self_via_context bytecodes.
    if ( _parent->isInlinedScope() and method()->hasNestedBlocks() ) {
        _gen.set_self_via_context();
    }
}


void InlinedScope::createTemporaries( int nofTemps ) {
    // add nofTemps temporaries (may be called multiple times)
    int firstNew;
    if ( not hasTemporaries() ) {
        // first time we're called
        _temporaries = new GrowableArray <Expression *>( nofTemps, nofTemps, nullptr );
        // The canonical model has the context in the first temporary.
        // To preserve this model the first temporary is aliased to _context.
        // Lars, 3/8/96
        if ( _context ) {
            _temporaries->at_put( 0, new ContextExpression( _context ) );
            firstNew = 1;
        } else {
            firstNew = 0;
        }
    } else {
        // grow existing temp array
        const GrowableArray <Expression *> * oldTemps = _temporaries;
        const int                          n          = nofTemps + oldTemps->length();
        _temporaries = new GrowableArray <Expression *>( n, n, nullptr );
        firstNew     = oldTemps->length();
        nofTemps += oldTemps->length();
        for ( int i = 0; i < firstNew; i++ )
            _temporaries->at_put( i, oldTemps->at( i ) );
    }
    // initialize new temps
    ConstPseudoRegister * nil = new_ConstPReg( this, nilObj );
    for ( int i = firstNew; i < nofTemps; i++ ) {
        PseudoRegister * r = new PseudoRegister( this );
        _temporaries->at_put( i, new UnknownExpression( r, nullptr ) );
        if ( isTop() ) {
            // temps are initialized by PrologueNode
        } else {
            gen()->append( NodeFactory::AssignNode( nil, r ) );
        }
    }
}


void InlinedScope::createFloatTemporaries( int nofFloats ) {
    st_assert( not hasFloatTemporaries(), "cannot be called twice" );
    _floatTemporaries = new GrowableArray <Expression *>( nofFloats, nofFloats, nullptr );
    // initialize float temps
    for ( int i = 0; i < nofFloats; i++ ) {
        PseudoRegister * preg = new PseudoRegister( this, Location::floatLocation( scopeID(), i ), false, false );
        _floatTemporaries->at_put( i, new UnknownExpression( preg, nullptr ) );
        if ( isTop() ) {
            // floats are initialized by PrologueNode
        } else {
            warning( "float initialization of floats in inlined scopes not implemented yet" );
        }
    }
}


void InlinedScope::createContextTemporaries( int nofTemps ) {
    // create _contextTemporaries and initialize all elements immediately;
    // after creation, some of the elements may be overwritten
    // (e.g., copying self or a method argument to the context)
    st_assert( _contextTemporaries == nullptr, "more than one context created" );
    st_assert( allocatesInterpretedContext(), "inconsistent context info" );
    _contextTemporaries = new GrowableArray <Expression *>( nofTemps, nofTemps, nullptr );
    for ( int i = 0; i < nofTemps; i++ ) {
        PseudoRegister * r = new PseudoRegister( this );
        _contextTemporaries->at_put( i, new UnknownExpression( r, nullptr ) );
    }
    // create context if not there yet
    if ( _context == nullptr ) {
        // assert(isMethodScope(), "BlockScope should have correct context already");
        // replaced old assertion with the one below. Since assert disappears in the
        // fast version, put in a warning so that we can look at this if it happens
        // again (couldn't re-create the situation yet) - gri 5/10/96
        st_assert( isMethodScope() or isBlockScope() and method()->block_info() == MethodOopDescriptor::expects_nil, "check this" );
        //if (isBlockScope()) warning("possibly a bug in InlinedScope::createContextTemporaries - tell Robert");
        _context = new SinglyAssignedPseudoRegister( this, PrologueByteCodeIndex, EpilogueByteCodeIndex );
    }
    // The canonical model has the context in the first temporary.
    // To preserve this model the first temporary is aliased to _context.
    // Lars, 10/9/95
    _temporaries->at_put( 0, new ContextExpression( _context ) );
}


void InlinedScope::contextTemporariesAtPut( int no, Expression * e ) {
    st_assert( not e->preg()->isSAPReg() or e->preg()->isBlockPReg() or ( ( SinglyAssignedPseudoRegister * ) e->preg() )->isInContext(), "not in context" );
    _contextTemporaries->at_put( no, e );
}


bool_t InlinedScope::allocatesCompiledContext() const {
    if ( not allocatesInterpretedContext() )
        return false;
    ContextCreateNode * c = _contextInitializer->creator();
    if ( bbIterator->_usesBuilt and c->_deleted ) {
        // logically has a context, but it has been optimized away
        return false;
    } else {
        return true;
    }
}


void InlinedScope::prologue() {
    theCompiler->enterScope( this );
    initializeSelf();
}


static int compare_scopeByteCodeIndexs( InlinedScope ** a, InlinedScope ** b ) {
    // put unused scopes at the end so they can be deleted
    if ( ( *a )->hasBeenGenerated() == ( *b )->hasBeenGenerated() ) {
        return ( *a )->senderByteCodeIndex() - ( *b )->senderByteCodeIndex();
    } else {
        return ( *a )->hasBeenGenerated() ? -1 : 1;
    }
}


void InlinedScope::epilogue() {
    // generate epilogue code (i.e., everything after last byte code has been processed)
    st_assert( exprStack()->isEmpty(), "expr. stack should be empty now" );

    // first make sure subScopes are sorted by byteCodeIndex
    _subScopes->sort( compare_scopeByteCodeIndexs );

    // now remove all subscopes that were created but not used (not inlined)
    while ( not _subScopes->isEmpty() and not _subScopes->last()->hasBeenGenerated() )
        _subScopes->pop();
    for ( int i = 0; i < _subScopes->length(); i++ ) {
        if ( not _subScopes->at( i )->hasBeenGenerated() ) fatal( "unused scopes should be at end" );
    }

    if ( _nofSends > 0 and containsNonLocalReturn() ) {
        // this scope *could* be the target of a non-inlined NonLocalReturn; add an UnknownExpression to
        // the scope's result expression to account for this possibility
        // note: to be sure, we'd have to know that at least one nested block wasn't inlined,
        // but this analysis isn't performed until later
        addResult( new UnknownExpression( resultPR, nullptr ) );
        // also make sure we have an NonLocalReturn test point to catch the NonLocalReturn
        ( void ) nlrTestPoint();
        st_assert( has_nlrTestPoint(), "should have a NonLocalReturn test point now" );
    }

    // generate NonLocalReturn code if needed
    if ( has_nlrTestPoint() ) {
        // NB: assertion below is too strict -- may have an nlr node that will be connected
        // only during fixupNonLocalReturnPoints()
        // assert(nlrTestPoint()->nPredecessors() > 0, "nlr node is unused??");
    } else if ( isTop() and theCompiler->nlrTestPoints->nonEmpty() ) {
        // the top scope doesn't have an NonLocalReturn point, but needs one anyway so that inlined
        // scopes have somewhere to jump to
        ( void ) nlrTestPoint();
    }
    if ( not result )
        result = new NoResultExpression;
    theCompiler->exitScope( this );
}


bool_t InlinedScope::isSenderOf( InlinedScope * callee ) const {
    st_assert( callee, "should have a scope" );
    if ( depth > callee->depth )
        return false;
    int d = callee->depth - 1;
    for ( InlinedScope * s = callee->sender(); s and d >= depth; s = s->sender(), d-- ) {
        if ( this == s )
            return true;
    }
    return false;
}


void InlinedScope::addSend( GrowableArray <PseudoRegister *> * expStk, bool_t isSend ) {
    // add send or prim. call / uncommon branch to this scope and mark locals as debug-visible
    if ( not expStk )
        return;            // not an exposing send
    for ( InlinedScope * s = this; s and s->isInlinedScope(); s = s->sender() ) {
        if ( isSend )
            s->_nofSends++;
        s->_nofInterruptPoints++;
        s->markLocalsDebugVisible( expStk );    // mark locals as debug-visible
    }
}


void InlinedScope::markLocalsDebugVisible( GrowableArray <PseudoRegister *> * exprStack ) {
    // this scope has at least one send - mark params & locals as debug-visible
    int       i;
    if ( _nofSends <= 1 ) {
        // first time we're called
        self()->preg()->_debug            = true;
        for ( int                    i    = nofArguments() - 1; i >= 0; i-- ) {
            argument( i )->preg()->_debug = true;
        }
        for ( int                    i    = nofTemporaries() - 1; i >= 0; i-- ) {
            temporary( i )->preg()->_debug = true;
        }
        // if there's a context, mark all context variables as debug-visible too.
        GrowableArray <Expression *> * ct = contextTemporaries();
        if ( ct not_eq nullptr ) {
            for ( int i = 0; i < ct->length(); i++ ) {
                ct->at( i )->preg()->_debug = true;
            }
        }
    }
    // also mark expression stack as debug-visible (excluding arguments to
    // current send) (the args are already excluded from the CallNode's
    // expression stack, so just use that one instead of this->exprStack)
    for ( int i = 0; i < exprStack->length(); i++ ) {
        exprStack->at( i )->_debug = true;
    }
}


void InlinedScope::setExprForByteCodeIndex( int byteCodeIndex, Expression * expr ) {
    st_assert( _exprStackElems->at_grow( byteCodeIndex ) == nullptr, "only one expr per ByteCodeIndex allowed" );
    _exprStackElems->at_put_grow( byteCodeIndex, expr );
}


void InlinedScope::set2ndExprForByteCodeIndex( int byteCodeIndex, Expression * expr ) {
    _exprStackElems->at_put_grow( byteCodeIndex, expr );
}


void InlinedScope::set_self( Expression * e ) {
    st_assert( not _self, "self already set" );
    st_assert( e->scope()->isSenderOrSame( this ), "must be in sender scope" );
    _self = e;
}


int InlinedScope::homeContext() const {
    // count the number of logical (i.e. interpreter) contexts from here up to the home method
    // contexts are numbered starting with zero and there is at least one context
    int       context = -1;
    MethodOop method  = _method;
    while ( method not_eq nullptr ) {
        if ( method->allocatesInterpretedContext() ) {
            context++;
        }
        method = method->parent();
    }
    st_assert( context >= 0, "there must be at least one context" );
    return context;
}


InlinedScope * InlinedScope::find_scope( int c, int & nofIndirections, OutlinedScope *& out ) {
    // return the InlinedScope that contains context c
    // IN : context no. c for this scope (in interpreter terms)
    // OUT: number of indirections required at run time (-1 = in same stack frame,
    //      0 = in context of this frame, 1 = in parent context of this frame's context, etc.)
    // if the inlined scope is found (nofIndirections = -1) it is returned as the result
    // if the inlined scope is not found (nofIndirections >= 0), the highest possible scope
    // is returned and out is set to the outlined scope containing the context
    st_assert( c >= 0, "context must be >= 0" );
    int distance = _method->lexicalDistance( c );
    nofIndirections = -1;
    Scope * s = this;
    out = nullptr;
    // first, go up as far as possible
    int d = distance;
    for ( ; d > 0 and s->parent()->isInlinedScope(); d--, s = s->parent() );
    if ( d == 0 ) {
        // found scope in our NativeMethod
        return ( InlinedScope * ) s;
    }

    // InlinedScope not found; go up the rest of the scopes and count how many
    // stack frames are traversed
    InlinedScope * top = ( InlinedScope * ) s;
    if ( top->allocatesCompiledContext() )
        nofIndirections++;
    Scope * prev = s;
    for ( s = s->parent(); d > 0; d--, prev = s, s = s->parent() ) {
        if ( s->allocatesCompiledContext() )
            nofIndirections++;
    }
    st_assert( prev->isOutlinedScope(), "must be outlined scope" );
    out = ( OutlinedScope * ) prev;
    st_assert( nofIndirections >= 0, "must have at least one context access" );
    return top;
}


void InlinedScope::collectContextInfo( GrowableArray <InlinedScope *> * contextList ) {
    // collect all scopes with contexts
    if ( allocatesInterpretedContext() )
        contextList->append( this );
    for ( int i = _subScopes->length() - 1; i >= 0; i-- ) {
        _subScopes->at( i )->collectContextInfo( contextList );
    }
}


int InlinedScope::number_of_noninlined_blocks() {
    // return the number of non-inlined blocks in this scope or its callees
    int       nblocks = 0;
    for ( int i       = bbIterator->exposedBlks->length() - 1; i >= 0; i-- ) {
        BlockPseudoRegister * blk = bbIterator->exposedBlks->at( i );
        if ( blk->isUsed() and isSenderOrSame( blk->scope() ) )
            nblocks++;
    }
    return nblocks;
}


void InlinedScope::generateDebugInfoForNonInlinedBlocks() {
    for ( int i = bbIterator->exposedBlks->length() - 1; i >= 0; i-- ) {
        BlockPseudoRegister * blk = bbIterator->exposedBlks->at( i );
        if ( blk->isUsed() )
            blk->closure()->generateDebugInfo();
    }
}


void InlinedScope::copy_noninlined_block_info( NativeMethod * nm ) {
    for ( int i = bbIterator->exposedBlks->length() - 1; i >= 0; i-- ) {
        BlockPseudoRegister * blk = bbIterator->exposedBlks->at( i );
        if ( blk->isUsed() ) {
            int offset = theCompiler->scopeDescRecorder()->offset_for_noninlined_scope_node( blk->closure()->noninlined_block_scope() );
            nm->noninlined_block_at_put( blk->closure()->id().minor(), offset );
        }
    }
}


// loop optimizations

void InlinedScope::addTypeTest( NonTrivialNode * t ) {
    st_assert( t->doesTypeTests(), "shouldn't add" );
    _typeTests->append( t );
}


CompiledLoop * InlinedScope::addLoop() {
    CompiledLoop * l = new CompiledLoop;
    _loops->append( l );
    return l;
}


void InlinedScope::optimizeLoops() {
    for ( int i = _loops->length() - 1; i >= 0; i-- ) {

        CompiledLoop * loop = _loops->at( i );
        const char   * msg  = loop->recognize();

        if ( msg ) {
            cout( PrintLoopOpts )->print( "*loop %d in scope %s not an integer loop: %s\n", i, key()->print_string(), msg );
        } else {
            cout( PrintLoopOpts )->print( "*optimizing integer loop %d in scope %s\n", i, key()->print_string() );
            loop->optimizeIntegerLoop();
        }
        if ( OptimizeLoops )
            loop->optimize();
    }
    for ( int i = _subScopes->length() - 1; i >= 0; i-- ) {
        _subScopes->at( i )->optimizeLoops();
    }
}



// register allocation

void InlinedScope::addToPRegsBegSorted( PseudoRegister * r ) {
    st_assert( PrologueByteCodeIndex <= r->begByteCodeIndex() and r->begByteCodeIndex() <= EpilogueByteCodeIndex, "illegal byteCodeIndex" );
    st_assert( _pregsBegSorted->isEmpty() or _pregsBegSorted->last()->begByteCodeIndex() <= r->begByteCodeIndex(), "sort order wrong" );
    _pregsBegSorted->append( r );
}


void InlinedScope::addToPRegsEndSorted( PseudoRegister * r ) {
    st_assert( PrologueByteCodeIndex <= r->endByteCodeIndex() and r->endByteCodeIndex() <= EpilogueByteCodeIndex, "illegal byteCodeIndex" );
    st_assert( _pregsEndSorted->isEmpty() or _pregsEndSorted->last()->endByteCodeIndex() <= r->endByteCodeIndex(), "sort order wrong" );
    _pregsEndSorted->append( r );
}


void InlinedScope::allocatePRegs( IntFreeList * f ) {
    int byteCodeIndex = PrologueByteCodeIndex;
    int bi            = 0;
    int si            = 0;
    int ei            = 0;
    int blen          = _pregsBegSorted->length();
    int slen          = _subScopes->length();
    int elen          = _pregsEndSorted->length();
    int n             = f->allocated();
    st_assert( blen == elen, "should be the same" );
    while ( byteCodeIndex <= EpilogueByteCodeIndex ) {
        // allocate registers that begin at byteCodeIndex
        while ( bi < blen and _pregsBegSorted->at( bi )->begByteCodeIndex() == byteCodeIndex ) {
            _pregsBegSorted->at( bi )->allocateTo( Mapping::localTemporary( f->allocate() ) );
            bi++;
            st_assert( bi == blen or _pregsBegSorted->at( bi )->begByteCodeIndex() >= _pregsBegSorted->at( bi - 1 )->begByteCodeIndex(), "_pregsBegSorted not sorted" );
        }
        // allocate registers for subscopes that begin at byteCodeIndex
        while ( si < slen and _subScopes->at( si )->senderByteCodeIndex() == byteCodeIndex ) {
            _subScopes->at( si )->allocatePRegs( f );
            si++;
            st_assert( si == slen or _subScopes->at( si )->senderByteCodeIndex() >= _subScopes->at( si - 1 )->senderByteCodeIndex(), "_subScopes not sorted" );
        }
        // release registers that end at byteCodeIndex
        while ( ei < elen and _pregsEndSorted->at( ei )->endByteCodeIndex() == byteCodeIndex ) {
            f->release( Mapping::localTemporaryIndex( _pregsEndSorted->at( ei )->_location ) );
            ei++;
            st_assert( ei == elen or _pregsEndSorted->at( ei )->endByteCodeIndex() >= _pregsEndSorted->at( ei - 1 )->endByteCodeIndex(), "_pregsEndSorted not sorted" );
        }
        // advance byteCodeIndex
        byteCodeIndex = min( bi < blen ? _pregsBegSorted->at( bi )->begByteCodeIndex() : EpilogueByteCodeIndex + 1, si < slen ? _subScopes->at( si )->senderByteCodeIndex() : EpilogueByteCodeIndex + 1, ei < elen ? _pregsEndSorted->at( ei )->endByteCodeIndex() : EpilogueByteCodeIndex + 1 );
    }
    st_assert( f->allocated() == n, "inconsistent allocation/release" );
}


int InlinedScope::allocateFloatTemporaries( int firstFloatIndex ) {
    st_assert( firstFloatIndex >= 0, "illegal firstFloatIndex" );
    _firstFloatIndex = firstFloatIndex;                // start index for first float of this scope
    int       nofFloatTemps             = hasFloatTemporaries() ? nofFloatTemporaries() : 0;
    // convert floatLocs into stackLocs
    for ( int k                         = 0; k < nofFloatTemps; k++ ) {
        PseudoRegister * preg = floatTemporary( k )->preg();
        Location loc = preg->_location;
        st_assert( loc.scopeNo() == scopeID() and loc.floatNo() == k, "inconsistency" );
        preg->_location = Mapping::floatTemporary( scopeID(), k );
        st_assert( preg->_location.isStackLocation(), "must be a stack location" );
    }
    int       startFloatIndex           = firstFloatIndex + nofFloatTemps;    // start index for first float in subscopes
    int       totalNofFloatsInSubscopes = 0;
    // allocate float temporaries of subscopes
    int       len                       = _subScopes->length();
    for ( int i                         = 0; i < len; i++ ) {
        InlinedScope * scope = _subScopes->at( i );
        totalNofFloatsInSubscopes = max( totalNofFloatsInSubscopes, scope->allocateFloatTemporaries( startFloatIndex ) );
    }
    return nofFloatTemps + totalNofFloatsInSubscopes;
}


bool_t MethodScope::isRecursiveCall( MethodOop method, KlassOop rcvrKlass, int depth ) {
    // test is method/rcvrKlass would be a recursive invocation of this scope
    if ( method == _method and rcvrKlass == selfKlass() ) {
        if ( depth <= 1 ) {
            return true;    // terminate recursion here
        } else {
            // it's recursive, but the unrolling depth hasn't been reached yet
            depth--;
        }
    }
    // check sender
    if ( isTop() ) {
        return false;
    } else {
        return sender()->isRecursiveCall( method, rcvrKlass, depth );
    }
}


bool_t BlockScope::isRecursiveCall( MethodOop method, KlassOop rcvrKlass, int depth ) {
    if ( method == _method ) {
        if ( depth <= 1 ) {
            return true;    // terminate recursion here
        } else {
            // it's recursive, but the unrolling depth hasn't been reached yet
            // NB: don't just fall through to parent check below because of
            // x := [ x value ]. x value
            return sender()->isRecursiveCall( method, rcvrKlass, --depth );
        }
    }
    // don't check sender, only check parent and it's senders
    // otherwise, in code like [ x foo. y do: [ z foo ]] the z foo send
    // would be considered a recursive invocation of x foo if x class = z class
    if ( parent()->isOutlinedScope() ) {
        return false;
    } else {
        return parent()->isRecursiveCall( method, rcvrKlass, depth );
    }
}


void BlockScope::setContext( PseudoRegister * newContext ) {
    _context = newContext;
    if ( _temporaries->first()->isContextExpression() ) {
        _temporaries->at_put( 0, new ContextExpression( newContext ) );
    } else {
        GrowableArray <Expression *> * oldTemps = _temporaries;
        _temporaries                            = new GrowableArray <Expression *>( oldTemps->length() + 1 );
        _temporaries->append( new ContextExpression( newContext ) );
        _temporaries->appendAll( oldTemps );
    }
}


void InlinedScope::genCode() {
    _hasBeenGenerated = true;
    prologue();
    // always generate (shared) entry points for ordinary & non-local return
    _returnPoint              = NodeFactory::MergeNode( EpilogueByteCodeIndex );
    _NonLocalReturneturnPoint = NodeFactory::MergeNode( EpilogueByteCodeIndex );
    _nlrTestPoint             = nullptr;
    _contextInitializer       = nullptr;
    int nofTemps = method()->number_of_stack_temporaries();
    if ( isTop() ) {
        _returnPoint->append( NodeFactory::ReturnNode( resultPR, EpilogueByteCodeIndex ) );
        _NonLocalReturneturnPoint->append( NodeFactory::NonLocalReturnSetupNode( resultPR, EpilogueByteCodeIndex ) );
        Node * first = NodeFactory::PrologueNode( key(), nofArguments(), nofTemps );
        theCompiler->firstNode = first;
        gen()->setCurrent( first );
    }
    // allocate Space for temporaries - initialization done in the prologue code
    st_assert( not hasTemporaries(), "should have no temporaries yet\n" );
    createTemporaries( nofTemps );
    // allocate Space for float temporaries
    int nofFloats = method()->total_number_of_floats();
    if ( UseFPUStack ) {
        const int FPUStackSize = 8;
        if ( method()->float_expression_stack_size() <= FPUStackSize ) {
            // float expression stack fits in FPU stack, use it instead and allocate only Space for the real float temporaries
            nofFloats = method()->number_of_float_temporaries();
        } else {
            warning( "possible performance bug: cannot use FPU stack for float expressions" );
        }
    }
    createFloatTemporaries( nofFloats );
    // build the intermediate representation
    st_assert( gen()->current() not_eq nullptr, "current() should have been set before" );
    MethodIterator iter( method(), gen() );
    if ( gen()->aborting() ) {
        // ends with dead code -- clean up expression stack
        while ( not exprStack()->isEmpty() )
            exprStack()->pop();
    }
    epilogue();
}


// debugging info
void print_selector_cr( SymbolOop selector ) {
    char buffer[100];
    int  length = selector->length();
    st_assert( length < 100, "selector longer than 99 characters - buffer overrun" );
    strncpy( buffer, selector->chars(), length );
    buffer[ length ] = '\0';
    _console->print_cr( "%s", buffer );
}


void InlinedScope::generateDebugInfo() {
    // Generate debug info for the common parts of methods and blocks

    if ( PrintDebugInfoGeneration ) {
        if ( isMethodScope() ) {
            _console->print( "Method: " );
            print_selector_cr( method()->selector() );
            _console->print_cr( "self: %s", _self->preg()->name() );
        } else {
            MethodOop m;
            _console->print( "Method: " );
            for ( m = method(); m->is_blockMethod(); m = m->parent() ) {
                _console->print( "[] in " );
            }
            print_selector_cr( m->selector() );
            method()->print_codes();
            _console->print_cr( "no receiver (block method)" );
        }
    }

    ScopeDescriptorRecorder * rec = theCompiler->scopeDescRecorder();
    int len, i;

    if ( not isLite() ) {
        // temporaries
        if ( hasTemporaries() ) {
            len = _temporaries->length();
            for ( int i = 0; i < len; i++ ) {
                PseudoRegister * preg = _temporaries->at( i )->preg();
                rec->addTemporary( _scopeInfo, i, preg->createLogicalAddress() );
                if ( PrintDebugInfoGeneration )
                    _console->print_cr( "temp[%2d]: %s", i, preg->name() );
            }
        }
        // float temporaries
        if ( hasFloatTemporaries() ) {
            len = _floatTemporaries->length();
            for ( int i = 0; i < len; i++ ) {
                PseudoRegister * preg = _floatTemporaries->at( i )->preg();
                rec->addTemporary( _scopeInfo, i, preg->createLogicalAddress() );
                if ( PrintDebugInfoGeneration )
                    _console->print_cr( "float[%2d]: %s", i, preg->name() );
            }
        }
        // context temporaries
        if ( allocatesInterpretedContext() ) {
            len = _contextTemporaries->length();
            for ( int i = 0; i < len; i++ ) {
                PseudoRegister * preg = _contextTemporaries->at( i )->preg();
                rec->addContextTemporary( _scopeInfo, i, preg->createLogicalAddress() );
                if ( PrintDebugInfoGeneration )
                    _console->print_cr( "c_temp[%2d]: %s", i, preg->name() );
            }
        }
        // expr stack
        len = _exprStackElems->length();
        for ( int i = 0; i < len; i++ ) {
            Expression * elem = _exprStackElems->at( i );
            if ( elem not_eq nullptr ) {
                PseudoRegister * r = elem->preg()->cpReg();
                if ( r->scope()->isSenderOrSame( this ) ) {
                    // Note: Is it still needed to create this info here, since the
                    //       PseudoRegister locations may change over time and thus produce more
                    //       debug info than actually needed for the new backend. Discuss
                    //       this with Lars.
                    rec->addExprStack( _scopeInfo, i, r->createLogicalAddress() );
                    if ( PrintDebugInfoGeneration )
                        _console->print_cr( "expr[%2d]: %s", i, r->name() );
                } else {
                    // r's scope is too low (i.e. it's not actually live anymore)
                    // this can only happen if the expression is discarded; thus it's safe not to describe this entry
                    // Urs 5/96
                    // fix this: should check that byteCodeIndex (i) is statement end (or r is NoResultPseudoRegister)
                }
            }
        }
    }

    // subscopes
    len = _subScopes->length();
    for ( int i = 0; i < len; i++ ) {
        InlinedScope * s = _subScopes->at( i );
        if ( PrintDebugInfoGeneration )
            _console->print_cr( "Subscope %d (id = %d):", i, s->scopeID() );
        s->generateDebugInfo();
    }
}


void MethodScope::generateDebugInfo() {
    ScopeDescriptorRecorder * rec = theCompiler->scopeDescRecorder();
    const bool_t visible = true;
    _scopeInfo = rec->addMethodScope( _key, _method, _self->preg()->createLogicalAddress(), allocatesCompiledContext(), isLite(), _scopeID, _sender ? _sender->getScopeInfo() : nullptr, _senderByteCodeIndex, visible );
    InlinedScope::generateDebugInfo();
}


void BlockScope::generateDebugInfo() {
    ScopeDescriptorRecorder * rec = theCompiler->scopeDescRecorder();
    if ( _parent->isOutlinedScope() ) {
        _scopeInfo = rec->addTopLevelBlockScope( _method, _self->preg()->createLogicalAddress(), _self->klass(), allocatesCompiledContext() );
    } else {
        st_assert( _parent->isInlinedScope(), "oops" );
        const bool_t visible = true;
        _scopeInfo = rec->addBlockScope( _method, ( ( InlinedScope * ) _parent )->getScopeInfo(), allocatesCompiledContext(), isLite(), _scopeID, _sender->getScopeInfo(), _senderByteCodeIndex, visible );
    }
    InlinedScope::generateDebugInfo();
}


// Outlined scopes

OutlinedScope * new_OutlinedScope( NativeMethod * nm, ScopeDescriptor * sc ) {
    if ( sc->isMethodScope() ) {
        return new OutlinedMethodScope( nm, sc );
    } else {
        return new OutlinedBlockScope( nm, sc );
    }
}


OutlinedScope::OutlinedScope( NativeMethod * nm, ScopeDescriptor * scope ) {
    _nm    = nm;
    _scope = scope;
}


OutlinedBlockScope::OutlinedBlockScope( NativeMethod * nm, ScopeDescriptor * sc ) :
    OutlinedScope( nm, sc ) {
    ScopeDescriptor * parent = sc->parent( true );
    if ( parent ) {
        _parent = new_OutlinedScope( nm, parent );
    } else {
        _parent = nullptr;
    }
}


Expression * OutlinedScope::receiverExpression( PseudoRegister * p ) const {
    return _scope->selfExpression( p );
}


MethodOop OutlinedScope::method() const {
    return _scope->method();
}


KlassOop OutlinedMethodScope::methodHolder() const {
    return selfKlass()->klass_part()->lookup_method_holder_for( method() );
}


// printing code

void SendInfo::print() {
    lprintf( "SendInfo %#lx \"", this );
    _selector->print_symbol_on();
    lprintf( "\" (receiver = %#lx, nsends = %ld)\n", _receiver, _sendCount );
}


void InlinedScope::printTree() {
    print();
    for ( int i = 0; i < _subScopes->length(); i++ ) {
        _subScopes->at( i )->printTree();
    }
}


void InlinedScope::print() {
    lprintf( " method: %#lx\n\tid: %ld", method(), scopeID() );
    lprintf( "\nself:   " );
    self()->print();
    for ( int i = 0; i < nofArguments(); i++ ) {
        lprintf( "arg %2d: ", i );
        argument( i )->print();
    }
    lprintf( "\n" );
}


void MethodScope::print_short() {
    lprintf( "(MethodScope*)%#lx (", this );
    selector()->print_symbol_on();
    lprintf( ")" );
}


void MethodScope::print() {
    print_short();
    InlinedScope::print();
    if ( sender() ) {
        lprintf( "  sender: " );
        sender()->print_short();
    }
    lprintf( " @ %ld\n", senderByteCodeIndex() );
    method()->pretty_print();
}


void BlockScope::print() {
    print_short();
    InlinedScope::print();
    if ( parent() ) {
        printf( "\tparent: " );
        parent()->print_short();
    }
    if ( sender() ) {
        lprintf( "  sender: " );
        sender()->print_short();
    }
    lprintf( " @ %ld\n", senderByteCodeIndex() );
    method()->pretty_print();
}


void BlockScope::print_short() {
    lprintf( "(BlockScope*)%#lx (", this );
    selector()->print_symbol_on();
    lprintf( " %#lx)", method() );
}


void OutlinedScope::print_short( const char * name ) {
    lprintf( "(%s*)%#lx (", name, this );
    _scope->selector()->print_symbol_on();
    lprintf( ")" );
}


void OutlinedScope::print( const char * name ) {
    print_short( name );
    lprintf( "  _nm = %#lx, _scope = %#lx", _nm, _scope );
}


void OutlinedMethodScope::print_short() {
    OutlinedScope::print_short( "OutlinedMethodScope" );
}


void OutlinedMethodScope::print() {
    OutlinedScope::print( "OutlinedMethodScope" );
}


void OutlinedBlockScope::print() {
    OutlinedScope::print( "OutlinedMethodScope" );
    if ( parent() ) {
        lprintf( "\n    parent: " );
        parent()->print_short();
    }
    lprintf( "\n" );
}


void OutlinedBlockScope::print_short() {
    OutlinedScope::print_short( "OutlinedMethodScope" );
}
