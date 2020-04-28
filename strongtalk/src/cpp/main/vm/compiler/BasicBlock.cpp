//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "BasicBlock.hpp"
#include "vm/compiler/DefinitionUsageInfo.hpp"
#include "vm/compiler/defUse.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/CopyPropagationInfo.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/compiler/RegisterAllocator.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/runtime/ResourceMark.hpp"


int BasicBlock::genCounter = 0;


void BasicBlock::init( Node * first, Node * last, int n ) {
    _first                 = first;
    _last                  = last;
    _nodeCount             = n;
    _id                    = 0;
    _genCount              = 0;
    BasicBlock::genCounter = 0;
    _loopDepth             = 0;
}


BasicBlock * BasicBlock::next() const {
    Node * n = _last->next();
    return n ? n->bb() : nullptr;
}


BasicBlock * BasicBlock::firstPrev() const {
    Node * n = _first->firstPrev();
    return n ? n->bb() : nullptr;
}


BasicBlock * BasicBlock::next( int i ) const {
    Node * n = _last->next( i );
    return n ? n->bb() : nullptr;
}


BasicBlock * BasicBlock::prev( int i ) const {
    Node * n = _first->prev( i );
    return n ? n->bb() : nullptr;
}


int BasicBlock::nSuccessors() const {
    return _last->nSuccessors();
}


int BasicBlock::nPredecessors() const {
    return _first->nPredecessors();
}


bool_t BasicBlock::isSuccessor( const BasicBlock * bb ) const {
    return _last->isSuccessor( bb->_first );
}


bool_t BasicBlock::isPredecessor( const BasicBlock * bb ) const {
    return _first->isPredecessor( bb->_last );
}


// note: the functions below are defined here rather than in PseudoRegister to localize this temporary hack
constexpr int MaxSearch = 50;    // max. # of nodes to search backwards

NonTrivialNode * findDefinitionOf( Node * endNode, const PseudoRegister * r, int max = MaxSearch ) {
    // search backwards for a definition of r
    Node      * current = endNode;
    for ( int i         = 0; i < max and current not_eq nullptr; i++ ) {
        if ( current->_deleted )
            continue;
        if ( current->hasSinglePredecessor() ) {
            if ( ( current->isAssignNode() or current->isInlinedReturnNode() ) and ( ( NonTrivialNode * ) current )->dest() == r ) {
                return ( NonTrivialNode * ) current;
            }
        } else {
            // merge node with several predecessors: search each one and make sure all find the same def
            st_assert( current->isMergeNode(), "must be a merge" );
            MergeNode      * merge     = ( MergeNode * ) current;
            NonTrivialNode * candidate = nullptr;    // possible def of r
            for ( int      j           = merge->nPredecessors() - 1; j >= 0; j-- ) {
                Node           * prev = merge->prev( j );
                NonTrivialNode * cand = findDefinitionOf( prev, r, max - i );
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


void propagateTo( BasicBlock * useBasicBlock, Usage * use, NonTrivialNode * fromNode, PseudoRegister * src, NonTrivialNode * toNode ) {

    // r1 := r2; ...; r3 := op(r1)  -->  r1 := r2; ...; r3 := op(r2)
    if ( toNode->canCopyPropagate( fromNode ) ) {
        if ( not src->extendLiveRange( toNode ) ) {
            if ( CompilerDebug ) {
                cout( PrintCopyPropagation )->print( "*brute-force copy-propagation: cannot propagate %s from N%ld to N%ld because of extendLiveRange\n", src->name(), fromNode->id(), toNode->id() );
            }
            return;
        }
        PseudoRegister * replaced = fromNode->dest();
        bool_t         ok         = toNode->copyPropagate( useBasicBlock, use, src, true );

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


bool_t regAssignedBetween( const PseudoRegister * r, const Node * startNode, Node * endNode ) {
    // check if r is assigned somewhere between start and end node
    // quite inefficient
    // search backwards from end to start
    BasicBlock * bbWithoutDefs = nullptr;    // BasicBlock w/o definitions of r
    for ( Node * n             = endNode; n not_eq startNode; n = n->firstPrev() ) {
        // see if n's bb has a def or r
        BasicBlock * bb = n->bb();
        if ( bb == bbWithoutDefs )
            continue; // no definitions here
        bool_t    hasDefs = false;
        for ( int i       = 0; i < bb->duInfo.info->length(); i++ ) {// forall def/use info lists
            DefinitionUsageInfo * dui = bb->duInfo.info->at( i );
            if ( dui->_pseudoRegister == r and not dui->_definitions.isEmpty() ) {
                // yes, it has a def
                hasDefs                            = true;
                for ( SListElem <Definition *> * d = dui->_definitions.head(); d; d = d->next() ) {
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

    for ( int i = 0; i < len; i++ ) {        // forall def/use info lists
        DefinitionUsageInfo  * dui = duInfo.info->at( i );
        const PseudoRegister * r   = dui->_pseudoRegister;
        if ( not r->isSAPReg() or not r->_location.equals( unAllocated ) ) {
            // optimize only SAPseudoRegisters for now
            // preallocated PseudoRegister may have aliases - don't do copy-propagation
            continue;
        }

        // try to find a def them with other PseudoRegisters
        if ( dui->_usages.isEmpty() )
            continue;    // has only definitions
        const Usage    * use          = dui->_usages.head()->data();
        Node           * firstUseNode = use->_node;
        NonTrivialNode * defNode      = findDefinitionOf( firstUseNode, r );
        if ( defNode == nullptr )
            continue;    // no def found

        PseudoRegister * candidate = ( ( NonTrivialNode * ) defNode )->src();
        if ( not candidate->_location.equals( unAllocated ) )
            continue;
        st_assert( candidate->isUsed(), "should not be unused" );
        if ( not regAssignedBetween( candidate, defNode, firstUseNode ) ) {
            // ok: the candidate reaches the use at firstUseNode
            // try to propagate it to all uses of r in this BasicBlock
            SListElem <Usage *>       * nextu;
            for ( SListElem <Usage *> * u = dui->_usages.head(); u; u = nextu ) {
                nextu = u->next();    // because we're mutating the list
                Usage          * thisUse  = u->data();
                NonTrivialNode * thisNode = thisUse->_node;
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
    for ( int       i         = 0; i < len; i++ ) {
        PseudoRegister * r = duInfo.info->at( i )->_pseudoRegister;
        if ( not r->_location.equals( unAllocated ) and r->_location.isRegisterLocation() ) {
            if ( used.isAllocated( r->_location.number() ) ) {
                // two PseudoRegisters have same preallocated reg - algorithm below can't handle this
                usedTwice = usedTwice.allocate( r->_location.number() );
            } else {
                used = used.allocate( r->_location.number() );
            }
        }
    }

    for ( int i = 0; i < len; i++ ) {
        constexpr int       BIG   = 9999999;
        DefinitionUsageInfo * dui = duInfo.info->at( i );
        PseudoRegister      * r   = dui->_pseudoRegister;
        if ( not r->_location.equals( unAllocated ) and r->_location.isRegisterLocation() and usedTwice.isAllocated( r->_location.number() ) ) {
            // this preallocated PseudoRegister has aliases - don't do copy-propagation
            continue;
        }
        SListElem <Usage *>      * u = dui->_usages.head();
        SListElem <Definition *> * nextd;
        SListElem <Definition *> * d;
        for ( d = dui->_definitions.head(); d and u; d = nextd ) {
            // try to find a use of the def at d
            nextd                          = d->next();
            const Definition     * def     = d->data();
            SList <Definition *> * srcDefs = nullptr;    // redefinition of src that defines this def (if any)
            if ( def->_node->hasSrc() ) {
                PseudoRegister * src = def->_node->src();
                if ( src->_location.isRegisterLocation() and usedTwice.isAllocated( src->_location.number() ) ) {
                    // r := f(r2), and r2 is aliased preallocated - can't handle
                    continue;
                }
                if ( not src->isSinglyAssigned() ) {
                    // careful: r is only equivalent to src as int32_t as src isn't reassigned within this basic block
                    int j = 0;
                    for ( ; j < len; j++ ) {
                        PseudoRegister * r = duInfo.info->at( j )->_pseudoRegister;
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
            const int            d_id      = def->_node->num();
            int                  u_id;
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
    duInfo.info = new GrowableArray <DefinitionUsageInfo *>( _nodeCount + 10 );
    for ( Node * n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n->_deleted )
            continue;
        n->makeUses( this );
    }
}


void BasicBlock::renumber() {
    int        count = 0;
    for ( Node * n   = _first; n not_eq _last->next(); n = n->next() )
        n->setNum( count++ );
    _nodeCount = count;
}


void BasicBlock::remove( Node * n ) {
    // remove this node and its definitions & uses
    // NB: nodes aren't actually removed from the graph but just marked as
    // deleted.  This is much simpler because the topology of the flow graph
    // doesn't change this way
    st_assert( contains( n ), "node isn't in this BasicBlock" );
    n->removeUses( this );
    n->_deleted = true;
}


void BasicBlock::addAfter( Node * prev, Node * newNode ) {
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


static BasicBlock * thisBasicBlock;


static void duChecker( PseudoRegisterBasicBlockIndex * p ) {
    if ( p->_basicBlock == thisBasicBlock ) fatal( "should not be in middle of list" );
}


static bool_t findMyBasicBlock( void * bb, PseudoRegisterBasicBlockIndex * p ) {
    return p->_basicBlock == ( BasicBlock * ) bb;
}


int BasicBlock::addUDHelper( PseudoRegister * r ) {
    // we're computing the uses block by block, and the current BasicBlock's
    // PseudoRegisterBasicBlockIndex is always the last entry in the preg's list.
    st_assert( _nodeCount, "shouldn't add anything to this BasicBlock" );
    bbIterator->pregTable->at_put_grow( r->id(), r );
    PseudoRegisterBasicBlockIndex * p;
    if ( bbIterator->_usesBuilt ) {
        // find entry for the PseudoRegister
        int i = r->_dus.find( this, findMyBasicBlock );
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


Usage * BasicBlock::addUse( NonTrivialNode * n, PseudoRegister * r, bool_t soft ) {
    st_assert( not soft, "soft use" );
    st_assert( contains( n ), "node isn't in this BasicBlock" );
    if ( r->isNoPReg() )
        return nullptr;
    Usage * u = soft ? new PSoftUsage( n ) : new Usage( n );
    r->incUses( u );
    int index = addUDHelper( r );
    duInfo.info->at( index )->_usages.append( u );
    return u;
}


Definition * BasicBlock::addDef( NonTrivialNode * n, PseudoRegister * r ) {
    st_assert( contains( n ), "node isn't in this BasicBlock" );
    if ( r->isNoPReg() )
        return nullptr;
    Definition * d = new Definition( n );
    r->incDefs( d );
    int index = addUDHelper( r );
    duInfo.info->at( index )->_definitions.append( d );
    return d;
}


// allocate PseudoRegisters that are used & defined solely within this BasicBlock
void BasicBlock::localAlloc( GrowableArray <BitVector *> * hardwired, GrowableArray <PseudoRegister *> * localRegs, GrowableArray <BitVector *> * lives ) {
    // try fast allocation first -- use only local regs that aren't touched
    // by any pre-allocated (hardwired) registers

    // hardwired, localRegs, lives: just passed on to slowLocalAlloc

    // Note: Fix problem with registers that are used after a call in PrimNodes
    // such as ContextCreateNode, etc. Problem is that values like self might
    // be allocated in registers but the registers are trashed after a call.
    // Right now: PrologueNode terminates BasicBlock to fix the problem.

    if ( not _nodeCount )
        return;            // empty BasicBlock

    GrowableArray <RegCandidate *>    cands( _nodeCount );
    GrowableArray <RegisterEqClass *> regClasses( _nodeCount + 1 );
    regClasses.append( nullptr );        // first reg class has index 1

    int       use_count[REGISTER_COUNT], def_count[REGISTER_COUNT];
    for ( int i = 0; i < REGISTER_COUNT; i++ )
        use_count[ i ] = def_count[ i ] = 0;

    for ( Node * nn = _first; nn not_eq _last->next(); nn = nn->next() ) {
        if ( nn->_deleted )
            continue;
        nn->markAllocated( use_count, def_count );
        if ( nn->isAssignNode() ) {
            NonTrivialNode * n       = ( NonTrivialNode * ) nn;
            PseudoRegister * src     = n->src();
            PseudoRegister * dest    = n->dest();
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
        RegCandidate * c = cands.pop();
        if ( def_count[ c->_location.number() ] == c->_ndefs ) {
            doAlloc( c->_pseudoRegister, c->_location );
        }
    }

    // allocate other local regs (using the untouched temp regs of this BasicBlock)
    int       temp = 0;
    for ( int i    = 0; i < duInfo.info->length(); i++ ) {
        // collect local regs
        PseudoRegister * r = duInfo.info->at( i )->_pseudoRegister;
        if ( r->_location.equals( unAllocated ) and not r->isUnused() and r->isLocalTo( this ) ) {
            st_assert( r->_dus.first()->_index == i, "should be the same" );
            for ( ; temp < nofLocalRegisters and use_count[ Mapping::localRegister( temp ).number() ] + def_count[ Mapping::localRegister( temp ).number() ] > 0; temp++ );
            if ( temp == nofLocalRegisters )
                break;        // ran out of regs
            // ok, allocate Mapping::localRegisters[temp] to the preg and equivalent pregs
            Location             t      = Mapping::localRegister( temp++ );
            PseudoRegister       * frst = r->regClass ? regClasses.at( r->regClass )->first : r;
            for ( PseudoRegister * pr   = frst; pr; pr = pr->regClassLink ) {
                doAlloc( pr, t );
                pr->regClass = 0;
            }
        }
        r->regClass        = 0;
    }

    if ( temp == nofLocalRegisters ) {
        // ran out of local regs with the simple strategy - try using slow
        // allocation algorithm
        slowLocalAlloc( hardwired, localRegs, lives );
    }
}


// slower but more capable version of local allocation; keeps track of live
// ranges via a bit map
// note: temporary data structs are passed in so they can be reused for all
// BBs (otherwise would allocate too much junk in resource area)
void BasicBlock::slowLocalAlloc( GrowableArray <BitVector *> * hardwired, GrowableArray <PseudoRegister *> * localRegs, GrowableArray <BitVector *> * lives ) {
    // clear temporary data structures
    localRegs->clear();
    lives->clear();
    for ( int i = 0; i < nofLocalRegisters; i++ ) {
        hardwired->at( i )->setLength( _nodeCount );
        hardwired->at( i )->clear();
    }
    // hardwired->at(i): indexed by reg no, gives nodes in which register is busy
    // localRegs: collects all PseudoRegisters that could be allocated locally
    // lives: for each reg in localRegs, holds live range (bit vector with one bit per node)

    for ( int i = 0; i < duInfo.info->length(); i++ ) {
        // collect local regs
        PseudoRegister * r = duInfo.info->at( i )->_pseudoRegister;
        if ( r->isLocalTo( this ) ) {
            st_assert( r->_dus.first()->_index == i, "should be the same" );
            if ( r->isUnused() ) {
                // unused register - ignore
            } else {
                DefinitionUsageInfo * info = duInfo.info->at( r->_dus.first()->_index );
                localRegs->append( r );
                BitVector * bv = new BitVector( _nodeCount );
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
    for ( Node * n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n->_deleted )
            continue;
        SimpleBitVector v = n->trashedMask();
        if ( v.isEmpty() )
            continue;    // nothing trashed (normal case)
        for ( int i = 0; i < nofLocalRegisters; i++ ) {
            if ( v.isAllocated( i ) )
                hardwired->at( i )->add( n->num() );
        }
    }


    // cycle through the temp registers to (hopefully) allow more optimizations
    // later (e.g. scheduling)
    int lastTemp = 0;
#define nextTemp( n ) (n == nofLocalRegisters - 1) ? 0 : n + 1

    for ( int i = 0; i < localRegs->length(); i++ ) {
        // try to allocate localRegs[i] to a local (temp) register
        PseudoRegister * r = localRegs->at( i );
        if ( not r->_location.equals( unAllocated ) ) {
            st_assert( r->regClass == 0, "should have been cleared" );
            continue;
        }
        BitVector * liveRange = lives->at( i );
        for ( int tempNo      = lastTemp, ntries = 0; ntries < nofLocalRegisters; tempNo = nextTemp( tempNo ), ntries++ ) {
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


void BasicBlock::doAlloc( PseudoRegister * r, Location l ) {
    if ( CompilerDebug )
        cout( PrintLocalAllocation )->print( "*assigning %s to local %s in BasicBlock%ld\n", l.name(), r->name(), id() );
    st_assert( not r->_debug, "should not allocate to temp reg" );
    r->_location = l;
}


void BasicBlock::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * l ) {
    // add all escaping blocks to l
    for ( Node * n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n->_deleted )
            continue;
        n->computeEscapingBlocks( l );
    }
}


void BasicBlock::apply( NodeVisitor * v ) {
    if ( _nodeCount > 0 ) {
        Node       * end = _last->next();
        for ( Node * n   = _first; n not_eq end; n = n->next() ) {
            if ( not n->_deleted ) {
                v->beginOfNode( n );
                n->apply( v );
                v->endOfNode( n );
            }
        }
    } else {
        st_assert( _nodeCount == 0, "nnodes should be 0" );
        Node       * end = _last->next();
        for ( Node * n   = _first; n not_eq end; n = n->next() ) {
            st_assert( n->_deleted, "basic block is not empty even though nnodes == 0!" );
        }
    }
}


bool_t BasicBlock::verifyLabels() {
    bool_t ok = true;
    if ( _nodeCount > 0 ) {
        for ( Node * n = _first; n not_eq _last->next(); n = n->next() ) {
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


static void printPrevBBs( BasicBlock * b, const char * str ) {
    lprintf( "BasicBlock%ld%s", b->id(), str );
}


void BasicBlock::print_short() {
    lprintf( "BasicBlock%-3ld[%d] %#lx (%ld, %ld); prevs ", id(), _loopDepth, this, _first->id(), _last->id() );
    for ( int i = 0; i < nPredecessors(); i++ )
        printPrevBBs( prev( i ), ( i == nPredecessors() - 1 ) ? " : " : ", " );
    lprintf( "; " );
    if ( next() )
        lprintf( "next BasicBlock%ld", next()->id() );
    for ( int i = 1; i < nSuccessors(); i++ )
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
    for ( Node * n = _first; n not_eq _last->next(); n = n->next() ) {
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


bool_t BasicBlock::contains( const Node * which ) const {
    for ( Node * n = _first; n not_eq _last->next(); n = n->next() ) {
        if ( n == which )
            return true;
    }
    return false;
}


void BasicBlock::verify() {
    int        count = 0;
    for ( Node * n   = _first; n not_eq _last->next(); n = n->next() ) {
        count++;
        if ( n->_deleted )
            continue;
        n->verify();
        if ( n->bb() not_eq this )
            error( "BasicBlock %#lx: Node %#lx doesn't point back to me", this, n );
        if ( n == _last and not n->endsBasicBlock() and not( n->next() and n->next()->isMergeNode() and ( ( MergeNode * ) ( n->next() ) )->didStartBasicBlock ) ) {
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


BasicBlockIterator * bbIterator;


void BasicBlockIterator::build( Node * first ) {
    st_assert( _basicBlockTable == nullptr, "already built" );
    _first           = first;
    _basicBlockCount = 0;
    pregTable        = globals = nullptr;
    buildBBs();
    _blocksBuilt = true;
}


void BasicBlockIterator::buildBBs() {
    // build basic blocks
    // first, sort them topologically (ignoring backward arcs)
    // some things (e.g. SplitPReg::isLiveAt) depend on correct ordering
    GrowableArray <BasicBlock *> * list = new GrowableArray <BasicBlock *>( max( BasicNode::currentID >> 3, 10 ) );
    _first->newBasicBlock()->dfs( list, 0 );
    // now, the list contains the BBs in reverse order
    _basicBlockCount = list->length();
    _basicBlockTable = new GrowableArray <BasicBlock *>( _basicBlockCount );
    for ( int i = _basicBlockCount - 1; i >= 0; i-- ) {
        BasicBlock * bb = list->at( i );
        bb->_id = _basicBlockCount - i - 1;
        _basicBlockTable->append( bb );
    }
}


void BasicBlock::dfs( GrowableArray <BasicBlock *> * list, int loopDepth ) {
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
      } else if (m->isLoopEnd) {
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
    for ( int i = 0; i < n; i++ ) {
        Node       * next   = _last->next( i );
        BasicBlock * nextBB = next->newBasicBlock();
    }
    for ( int i = nSuccessors() - 1; i >= 0; i-- ) {
        BasicBlock * nextBB = next( i );
        // only follow the link if next->bb hasn't been visited yet
        if ( nextBB->id() == 0 )
            nextBB->dfs( list, loopDepth );
    }
    list->append( this );
}


static void clearNodes( BasicBlock * bb ) {
    for ( Node * n = bb->_first; n not_eq bb->_last->next(); n = n->next() ) {
        n->setBasicBlock( nullptr );
    }
}


void BasicBlockIterator::clear() {
    apply( clearNodes );
    _basicBlockTable = nullptr;
    pregTable        = globals = nullptr;
}


void BasicBlockIterator::makeUses() {
    // a few PseudoRegisters may be created during makeUses (e.g. for deadBlockObj,
    // true/false etc), so leave a bit of room
    constexpr int ExtraPRegs = 10;
    int           n          = PseudoRegister::currentNo + ExtraPRegs;
    pregTable = new GrowableArray <PseudoRegister *>( n );
    for ( int i = 0; i < n; i++ )
        pregTable->append( nullptr );
    for ( int i = 0; i < _basicBlockCount; i++ ) {
        _basicBlockTable->at( i )->makeUses();
    }
    _usesBuilt = true;
}


void BasicBlockIterator::localAlloc() {
    // speed/Space optimization: allocate hardwired et al only once, not for every BasicBlock
    {
        ResourceMark                     rm;
        GrowableArray <BitVector *>      hardwired( nofLocalRegisters, nofLocalRegisters, nullptr );
        GrowableArray <PseudoRegister *> localRegs( BasicNode::currentID );
        GrowableArray <BitVector *>      lives( BasicNode::currentID );

        for ( int i = 0; i < nofLocalRegisters; i++ ) {
            hardwired.at_put( i, new BitVector( roundTo( BasicNode::currentID, BitsPerWord ) ) );
        }

        for ( int i = 0; i < _basicBlockCount; i++ ) {
            _basicBlockTable->at( i )->localAlloc( &hardwired, &localRegs, &lives );
        }
    }

    int done = 0;
    globals = new GrowableArray <PseudoRegister *>( pregTable->length() );
    for ( int i = 0; i < pregTable->length(); i++ ) {
        PseudoRegister * r = pregTable->at( i );
        if ( r ) {
            if ( r->isUnused() ) {
                pregTable->at_put( i, nullptr );        // no longer needed
            } else if ( r->_location.equals( unAllocated ) ) {
                globals->append( r );
            } else {
                done++;                // locally allocated
            }
        }
    }
    if ( PrintLocalAllocation ) {
        int total = globals->length() + done;
        lprintf( "*local reg. allocations done; %ld out of %ld = (%3.1f%%).\n", done, total, 100.0 * done / total );
    }
}


void BasicBlockIterator::print() {
    for ( int i = 0; i < _basicBlockCount; i++ ) {
        lprintf( "  " );
        _basicBlockTable->at( i )->print_short();
        lprintf( "\n" );
    }
}


void BasicBlockIterator::localCopyPropagate() {
    for ( int i = 0; i < _basicBlockCount; i++ ) {
        _basicBlockTable->at( i )->localCopyPropagate();
    }
}


void BasicBlockIterator::bruteForceCopyPropagate() {
    for ( int i = 0; i < _basicBlockCount; i++ ) {
        _basicBlockTable->at( i )->bruteForceCopyPropagate();
    }
}


void BasicBlockIterator::computeEscapingBlocks() {
    // Escape analysis for blocks: compute initial approximation (lower bound)
    // by collecting all blocks that are stored into the heap or returned from
    // nativeMethods.
    // Terminology: an "escaping" block is a block closure that
    // could potentially be created at runtime and passed to some code which
    // could store it in the heap (thus "escape").  An escaping block could
    // thus be invoked during any non-inlined send (since the callee could
    // read the block from the heap and send value to it) or after the method
    // has returned.  Consequently, the variables uplevel-accessed by an escaping
    // block cannot be stack-allocated, and any uplevel-written variables cannot
    // be cached across calls.
    exposedBlks = new GrowableArray <BlockPseudoRegister *>( BlockPseudoRegister::numBlocks() );
    for ( int i = 0; i < _basicBlockCount; i++ ) {
        _basicBlockTable->at( i )->computeEscapingBlocks( exposedBlks );
    }
}


void BasicBlockIterator::computeUplevelAccesses() {
    // Compute the set of uplevel-accessed variables for each exposed block.
    // Terminology: variables are considered "uplevel-accessed" only if they
    // are accessed by an exposed block.  I.e., if a block is completely inlined,
    // it's uplevel accesses aren't really "uplevel" anymore and thus are ignored.
    int       len = exposedBlks->length();
    for ( int i   = 0; i < len; i++ ) {
        BlockPseudoRegister * r = exposedBlks->at( i );
        st_assert ( r->parent() == r->scope(), "block preg was copied/changed scope" );
        st_assert( r->scope()->isInlinedScope(), "must be code scope" );
        r->computeUplevelAccesses();
    }
}


bool_t BasicBlockIterator::isSequentialCode( BasicBlock * curr, BasicBlock * next ) const {
    return curr == next or ( curr->genCount() + 1 == next->genCount() and curr->next() == next );
}


bool_t BasicBlockIterator::isSequential( int curr, int next ) const {
    // is control flow between current and next BasicBlock sequential?
    if ( curr == next )
        return true;
    if ( next < _basicBlockCount and _basicBlockTable->at( curr )->next() not_eq nullptr and _basicBlockTable->at( curr )->next() not_eq _basicBlockTable->at( next ) ) {
        return false; // non-sequential control flow
    }

    if ( next == _basicBlockCount and _basicBlockTable->at( curr )->next() not_eq nullptr ) {
        // we're "running off the end", so must be non-seq
        return false;
    }

    return true;        // none of the above, so must be sequential
}


BasicBlock * BasicBlockIterator::bb_for( Node * n ) {
    return n not_eq nullptr ? n->bb() : nullptr;
}


void BasicBlockIterator::add_BBs_to_list( GrowableArray <BasicBlock *> & list, GrowableArray <BasicBlock *> & work ) {
    if ( not work.isEmpty() ) {
        GrowableArray <BasicBlock *> uncommon;    // uncommon BBs are handled at the very end
        do {
            BasicBlock * bb = work.pop();
            st_assert( bb->visited(), "must have been visited" );
            list.append( bb );            // do this even if empty (label may be referenced)
            bb->setGenCount();        // can probably be removed at some point
            // determine likely & uncommon successors
            BasicBlock * likely_next   = bb_for( bb->_last->likelySuccessor() );
            BasicBlock * uncommon_next = bb_for( bb->_last->uncommonSuccessor() );
            st_assert( likely_next not_eq uncommon_next or likely_next == nullptr, "likely BasicBlock cannot be uncommon BasicBlock at the same time" );
            // first, push all other successors
            for ( int i = bb->nSuccessors(); i-- > 0; ) {
                BasicBlock * next = ( BasicBlock * ) bb->next( i );
                if ( next not_eq nullptr and not next->visited() and next not_eq likely_next and next not_eq uncommon_next ) {
                    work.push( next->after_visit() );
                }
            }
            // then, push likely successor (will be handled next)
            if ( likely_next not_eq nullptr and not likely_next->visited() ) {
                work.push( likely_next->after_visit() );
            }
            // remember uncommon successor for the very end
            if ( uncommon_next not_eq nullptr and not uncommon_next->visited() )
                uncommon.push( uncommon_next->after_visit() );
        } while ( not work.isEmpty() );
        // add all the uncommon cases
        add_BBs_to_list( list, uncommon );
    }
}


GrowableArray <BasicBlock *> * BasicBlockIterator::code_generation_order() {
    if ( not ReorderBBs )
        return _basicBlockTable;
    // initialize visited field for all nodes
    for ( int                    i      = 0; i < _basicBlockCount; i++ )
        _basicBlockTable->at( i )->before_visit();
    // working sets
    GrowableArray <BasicBlock *> * list = new GrowableArray <BasicBlock *>( _basicBlockCount );    // eventually holds all reachable BBs again
    GrowableArray <BasicBlock *> work( _basicBlockCount );                // may hold only a part of all BBs at any given time
    // setup starting point
    BasicBlock                   * bb   = _first->bb();
    work.push( bb->after_visit() );
    // compute order
    add_BBs_to_list( *list, work );
    // NB: "loosing" BBs is ok since some may become unreachable during optimization  -Robert 7/96
    st_assert( list->length() <= _basicBlockCount, "gained BBs?" );
    return list;
}


void BasicBlockIterator::print_code( bool_t suppressTrivial ) {
    GrowableArray <BasicBlock *> * list = code_generation_order();
    for ( int                    i      = 0; i < list->length(); i++ ) {
        BasicBlock * bb = list->at( i );
        if ( bb->_nodeCount > 0 )
            bb->print_code( suppressTrivial );
        if ( bb->next() not_eq nullptr )
            _console->print_cr( "\tgoto N%d", bb->next()->_first->id() );
        if ( not suppressTrivial )
            lprintf( "\n" );
    }
}


void BasicBlockIterator::apply( NodeVisitor * v ) {
    GrowableArray <BasicBlock *> * list = code_generation_order();
    int                          length = list->length();
    for ( int                    i      = 0; i < length; i++ ) {
        BasicBlock * bb    = list->at( i );
        Node       * first = bb->_first;
        Node       * last  = bb->_last;
        v->beginOfBasicBlock( first );
        bb->apply( v );
        v->endOfBasicBlock( last );
    }
}


bool_t BasicBlockIterator::verifyLabels() {
    bool_t    ok = true;
    for ( int i  = 0; i < _basicBlockCount; i++ )
        ok &= _basicBlockTable->at( i )->verifyLabels();
    return ok;
}


void BasicBlockIterator::globalCopyPropagate() {
    // do global copy propagation of singly-assigned PseudoRegisters

    for ( int i = 0; i < pregTable->length(); i++ ) {
        PseudoRegister * r = pregTable->at( i );
        if ( not r or r->isConstPReg() or not r->canCopyPropagate() )
            continue;
        Definition * def    = nullptr;
        // get definition
        int        dulength = r->_dus.length();
        int        e        = 0;
        for ( ; e < dulength; e++ ) {
            PseudoRegisterBasicBlockIndex * index = r->_dus.at( e );
            DefinitionUsageInfo           * info  = index->_basicBlock->duInfo.info->at( index->_index );
            if ( info->_definitions.length() ) {
                st_assert( info->_definitions.length() == 1, "should be one definition only" );
                st_assert( def == nullptr, "already have def!?!" );
                def = info->_definitions.first();
                break;
            }
        }
        st_assert( def, "should have found the definition" );
        NonTrivialNode * defNode = def->_node;

        // don't propagate if the defNode isn't assignment
        if ( not defNode->isAssignmentLike() )
            continue;
        st_assert( defNode->dest() == r, "not assignment-like" );

        PseudoRegister * defSrc = defNode->src();

        // don't propagate if source does not survive calls (i.e. is local to BasicBlock),
        // or if it isn't singly-assigned as well (if it's multiply assigned,
        // we'd have to make sure it's not assigned between defNode and the use nodes
        if ( not defSrc->isSinglyAssigned() or defSrc->_location.isTrashedRegister() )
            continue;

        // ok, everything is fine - propagate the def to the uses
        for ( e = 0; e < dulength; e++ ) {
            PseudoRegisterBasicBlockIndex * index = r->_dus.at( e );
            BasicBlock                    * bb    = index->_basicBlock;
            DefinitionUsageInfo           * info  = bb->duInfo.info->at( index->_index );
            // caution: propagateTo may eliminate nodes and thus shorten
            // info->uses
            int                           j       = 0;
            while ( j < info->_usages.length() ) {
                int oldLen = info->_usages.length();
                info->propagateTo( bb, info->_pseudoRegister, def, info->_usages.at( j ), true );
                if ( info->_usages.length() == oldLen ) {
                    // propagate didn't eliminate this use; try next one
                    j++;
                }
            }
        }
    }
}


void BasicBlockIterator::eliminateUnneededResults() {
    // eliminate nodes producing results that are never used
    for ( int i = 0; i < pregTable->length(); i++ ) {
        PseudoRegister * r = pregTable->at( i );
        if ( r and r->isOnlySoftUsed() ) {
            r->eliminate( false );
        }
    }
}


void BasicBlockIterator::apply( BBDoFn f ) {
    for ( int i = 0; i < _basicBlockCount; i++ ) {
        f( _basicBlockTable->at( i ) );
    }
}


void BasicBlockIterator::verify() {
    if ( _basicBlockTable ) {
        int i = 0;
        for ( ; i < _basicBlockCount; i++ ) {
            _basicBlockTable->at( i )->verify();
        }
        if ( pregTable ) {
            for ( int i = 0; i < pregTable->length(); i++ ) {
                if ( pregTable->at( i ) )
                    pregTable->at( i )->verify();
            }
        }
    }
}


void DefinitionUsageInfo::propagateTo( BasicBlock * useBasicBlock, Usage * use, const NonTrivialNode * fromNode, PseudoRegister * src, NonTrivialNode * toNode, const bool_t global ) {
    // r1 := r2; ...; r3 := op(r1)  -->  r1 := r2; ...; r3 := op(r2)
    bool_t ok = toNode->copyPropagate( useBasicBlock, use, src );
    if ( CompilerDebug ) {
        cout( PrintCopyPropagation )->print( "*%s copy-propagation:%s propagate %s from N%ld (%#lx) to N%ld (%#lx)\n", global ? "global" : "local", ok ? "" : " couldn't", src->name(), fromNode ? fromNode->id() : 0, PrintHexAddresses ? fromNode : 0, toNode->id(), PrintHexAddresses ? toNode : 0 );
    }
}


void DefinitionUsageInfo::propagateTo( BasicBlock * useBasicBlock, const PseudoRegister * r, const Definition * def, Usage * use, const bool_t global ) {
    // def reaches use; try to eliminate r's use by using copy propagation
    NonTrivialNode * fromNode   = def->_node;
    const bool_t   isAssignment = fromNode->isAssignmentLike();
    NonTrivialNode * toNode     = use->_node;
    const bool_t   hasSrc       = fromNode->hasSrc();
    PseudoRegister * fromPR     = hasSrc ? fromNode->src() : nullptr;
    const bool_t   isConst      = hasSrc and fromPR->isConstPReg();

    if ( isAssignment and isConst and toNode->canCopyPropagateOop() ) {
        // loadOop r1, Oop; ...; r2 := op(r1)    --->
        // loadOop r1, Oop; ...; r2 := op(Oop)
        bool_t ok = toNode->copyPropagate( useBasicBlock, use, fromPR );
        if ( CompilerDebug ) {
            // the original code broke the MSC++ 3.0 optimizer, so now it looks a bit weird.  -Urs 7/96
            const char * glob = global ? "global" : "local";
            const char * prop = ok ? "" : " couldn't propagate";
            const char * name = def->_node->src()->name();        // using fromNode or fromPR here will break
            void       * from = PrintHexAddresses ? fromNode : 0;
            void       * to   = PrintHexAddresses ? toNode : 0;
            cout( PrintCopyPropagation )->print( "*%s copy-propagation:%s %s from N%ld (%#lx) to N%ld (%#lx)\n", glob, prop, name, def->_node->id(), from, toNode->id(), to );
        }
        return;
    }

    if ( fromNode->loopDepth() not_eq toNode->loopDepth() ) {
        // currently can't propagate into a loop: PseudoRegister would have to be extended to
        // end of loop, but is only extended to use -- fix this later
        // NB: this is not completely safe since fromNode might be in different loop than
        // toNode (but at same loop nesting), so loopDepths match
        st_assert( global, "can't be local copy-propagation" );
        if ( CompilerDebug ) {
            cout( PrintCopyPropagation )->print( "*global copy-propagation: can't propagate %s from N%ld (%#lx) to N%ld (%#lx) -- loopDepth mismatch\n", fromPR->name(), fromNode->id(), PrintHexAddresses ? fromNode : 0, toNode->id(), PrintHexAddresses ? toNode : 0 );
        }
        return;
    }

    if ( isAssignment and not isConst and hasSrc and toNode->canCopyPropagate( fromNode ) ) {
        // r1 := r2; ...; r3 := op(r1)  -->  r1 := r2; ...; r3 := op(r2)
        propagateTo( useBasicBlock, use, fromNode, fromPR, toNode, global );
        return;
    }

    if ( not global and r->isSinglyUsed() and ( toNode->isAssignNode() or toNode->isInlinedReturnNode() ) and
         // fix this -- should it be isAssignmentLike?
         toNode->canBeEliminated() and fromNode->hasDest() and fromNode->canChangeDest() and r->canBeEliminated( true ) and not toNode->dest()->_location.isTopOfStack() ) {
        // fromNode: r := ...;  ... ; toNode: x := r    --->
        // fromNode: x := ...;
        // NB: should extend live range of toNode->dest() backwards to include fromNode
        // currently not done (extendLiveRange handles only forward extensions), but as
        // int32_t as this optimization isn't performed globally this shouldn't hurt
        if ( CompilerDebug ) {
            cout( PrintCopyPropagation )->print( "*%s copy-propagation: changing dest of N%ld (%#lx) to %s to match use at N%ld (%#lx)\n", global ? "global" : "local", fromNode->id(), PrintHexAddresses ? fromNode : 0, toNode->dest()->name(), toNode->id(), PrintHexAddresses ? toNode : 0 );
        }
        st_assert( not r->incorrectDU(), "shouldn't try copy-propagation on this" );
        st_assert( not global or r->isSinglyAssigned(), "not safe with >1 definitions" );
        st_assert( fromNode->dest() == r, "unexpected dest" );
        fromNode->setDest( fromNode->bb(), toNode->dest() );
        toNode->eliminate( toNode->bb(), nullptr, false, true );
        return;
    }

    // nothing worked
    if ( CompilerDebug ) {
        cout( PrintCopyPropagation )->print( "*%s copy-propagation: can't propagate N%ld (%#lx) to N%ld (%#lx)\n", global ? "global" : "local", fromNode->id(), PrintHexAddresses ? fromNode : 0, toNode->id(), PrintHexAddresses ? toNode : 0 );
    }
}


void DefinitionUsageInfo::getLiveRange( int & firstNodeNum, int & lastNodeNum ) {
    // compute live range of a local register
    // IN:  first & last node ID of BasicBlock
    // OUT: first & last node ID where this PseudoRegister is live
    // algorithm: first = first def if def precedes first use
    //			  0 if first use precedes def (live from start)
    // 		  last  = last use if it is later than last def
    //		  	  lastNodeNum if last def is later (live until end)
    SListElem <Definition *> * d = _definitions.head();
    SListElem <Usage *>      * u = _usages.head();

    // set firstNodeNum
    if ( u and d ) {
        int unode = u->data()->_node->num();
        int dnode = d->data()->_node->num();
        if ( unode > dnode ) {
            firstNodeNum = dnode;    // live range starts at definition
        } else {
            // use before def -- leave first at zero
            st_assert( firstNodeNum == 0, "should be zero" );
        }
    } else if ( u ) {
        // only a use -- live range = [0..unode]
        st_assert( firstNodeNum == 0, "should be zero" );
        while ( u and u->next() )
            u       = u->next();
        lastNodeNum = u->data()->_node->num();
        return;
    } else if ( d ) {
        firstNodeNum = d->data()->_node->num();    // live range starts at definition and ends at end of BasicBlock
        return;
    } else {
        // no definitions or uses in this BasicBlock; must be a result register whose use has been
        // eliminated (the def is in the last node of the previous BasicBlock)
        st_assert( _pseudoRegister->_location == resultLoc, "must be result loc" );
        firstNodeNum = lastNodeNum = 0;
        return;
    }

    // set lastNodeNum; first find last def and last use
    while ( u and u->next() )
        u = u->next();
    while ( d and d->next() )
        d = d->next();
    int unode = u->data()->_node->num();
    int dnode = d->data()->_node->num();
    if ( unode > dnode ) {
        lastNodeNum = unode;    // live range ends at last use
    } else {
        // defined last -- live until end of BasicBlock
    }
}


void DefinitionUsageInfo::print_short() {
    lprintf( "DefinitionUsageInfo %#lx", this );
}


void DefinitionUsageInfo::print() {
    print_short();
    lprintf( " for " );
    _pseudoRegister->print_short();
    lprintf( ": " );
    _usages.print();
    lprintf( "; " );
    _definitions.print();
    lprintf( "\n" );
}
