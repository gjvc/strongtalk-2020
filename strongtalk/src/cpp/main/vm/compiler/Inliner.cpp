//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/system/asserts.hpp"
#include "vm/compiler/InliningPolicy.hpp"
#include "vm/compiler/Inliner.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/lookup/LookupCache.hpp"


// ----------- inlining policy ---------------

static const int DefaultCompilerMaxSplitCost        = 50;
static const int DefaultCompilerMaxBlockInstrSize   = 400;
static const int DefaultCompilerMaxFnInstrSize      = 250;
static const int DefaultCompilerMaxBlockFnInstrSize = 600;
static const int DefaultCompilerMaxNmethodInstrSize = 5000;

static CompilerInliningPolicy inliningPolicy;


// --------------- Inliner ------------------

void Inliner::initialize() {
    _info             = nullptr;
    _result           = nullptr;
    _callee           = nullptr;
    _generator        = nullptr;
    _merge            = nullptr;
    _resultPR         = nullptr;
    depth             = 0;
    _msg              = nullptr;
    _lastLookupFailed = false;
}


void Inliner::initialize( SendInfo * info, SendKind kind ) {
    initialize();
    _info      = info;
    _sendKind  = kind;
    _sender    = _info->_senderScope;
    _generator = _sender->gen();
    _info->_resultRegister = _resultPR = new SinglyAssignedPseudoRegister( _sender );
    depth = _sender->depth;
}


Expression * Inliner::inlineNormalSend( SendInfo * info ) {
    initialize( info, SendKind::NormalSend );
    return inlineSend();
}


Expression * Inliner::inlineSuperSend( SendInfo * info ) {
    initialize( info, SendKind::SuperSend );
    return inlineSend();
}


Expression * Inliner::inlineSelfSend( SendInfo * info ) {
    initialize( info, SendKind::SelfSend );
    return inlineSend();
}


Expression * Inliner::inlineSend() {
    if ( not Inline ) {
        // don't do any inlining
        _info->_needRealSend = true;
    } else if ( _generator->is_in_dead_code() ) {
        // don't waste time inlining dead code
        _result = new NoResultExpression;
        _generator->abort();   // the rest of this method is dead code, too
        if ( CompilerDebug )
            cout( PrintInlining )->print( "%*s*skipping %s (dead code)\n", depth, "", _info->_selector->as_string() );
    } else {
        tryInlineSend();
    }

    // generate a real send if necessary
    if ( _info->_needRealSend ) {
        Expression * r = genRealSend();
        _result = _result ? _result->mergeWith( r, _merge ) : r;
        st_assert( _result, "must have result" );
    }
    // merge end of inlined version with end of noninlined version
    if ( _merge and _result and not _result->isNoResultExpression() )
        _generator->branch( _merge );

    // update caller's current node
    if ( _generator not_eq _sender->gen() )
        _sender->gen()->setCurrent( _generator->current() );

    // ...and return result (sender is responsible for popping expr stack)
    if ( not _result )
        _result = new NoResultExpression;
    return _result;
}


Expression * Inliner::genRealSend() {
    const int nofArgs     = _info->_selector->number_of_arguments();
    bool_t    uninlinable = theCompiler->registerUninlinable( this );
    if ( CompilerDebug ) {
        cout( PrintInlining )->print( "%*s*sending %s %s%s\n", depth, "", _info->_selector->as_string(), uninlinable ? "(unlinlinable) " : "", _info->_counting ? "(counting) " : "" );
    }
    switch ( _sendKind ) {
        case SendKind::NormalSend:
            _generator->gen_normal_send( _info, nofArgs, _resultPR );
            break;
        case SendKind::SelfSend:
            _generator->gen_self_send( _info, nofArgs, _resultPR );
            break;
        case SendKind::SuperSend:
            _generator->gen_super_send( _info, nofArgs, _resultPR );
            break;
        default: fatal1( "illegal SendKind %d", _sendKind );
    }
    return new UnknownExpression( _resultPR, _generator->current() );
}


void Inliner::tryInlineSend() {
    const SymbolOop sel = _info->_selector;

    UnknownExpression * u = _info->_receiver->findUnknown();
    bool_t usingInliningDB = _sender->rscope->isDatabaseScope();
    // first, use type feedback
    if ( TypeFeedback and u ) {
        st_assert( _sendKind not_eq SendKind::SuperSend, "shouldn't PolymorphicInlineCache-predict super sends" );
        // note: when compiling using the inlining database, picPredict won't actually look at any PICs, just at the inlining DB
        _info->_receiver = picPredict();
    }

    // now try static type prediction (but only if type isn't good enough yet)
    if ( TypePredict and not usingInliningDB and u and not u->isUnlikely() and not _info->uninlinable ) {
        _info->_receiver = typePredict();
    }

    if ( _info->_receiver->really_hasKlass( _sender ) ) {
        // single klass - try to inline this send
        _callee = tryLookup( _info->_receiver );
        if ( _callee ) {
            Expression * r = doInline( _sender->current() );
            _result        = makeResult( r );
        } else {
            // must distinguish between lookup failures and rejected successes
            if ( _lastLookupFailed ) {
                // receiver type is constant (but e.g. method was too big to inline)
                _info->_receiverStatic = true;
            }
            _info->_needRealSend = true;
        }
    } else if ( _info->_receiver->isMergeExpression() ) {
        _result = inlineMerge( _info );        // inline some cases
        if ( _result ) {
            // inlined some cases; inlineMerge decided whether needRealSend should be set or not
            if ( not theCompiler->is_uncommon_compile() ) {
                _info->uninlinable = true;      // remaining sends are here by choice
                _info->_counting   = false;
            }
        } else {
            // didn't inline anything -- just generate a non-inlined send
            _info->_needRealSend = true;
        }
    } else {
        // unknown receiver
        // NB: *must* use uncommon branch if marked unlikely because future type tests won't test for unknown
        if ( CompilerDebug )
            cout( PrintInlining )->print( "%*s*cannot inline %s (unknown receiver)\n", depth, "", sel->as_string() );
        if ( _info->_receiver->findUnknown()->isUnlikely() ) {
            // generate an uncommon branch for the unknown case, not a send
            _generator->append_exit( NodeFactory::UncommonNode( _sender->gen()->copyCurrentExprStack(), _sender->byteCodeIndex() ) );
            _info->_needRealSend = false;
            if ( CompilerDebug )
                cout( PrintInlining )->print( "%*s*making %s uncommon\n", depth, "", sel->as_string() );
            _result = new NoResultExpression();
            // rest of method's code is unreachable
            st_assert( _generator->current() == nullptr, "expected no current node" );
            _generator->abort();
        } else {
            _info->_needRealSend = true;
        }
    }
}


Expression * Inliner::inlineMerge( SendInfo * info ) {
    // try to inline the send by type-casing
    _merge = NodeFactory::MergeNode( _sender->byteCodeIndex() );        // where all cases merge again
    Expression * res = nullptr;                            // final (merged) result
    st_assert( info->_receiver->isMergeExpression(), "must be a merge" );
    MergeExpression * r = ( MergeExpression * ) info->_receiver;                // receiver type
    SymbolOop sel = info->_selector;

    int nexprs = r->exprs->length();
    int ncases = nexprs - ( r->containsUnknown() ? 1 : 0 );
    if ( ncases > MaxTypeCaseSize ) {
        info->_needRealSend = true;
        info->uninlinable   = true;
        info->_counting     = false;
        if ( CompilerDebug )
            cout( PrintInlining )->print( "%*s*not type-casing %s (%ld > MaxTypeCaseSize)\n", depth, "", sel->as_string(), ncases );
        return res;
    }

    // build list of cases to inline
    // (add only immediate klasses (currently only smis) at first, collect others in ...2 lists
    GrowableArray <InlinedScope *> * scopes  = new GrowableArray <InlinedScope *>( nexprs );
    GrowableArray <InlinedScope *> * scopes2 = new GrowableArray <InlinedScope *>( nexprs );
    GrowableArray <Expression *>   * exprs   = new GrowableArray <Expression *>( nexprs );
    GrowableArray <Expression *>   * exprs2  = new GrowableArray <Expression *>( nexprs );
    GrowableArray <Expression *>   * others  = new GrowableArray <Expression *>( nexprs );
    GrowableArray <KlassOop> * klasses  = new GrowableArray <KlassOop>( nexprs );
    GrowableArray <KlassOop> * klasses2 = new GrowableArray <KlassOop>( nexprs );
    const bool_t containsUnknown = r->containsUnknown();

    for ( int i = 0; i < nexprs; i++ ) {
        Expression * nth = r->exprs->at( i )->shallowCopy( r->preg(), nullptr );
        st_assert( not nth->isConstantExpression() or nth->next == nullptr or nth->constant() == nth->next->constant(), "shouldn't happen: merged consts - convert to klass" );
        // NB: be sure to generalize constants to klasses before inlining, so that values from an unknown source are dispatched to the optimized code also, right now the TypeTestNode only tests for klasses, not constants
        if ( containsUnknown and nth->isConstantExpression() ) {
            nth = nth->convertToKlass( nth->preg(), nth->node() );
        }

        InlinedScope * s;
        if ( nth->hasKlass() and ( s = tryLookup( nth ) ) not_eq nullptr ) {
            // can inline this case
            KlassOop klass = nth->klass();
            if ( klass == smiKlassObj ) {
                scopes->append( s );        // smis go first
                exprs->append( nth );
                klasses->append( klass );
            } else {
                scopes2->append( s );        // append later
                exprs2->append( nth );
                klasses2->append( klass );
            }
        } else {
            if ( _lastLookupFailed ) {
                // ignore this case -- most probably it will never happen
                // (typical case: the class is returned by something like
                // self error: 'should not happen'. ^ self)
                if ( others->isEmpty() ) {
                    others->append( new UnknownExpression( nth->preg(), nullptr, theCompiler->useUncommonTraps ) );
                }
            } else {
                others->append( nth );
            }
            info->_needRealSend = true;
        }
    }

    // combine all lists into one (with immediate case first)
    klasses->appendAll( klasses2 );
    exprs->appendAll( exprs2 );
    scopes->appendAll( scopes2 );

    // decide whether to use uncommon branch for unknown case (if any) NB: *must* use uncommon branch if marked unlikely because future type tests won't test for unknown
    bool_t useUncommonBranchForUnknown = false;
    if ( others->length() == 1 and others->first()->isUnknownExpression() and ( ( UnknownExpression * ) others->first() )->isUnlikely() ) {
        // generate an uncommon branch for the unknown case, not a send
        useUncommonBranchForUnknown = true;
        if ( CompilerDebug )
            cout( PrintInlining )->print( "%*s*making %s uncommon (2)\n", depth, "", sel->as_string() );
    }

    // now do the type test and inline the individual cases
    Node * typeCase    = nullptr;
    Node * fallThrough = nullptr;
    if ( scopes->length() > 0 ) {
//        memoizeBlocks(sel);

        if ( CompilerDebug ) {
            char * s = new_resource_array <char>( 200 );
            sprintf( s, "begin type-case of %s (ends at node N%ld)", sel->copy_null_terminated(), _merge->id() );
            _generator->comment( s );
        }
        if ( CompilerDebug ) {
            cout( PrintInlining )->print( "%*s*type-casing %s (%d cases)\n", depth, "", sel->as_string(), scopes->length() );
        }

        typeCase = NodeFactory::TypeTestNode( r->preg(), klasses, info->_needRealSend or containsUnknown );
        _generator->append( typeCase );
        fallThrough = typeCase->append( NodeFactory::NopNode() );    // non-predicted case
        for ( int i = 0; i < scopes->length(); i++ ) {
            // inline one case
            Inliner * inliner = new Inliner( _sender );
            inliner->initialize( new SendInfo( *info ), _sendKind );
            inliner->_callee    = scopes->at( i );            // scope to inline
            inliner->_generator = _callee->gen();            // node builder to use
            inliner->_generator->setCurrent( typeCase->append( i + 1, NodeFactory::NopNode() ) );
            Expression * receiver = exprs->at( i );
            inliner->info()->_receiver = receiver;
            st_assert( r->scope()->isSenderOf( inliner->_callee ), "r must be from caller scope" );

            Expression * e = inliner->doInline( inliner->_generator->current() );
            if ( e->isNoResultExpression() ) {
                if ( not res )
                    res = e;    // must return non-nullptr result (otherwise sender thinks no inlining happened)
            } else {
                st_assert( e->preg()->scope()->isSenderOf( inliner->_callee ), "result register must be from caller scope" );
                _generator->append( NodeFactory::NopNode() );
                e   = e->shallowCopy( info->_resultRegister, _generator->current() );
                res = res ? res->mergeWith( e, _merge ) : e;
            }
            _generator->branch( _merge );
        }
        st_assert( _generator == _sender->gen(), "oops" );
        _generator->setCurrent( fallThrough );

    } else {
        // no case was deemed inlinable
        if ( not info->_counting and not theCompiler->is_uncommon_compile() )
            info->uninlinable = true;
        useUncommonBranchForUnknown = false;
    }

    if ( res and res->isMergeExpression() )
        res->setNode( _merge, info->_resultRegister );

    st_assert( info->_needRealSend and others->length() or not info->_needRealSend and not others->length(), "inconsistent" );

    if ( useUncommonBranchForUnknown ) {
        // generate an uncommon branch for the unknown case, not a send
        // use an uncommon send rather than an uncommon node to capture
        // the argument usage in case none of the type tests uses the arguments.
        // - was a bug slr 12/02/2009.
        _generator->append_exit( NodeFactory::UncommonSendNode( _generator->copyCurrentExprStack(), _sender->byteCodeIndex(), info->_selector->number_of_arguments() ) );
        info->_needRealSend = false;
    } else if ( others->isEmpty() ) {
        // typecase cannot fail
        _generator->append_exit( NodeFactory::FixedCodeNode( FixedCodeNode::FixedCodeKind::dead_end ) );
    }

    return res;
}


Expression * Inliner::makeResult( Expression * r ) {
    Expression * res;
    // massage the result expression so it fulfills all constraints
    if ( r->isNoResultExpression() ) {
        res = r;
    } else {
        // result's scope must be in sender, not in callee (for correct reg. alloc.)
        if ( r->scope() == _sender ) {
            res = r;
        } else {
            st_assert( r->scope() == _callee, "what scope?" );
            //fatal("Urs thinks this shouldn't happen");
            ShouldNotReachHere(); // added instead of the fatal (gri 11/27/01)
            // bind it to new NopNode to fix scope
            Node * n = NodeFactory::NopNode();
            st_assert( n->scope() == _sender, "wrong scope" );
            _generator->append( n );
            res = r->shallowCopy( r->preg(), n );
        }
    }
    return res;
}


Expression * Inliner::doInline( Node * start ) {
    // callee scope should be inlined; do it
    // HACK -- fix
#define calleeSize( n ) 0

    if ( CompilerDebug ) {
        cout( PrintInlining )->print( "%*s*inlining %s, cost %ld/size %ld (%#lx)%s\n", depth, "", _callee->selector()->as_string(), inliningPolicy.calleeCost, calleeSize( _callee->rscope ), PrintHexAddresses ? _callee : 0, _callee->rscope->isNullScope() ? "" : "*" );
        if ( PrintInlining )
            _callee->method()->pretty_print();
    }

    // Save dependency information in the scopeDesc recorder.
    theCompiler->rec->add_dependent( _callee->key() );

    Node * next = start->next();
    st_assert( not next, "can't insert code (?)" );

    // set callee's starting point for code generation
    _generator = _callee->gen();
    _generator->setCurrent( start );

    reportInline( "Inline " );

    _callee->genCode();

    // make sure caller appends new code after inlined return of callee
    _generator = _sender->gen();
    _generator->setCurrent( _callee->returnPoint() );
    st_assert( _generator->current()->next() == nullptr, "shouldn't have successor" );

    reportInline( "End of " );

    return _callee->result;
}


void Inliner::reportInline( const char * prefix ) {

    if ( not info()->_selector )
        return;

    if ( not _callee->methodHolder() )
        return;

    const char * klassName = _callee->methodHolder()->klass_part()->delta_name();
    if ( not klassName )
        return;

    int       prefixLen   = strlen( prefix );
    SymbolOop selector    = info()->_selector;
    int       length      = selector->length();
    int       klassLength = strlen( klassName );
    char * buffer = new_resource_array <char>( klassLength + length + prefixLen + 3 );
    strcpy( buffer, prefix );
    strcpy( buffer + prefixLen, klassName );
    strcpy( buffer + klassLength + prefixLen, ">>" );
    strncpy( buffer + klassLength + prefixLen + 2, selector->chars(), length );
    buffer[ length + klassLength + prefixLen + 2 ] = '\0';

    _generator->append( NodeFactory::CommentNode( buffer ) );
}


Expression * Inliner::picPredict() {
    // check PICs for information
    const int byteCodeIndex = _sender->byteCodeIndex();
    RecompilationScope * rscope = _sender->rscope;

    if ( not rscope->hasSubScopes( byteCodeIndex ) ) {
        // note: not fully implemented yet -- if no subScopes, should check to see if send should be made unlikely (e.g. sends in prim. failure branches)
        // fix this
        return _info->_receiver;
    }

    // l is the list of receivers predicted by the PolymorphicInlineCache
    GrowableArray <RecompilationScope *> * predictedReceivers = _sender->rscope->subScopes( _sender->byteCodeIndex() );

    // check special cases: never executed or uninlinable/megamorphic
    if ( predictedReceivers->length() == 1 ) {
        if ( predictedReceivers->first()->isUntakenScope() ) {
            // send was never executed
            return picPredictUnlikely( _info, ( UntakenRecompilationScope * ) predictedReceivers->first() );

        } else if ( predictedReceivers->first()->isUninlinableScope() ) {
            if ( CompilerDebug )
                cout( PrintInlining )->print( "%*s*PolymorphicInlineCache-predicting %s as uninlinable/megamorphic\n", depth, "", _info->_selector->as_string() );
            _info->uninlinable = true;    // prevent static type prediction
            return _info->_receiver;
        }
    }

    // check special case: perfect information (from dataflow information)
    if ( not _info->_receiver->containsUnknown() ) {
        return _info->_receiver;    // already know type exactly, don't use PolymorphicInlineCache
    }

    // extract klasses from PolymorphicInlineCache
    GrowableArray <Expression *> klasses( 5 );
    MergeExpression * allKlasses = new MergeExpression( _info->_receiver->preg(), nullptr );
    int i = 0;
    for ( ; i < predictedReceivers->length(); i++ ) {
        RecompilationScope * r    = predictedReceivers->at( i );
        Expression         * expr = r->receiverExpression( _info->_receiver->preg() );
        if ( expr->isUnknownExpression() ) {
            // untaken real send (from PolymorphicInlineCache) (usually the "otherwise" branch of predicted sends)
            // since prediction was always correct, make sure unknown case is unlikely
            st_assert( ( ( UnknownExpression * ) expr )->isUnlikely(), "performance bug: should make unknown unlikely" );
            continue;
        }
        klasses.append( expr );
        allKlasses = ( MergeExpression * ) allKlasses->mergeWith( expr, nullptr );
    }

    // check if PolymorphicInlineCache info is better than static type info; discard all static info
    // that's not in the PolymorphicInlineCache
    int npic    = klasses.length();
    int nstatic = _info->_receiver->nklasses();
    if ( npic not_eq 0 and _info->_receiver->isMergeExpression() ) {
        Expression * newReceiver = _info->_receiver;
        for ( int i = ( ( MergeExpression * ) _info->_receiver )->exprs->length() - 1; i >= 0; i-- ) {
            Expression * e = ( ( MergeExpression * ) _info->_receiver )->exprs->at( i );
            if ( e->isUnknownExpression() )
                continue;
            if ( not allKlasses->findKlass( e->klass() ) ) {
                if ( PrintInlining ) {
                    _console->print( "%*s*discarding static type info for send %s (not found in PolymorphicInlineCache): ", depth, "", _info->_selector->as_string() );
                    e->print();
                }
                newReceiver = newReceiver->copyWithout( e );
            }
        }
        _info->_receiver = newReceiver;
    }

    if ( CompilerDebug )
        cout( PrintInlining )->print( "%*s*PolymorphicInlineCache-type-predicting %s (%ld klasses): ", depth, "", _info->_selector->as_string(), npic );

    // iterate through PolymorphicInlineCache _info and add it to the receiver type (_info->receiver)
    for ( int i = 0; i < klasses.length(); i++ ) {
        Expression * expr = klasses.at( i );
        // use the PolymorphicInlineCache information for this case
        if ( CompilerDebug ) {
            expr->klass()->klass_part()->print_name_on( cout( PrintInlining ) );
            cout( PrintInlining )->print( " " );
        }
        Expression * alreadyThere = _info->_receiver->findKlass( expr->klass() );
        // add klass only if not already present (for splitting -- adding klass
        // expr with node == nullptr destroys splitting opportunity)
        if ( alreadyThere ) {
            // generalize constant to klass
            if ( alreadyThere->isConstantExpression() ) {
                _info->_receiver = _info->_receiver->mergeWith( expr, nullptr );
            }
        } else {
            _info->_predicted = true;
            _info->_receiver  = _info->_receiver->mergeWith( expr, nullptr );
            if ( expr->hasConstant() and klasses.length() == 1 ) {
                // check to see if single predicted receiver is true or false;
                // if so, add other boolean to prediction.  Reduces the number
                // of uncommon branches; not doing so appears to be overly
                // aggressive (as observed experimentally)
                Oop c = expr->constant();
                PseudoRegister * p = _info->_receiver->preg();
                if ( c == trueObj and not _info->_receiver->findKlass( falseObj->klass() ) ) {
                    Expression * f   = new ConstantExpression( falseObj, p, nullptr );
                    _info->_receiver = _info->_receiver->mergeWith( f, nullptr );
                } else if ( c == falseObj and not _info->_receiver->findKlass( trueObj->klass() ) ) {
                    Expression * t = new ConstantExpression( trueObj, p, nullptr );
                    _info->_receiver = _info->_receiver->mergeWith( t, nullptr );
                }
            }
        }
    } // for
    if ( CompilerDebug )
        cout( PrintInlining )->print_cr( "" );

    // mark unknown branch as unlikely
    UnknownExpression * u = _info->_receiver->findUnknown();
    const bool_t canBeUnlikely = theCompiler->useUncommonTraps;
    if ( u and canBeUnlikely and not rscope->isNotUncommonAt( byteCodeIndex ) ) {
        _info->_receiver = _info->_receiver->makeUnknownUnlikely( _sender );
    }

    st_assert( _info->_receiver->preg(), "should have a preg" );
    return _info->_receiver;
}


Expression * Inliner::picPredictUnlikely( SendInfo * info, UntakenRecompilationScope * uscope ) {
    if ( not theCompiler->useUncommonTraps )
        return info->_receiver;

    bool_t makeUncommon = uscope->isUnlikely();
    if ( not makeUncommon and info->_inPrimitiveFailure ) {
        // this send was never executed in the recompilee only make the send unlikely if it had a chance to execute
        // (If the send isn't a prim failure, don't trust the info -- it's unlikely that the method just stops executing in the middle.
        // What probably happened is that recompilation occurred before the rest of the method got a chance to execute (e.g. recursion), or it
        // always quit via NonLocalReturn.  In any case, the compiler can't handle this yet - need to treat it specially similar to endsDead.)
        makeUncommon = false;
    }
    if ( false and CompilerDebug ) {
        cout( PrintInlining )->print( "%*s*%sPIC-type-predicting %s as never executed\n", depth, "", makeUncommon ? "" : "NOT ", info->_selector->copy_null_terminated() );
    }
    if ( makeUncommon ) {
        return new UnknownExpression( info->_receiver->preg(), nullptr, true );
    } else {
        return info->_receiver;
    }
}


Expression * Inliner::typePredict() {
    // NB: all non-predicted cases exit this function early
    Expression * r = _info->_receiver;
    if ( not( r->isUnknownExpression() or r->isMergeExpression() and ( ( MergeExpression * ) r )->exprs->length() == 1 and ( ( MergeExpression * ) r )->exprs->at( 0 )->isUnknownExpression() ) ) {
        // r already has a type (e.g. something predicted via PICs)
        // trust that information more than the static type prediction
        // NB: UnknownExprs are sometimes merged into a MergeExpression, that's why the above
        // test looks a bit more complicated
        return _info->_receiver;
    }

    // perform static type prediction
    if ( InliningPolicy::isPredictedSmiSelector( _info->_selector ) ) {
        r = r->mergeWith( new KlassExpression( smiKlassObj, r->preg(), nullptr ), nullptr );
    } else if ( InliningPolicy::isPredictedArraySelector( _info->_selector ) ) {
        // don't know what to predict -- objArray? byteArray?
        if ( TypePredictArrays ) {
            r = r->mergeWith( new KlassExpression( Universe::objArrayKlassObj(), r->preg(), nullptr ), nullptr );
        } else {
            return r;
        }
    } else if ( InliningPolicy::isPredictedBoolSelector( _info->_selector ) ) {
        r = r->mergeWith( new ConstantExpression( trueObj, r->preg(), nullptr ), nullptr );
        r = r->mergeWith( new ConstantExpression( falseObj, r->preg(), nullptr ), nullptr );
    } else {
        return r;
    }

    // receiver was type-predicted
    if ( theCompiler->useUncommonTraps ) {
        // make unknon case unlikely
        r = r->makeUnknownUnlikely( _sender );
    }
    return r;
}


bool_t SuperSendsAreAlwaysInlined = true;    // remove when removing super hack

InlinedScope * Inliner::tryLookup( Expression * receiver ) {

    // try to lookup the send to receiver receiver and determine if it should be inlined;
    // return new InlinedScope if ok, nullptr if lookup error or non-inlinable
    // NB: _info->receiver is the overall receiver (e.g. a merge expr), receiver is the particular branch we're looking at right now
    st_assert( receiver->hasKlass(), "should know klass" );

    const KlassOop klass = ( _sendKind == SendKind::SuperSend ) ? _sender->methodHolder() : receiver->klass();
    if ( klass == nullptr ) {
        _info->uninlinable = true;
        st_assert( _sendKind == SendKind::SuperSend, "shouldn't happen for normal sends" );
        return notify( "super send in Object" );
    }

    const SymbolOop selector = _info->_selector;
    const MethodOop method   = ( _sendKind == SendKind::SuperSend ) ? LookupCache::compile_time_super_lookup( klass, selector ) : LookupCache::compile_time_normal_lookup( klass, selector );
    _lastLookupFailed = method == nullptr;

    if ( _lastLookupFailed ) {
        // nothing found statically (i.e., lookup error)
        _info->uninlinable = true;        // fix this -- probably wrong for merge exprs.
        return notify( "lookup failed" );
    }

    // Construct a lookup key
    LookupKey * key = _sendKind == SendKind::SuperSend ? LookupKey::allocate( receiver->klass(), method ) : LookupKey::allocate( receiver->klass(), selector );

    if ( CompilerDebug )
        cout( PrintInlining )->print( "%*s found %s --> %#x\n", _sender->depth, "", key->print_string(), PrintHexAddresses ? method : 0 );

    // NB: use receiver->klass() (not klass) for the scope -- klass may be the method holder (for super sends)
    // was bug -- Urs 3/16/96
    InlinedScope * callee = makeScope( receiver, receiver->klass(), key, method );
    st_assert( _sendKind not_eq SendKind::SuperSend or callee not_eq nullptr, "Super send must be inlined" );

    _msg = inliningPolicy.shouldInline( _sender, callee );
    if ( _sendKind == SendKind::SuperSend ) {
        // for now, always inline super sends because run-time system can't handle them yet fix this later -- Urs 12/95
        SuperSendsAreAlwaysInlined = true;
        st_assert( not _info->_predicted, "shouldn't PolymorphicInlineCache-predict super sends" );
        _msg = nullptr;

    } else {
        if ( _msg == nullptr and _sender->gen()->in_primitive_failure_block() ) {
            _msg = checkSendInPrimFailure();
        }
    }
    if ( _msg )
        return notify( _msg );        // shouldn't inline this call

    return callee;
}


const char * Inliner::checkSendInPrimFailure() {

    // The current send could be inlined, but it is in a primitive failure block.
    // Decide if it really should be inlined (return nullptr if so, msg if not).
    RecompilationScope * rs = _sender->rscope;
    if ( rs->isNullScope() or rs->isInterpretedScope() ) {
        // not compiled -- don't inline, failure probably never happens but make sure we'll detect if it does happen often
        if ( UseRecompilation )
            _info->_counting = true;
        return "send in primitive failure (null/interpreted sender)";
    }

    RecompilationScope * callee = rs->subScope( _sender->byteCodeIndex(), _info->_lookupKey );
    if ( callee->isUntakenScope() ) {
        // never executed; shouldn't even generate code for failure!
        // (note: if failure block has several sends, this can happen, but in the standard system it's probably a performance bug)
        if ( WizardMode )
            warning( "probably should have made primitive failure uncommon" );

        return "untaken send in primitive failure";
    }

    if ( callee->isInlinedScope() ) {
        // was inlined in previous versions, so I guess it's ok to inline it again
        return nullptr;
    }

    if ( callee->isNullScope() ) {
        // no info; conservative thing is not to inline
        return "untaken send in primitive failure (NullScope)";
    }

    // ok, inline this send
    return nullptr;
}


RecompilationScope * Inliner::makeBlockRScope( const Expression * receiver, LookupKey * key, const MethodOop method ) {
    // create an InlinedScope for this block method
    if ( not TypeFeedback )
        return new NullRecompilationScope;
    if ( not receiver->preg()->isBlockPReg() ) {
        return new NullRecompilationScope;      // block parent is in a different NativeMethod -- won't inline
    }

    // first check if block was inlined in previous NativeMethod (or comes from inlining database)
    const int                            senderByteCodeIndex = _sender->byteCodeIndex();
    GrowableArray <RecompilationScope *> * subScopes         = _sender->rscope->subScopes( senderByteCodeIndex );
    if ( subScopes->length() == 1 and subScopes->first()->method() == method ) {
        RecompilationScope * rs = subScopes->first();
        st_assert( rs->method() == method, "mismatched block methods" );
        return rs;
    }

    // must compute rscope differently -- primitiveValue has no inline cache, so must get callee NativeMethod or method manually
    InlinedScope       * parent  = receiver->preg()->scope();
    RecompilationScope * rparent = parent->rscope;
    const int level = rparent->isNullScope() ? 1 : ( ( NonDummyRecompilationScope * ) rparent )->level();
    if ( rparent->isCompiled() ) {
        // get type feedback info from compiled block NativeMethod
        NativeMethod * parentNM = rparent->get_nativeMethod();
        // search for corresponding noninlined block NativeMethod
        int blockIndex = 0;
        for ( blockIndex = parentNM->number_of_noninlined_blocks(); blockIndex >= 1; blockIndex-- ) {
            if ( parentNM->noninlined_block_method_at( blockIndex ) == method and checkSenderPath( parent, parentNM->noninlined_block_scope_at( blockIndex )->parent() ) ) {
                break;      // found it
            }
        }
        if ( blockIndex >= 1 ) {
            // try to use non-inlined block NativeMethod (if it was compiled)
            JumpTableEntry * jte = parentNM->noninlined_block_jumpEntry_at( blockIndex );
            if ( jte->is_NativeMethod_stub() ) {
                // use non-inlined block NativeMethod
                NativeMethod * blockNM = jte->method();
                st_assert( blockNM->method() == method, "method mismatch" );
                return new InlinedRecompilationScope( nullptr, senderByteCodeIndex, blockNM, blockNM->scopes()->root(), level );
            } else {
                // block NativeMethod hasn't been compiled yet -- use interpreted method
                return new InterpretedRecompilationScope( nullptr, senderByteCodeIndex, key, method, level, true );
            }
        } else {
            // block was inlined in previous NativeMethod, but not found in rscope
            st_assert( _sender->rscope->isNullScope() or _sender->rscope->isLite(), "should have found subscope" );
            return new InterpretedRecompilationScope( nullptr, senderByteCodeIndex, key, method, level, true );
        }
    } else {
        // get info from interpreted block method
        return new InterpretedRecompilationScope( nullptr, senderByteCodeIndex, key, method, level, true );
    }
}


bool_t Inliner::checkSenderPath( Scope * here, ScopeDescriptor * there ) const {
    // return true if sender paths of here and there match
    // NB: I believe the code below isn't totally correct, in the sense that it
    // will return ok == true if the sender paths match up to the point where
    // one path crosses an NativeMethod boundary -- but there could be a difference
    // further up.
    // However, this imprecision isn't dangerous -- we'll just use type information
    // that may not be accurate.  But since the sender paths agreed up to that
    // point, it is likely the types are at least similar.
    // Fix this later.  -Urs 8/96
    while ( here and not there->isTop() ) {
        InlinedScope * sen = here->sender();
        if ( sen == nullptr )
            break;
        if ( sen->byteCodeIndex() not_eq there->senderByteCodeIndex() )
            return false;
        here  = here->sender();
        there = there->sender();
    }
    return true;
}


InlinedScope * Inliner::makeScope( const Expression * receiver, const KlassOop klass, const LookupKey * key, const MethodOop method ) {

    // create an InlinedScope for this method/receiver
    SendInfo * calleeInfo = new SendInfo( *_info );
    calleeInfo->_lookupKey = ( LookupKey * ) key;
    const KlassOop methodHolder = klass->klass_part()->lookup_method_holder_for( method );

    if ( method->is_blockMethod() ) {
        RecompilationScope * rs = makeBlockRScope( receiver, calleeInfo->_lookupKey, method );
        bool_t isNullRScope = rs->isNullScope();    // for conditional breakpoints (no type feedback info)
        if ( receiver->preg()->isBlockPReg() ) {
            InlinedScope * parent = receiver->preg()->scope();
            calleeInfo->_receiver = parent->self();
            _callee = BlockScope::new_BlockScope( method, methodHolder, parent, _sender, rs, calleeInfo );
            _callee->set_self( parent->self() );
        } else {
            // block parent is in a different NativeMethod -- don't inline
            return notify( "block parent in different NativeMethod" );
        }

    } else {
        // normal method
        RecompilationScope * rs = _sender->rscope->subScope( _sender->byteCodeIndex(), calleeInfo->_lookupKey );
        bool_t isNullRScope = rs->isNullScope();    // for conditional breakpoints (no type feedback info)
        _callee = MethodScope::new_MethodScope( method, methodHolder, _sender, rs, calleeInfo );
        _callee->set_self( receiver->asReceiver() );
    }

    return _callee;
}


InlinedScope * Inliner::notify( const char * msg ) {
    if ( CompilerDebug ) {
        cout( PrintInlining )->print( "%*s*cannot inline %s, cost = %ld (%s)\n", depth, "", _info->_selector->as_string(), inliningPolicy.calleeCost, msg );
    }
    _msg = ( const char * ) msg;
    return nullptr;    // cheap trick to make notify more convenient (can say "return notify(...)")
}


void Inliner::print() {
    _console->print_cr( "((Inliner*)%#x)", PrintHexAddresses ? this : 0 );
}


Expression * Inliner::inlineBlockInvocation( SendInfo * info ) {
    initialize( info, SendKind::NormalSend );
    Expression * blockExpression = info->_receiver;
    st_assert( blockExpression->preg()->isBlockPReg(), "must be a BlockPR" );
    const BlockPseudoRegister * block = ( BlockPseudoRegister * ) blockExpression->preg();
    const MethodOop method = block->closure()->method();
    const InlinedScope * parent = block->closure()->parent_scope();

    // should decide here whether to actually inline -- fix this
    if ( CompilerDebug )
        cout( PrintInlining )->print( "%*s*inlining block invocation\n", _sender->depth, "" );

    // Construct a fake lookupKey
    LookupKey * key = LookupKey::allocate( parent->selfKlass(), method );

    makeScope( blockExpression, parent->selfKlass(), key, method );
    if ( _callee ) {
        Expression * r = doInline( _sender->current() );
        return makeResult( r );
    } else {
        return nullptr;    // couldn't inline the block
    }
}
