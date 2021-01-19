//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/defUse.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/CopyPropagationInfo.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/compiler/RegisterAllocator.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/compiler/BasicBlock.hpp"


int BasicBlock::genCounter = 0;


void BasicBlock::init( Node *first, Node *last, int n ) {
    _first                 = first;
    _last                  = last;
    _nodeCount             = n;
    _id                    = 0;
    _genCount              = 0;
    BasicBlock::genCounter = 0;
    _loopDepth             = 0;
}


BasicBlock *BasicBlock::next() const {
    Node *n = _last->next();
    return n ? n->bb() : nullptr;
}


BasicBlock *BasicBlock::firstPrev() const {
    Node *n = _first->firstPrev();
    return n ? n->bb() : nullptr;
}


BasicBlock *BasicBlock::next( std::size_t i ) const {
    Node *n = _last->next( i );
    return n ? n->bb() : nullptr;
}


BasicBlock *BasicBlock::prev( std::size_t i ) const {
    Node *n = _first->prev( i );
    return n ? n->bb() : nullptr;
}


int BasicBlock::nSuccessors() const {
    return _last->nSuccessors();
}


int BasicBlock::nPredecessors() const {
    return _first->nPredecessors();
}


bool_t BasicBlock::isSuccessor( const BasicBlock *bb ) const {
    return _last->isSuccessor( bb->_first );
}


bool_t BasicBlock::isPredecessor( const BasicBlock *bb ) const {
    return _first->isPredecessor( bb->_last );
}


// note: the functions below are defined here rather than in PseudoRegister to localize this temporary hack
constexpr int MaxSearch = 50;    // max. # of nodes to search backwards

NonTrivialNode *findDefinitionOf( Node *endNode, const PseudoRegister *r, int max = MaxSearch ) {
    // search backwards for a definition of r
    Node      *current = endNode;
    for ( std::size_t i        = 0; i < max and current not_eq nullptr; i++ ) {
        if ( current->_deleted )
            continue;
        if ( current->hasSinglePredecessor() ) {
            if ( ( current->isAssignNode() or current->isInlinedReturnNode() ) and ( (NonTrivialNode *) current )->dest() == r ) {
                return (NonTrivialNode *) current;
            }
        } else {
            // merge node with several predecessors: search each one and make sure all find the same def
            st_assert( current->isMergeNode(), "must be a merge" );
            MergeNode      *merge     = (MergeNode *) current;
            NonTrivialNode *candidate = nullptr;    // possible def of r
            for ( int      j          = merge->nPredecessors() - 1; j >= 0; j-- ) {
                Node           *prev = merge->prev( j );
                NonTrivialNode *cand = findDefinitionOf( prev, r, max - i );
                if ( cand == nullptr or ( candidate and cand not_eq candidate ) )
                    return nullptr;
                candidate = cand;
            }
            return candidate;    // all paths lead to the same candidate
        }
        current = current->firstPrev();
    }
    return nullptr;    // search limit exceeded
}


void propagateTo( BasicBlock *useBasicBlock, Usage *use, NonTrivialNode *fromNode, PseudoRegister *src, NonTrivialNode *toNode ) {

    // r1 := r2; ...; r3 := op(r1)  -->  r1 := r2; ...; r3 := op(r2)
    if ( toNode->canCopyPropagate( fromNode ) ) {
        if ( not src->extendLiveRange( toNode ) ) {
            if ( CompilerDebug ) {
                cout( PrintCopyPropagation )->print( "*brute-force copy-propagation: cannot propagate %s from N%ld to N%ld because of extendLiveRange\n", src->name(), fromNode->id(), toNode->id() );
            }
            return;
        }
        PseudoRegister *replaced = fromNode->dest();
        bool_t         ok        = toNode->copyPropagate( useBasicBlock, use, src, true );

        if ( not ok ) {
            // This if statement has been added by Lars Bak 29-4-96 to work around the type check node elimination problem. (Ask Urs for details).
            return;
        }
        st_assert( ok, "should have worked" );
        if ( CompilerDebug ) {
            cout( PrintCopyPropagation )->print( "*brute-force copy-propagation: propagate %s from N%ld to N%ld\n", src->name(), fromNode->id(), toNode->id() );
        }

        // if this was the last use, make sure its value can be recovered for debugging
        if ( replaced->_debug and replaced->hasNoUses() ) {
            st_assert( not replaced->_copyPropagationInfo, "already has cpInfo" );
            replaced->_copyPropagationInfo = new_CPInfo( fromNode );
            st_assert( replaced->_copyPropagationInfo, "should have cpInfo now" );
        }

    } else {
        if ( CompilerDebug )
            cout( PrintCopyPropagation )->print( "*Node N%d cannot copy-propagate\n", toNode->id() );
    }
}


bool_t regAssignedBetween( const PseudoRegister *r, const Node *startNode, Node *endNode ) {
    // check if r is assigned somewhere between start and end node
    // quite inefficient
    // search backwards from end to start
    BasicBlock *bbWithoutDefs = nullptr;    // BasicBlock w/o definitions of r

    for ( Node *n = endNode; n not_eq startNode; n = n->firstPrev() ) {

        // see if n's bb has a def or r
        BasicBlock *bb = n->bb();
        if ( bb == bbWithoutDefs )
            continue; // no definitions here
        bool_t    hasDefs = false;
        for ( std::size_t i       = 0; i < bb->duInfo.info->length(); i++ ) {// forall def/use info lists
            DefinitionUsageInfo *dui = bb->duInfo.info->at( i );
            if ( dui->_pseudoRegister == r and not dui->_definitions.isEmpty() ) {
                // yes, it has a def

                hasDefs = true;
                for ( SListElem<Definition *> *d = dui->_definitions.head(); d; d = d->next() ) {
                    if ( d->data()->_node == n )
                        return true;
                }
                break;    // no need to search the other duInfo entries
            }
        }
        if ( not hasDefs ) {
            // r has no def in this BasicBlock, avoid search next time
            bbWithoutDefs = bb;
        }
    }
    return false;        // no def found
}


void BasicBlock::bruteForceCopyPropagate() {
    const int len = duInfo.info->length();

    for ( std::size_t i = 0; i < len; i++ ) {        // forall def/use info lists
        DefinitionUsageInfo  *dui = duInfo.info->at( i );
        const PseudoRegister *r   = dui->_pseudoRegister;
        if ( not r->isSinglyAssignedPseudoRegister() or not r->_location.equals( unAllocated ) ) {
            // optimize only SinglyAssignedPseudoRegisters for now
            // preallocated PseudoRegister may have aliases - don't do copy-propagation
            continue;
        }

        // try to find a def them with other PseudoRegisters
        if ( dui->_usages.isEmpty() )
            continue;    // has only definitions
        const Usage    *use          = dui->_usages.head()->data();
        Node           *firstUseNode = use->_node;
        NonTrivialNode *defNode      = findDefinitionOf( firstUseNode, r );
        if ( defNode == nullptr )
            continue;    // no def found

        PseudoRegister *candidate = ( (NonTrivialNode *) defNode )->src();
        if ( not candidate->_location.equals( unAllocated ) )
            continue;
        st_assert( candidate->isUsed(), "should not be unused" );
        if ( not regAssignedBetween( candidate, defNode, firstUseNode ) ) {
            // ok: the candidate reaches the use at firstUseNode
            // try to propagate it to all uses of r in this BasicBlock
            SListElem<Usage *>       *nextu;
            for ( SListElem<Usage *> *u = dui->_usages.head(); u; u = nextu ) {
                nextu = u->next();    // because we're mutating the list
                Usage          *thisUse  = u->data();
                NonTrivialNode *thisNode = thisUse->_node;
                if ( not regAssignedBetween( candidate, firstUseNode, thisNode ) ) {
                    propagateTo( this, thisUse, defNode, candidate, thisNode );
                } else {
                    // candidate can't reach anything downstream of this use
                    break;
                }
            }
        }
    }
}


void BasicBlock::localCopyPropagate() {
    // perform local copy propagation using the local def/use information
    // should be rewritten to make just one pass over the nodes, keeping track of
    // aliases created by assignments -- fix this
    const int       len       = duInfo.info->length();
    SimpleBitVector used      = 0;        // hardwired registers used
    SimpleBitVector usedTwice = 0;

    for ( std::size_t i = 0; i < len; i++ ) {
        PseudoRegister *r = duInfo.info->at( i )->_pseudoRegister;
        if ( not r->_location.equals( unAllocated ) and r->_location.isRegisterLocation() ) {
            if ( used.isAllocated( r->_location.number() ) ) {
                // two PseudoRegisters have same preallocated reg - algorithm below can't handle this
                usedTwice = usedTwice.allocate( r->_location.number() );
            } else {
                used = used.allocate( r->_location.number() );
            }
        }
    }

    for ( std::size_t i = 0; i < len; i++ ) {
        constexpr int       BIG  = 9999999;
        DefinitionUsageInfo *dui = duInfo.info->at( i );
        PseudoRegister      *r   = dui->_pseudoRegister;
        if ( not r->_location.equals( unAllocated ) and r->_location.isRegisterLocation() and usedTwice.isAllocated( r->_location.number() ) ) {
            // this preallocated PseudoRegister has aliases - don't do copy-propagation
            continue;
        }
        SListElem<Usage *>      *u = dui->_usages.head();
        SListElem<Definition *> *nextd;
        SListElem<Definition *> *d;
        for ( d = dui->_definitions.head(); d and u; d = nextd ) {
            // try to find a use of the def at d
            nextd                        = d->next();
            const Definition    *def     = d->data();
            SList<Definition *> *srcDefs = nullptr;    // redefinition of src that defines this def (if any)
            if ( def->_node->hasSrc() ) {
                PseudoRegister *src = def->_node->src();
                if ( src->_location.isRegisterLocation() and usedTwice.isAllocated( src->_location.number() ) ) {
                    // r := f(r2), and r2 is aliased preallocated - can't handle
                    continue;
                }
                if ( not src->isSinglyAssigned() ) {
                    // careful: r is only equivalent to src as std::int32_t as src isn't reassigned within this basic block
                    int j = 0;
                    for ( ; j < len; j++ ) {
                        PseudoRegister *r = duInfo.info->at( j )->_pseudoRegister;
                        if ( r == src )
                            break;
                    }
                    st_assert( j < len, "should have found duInfo for src" );
                    srcDefs = &duInfo.info->at( j )->_definitions;
                    if ( srcDefs->length() == 0 ) {
                        srcDefs = nullptr;   // ok, src is constant during this BasicBlock
                    }
                }
            }

            const int d_id = def->_node->num();
            int       u_id;
            // find a use in a node following the current def
            for ( ; u and ( u_id = u->data()->_node->num() ) <= d_id; u = u->next() );
            if ( not u )
                break;      // no such use in this BasicBlock

            // visit all uses with a node ID between here and the next def of either the
            // current PseudoRegister or the source PseudoRegister that defines it
            int stop_id = nextd ? nextd->data()->_node->num() : BIG;
            if ( srcDefs ) {
                for ( d = srcDefs->head(); d and d->data()->_node->num() < u_id; d = d->next() );
                if ( d )
                    stop_id = min( stop_id, d->data()->_node->num() );
            }

            while ( u_id <= stop_id ) {
                // the def at d_id reaches the use at u_id
                st_assert( d_id < u_id, "just checking" );
                dui->propagateTo( this, r, def, u->data(), false );
                u    = u->next();
                u_id = u ? u->data()->_node->num() : BIG + 1;
            }
        }
    }
}


void BasicBlock::makeUses() {
    // collect definitions and uses for all pregs (and fill pregTable in the process)
    st_assert( duInfo.info == nullptr, "shouldn't be set" );
    duInfo.info = new GrowableArray<DefinitionUsageInfo *>( _nodeCount + 10 );
    for ( Node *n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n->_deleted )
            continue;
        n->makeUses( this );
    }
}


void BasicBlock::renumber() {
    int        count = 0;
    for ( Node *n    = _first; n not_eq _last->next(); n = n->next() )
        n->setNum( count++ );
    _nodeCount = count;
}


void BasicBlock::remove( Node *n ) {
    // remove this node and its definitions & uses
    // NB: nodes aren't actually removed from the graph but just marked as
    // deleted.  This is much simpler because the topology of the flow graph
    // doesn't change this way
    st_assert( contains( n ), "node isn't in this BasicBlock" );
    n->removeUses( this );
    n->_deleted = true;
}


void BasicBlock::addAfter( Node *prev, Node *newNode ) {
    // prev == nullptr means add as first node
    st_assert( _nodeCount, "shouldn't add anything to this BasicBlock" );
    st_assert( prev == nullptr or contains( prev ), "node isn't in this BasicBlock" );
    if ( prev ) {
        prev->insertNext( newNode );
        if ( prev == _last )
            _last = newNode;
    } else {
        _first->insertPrev( newNode );
        _first = newNode;
    }
    if ( bbIterator->_usesBuilt ) {
        newNode->makeUses( this );
    } else {
        newNode->setBasicBlock( this );
    }
    renumber();
    st_assert( newNode->bb() == this, "should point to me now" );
}


static BasicBlock *thisBasicBlock;


static void duChecker( PseudoRegisterBasicBlockIndex *p ) {
    if ( p->_basicBlock == thisBasicBlock ) st_fatal( "should not be in middle of list" );
}


static bool_t findMyBasicBlock( void *bb, PseudoRegisterBasicBlockIndex *p ) {
    return p->_basicBlock == (BasicBlock *) bb;
}


int BasicBlock::addUDHelper( PseudoRegister *r ) {
    // we're computing the uses block by block, and the current BasicBlock's
    // PseudoRegisterBasicBlockIndex is always the last entry in the preg's list.
    st_assert( _nodeCount, "shouldn't add anything to this BasicBlock" );
    bbIterator->pregTable->at_put_grow( r->id(), r );
    PseudoRegisterBasicBlockIndex *p;
    if ( bbIterator->_usesBuilt ) {
        // find entry for the PseudoRegister
        std::size_t i = r->_dus.find( this, findMyBasicBlock );
        if ( i >= 0 ) {
            p = r->_dus.at( i );
        } else {
            // create new entry
            duInfo.info->append( new DefinitionUsageInfo( r ) );
            r->_dus.append( p = new PseudoRegisterBasicBlockIndex( this, duInfo.info->length() - 1, r ) );
        }
    } else {
        // while building the definitions & uses, the PseudoRegister's entry must be the last
        // one in the list (if there is an entry for this BasicBlock)
        if ( r->_dus.isEmpty() or r->_dus.last()->_basicBlock not_eq this ) {
            // PseudoRegister doesn't yet have an entry for this block
            thisBasicBlock = this;
            r->_dus.apply( duChecker );
            duInfo.info->append( new DefinitionUsageInfo( r ) );
            r->_dus.append( new PseudoRegisterBasicBlockIndex( this, duInfo.info->length() - 1, r ) );
        }
        p = r->_dus.last();
    }
    st_assert( p->_basicBlock == this, "wrong BasicBlock" );
    st_assert( duInfo.info->at( p->_index )->_pseudoRegister == r, "wrong PseudoRegister" );
    return p->_index;
}


Usage *BasicBlock::addUse( NonTrivialNode *n, PseudoRegister *r, bool_t soft ) {
    st_assert( not soft, "soft use" );
    st_assert( contains( n ), "node isn't in this BasicBlock" );
    if ( r->isNoPseudoRegister() )
        return nullptr;
    Usage *u = soft ? new PSoftUsage( n ) : new Usage( n );
    r->incUses( u );
    int index = addUDHelper( r );
    duInfo.info->at( index )->_usages.append( u );
    return u;
}


Definition *BasicBlock::addDef( NonTrivialNode *n, PseudoRegister *r ) {
    st_assert( contains( n ), "node isn't in this BasicBlock" );
    if ( r->isNoPseudoRegister() )
        return nullptr;
    Definition *d = new Definition( n );
    r->incDefs( d );
    int index = addUDHelper( r );
    duInfo.info->at( index )->_definitions.append( d );
    return d;
}


// allocate PseudoRegisters that are used & defined solely within this BasicBlock
void BasicBlock::localAlloc( GrowableArray<BitVector *> *hardwired, GrowableArray<PseudoRegister *> *localRegs, GrowableArray<BitVector *> *lives ) {
    // try fast allocation first -- use only local regs that aren't touched
    // by any pre-allocated (hardwired) registers

    // hardwired, localRegs, lives: just passed on to slowLocalAlloc

    // Note: Fix problem with registers that are used after a call in PrimNodes
    // such as ContextCreateNode, etc. Problem is that values like self might
    // be allocated in registers but the registers are trashed after a call.
    // Right now: PrologueNode terminates BasicBlock to fix the problem.

    if ( not _nodeCount )
        return;            // empty BasicBlock

    GrowableArray<RegCandidate *>    cands( _nodeCount );
    GrowableArray<RegisterEqClass *> regClasses( _nodeCount + 1 );
    regClasses.append( nullptr );        // first reg class has index 1

    int use_count[REGISTER_COUNT], def_count[REGISTER_COUNT];
//    std::array <int, REGISTER_COUNT> use_count, def_count;

    for ( std::size_t i = 0; i < REGISTER_COUNT; i++ ) {
        use_count[ i ] = def_count[ i ] = 0;
    }

    for ( Node *nn = _first; nn not_eq _last->next(); nn = nn->next() ) {
        if ( nn->_deleted )
            continue;
        nn->markAllocated( use_count, def_count );
        if ( nn->isAssignNode() ) {
            NonTrivialNode *n        = (NonTrivialNode *) nn;
            PseudoRegister *src      = n->src();
            PseudoRegister *dest     = n->dest();
            bool_t         localSrc  = src->isLocalTo( this );
            bool_t         localDest = dest->isLocalTo( this );
            if ( src->_location.isRegisterLocation() ) {
                if ( dest->_location.equals( unAllocated ) and localDest ) {
                    // PR = PR2(reg)
                    // allocate dest->loc to src->loc, but only if src->loc
                    // isn't defined again
                    cands.append( new RegCandidate( dest, src->_location, def_count[ src->_location.number() ] ) );
                }
            } else if ( dest->_location.isRegisterLocation() ) {
                if ( src->_location.equals( unAllocated ) and localSrc ) {
                    // PR2(reg) = PR
                    // should allocate src->loc to dest->loc, but only if dest->loc
                    // has single definition (this one) and isn't used before
                    // this point   [simplification]
                    if ( def_count[ dest->_location.number() ] not_eq 1 or use_count[ dest->_location.number() ] ) {
                        // not eligible for special treatment
                    } else {
                        cands.append( new RegCandidate( src, dest->_location, 1 ) );
                    }
                }
            } else if ( localSrc and localDest ) {
                // both regs are local and unallocated - put them in same
                // equivalence class
                // fix this - must check for overlapping live ranges first
            } else {
                // non-local registers - skip
            }
        }
    }

    // now examine all candidates and allocate them to preferred register
    // if possible
    while ( cands.nonEmpty() ) {
        RegCandidate *c = cands.pop();
        if ( def_count[ c->_location.number() ] == c->_ndefs ) {
            doAlloc( c->_pseudoRegister, c->_location );
        }
    }

    // allocate other local regs (using the untouched temp regs of this BasicBlock)
    int       temp = 0;
    for ( std::size_t i    = 0; i < duInfo.info->length(); i++ ) {
        // collect local regs
        PseudoRegister *r = duInfo.info->at( i )->_pseudoRegister;
        if ( r->_location.equals( unAllocated ) and not r->isUnused() and r->isLocalTo( this ) ) {
            st_assert( r->_dus.first()->_index == i, "should be the same" );
            for ( ; temp < nofLocalRegisters and use_count[ Mapping::localRegister( temp ).number() ] + def_count[ Mapping::localRegister( temp ).number() ] > 0; temp++ );
            if ( temp == nofLocalRegisters )
                break;        // ran out of regs
            // ok, allocate Mapping::localRegisters[temp] to the preg and equivalent pregs
            Location             t     = Mapping::localRegister( temp++ );
            PseudoRegister       *frst = r->regClass ? regClasses.at( r->regClass )->first : r;
            for ( PseudoRegister *pr   = frst; pr; pr = pr->regClassLink ) {
                doAlloc( pr, t );
                pr->regClass = 0;
            }
        }
        r->regClass       = 0;
    }

    if ( temp == nofLocalRegisters ) {
        // ran out of local regs with the simple strategy - try using slow
        // allocation algorithm
        slowLocalAlloc( hardwired, localRegs, lives );
    }
}


constexpr auto nextTemp( auto const & n ) {
    return (n == nofLocalRegisters - 1) ? 0 : n + 1;
}

// slower but more capable version of local allocation; keeps track of live
// ranges via a bit map
// note: temporary data structs are passed in so they can be reused for all
// BBs (otherwise would allocate too much junk in resource area)
void BasicBlock::slowLocalAlloc( GrowableArray<BitVector *> *hardwired, GrowableArray<PseudoRegister *> *localRegs, GrowableArray<BitVector *> *lives ) {
    // clear temporary data structures
    localRegs->clear();
    lives->clear();
    for ( std::size_t i = 0; i < nofLocalRegisters; i++ ) {
        hardwired->at( i )->setLength( _nodeCount );
        hardwired->at( i )->clear();
    }
    // hardwired->at(i): indexed by reg no, gives nodes in which register is busy
    // localRegs: collects all PseudoRegisters that could be allocated locally
    // lives: for each reg in localRegs, holds live range (bit vector with one bit per node)

    for ( std::size_t i = 0; i < duInfo.info->length(); i++ ) {
        // collect local regs
        PseudoRegister *r = duInfo.info->at( i )->_pseudoRegister;
        if ( r->isLocalTo( this ) ) {
            st_assert( r->_dus.first()->_index == i, "should be the same" );
            if ( r->isUnused() ) {
                // unused register - ignore
            } else {
                DefinitionUsageInfo *info = duInfo.info->at( r->_dus.first()->_index );
                localRegs->append( r );
                BitVector *bv = new BitVector( _nodeCount );
                lives->append( bv );
                int firstUse = 0, lastUse = _nodeCount - 1;
                duInfo.info->at( i )->getLiveRange( firstUse, lastUse );
                bv->addFromTo( firstUse, lastUse );
            }
        } else if ( r->_location.isLocalRegister() ) {
            // already allocated (i.e., hardwired register)
            st_assert( not r->_location.equals( unAllocated ), "unAllocated should not count as isRegister()" );
            int firstUse = 0, lastUse = _nodeCount - 1;
            if ( not r->incorrectDU() ) {
                duInfo.info->at( i )->getLiveRange( firstUse, lastUse );
            } else {
                // can't really compute live range since the temp might be non-local
                // so assume it's live from first node til the end
            }
            hardwired->at( Mapping::localRegisterIndex( r->_location ) )->addFromTo( firstUse, lastUse );
        }
    }

    // now, localRegs holds all local regs, and lives contains each register's
    // live range (one bit per node, 1 = reg is live); hardwired contains
    // the ranges where temp regs are already taken (e.g. for NonLocalReturn, calls, etc)

    // just add trashed registers now
    for ( Node *n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n->_deleted )
            continue;
        SimpleBitVector v = n->trashedMask();
        if ( v.isEmpty() )
            continue;    // nothing trashed (normal case)
        for ( std::size_t i = 0; i < nofLocalRegisters; i++ ) {
            if ( v.isAllocated( i ) )
                hardwired->at( i )->add( n->num() );
        }
    }


    // cycle through the temp registers to (hopefully) allow more optimizations
    // later (e.g. scheduling)
    int lastTemp = 0;


    for ( std::size_t i = 0; i < localRegs->length(); i++ ) {
        // try to allocate localRegs[i] to a local (temp) register
        PseudoRegister *r = localRegs->at( i );
        if ( not r->_location.equals( unAllocated ) ) {
            st_assert( r->regClass == 0, "should have been cleared" );
            continue;
        }
        BitVector *liveRange = lives->at( i );
        for ( std::size_t tempNo     = lastTemp, ntries = 0; ntries < nofLocalRegisters; tempNo = nextTemp( tempNo ), ntries++ ) {
            if ( liveRange->isDisjointFrom( hardwired->at( tempNo ) ) ) {
                Location temp = Mapping::localRegister( tempNo );
                doAlloc( r, temp );
                hardwired->at( tempNo )->unionWith( liveRange );
                lastTemp = nextTemp( tempNo );
                break;
            }
        }
        if ( CompilerDebug ) {
            if ( r->_location.equals( unAllocated ) ) {
                cout( PrintLocalAllocation )->print( "*could NOT find local assignment for local %s in BasicBlock%ld\n", r->name(), id() );
            }
        }
        r->regClass = 0;
    }
}


void BasicBlock::doAlloc( PseudoRegister *r, Location l ) {
    if ( CompilerDebug )
        cout( PrintLocalAllocation )->print( "*assigning %s to local %s in BasicBlock%ld\n", l.name(), r->name(), id() );
    st_assert( not r->_debug, "should not allocate to temp reg" );
    r->_location = l;
}


void BasicBlock::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *l ) {
    // add all escaping blocks to l
    for ( Node *n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n->_deleted )
            continue;
        n->computeEscapingBlocks( l );
    }
}


void BasicBlock::apply( NodeVisitor *v ) {
    if ( _nodeCount > 0 ) {
        Node       *end = _last->next();
        for ( Node *n   = _first; n not_eq end; n = n->next() ) {
            if ( not n->_deleted ) {
                v->beginOfNode( n );
                n->apply( v );
                v->endOfNode( n );
            }
        }
    } else {
        st_assert( _nodeCount == 0, "nnodes should be 0" );
        Node       *end = _last->next();
        for ( Node *n   = _first; n not_eq end; n = n->next() ) {
            st_assert( n->_deleted, "basic block is not empty even though nnodes == 0!" );
        }
    }
}


bool_t BasicBlock::verifyLabels() {
    bool_t ok = true;
    if ( _nodeCount > 0 ) {
        for ( Node *n = _first; n not_eq _last->next(); n = n->next() ) {
            if ( n->_deleted )
                continue;
            if ( n->_label.is_unbound() ) {
                ok = false;
                lprintf( "unbound label at N%d\n", n->id() );
            }
        }
    }
    return ok;
}


static void printPrevBBs( BasicBlock *b, const char *str ) {
    lprintf( "BasicBlock%ld%s", b->id(), str );
}


void BasicBlock::print_short() {
    lprintf( "BasicBlock%-3ld[%d] %#lx (%ld, %ld); prevs ", id(), _loopDepth, this, _first->id(), _last->id() );
    for ( std::size_t i = 0; i < nPredecessors(); i++ )
        printPrevBBs( prev( i ), ( i == nPredecessors() - 1 ) ? " : " : ", " );
    lprintf( "; " );
    if ( next() )
        lprintf( "next BasicBlock%ld", next()->id() );
    for ( std::size_t i = 1; i < nSuccessors(); i++ )
        printPrevBBs( next( i ), ( i == nSuccessors() - 1 ) ? " : " : ", " );
}


void BasicBlock::print() {
    print_short();
    lprintf( "(%ld nodes):\n", _nodeCount );
    print_code( false );
    lprintf( "duInfo: " );
    duInfo.print();
}


void BasicBlock::print_code( bool_t suppressTrivial ) {
    for ( Node *n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n->_deleted and not( n == _first or n == _last ) )
            continue;
        if ( suppressTrivial and n->isTrivial() ) {
            // don't print
        } else {
            n->printID();
            n->print_short();
            lprintf( "\n" );
        }
    }
}


bool_t BasicBlock::contains( const Node *which ) const {
    for ( Node *n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n == which )
            return true;
    }
    return false;
}


void BasicBlock::verify() {
    int        count = 0;
    for ( Node *n    = _first; n not_eq _last->next(); n = n->next() ) {
        count++;
        if ( n->_deleted )
            continue;
        n->verify();
        if ( n->bb() not_eq this )
            error( "BasicBlock %#lx: Node %#lx doesn't point back to me", this, n );
        if ( n == _last and not n->endsBasicBlock() and not( n->next() and n->next()->isMergeNode() and ( (MergeNode *) ( n->next() ) )->_didStartBasicBlock ) ) {
            error( "BasicBlock %#lx: last Node %#lx isn't endsBasicBlock()", this, n );
        }
        if ( n->endsBasicBlock() and n not_eq _last )
            error( "BasicBlock %#lx: Node %#lx ends BasicBlock but isn't last node", this, n );
    }
    if ( count not_eq _nodeCount )
        error( "incorrect nnodes in BasicBlock %#lx", this );
    if ( _loopDepth < 0 )
        error( "BasicBlock %#lx: negative loopDepth %d", this, _loopDepth );

    // Fix this Urs, 3/11/96
    if ( _loopDepth > 9 )
        warning( "BasicBlock %#lx: suspiciously high loopDepth %d", this, _loopDepth );
}



void BasicBlock::dfs( GrowableArray<BasicBlock *> *list, int loopDepth ) {
    // build BasicBlock graph with depth-first traversal
    if ( _id == 1 )
        return;
    _id = 1;        // mark as visited
#ifdef fix_this
    setting of _isLoopStart/End is broken -- fix this (currently not used)
    if (first->isMergeNode()) {
      MergeNode* m = (MergeNode*)first;
      if (m->_isLoopStart) {
        loopDepth++;
      } else if (m->_isLoopEnd) {
        assert(loopDepth > 0, "bad loop end marker");
        loopDepth--;
      }
    }
#endif
    _loopDepth = loopDepth;
    // Note: originally this code simply skipped missing (nullptr) successors;
    //       however it doesn't work correctly because if a node (or a BasicBlock)
    //       has n successors they're all assumed to be non-nullptr. Code with
    //       missing successors (e.g. TypeTestNode with no next(0) = nullptr)
    //       cause the BasicBlock graph to screw up after this node. (gri 7/22/96)
    int       n = _last->nSuccessors();
    for ( std::size_t i = 0; i < n; i++ ) {
        Node       *next   = _last->next( i );
        BasicBlock *nextBB = next->newBasicBlock();
    }
    for ( std::size_t i = nSuccessors() - 1; i >= 0; i-- ) {
        BasicBlock *nextBB = next( i );
        // only follow the link if next->bb hasn't been visited yet
        if ( nextBB->id() == 0 )
            nextBB->dfs( list, loopDepth );
    }
    list->append( this );
}
