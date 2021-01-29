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
#include "vm/compiler/DefinitionUsageInfo.hpp"


void DefinitionUsageInfo::propagateTo( BasicBlock *useBasicBlock, Usage *use, const NonTrivialNode *fromNode, PseudoRegister *src, NonTrivialNode *toNode, const bool global ) {
    // r1 := r2; ...; r3 := op(r1)  -->  r1 := r2; ...; r3 := op(r2)
    bool ok = toNode->copyPropagate( useBasicBlock, use, src );
    if ( CompilerDebug ) {
        cout( PrintCopyPropagation )->print( "*%s copy-propagation:%s propagate %s from N%ld (0x{0:x}) to N%ld (0x{0:x})\n", global ? "global" : "local", ok ? "" : " couldn't", src->name(), fromNode ? fromNode->id() : 0, PrintHexAddresses ? fromNode : 0, toNode->id(), PrintHexAddresses ? toNode : 0 );
    }
}


void DefinitionUsageInfo::propagateTo( BasicBlock *useBasicBlock, const PseudoRegister *r, const Definition *def, Usage *use, const bool global ) {
    // def reaches use; try to eliminate r's use by using copy propagation
    NonTrivialNode *fromNode    = def->_node;
    const bool     isAssignment = fromNode->isAssignmentLike();
    NonTrivialNode *toNode      = use->_node;
    const bool     hasSrc       = fromNode->hasSrc();
    PseudoRegister *fromPR      = hasSrc ? fromNode->src() : nullptr;
    const bool     isConst      = hasSrc and fromPR->isConstPseudoRegister();

    if ( isAssignment and isConst and toNode->canCopyPropagateOop() ) {
        // loadOop r1, Oop; ...; r2 := op(r1)    --->
        // loadOop r1, Oop; ...; r2 := op(Oop)
        bool ok = toNode->copyPropagate( useBasicBlock, use, fromPR );
        if ( CompilerDebug ) {
            // the original code broke the MSC++ 3.0 optimizer, so now it looks a bit weird.  -Urs 7/96
            const char *glob = global ? "global" : "local";
            const char *prop = ok ? "" : " couldn't propagate";
            const char *name = def->_node->src()->name();        // using fromNode or fromPR here will break
            void       *from = PrintHexAddresses ? fromNode : 0;
            void       *to   = PrintHexAddresses ? toNode : 0;
            cout( PrintCopyPropagation )->print( "*%s copy-propagation:%s %s from N%ld (0x{0:x}) to N%ld (0x{0:x})\n", glob, prop, name, def->_node->id(), from, toNode->id(), to );
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
            cout( PrintCopyPropagation )->print( "*global copy-propagation: can't propagate %s from N%ld (0x{0:x}) to N%ld (0x{0:x}) -- loopDepth mismatch\n", fromPR->name(), fromNode->id(), PrintHexAddresses ? fromNode : 0, toNode->id(), PrintHexAddresses ? toNode : 0 );
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
        // std::int32_t as this optimization isn't performed globally this shouldn't hurt
        if ( CompilerDebug ) {
            cout( PrintCopyPropagation )->print( "*%s copy-propagation: changing dest of N%ld (0x{0:x}) to %s to match use at N%ld (0x{0:x})\n", global ? "global" : "local", fromNode->id(), PrintHexAddresses ? fromNode : 0, toNode->dest()->name(), toNode->id(), PrintHexAddresses ? toNode : 0 );
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
        cout( PrintCopyPropagation )->print( "*%s copy-propagation: can't propagate N%ld (0x{0:x}) to N%ld (0x{0:x})\n", global ? "global" : "local", fromNode->id(), PrintHexAddresses ? fromNode : 0, toNode->id(), PrintHexAddresses ? toNode : 0 );
    }
}


void DefinitionUsageInfo::getLiveRange( std::int32_t &firstNodeNum, std::int32_t &lastNodeNum ) {
    // compute live range of a local register
    // IN:  first & last node ID of BasicBlock
    // OUT: first & last node ID where this PseudoRegister is live
    // algorithm: first = first def if def precedes first use
    //			  0 if first use precedes def (live from start)
    // 		  last  = last use if it is later than last def
    //		  	  lastNodeNum if last def is later (live until end)
    SListElem<Definition *> *d = _definitions.head();
    SListElem<Usage *>      *u = _usages.head();

    // set firstNodeNum
    if ( u and d ) {
        std::int32_t unode = u->data()->_node->num();
        std::int32_t dnode = d->data()->_node->num();
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
    std::int32_t unode = u->data()->_node->num();
    std::int32_t dnode = d->data()->_node->num();
    if ( unode > dnode ) {
        lastNodeNum = unode;    // live range ends at last use
    } else {
        // defined last -- live until end of BasicBlock
    }
}


void DefinitionUsageInfo::print_short() {
    spdlog::info( "DefinitionUsageInfo 0x{0:x}", static_cast<void *>( this ) );
}


void DefinitionUsageInfo::print() {
    print_short();
    spdlog::info( " for " );
    _pseudoRegister->print_short();
    spdlog::info( ": " );
    _usages.print();
    spdlog::info( "; " );
    _definitions.print();
    spdlog::info( "" );
}
