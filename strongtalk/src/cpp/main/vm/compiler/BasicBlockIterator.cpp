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
#include "vm/compiler/BasicBlockIterator.hpp"



static void clearNodes( BasicBlock *bb ) {
    for ( Node *n = bb->_first; n not_eq bb->_last->next(); n = n->next() ) {
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
    pregTable = new GrowableArray<PseudoRegister *>( n );
    for ( std::size_t i = 0; i < n; i++ )
        pregTable->append( nullptr );
    for ( std::size_t i = 0; i < _basicBlockCount; i++ ) {
        _basicBlockTable->at( i )->makeUses();
    }
    _usesBuilt = true;
}


void BasicBlockIterator::localAlloc() {
    // speed/Space optimization: allocate hardwired et al only once, not for every BasicBlock
    {
        ResourceMark                    rm;
        GrowableArray<BitVector *>      hardwired( nofLocalRegisters, nofLocalRegisters, nullptr );
        GrowableArray<PseudoRegister *> localRegs( BasicNode::currentID );
        GrowableArray<BitVector *>      lives( BasicNode::currentID );

        for ( std::size_t i = 0; i < nofLocalRegisters; i++ ) {
            hardwired.at_put( i, new BitVector( roundTo( BasicNode::currentID, BitsPerWord ) ) );
        }

        for ( std::size_t i = 0; i < _basicBlockCount; i++ ) {
            _basicBlockTable->at( i )->localAlloc( &hardwired, &localRegs, &lives );
        }
    }

    int done = 0;
    globals = new GrowableArray<PseudoRegister *>( pregTable->length() );
    for ( std::size_t i = 0; i < pregTable->length(); i++ ) {
        PseudoRegister *r = pregTable->at( i );
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
    for ( std::size_t i = 0; i < _basicBlockCount; i++ ) {
        lprintf( "  " );
        _basicBlockTable->at( i )->print_short();
        lprintf( "\n" );
    }
}


void BasicBlockIterator::localCopyPropagate() {
    for ( std::size_t i = 0; i < _basicBlockCount; i++ ) {
        _basicBlockTable->at( i )->localCopyPropagate();
    }
}


void BasicBlockIterator::bruteForceCopyPropagate() {
    for ( std::size_t i = 0; i < _basicBlockCount; i++ ) {
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
    exposedBlks = new GrowableArray<BlockPseudoRegister *>( BlockPseudoRegister::numBlocks() );
    for ( std::size_t i = 0; i < _basicBlockCount; i++ ) {
        _basicBlockTable->at( i )->computeEscapingBlocks( exposedBlks );
    }
}


void BasicBlockIterator::computeUplevelAccesses() {
    // Compute the set of uplevel-accessed variables for each exposed block.
    // Terminology: variables are considered "uplevel-accessed" only if they
    // are accessed by an exposed block.  I.e., if a block is completely inlined,
    // it's uplevel accesses aren't really "uplevel" anymore and thus are ignored.
    int       len = exposedBlks->length();
    for ( std::size_t i   = 0; i < len; i++ ) {
        BlockPseudoRegister *r = exposedBlks->at( i );
        st_assert ( r->parent() == r->scope(), "block preg was copied/changed scope" );
        st_assert( r->scope()->isInlinedScope(), "must be code scope" );
        r->computeUplevelAccesses();
    }
}


bool_t BasicBlockIterator::isSequentialCode( BasicBlock *curr, BasicBlock *next ) const {
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


BasicBlock *BasicBlockIterator::bb_for( Node *n ) {
    return n not_eq nullptr ? n->bb() : nullptr;
}


void BasicBlockIterator::add_BBs_to_list( GrowableArray<BasicBlock *> &list, GrowableArray<BasicBlock *> &work ) {
    if ( not work.isEmpty() ) {
        GrowableArray<BasicBlock *> uncommon;    // uncommon BBs are handled at the very end
        do {
            BasicBlock *bb = work.pop();
            st_assert( bb->visited(), "must have been visited" );
            list.append( bb );            // do this even if empty (label may be referenced)
            bb->setGenCount();        // can probably be removed at some point
            // determine likely & uncommon successors
            BasicBlock *likely_next   = bb_for( bb->_last->likelySuccessor() );
            BasicBlock *uncommon_next = bb_for( bb->_last->uncommonSuccessor() );
            st_assert( likely_next not_eq uncommon_next or likely_next == nullptr, "likely BasicBlock cannot be uncommon BasicBlock at the same time" );
            // first, push all other successors
            for ( std::size_t i = bb->nSuccessors(); i-- > 0; ) {
                BasicBlock *next = (BasicBlock *) bb->next( i );
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


GrowableArray<BasicBlock *> *BasicBlockIterator::code_generation_order() {
    if ( not ReorderBBs )
        return _basicBlockTable;
    // initialize visited field for all nodes
    for ( int                   i     = 0; i < _basicBlockCount; i++ )
        _basicBlockTable->at( i )->before_visit();
    // working sets
    GrowableArray<BasicBlock *> *list = new GrowableArray<BasicBlock *>( _basicBlockCount );    // eventually holds all reachable BBs again
    GrowableArray<BasicBlock *> work( _basicBlockCount );                // may hold only a part of all BBs at any given time
    // setup starting point
    BasicBlock                  *bb   = _first->bb();
    work.push( bb->after_visit() );
    // compute order
    add_BBs_to_list( *list, work );
    // NB: "loosing" BBs is ok since some may become unreachable during optimization  -Robert 7/96
    st_assert( list->length() <= _basicBlockCount, "gained BBs?" );
    return list;
}


void BasicBlockIterator::print_code( bool_t suppressTrivial ) {
    GrowableArray<BasicBlock *> *list = code_generation_order();
    for ( int                   i     = 0; i < list->length(); i++ ) {
        BasicBlock *bb = list->at( i );
        if ( bb->_nodeCount > 0 )
            bb->print_code( suppressTrivial );
        if ( bb->next() not_eq nullptr )
            _console->print_cr( "\tgoto N%d", bb->next()->_first->id() );
        if ( not suppressTrivial )
            lprintf( "\n" );
    }
}


void BasicBlockIterator::apply( NodeVisitor *v ) {
    GrowableArray<BasicBlock *> *list  = code_generation_order();
    int                         length = list->length();
    for ( int                   i      = 0; i < length; i++ ) {
        BasicBlock *bb    = list->at( i );
        Node       *first = bb->_first;
        Node       *last  = bb->_last;
        v->beginOfBasicBlock( first );
        bb->apply( v );
        v->endOfBasicBlock( last );
    }
}


bool_t BasicBlockIterator::verifyLabels() {
    bool_t    ok = true;
    for ( std::size_t i  = 0; i < _basicBlockCount; i++ )
        ok &= _basicBlockTable->at( i )->verifyLabels();
    return ok;
}


void BasicBlockIterator::globalCopyPropagate() {
    // do global copy propagation of singly-assigned PseudoRegisters

    for ( std::size_t i = 0; i < pregTable->length(); i++ ) {
        PseudoRegister *r = pregTable->at( i );
        if ( not r or r->isConstPseudoRegister() or not r->canCopyPropagate() )
            continue;
        Definition *def     = nullptr;
        // get definition
        int        dulength = r->_dus.length();
        int        e        = 0;
        for ( ; e < dulength; e++ ) {
            PseudoRegisterBasicBlockIndex *index = r->_dus.at( e );
            DefinitionUsageInfo           *info  = index->_basicBlock->duInfo.info->at( index->_index );
            if ( info->_definitions.length() ) {
                st_assert( info->_definitions.length() == 1, "should be one definition only" );
                st_assert( def == nullptr, "already have def!?!" );
                def = info->_definitions.first();
                break;
            }
        }
        st_assert( def, "should have found the definition" );
        NonTrivialNode *defNode = def->_node;

        // don't propagate if the defNode isn't assignment
        if ( not defNode->isAssignmentLike() )
            continue;
        st_assert( defNode->dest() == r, "not assignment-like" );

        PseudoRegister *defSrc = defNode->src();

        // don't propagate if source does not survive calls (i.e. is local to BasicBlock),
        // or if it isn't singly-assigned as well (if it's multiply assigned,
        // we'd have to make sure it's not assigned between defNode and the use nodes
        if ( not defSrc->isSinglyAssigned() or defSrc->_location.isTrashedRegister() )
            continue;

        // ok, everything is fine - propagate the def to the uses
        for ( e = 0; e < dulength; e++ ) {
            PseudoRegisterBasicBlockIndex *index = r->_dus.at( e );
            BasicBlock                    *bb    = index->_basicBlock;
            DefinitionUsageInfo           *info  = bb->duInfo.info->at( index->_index );
            // caution: propagateTo may eliminate nodes and thus shorten
            // info->uses
            int                           j      = 0;
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
    for ( std::size_t i = 0; i < pregTable->length(); i++ ) {
        PseudoRegister *r = pregTable->at( i );
        if ( r and r->isOnlySoftUsed() ) {
            r->eliminate( false );
        }
    }
}


void BasicBlockIterator::apply( BBDoFn f ) {
    for ( std::size_t i = 0; i < _basicBlockCount; i++ ) {
        f( _basicBlockTable->at( i ) );
    }
}


void BasicBlockIterator::verify() {
    if ( _basicBlockTable ) {
        std::size_t i = 0;
        for ( ; i < _basicBlockCount; i++ ) {
            _basicBlockTable->at( i )->verify();
        }
        if ( pregTable ) {
            for ( std::size_t i = 0; i < pregTable->length(); i++ ) {
                if ( pregTable->at( i ) )
                    pregTable->at( i )->verify();
            }
        }
    }
}



BasicBlockIterator *bbIterator;


void BasicBlockIterator::build( Node *first ) {
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
    GrowableArray<BasicBlock *> *list = new GrowableArray<BasicBlock *>( max( BasicNode::currentID >> 3, 10 ) );
    _first->newBasicBlock()->dfs( list, 0 );
    // now, the list contains the BBs in reverse order
    _basicBlockCount = list->length();
    _basicBlockTable = new GrowableArray<BasicBlock *>( _basicBlockCount );
    for ( std::size_t i = _basicBlockCount - 1; i >= 0; i-- ) {
        BasicBlock *bb = list->at( i );
        bb->_id = _basicBlockCount - i - 1;
        _basicBlockTable->append( bb );
    }
}
