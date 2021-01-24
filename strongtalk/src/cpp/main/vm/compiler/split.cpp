//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

/************* This file is not currently used and should not be in the build */

#if 0

#include "vm/compiler/split.hpp"

const std::uint32_t SplitSig::LevelMask = 0xf;

struct SplitSetting : StackObject {
    SplitSig *& sig;
    SplitSig * saved;
    SplitSetting( SplitSig *& oldsig, SplitSig * newsig ) : sig( oldsig ) {
        saved = oldsig;
        oldsig = newsig;
    }
    ~SplitSetting() { sig = saved; }
};

SplitSig * new_SplitSig( SplitSig * current, int splitID ) {
    int level = current->level() + 1;
    assert( level <= MaxSplitDepth, "max. split level exceeded" );
    std::uint32_t newID = splitID << ( ( MaxSplitDepth - level + 1 ) << 2 );
    SplitSig * sig =
                 ( SplitSig * )( ( std::uint32_t( current ) & ~SplitSig::LevelMask ) | newID | level );
    assert( current->contains( sig ), "should be in same branch" );
    return sig;
}

void SplitSig::print() {
    char buf[MaxSplitDepth + 1];
    lprintf( "SplitSig %#lx: %s", this, prefix( buf ) );
}

char * SplitSig::prefix( const char * buf ) {
    // fill buf with an ASCII representation of the receiver and return buf
    // e.g. a level-2 sig with first branch = 1 and 2nd branch = 3 --> "AB"
    int l = level();
    buf[ l-- ] = 0;
    std::uint32_t sig = std::uint32_t( this ) >> 4;
    while ( l >= 0 ) {
        buf[ l-- ] = 'A' + ( sig & 0xf );
        sig = sig >> 4;
    }
    return buf;
}

// compiler code for splitting

bool_t CodeScope::shouldSplit( SendInfo * info ) {
    assert( info->receiver->isMergeExpression(), "should be merge expr" );
    MergeExpression * r = ( MergeExpression * ) info->receiver;
    assert( r->isSplittable(), "should be splittable" );
    if ( not CompilerSplitting ) return false;
    if ( sig->level() == MaxSplitDepth ) return false;
    Node * current = theNodeGen->current;
    if ( not current->isSplittable() ) return false;

    int cost = theCompiler->inlineLimit[ InlineLimitIType::SplitCostLimit ];
    Node * n    = nullptr;
    // compute the cost of all nodes that would be copied (i.e. all exprs
    // with a map type)
    int       i;
    for ( std::size_t i = 0; i < r->exprs->length(); i++ ) {
        Expression * expr = r->exprs->at( i );
        if ( not expr->hasKlass() ) continue;    // won't copy these
        InlinedScope * theScope = expr->node()->scope();
        int theByteCodeIndex = expr->node()->byteCodeIndex();
        for ( Expression * e = expr; e; e = e->next ) {
            if ( e->node()->scope() not_eq theScope or e->node()->byteCodeIndex() not_eq theByteCodeIndex ) {
                // make sure all subexpressions have the same scope
                // (otherwise can't describe live range of that value when split)
                // could fix this with better splitting (introduce temps to
                // "synchronize" the value's scopes)
                if ( PrintInlining ) {
                    lprintf( "%*s*not splitting %s: too complicated (scopes)\n",
                             depth, "", info->sel->copy_null_terminated() );
                }
                r->setSplittable( false );    // no sense trying again
                return false;
            }
            Node * prev;
            for ( n = e->node(); cost > 0 and n and n not_eq current; n = n->next() ) {
                cost -= n->cost();
                if ( not n->isSplittable() ) {
                    if ( PrintInlining ) {
                        lprintf( "%*s*not splitting %s: unsplittable node\n",
                                 depth, "", info->sel->copy_null_terminated() );
                    }
                    return false;
                }
                prev = n;        // for easier debugging
            }
            assert( n, "why didn't we find current?" );
            if ( n == nullptr or cost < 0 ) goto done;
        }
    }

done:
    if ( n not_eq current or cost < 0 ) {
        if ( PrintInlining ) {
            lprintf( "%*s*not splitting %s: cost too high (>%ld)\n", depth, "",
                     info->sel->copy_null_terminated(),
                     theCompiler->inlineLimit[ InlineLimitIType::SplitCostLimit ] - cost );
        }
        if ( n == current ) theCompiler->registerUninlinable( info, InlineLimitIType::SplitCostLimit, cost );
        return false;
    }

    return true;
}

Expression * CodeScope::splitMerge( SendInfo * info, MergeNode *& merge ) {
    // Split this send: copy nodes between sources of r's parts and the current
    // node, then inline the send along all paths; merge paths back at merge,
    // result in info->resReg.
    MergeExpression * r = ( MergeExpression * ) info->receiver;
    assert( r->isSplittable(), "should be splittable" );

    // performance bug - fix this: can't split on MergeExpression more than once
    // because after changing the CFG the old expr points to the wrong nodes.
    // For now, just prohibit future splitting on this expr.
    r->setSplittable( false );

    Expression * res = nullptr;
    int ncases = r->exprs->length();
    memoizeBlocks( info->sel );
    if ( PrintInlining ) {
        lprintf( "%*s*splitting %s\n", depth, "", selector_string( info->sel ) );
    }
    Node * current = theNodeGen->current;
    bool_t first = true;
    GrowableArray <Oop> * splitReceiverKlasss = new GrowableArray <Oop>( 10 );// receiver map of each branch
    GrowableArray <PseudoRegister *> * splitReceivers = new GrowableArray <PseudoRegister *>( 10 );    // receiver reg of each branch
    GrowableArray <Node *>           * splitHeads = new GrowableArray <Node *>( 10 );    // first node of each branch
    bool_t needKlassLoad                          = false;

    for ( std::size_t i = 0; i < ncases; i++ ) {
        Expression * nth = r->exprs->at( i );
        assert( not nth->isConstantExpression() or nth->next == nullptr or
                nth->constant() == nth->next->constant(),
                "shouldn't happen: merged consts - convert to map" );
        InlinedScope * s;
        if ( nth->hasKlass() and
             ( s = tryLookup( info, nth ) ) not_eq nullptr ) {
            // Create a new PseudoRegister&Expression for the receiver so that it has the right
            // scope (the nodes will replace the old result reg with the new
            // one while they are copied).
            // (Since we're splitting starting at the original producer, the
            // original result may be in an arbitrary subscope of the receiver
            // or even in a sibling.  For reg. allocation et al., the result
            // must have a PseudoRegister that's live from the producing point to here.)
            SplitSetting setting( theCompiler->splitSig, new_SplitSig( sig, i + 1 ) );
            if ( s->isCodeScope() ) ( ( CodeScope * ) s )->sig = theCompiler->splitSig;
            SplitPReg  * newPR   = coveringRegFor( nth, theCompiler->splitSig );
            Expression * newReceiver = nth->shallowCopy( newPR, nth->node() );

            Node * mapMerge = new MergeNode;        // where all copied paths merge
            splitHeads->append( mapMerge );
            splitReceivers->append( newPR );
            if ( nth->isConstantExpression() ) {
                splitReceiverKlasss->append( nth->constant() );
            } else {
                splitReceiverKlasss->append( nth->klass() );
                Klass * m = nth->klass()->addr();
                needKlassLoad |= m not_eq smiKlassObject and m not_eq doubleKlassObject;
            }

            // split off paths of all Exprs with this map up to merge point
            Node * rmerge = r->node();
            assert( rmerge, "should have a node" );
            for ( Expression * expr = nth; expr; expr = expr->next ) {
                Node           * n     = expr->node();
                PseudoRegister * oldPR = expr->preg();
                assert( n->isSplittable(), "can't handle branches etc. yet" );
                Node * frst = n->next();
                n->removeNext( frst );
                // replace n's destination or insert an assignment
                if ( n->hasDest() and n->dest() == oldPR ) {
                    n->setDest( nullptr, newPR );
                } else if ( newPR ) {
                    n = n->append( new AssignNode( oldPR, newPR ) );
                }
                n           = copyPath( n, frst, rmerge, oldPR, newPR, r, newReceiver );
                n           = n->append( mapMerge );
            }

            // copy everything between mapMerge and current
            theNodeGen->current = copyPath( mapMerge, rmerge, current,
                                            nullptr, nullptr, r, newReceiver );

            // now inline the send
            Expression * e = doInline( s, newReceiver, theNodeGen->current, nullptr );
            if ( not e->isNoResultExpression() ) {
                theNodeGen->append( new NopNode );
                e   = e->shallowCopy( info->resReg, theNodeGen->current );
                res = res ? res->mergeWith( e, merge ) : e;
            }
            theNodeGen->branch( merge );
        } else {
            // can't inline - need to append a real send after current
            if ( not nth->isUnknownUnlikely() ) info->needRealSend = true;
        }
    }

    UnknownExpression * u = r->findUnknown();
    if ( u and splitReceiverKlasss->length() > 0 ) {
        // insert a type test after oldMerge (all unknown paths should meet
        // at that node)
        // Performance bug: the known-but-uninlinable sends will also go
        // through the type test; they should be redirected until after the
        // test.  The problem is that oldMerge may not be the actual merge
        // point but slightly later (i.e. a few InlinedReturns later).
        int diff;
        if ( WizardMode and PrintInlining and
             ( diff = r->exprs->length() - splitReceiverKlasss->length() ) > 1 ) {
            lprintf( "*unnecessary %d-way type test for %d cases\n",
                     splitReceiverKlasss->length(), diff );
        }
        Node * oldMerge = r->node();
        Node * oldNext  = oldMerge->next();
        if ( oldNext ) oldMerge->removeNext( oldNext );
        PseudoRegister * pr       = r->preg();
        Node           * typeCase = new TypeTestNode( pr, splitReceiverKlasss, needKlassLoad, true );
        oldMerge->append( typeCase );
        if ( info->needRealSend or not theCompiler->useUncommonTraps ) {
            // connect fall-through (unknown) case to old merge point's successor
            info->needRealSend = true;
            if ( oldNext ) {
                typeCase->append( oldNext );
            } else {
                assert( current == oldMerge, "oops" );
                current = typeCase->append( new NopNode );
            }
        } else {
            // make unknown case uncommon
            if ( oldNext ) {
                // must copy nodes between old merge point (i.e. end of the send
                // generating the receiver value) and current node; this code
                // computes the args of the current send
                theNodeGen->current = copyPath( typeCase, oldNext, current,
                                                nullptr, nullptr, r, u );
            } else {
                assert( current == oldMerge, "oops" );
                theNodeGen->current = typeCase;
            }
            theNodeGen->uncommonBranch( currentExprStack( 0 ), info->restartPrim );
            if ( PrintInlining ) {
                lprintf( "%*s*making %s uncommon (3)\n",
                         depth, "", selector_string( info->sel ) );
            }
        }
        for ( std::size_t j = 0; j < splitReceiverKlasss->length(); j++ ) {
            Node * n = new AssignNode( pr, splitReceivers->at( j ) );
            typeCase->append( j + 1, n );
            n->append( splitHeads->at( j ) );
        }
    }
    if ( info->needRealSend ) {
        // The non-inlined send will be generated along the original (merged)
        // path, which will then branch to "merge".
        theNodeGen->current = current;
    } else {
        // discard the original path - can no longer reach it
        theNodeGen->current = merge;
    }

    if ( res and res->isMergeExpression() ) res->setNode( merge, info->resReg );
    return res;
}

Node * CodeScope::copyPath( Node * n, Node * start, Node * end,
                            PseudoRegister * oldPR, PseudoRegister * newPR,
                            MergeExpression * receiver, Expression * newReceiver ) {
    // copy the path from start to end, replacing occurrences of oldPR
    // with newPR; append copies to n, return last node
    if ( CompilerDebug ) {
        char * s = new_resource_array<char>( 100 );
        sprintf( s, "start of copied code: %#lx(N%d) --> %#lx(N%d) @ %#lx(N%d)",
                 start, start->id(), end, end->id(), n, n->id() );
        n = n->append( new CommentNode( s ) );
    }
    assert( not oldPR or not oldPR->isBlockPseudoRegister(), "cannot handle BlockPseudoRegisters" );
    for ( Node * c = start; true; c = c->next() ) {
        assert( c->isSplittable(), "can't handle branches yet" );
        Node * copy = c->copy( oldPR, newPR );
        if ( copy ) n = n->append( copy );
        if ( c == end ) break;
    }
    if ( CompilerDebug ) n          = n->append( new CommentNode( "end of copied code" ) );
    return n;
}

SplitPReg * CodeScope::coveringRegFor( Expression * expr, SplitSig * sg ) {
    // create a PseudoRegister with a live range covering all nodes between the
    // producer and the receiver scope/byteCodeIndex
    // see also SinglyAssignedPseudoRegister::isLiveAt
    InlinedScope * s = expr->node()->scope();
    int byteCodeIndex = expr->node()->byteCodeIndex();
    assert( s->isCodeScope(), "oops" );
    SplitPReg * r = regCovering( this, _byteCodeIndex, ( CodeScope * ) s, byteCodeIndex, sg );

    for ( Expression * e = expr; e; e = e->next ) {
        InlinedScope * s2 = e->node()->scope();
        int byteCodeIndex2 = e->node()->byteCodeIndex();
        assert( s2 == s, "oops" );
        assert( byteCodeIndex2 == byteCodeIndex, "oops" );
    }

    return r;
}

#endif
