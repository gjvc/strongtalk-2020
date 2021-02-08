
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/assembler/Assembler.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/CopyPropagationInfo.hpp"
#include "vm/compiler/BasicBlock.hpp"


std::int32_t                                 PseudoRegister::currentNo       = 0;
std::int32_t                                 BlockPseudoRegister::_numBlocks = 0;
static GrowableArray<ConstPseudoRegister *>  *constants                      = 0;
static PseudoRegister                        *dummyPR;
const std::int32_t                           PseudoRegister::AvgBBIndexLen   = 10;
const std::int32_t                           PseudoRegister::VeryNegative    = -9999;        // fix this -- should be std::int16_t, really

#define BAD_SCOPE  ((InlinedScope*)1)


LogicalAddress *PseudoRegister::createLogicalAddress() {
    if ( _logicalAddress == nullptr ) {
        _logicalAddress = theCompiler->scopeDescRecorder()->createLogicalAddress( nameNode() );
    };
    return _logicalAddress;
}



//LogicalAddress * PseudoRegister::createLogicalAddress() {
//    PseudoRegister * r = cpseudoRegister();
//    if ( r->_logicalAddress == nullptr ) {
//        r->_logicalAddress = theCompiler->scopeDescRecorder()->createLogicalAddress( nameNode() );
//    };
//    return r->_logicalAddress;
//}
//

// usage / definition weights indexed by loop depth
static const std::int32_t udWeight[]  = { 1, 8, 8 * 8, 8 * 8 * 8, 8 * 8 * 8 * 8 };
const std::int32_t        udWeightLen = sizeof( udWeight ) / sizeof( std::int32_t ) - 1;


void PseudoRegister::initPseudoRegisters() {
    PseudoRegister::currentNo       = 0;
    BlockPseudoRegister::_numBlocks = 0;
    constants                       = new GrowableArray<ConstPseudoRegister *>( 50 );
    dummyPR                         = new PseudoRegister( BAD_SCOPE );
}


SinglyAssignedPseudoRegister::SinglyAssignedPseudoRegister( InlinedScope *s, std::int32_t stream, std::int32_t en, bool inContext ) :
    PseudoRegister( s ),
    _isInContext( inContext ),
    _creationScope{ nullptr },
    _begByteCodeIndex{ 0 },
    _endByteCodeIndex{ 0 },
    _creationStartByteCodeIndex{ 0 } {

    //
    _creationStartByteCodeIndex = stream == IllegalByteCodeIndex ? s->byteCodeIndex() : stream;
    _begByteCodeIndex           = stream == IllegalByteCodeIndex ? s->byteCodeIndex() : stream;
    _endByteCodeIndex           = en == IllegalByteCodeIndex ? s->byteCodeIndex() : en;
    _creationScope              = s;
}


BlockPseudoRegister::BlockPseudoRegister( InlinedScope *scope, CompileTimeClosure *closure, std::int32_t beg, std::int32_t end ) :
    SinglyAssignedPseudoRegister( scope, beg, end ),
    _closure{ closure },
    _memoized{ false },
    _escapes{ false },
    _escapeNodes{ nullptr },
    _uplevelRead{ nullptr },
    _uplevelWritten{ nullptr },
    _contextCopies{ nullptr } {

    //
    st_assert( closure, "need a closure" );
    _numBlocks++;
    theCompiler->blockClosures->append( this );
    if ( MemoizeBlocks ) {
        memoize();
    }
}


void BlockPseudoRegister::addContextCopy( Location *l ) {
    if ( not _contextCopies ) {
        _contextCopies = new GrowableArray<Location *>( 3 );
    }

    _contextCopies->append( l );
}


void PseudoRegister::makeIncorrectDU( bool incU, bool incD ) {

    if ( incU ) {
        _usageCount = VeryNegative;
    }

    if ( incD ) {
        _definitionCount = VeryNegative;
    }

}


bool PseudoRegister::isLocalTo( BasicBlock *bb ) const {
    // is this a pseudoRegister local to bb? (i.e. can it be allocated to temp regs?)
    // treat ConstPseudoRegisters as non-local so they don't get allocated prematurely
    // (possible performance bug)
    return _location.equals( Location::UNALLOCATED_LOCATION ) and not uplevelR() and not _debug and not incorrectDU() and not isConstPseudoRegister() and _dus.length() == 1 and _dus.first()->_basicBlock == bb;
}


// check basic conditions for global copy-propagation
bool PseudoRegister::canCopyPropagate() const {

    if ( nuses() == 0 or ndefs() not_eq 1 )
        return false;

    // don't propagate if register has incorrect def info or does not survive calls (i.e. is local to BasicBlock)
    if ( incorrectD() or _location.isTrashedRegister() )
        return false;

    return true;
}


// NB: _uplevelR/W are initialized lazily to reduce memory consumption
void PseudoRegister::addUplevelAccessor( BlockPseudoRegister *blk, bool read, bool write ) {

    if ( read ) {
        if ( not _uplevelReaders )
            _uplevelReaders = new GrowableArray<BlockPseudoRegister *>( 5 );

        if ( not _uplevelReaders->contains( blk ) )
            _uplevelReaders->append( blk );
    }

    if ( write ) {
        if ( not _uplevelWriters )
            _uplevelWriters = new GrowableArray<BlockPseudoRegister *>( 5 );

        if ( not _uplevelWriters->contains( blk ) )
            _uplevelWriters->append( blk );
    }
}


void PseudoRegister::removeUplevelAccessor( BlockPseudoRegister *blk ) {

    if ( _uplevelReaders ) {
        if ( _uplevelReaders->contains( blk ) )
            _uplevelReaders->remove( blk );
        if ( _uplevelReaders->isEmpty() )
            _uplevelReaders = nullptr;
    }

    if ( _uplevelWriters ) {
        if ( _uplevelWriters->contains( blk ) )
            _uplevelWriters->remove( blk );
        if ( _uplevelWriters->isEmpty() )
            _uplevelWriters = nullptr;
    }

}


void PseudoRegister::removeAllUplevelAccessors() {
    _uplevelReaders = _uplevelWriters = nullptr;
}


ConstPseudoRegister *new_ConstPseudoRegister( InlinedScope *s, Oop c ) {

    for ( std::int32_t i = 0; i < constants->length(); i++ ) {
        ConstPseudoRegister *r = constants->at( i );
        if ( r->constant == c ) {
            // needed to ensure high enough scope (otherwise will break assertions) but in reality it's not needed since ConstPseudoRegisters aren't register-allocated right now
            r->extendLiveRange( s );
            return r;
        }
    }

    // constant not found, create new ConstPseudoRegister*
    ConstPseudoRegister *r = new ConstPseudoRegister( s, c );
    constants->append( r );
    r->_definitionCount = 1;    // fake def

    return r;
}


ConstPseudoRegister *findConstPseudoRegister( Node *n, Oop c ) {

    // return const pseudoRegister for Oop or nullptr if none exists
    for ( std::int32_t i = 0; i < constants->length(); i++ ) {
        ConstPseudoRegister *r = constants->at( i );
        if ( r->constant == c ) {
            return r->covers( n ) ? r : nullptr;
        }
    }

    return nullptr;                    // constant not found
}


bool ConstPseudoRegister::needsRegister() const {
    // register only pays off if we're used more than once and aren't a small immediate constant
    //   return CompilerCSEConstants and weight > 1 and (std::int32_t(constant) > maxImmediate or std::int32_t(constant) < -maxImmediate);
    return false;
}


void ConstPseudoRegister::allocateTo( Location reg ) {
    st_assert( reg.isRegisterLocation(), "should be a register" );
    _location = reg;
    st_assert( _scope->isInlinedScope(), "expected non-access scope" );
//       ((InlinedScope*)_scope)->allocateConst(this);
    Unimplemented();
}


std::int32_t computeWeight( InlinedScope *s ) {
    constexpr std::int32_t scale = 16;    // normal use counts scale, uncommon use is 1
    if ( s and s->isInlinedScope() and ( (InlinedScope *) s )->primFailure() ) {
        return 1 * udWeight[ min( udWeightLen, s->loopDepth ) ];
    } else {
        return scale * udWeight[ min( udWeightLen, s ? s->loopDepth : 0 ) ];
    }
}


void PseudoRegister::incUses( Usage *use ) {
    _usageCount++;
    if ( use->isSoft() )
        _softUsageCount++;
    InlinedScope *s = use->_node->scope();
    _weight += computeWeight( s );
    st_assert( _weight >= _usageCount + _definitionCount or isConstPseudoRegister(), "weight too small" );
}


void PseudoRegister::decUses( Usage *use ) {
    _usageCount--;
    if ( use->isSoft() )
        _softUsageCount--;
    InlinedScope *s = use->_node->scope();
    _weight -= computeWeight( s );
    st_assert( _weight >= _usageCount + _definitionCount or isConstPseudoRegister(), "weight too small" );
}


void PseudoRegister::incDefs( Definition *def ) {
    _definitionCount++;
    InlinedScope *s = def->_node->scope();
    _weight += computeWeight( s );
    st_assert( _weight >= _usageCount + _definitionCount or isConstPseudoRegister(), "weight too small" );
}


void PseudoRegister::decDefs( Definition *def ) {
    _definitionCount--;
    InlinedScope *s = def->_node->scope();
    _weight -= computeWeight( s );
    st_assert( _weight >= _usageCount + _definitionCount or isConstPseudoRegister(), "weight too small" );
}


void PseudoRegister::removeUse( DefinitionUsageInfo *info, Usage *use ) {
    st_assert( info->_pseudoRegister == this, "wrong reg" );
    info->_usages.remove( use );
    decUses( use );
}


void PseudoRegister::removeUse( BasicBlock *bb, Usage *use ) {
    if ( use == nullptr ) {
        return;
    }

    for ( std::int32_t i = 0; i < _dus.length(); i++ ) {
        PseudoRegisterBasicBlockIndex *index = _dus.at( i );
        if ( index->_basicBlock == bb ) {
            DefinitionUsageInfo *info = bb->duInfo.info->at( index->_index );
            removeUse( info, use );
            return;
        }
    }
    ShouldNotReachHere(); // info not found
}


void PseudoRegister::removeDef( DefinitionUsageInfo *info, Definition *def ) {
    st_assert( info->_pseudoRegister == this, "wrong reg" );
    info->_definitions.remove( def );
    _definitionCount--;
    InlinedScope *s = def->_node->scope();
    _weight -= computeWeight( s );
    st_assert( _weight >= _usageCount + _definitionCount, "weight too small" );
}


void PseudoRegister::removeDef( BasicBlock *bb, Definition *def ) {
    if ( def == nullptr )
        return;
    for ( std::int32_t i = 0; i < _dus.length(); i++ ) {
        PseudoRegisterBasicBlockIndex *index = _dus.at( i );
        if ( index->_basicBlock == bb ) {
            DefinitionUsageInfo *info = bb->duInfo.info->at( index->_index );
            removeDef( info, def );
            return;
        }
    }
    ShouldNotReachHere(); // info not found
}


void PseudoRegister::addDUHelper( Node *n, SList<DefinitionUsage *> *l, DefinitionUsage *el ) {
    std::int32_t                 myNum = n->num();
    SListElem<DefinitionUsage *> *prev = nullptr;

    for ( SListElem<DefinitionUsage *> *e = l->head(); e and e->data()->_node->num() < myNum; prev = e, e = e->next() ) {
        static_cast<void>( 0 );
    }
    l->insertAfter( prev, el );
}


Usage *PseudoRegister::addUse( DefinitionUsageInfo *info, NonTrivialNode *n ) {
    st_assert( info->_pseudoRegister == this, "wrong reg" );
    Usage *u = new Usage( n );
    addDUHelper( n, (SList<DefinitionUsage *> *) &info->_usages, u );
    incUses( u );
    return u;
}


Usage *PseudoRegister::addUse( BasicBlock *bb, NonTrivialNode *n ) {
    for ( std::int32_t i = 0; i < _dus.length(); i++ ) {
        PseudoRegisterBasicBlockIndex *index = _dus.at( i );
        if ( index->_basicBlock == bb ) {
            DefinitionUsageInfo *info = bb->duInfo.info->at( index->_index );
            return addUse( info, n );
        }
    }
    return bb->addUse( n, this );
}


Definition *PseudoRegister::addDef( DefinitionUsageInfo *info, NonTrivialNode *n ) {
    st_assert( info->_pseudoRegister == this, "wrong reg" );
    Definition *d = new Definition( n );
    addDUHelper( n, (SList<DefinitionUsage *> *) &info->_definitions, d );
    incDefs( d );
    return d;
}


Definition *PseudoRegister::addDef( BasicBlock *bb, NonTrivialNode *n ) {
    for ( std::int32_t i = 0; i < _dus.length(); i++ ) {
        PseudoRegisterBasicBlockIndex *index = _dus.at( i );
        if ( index->_basicBlock == bb ) {
            DefinitionUsageInfo *info = bb->duInfo.info->at( index->_index );
            return addDef( info, n );
        }
    }
    return bb->addDef( n, this );
}


void PseudoRegister::forAllDefsDo( Closure<Definition *> *c ) {
    ::forAllDefsDo( &_dus, c );
}


void PseudoRegister::forAllUsesDo( Closure<Usage *> *c ) {
    ::forAllUsesDo( &_dus, c );
}


void PseudoRegister::allocateTo( Location r ) {
    if ( CompilerDebug ) {
        cout( PrintRegAlloc )->print( "allocating PseudoRegister %s to Location %s\n", name(), r.name() );
    }

    st_assert( _location.equals( Location::UNALLOCATED_LOCATION ), "already allocated" );
    _location = r;
}


bool PseudoRegister::extendLiveRange( Node *n ) {
    // the receiver is being copy-propagated to n
    // PseudoRegisters currently can't be propagated outside their scope
    // should fix copy-propagation: treat all PseudoRegisters like SinglyAssignedPseudoRegister so can propagate anywhere?
    return extendLiveRange( n->scope(), n->byteCodeIndex() );
}


bool PseudoRegister::extendLiveRange( InlinedScope *s, std::int32_t byteCodeIndex ) {
    static_cast<void>(byteCodeIndex); // unused

    // the receiver is being copy-propagated to n
    // PseudoRegisters currently can't be propagated outside their scope
    // should fix copy-propagation: treat all PseudoRegisters like SinglyAssignedPseudoRegister so can propagate anywhere?
    if ( s == _scope ) {
        return true;    // ok, same scope
    } else if ( _scope->isSenderOf( s ) ) {
        return true;    // scope is caller; already covers n
    } else {
        return false;
    }
}


bool SinglyAssignedPseudoRegister::extendLiveRange( Node *n ) {
    // the receiver is being copy-propagated to n; try to extend its live range
    return extendLiveRange( n->scope(), n->byteCodeIndex() );
}


bool SinglyAssignedPseudoRegister::extendLiveRange( InlinedScope *s, std::int32_t byteCodeIndex ) {
    // the receiver is being copy-propagated to scope s at byteCodeIndex; try to extend its live range
    st_assert( _begByteCodeIndex not_eq IllegalByteCodeIndex and _creationStartByteCodeIndex not_eq IllegalByteCodeIndex and _endByteCodeIndex not_eq IllegalByteCodeIndex, "live range not set" );
    if ( isInContext() ) {
        // context locations cannot be propagated beyond their scope
        // (otherwise the context pointer's live range would have to be extended)
        // was bug 5/3/96  -Urs
        return PseudoRegister::extendLiveRange( s, byteCodeIndex );
    }
    bool ok = true;
    if ( s == _scope ) {
        if ( byteCodeIndexLT( byteCodeIndex, _begByteCodeIndex ) ) {
            // seems like we're propagating backwards!  happens because of the non-source
            // order of the byte codes in while loops (condition comes after body), so
            // when propagating from code in the condition into code in the body it looks
            // like we're going backwards
            // can't handle this yet -- Urs 7/95
            return false;
        }
        if ( byteCodeIndexGT( byteCodeIndex, _endByteCodeIndex ) )
            _endByteCodeIndex = byteCodeIndex;

    } else if ( s->isSenderOf( _scope ) ) {
        // propagating upwards - promote receiver to higher scope
        InlinedScope *ss = _scope;
        for ( ; ss->sender() not_eq s; ss = ss->sender() );
        _scope            = s;
        _begByteCodeIndex = ss->senderByteCodeIndex();
        _endByteCodeIndex = byteCodeIndex;

    } else if ( _scope->isSenderOf( s ) ) {
        // scope is callee; check if already covered
        InlinedScope *ss = s;
        for ( ; ss->sender() not_eq _scope; ss = ss->sender() );
        std::int32_t byteCodeIndex = ss->senderByteCodeIndex();
        if ( byteCodeIndexLT( byteCodeIndex, _begByteCodeIndex ) ) {
            // seems like we're propagating backwards!  happens because of the non-source
            // order of the byte codes in while loops (condition comes after body), so
            // when propagating from code in the condition into code in the body it looks
            // like we're going backwards
            // can't handle this yet -- Urs 7/95
            return false;
        }
        if ( byteCodeIndexGT( byteCodeIndex, _endByteCodeIndex ) )
            _endByteCodeIndex = byteCodeIndex;

    } else {
        // can't propagate between siblings yet
        ok = false;
    }
    st_assert( byteCodeIndexLE( _begByteCodeIndex, _endByteCodeIndex ) and _begByteCodeIndex not_eq IllegalByteCodeIndex and ( byteCodeIndexLE( _endByteCodeIndex, scope()->nofBytes() ) or _endByteCodeIndex == EpilogueByteCodeIndex ), "invalid start/endByteCodeIndex" );
    return ok;
}


void ConstPseudoRegister::extendLiveRange( InlinedScope *s ) {
    // make sure the constant reg is in a high enough scope
    if ( _scope->isSenderOrSame( s ) ) {
        // scope is caller of s
    } else if ( s->isSenderOf( _scope ) ) {
        // s is caller of scope, so promote receiver to s
        _scope = s;
    } else {
        // scope and s are siblings of some sort - go up to common sender
        do {
            s = s->sender();
        } while ( not s->isSenderOf( _scope ) );
        _scope = s;
    }
}


bool ConstPseudoRegister::covers( Node *n ) const {
    // does receiver cover node n (is it live at n)?
    InlinedScope *s = n->scope();
    if ( _scope->isSenderOrSame( s ) ) {
        // ok, scope is caller of s
        return true;
    }
    return false;
}


bool ConstPseudoRegister::extendLiveRange( Node *n ) {
    extendLiveRange( n->scope() );
    return true;
}


bool PseudoRegister::checkEquivalentDefs() const {
    // check if all definitions are equivalent, i.e. assign the same pseudoRegister
    if ( ndefs() == 1 ) {
        return true;
    }

    PseudoRegister *rhs = nullptr;

    for ( std::int32_t i = 0; i < _dus.length(); i++ ) {
        PseudoRegisterBasicBlockIndex *index = _dus.at( i );
        BasicBlock                    *bb    = index->_basicBlock;
        DefinitionUsageInfo           *info  = bb->duInfo.info->at( index->_index );
        for ( SListElem<Definition *> *e     = info->_definitions.head(); e; e = e->next() ) {
            NonTrivialNode *n = e->data()->_node;
            if ( not n->isAssignmentLike() )
                return false;
            if ( rhs ) {
                if ( rhs not_eq n->src() )
                    return false;
            } else {
                rhs = n->src();
            }
        }
    }
    // yup, rhs is the only pseudoRegister ever assigned to me
    return true;
}


bool PseudoRegister::canBeEliminated( bool withUses ) const {

    // can this PseudoRegister be eliminated without compromising the debugging info?
    st_assert( _usageCount == 0 or withUses, "still has uses" );
    if ( _definitionCount + _usageCount == 0 )
        return false;    // nothing to eliminate

    // check if reg can be eliminated
    if ( incorrectDU() ) { // don't elim if uses are incorrect (hardwired pseudoRegisters)
        return false;
    }

    st_assert( not _softUsageCount or _debug, "nsoftUses should imply debug" );

    if ( isBlockPseudoRegister() and not withUses and not uplevelR() ) {
        // blocks can always be eliminated - can describe with BlockValueDesc
        return true;
    }

    if ( _debug ) {
        // debug-visible or uplevel-read: eliminate only if run-time value
        // can be reconstructed
        if ( _copyPropagationInfo ) {
            // already computed cpInfo, thus can be eliminated
            st_assert( not cpseudoRegister()->_location.isLocalRegister(), "shouldn't be eliminated (was bug 4/27  -Urs)" );
            return true;
        }
        if ( _definitionCount > 1 ) {
            if ( isBlockPseudoRegister() ) {
                // ok; we know all definitions of a block are equivalent
            } else if ( isSinglyAssignedPseudoRegister() and checkEquivalentDefs() ) {
                // ok, all definitions are the same
            } else {
                if ( not checkEquivalentDefs() ) {
                    if ( CompilerDebug )
                        cout( PrintEliminateUnnededNodes )->print( "*not eliminating %s: >1 def and debug-visible\n", name() );
                    return false;
                }
            }
        }
        PseudoRegisterBasicBlockIndex *index = _dus.first();
        DefinitionUsageInfo           *info  = index->_basicBlock->duInfo.info->at( index->_index );
        SListElem<Definition *>       *e     = info->_definitions.head();
        if ( not e ) {
            // info not in first elem - would have to search
            if ( CompilerDebug )
                cout( PrintEliminateUnnededNodes )->print( "*not eliminating %s: def not in first info\n", name() );
            return false;
        }
        NonTrivialNode *defNode = e->data()->_node;
        PseudoRegister *defSrc;
        bool           ok;
        if ( defNode->hasConstantSrc() ) {
            // constant assignment - easy to handle
            ok = true;
        } else if ( defNode->hasSrc() and ( defSrc = defNode->src() )->isSinglyAssignedPseudoRegister() and not defSrc->_location.isRegisterLocation() ) {

            // can substitute defSrc if its lifetime encompasses ours and if
            // it is singly-assigned and not a temp reg (last cond. is necessary to
            // prevent e.g. result of a send (in eax) to be used as the receiver of
            // subsequent scopes that have nsends > 0; fix this if it becomes a
            // performance problem)

            ok = defSrc->scope()->isSenderOf( _scope );
            if ( not ok and defSrc->scope() == _scope and isSinglyAssignedPseudoRegister() ) {
                // same scope, ok if defSrc lives std::int32_t enough
                ok = byteCodeIndexGE( ( (SinglyAssignedPseudoRegister *) defSrc )->endByteCodeIndex(), endByteCodeIndex() );
            }
            if ( not ok ) {
                // try to extend defSrc's live range to cover ours
                ok = defSrc->extendLiveRange( _scope, endByteCodeIndex() );
            }

        } else {
            ok = false;
        }

        if ( not ok ) {
            if ( CompilerDebug )
                cout( PrintEliminateUnnededNodes )->print( "*not eliminating %s: can't recover debug info\n", name() );
            return false;                // can't eliminate this PseudoRegister
        }
    }
    return true;
}


bool BlockPseudoRegister::canBeEliminated( bool withUses ) const {
    if ( not PseudoRegister::canBeEliminated( withUses ) )
        return false;

    if ( not _escapes )
        return true;

    if ( uplevelR() )
        return false;

    // escaping, unused block; can be eliminated
    // also, the block doesn't escape anymore
    // _escapes = false;
    st_assert( nuses() == 0, "still has uses" );
    return true;
}


// eliminate all nodes defining me (if possible)
void PseudoRegister::eliminate( bool withUses ) {
    if ( not canBeEliminated( withUses ) )
        return;
    for ( std::int32_t i = 0; i < _dus.length(); i++ ) {
        PseudoRegisterBasicBlockIndex *index = _dus.at( i );
        BasicBlock                    *bb    = index->_basicBlock;
        DefinitionUsageInfo           *info  = bb->duInfo.info->at( index->_index );
        eliminateDefs( info, bb, withUses );
        if ( withUses )
            eliminateUses( info, bb );
    }
}


void PseudoRegister::eliminateUses( DefinitionUsageInfo *info, BasicBlock *bb ) {

    // eliminate all use nodes in info
    SListElem<Usage *> *usageElement = info->_usages.head();
    while ( usageElement ) {
        std::int32_t oldLength = info->_usages.length();      // for debugging
        Node         *n        = usageElement->data()->_node;
        if ( CompilerDebug ) {
            char buf[1024];
            cout( PrintEliminateUnnededNodes )->print( "*%seliminating node N%ld: %s\n", n->canBeEliminated() ? "" : "not ", n->id(), n->toString( buf ) );
        }
        st_assert( n->canBeEliminated(), "must be able to eliminate this" );
        n->eliminate( bb, this );
        st_assert( info->_usages.length() < oldLength, "didn't remove use" );
        usageElement = info->_usages.head();
    }

}


void PseudoRegister::eliminateDefs( DefinitionUsageInfo *info, BasicBlock *bb, bool removing ) {
    // eliminate all defining nodes in info
    SListElem<Definition *> *e = info->_definitions.head();
    while ( e ) {
        std::int32_t   oldlen = info->_definitions.length();      // for debugging
        NonTrivialNode *n     = e->data()->_node;
        if ( n->canBeEliminated() ) {
            updateCPInfo( n );
            n->eliminate( bb, this );
            st_assert( info->_definitions.length() < oldlen, "didn't remove def" );
            e = info->_definitions.head();        // simple, but may rescan some uneliminatable nodes
        } else {
            if ( CompilerDebug ) {
                char buf[1024];
                cout( PrintEliminateUnnededNodes )->print( "*not eliminating node N%ld: %s\n", n->id(), n->toString( buf ) );
            }
            st_assert( not removing, "cannot eliminate this?" );
            e = e->next();
        }
    }
}


void BlockPseudoRegister::eliminate( bool withUses ) {
    PseudoRegister::eliminate( withUses );
    if ( nuses() == 0 ) {
        // the block has been eliminated; remove the uplevel accesses
        // (needed to enable eliminating the accessed contexts)
        if ( _uplevelRead ) {
            for ( std::int32_t i = _uplevelRead->length() - 1; i >= 0; i-- )
                _uplevelRead->at( i )->removeUplevelAccessor( this );
        }
        if ( _uplevelWritten ) {
            for ( std::int32_t i = _uplevelWritten->length() - 1; i >= 0; i-- )
                _uplevelWritten->at( i )->removeUplevelAccessor( this );
        }
    }
}


void PseudoRegister::updateCPInfo( NonTrivialNode *n ) {
    // update/compute cpInfo to keep track of copy-propagation effects for debugging info
    if ( _copyPropagationInfo ) {
        if ( _debug ) {
            // canBeEliminated assures that all definitions are equivalent
            CopyPropagationInfo *cpi = new_CPInfo( n );
            st_assert( cpi and cpi->_register->cpseudoRegister() == cpseudoRegister(), "can't handle this" );
        } else {
            // can't really handle copy-propagation w/multiple definitions; make sure we don't use
            // bad information
            _copyPropagationInfo->_register = dummyPR;
        }
    } else {
        _copyPropagationInfo = new_CPInfo( n );
        st_assert( not _debug or _copyPropagationInfo or isBlockPseudoRegister(), "couldn't create info" );
        if ( _copyPropagationInfo ) {
            PseudoRegister *r       = _copyPropagationInfo->_register;
            // if we're eliminating a debug-visible PseudoRegister, the replacement
            // must be debug-visible, too (so that it isn't allocated to
            // a temp reg)
            r->_debug |= _debug;
            if ( r->cpseudoRegisters == nullptr )
                r->cpseudoRegisters = new GrowableArray<PseudoRegister *>( 5 );
            r->cpseudoRegisters->append( this );
        }
    }
}


// for efficiency, node n in isLiveAt() must be "plausible", i.e. in a
// scope somewhere below the receiver's scope
// otherwise, use slow_isLiveAt

bool PseudoRegister::slow_isLiveAt( Node *n ) const {
    if ( _scope->isSenderOrSame( n->scope() ) ) {
        return isLiveAt( n );
    } else {
        return false;
    }
}


bool PseudoRegister::isLiveAt( Node *n ) const {
    // pseudoRegisters are live in the entire scope (according to Urs, 2/24/96)
    if ( not _scope->isSenderOrSame( n->scope() ) )
        return false; // cannot be live anymore if s is outside subscopes of _scope
    st_assert( PrologueByteCodeIndex == begByteCodeIndex() and endByteCodeIndex() == EpilogueByteCodeIndex, "must be live in the entire scope" );
    return true;
}


InlinedScope *findAncestor( InlinedScope *s1, std::int32_t &byteCodeIndex1, InlinedScope *s2, std::int32_t &byteCodeIndex2 ) {
    // find closest common ancestor of s1 and s2, and the
    // respective sender byteCodeIndexs in that scope
    if ( s1->depth > s2->depth ) {
        while ( s1->depth > s2->depth ) {
            byteCodeIndex1 = s1->senderByteCodeIndex();
            s1             = s1->sender();
        }
    } else {
        while ( s2->depth > s1->depth ) {
            byteCodeIndex2 = s2->senderByteCodeIndex();
            s2             = s2->sender();
        }
    }
    st_assert( s1->depth == s2->depth, "just checkin'..." );
    while ( s1 not_eq s2 ) {
        byteCodeIndex1 = s1->senderByteCodeIndex();
        s1             = s1->sender();
        byteCodeIndex2 = s2->senderByteCodeIndex();
        s2             = s2->sender();
    }
    st_assert( s1->isInlinedScope(), "oops" );
    return (InlinedScope *) s1;
}


bool SinglyAssignedPseudoRegister::isLiveAt( Node *n ) const {
    // is receiver live at Node n?  (may give conservative answer; i.e., it's ok to
    // return true even if the receiver is provably dead)
    // check if receiver is live in source-level terms; if that says
    // dead it really means dead
    InlinedScope *s   = n->scope();
    bool         live = basic_isLiveAt( s, n->byteCodeIndex() );
    if ( not live or not _location.isTemporaryRegister() )
        return live;
    st_fatal( "cannot handle temp registers" );
    return false;
}


bool SinglyAssignedPseudoRegister::basic_isLiveAt( InlinedScope *s, std::int32_t byteCodeIndex ) const {
//    std::int32_t id = this->id();

    if ( not _scope->isSenderOrSame( s ) ) {
        return false; // cannot be live anymore if s is outside subscopes of _scope
    }

    st_assert( byteCodeIndexLE( byteCodeIndex, s->nofBytes() ) or byteCodeIndex == EpilogueByteCodeIndex, "byteCodeIndex too high" );
    st_assert( _scope->isSenderOrSame( s ), "s is not below my scope" );

    // find closest common ancestor of s and creationScope, and the
    // respective byteCodeIndexs in that scope
    std::int32_t bs  = byteCodeIndex;
    std::int32_t bc  = _creationStartByteCodeIndex;
    InlinedScope *ss = findAncestor( s, bs, creationScope(), bc );
    if ( not _scope->isSenderOrSame( ss ) ) {
        st_fatal( "bad scope arg in basic_isLiveAt" );
    }

    // Attention: Originally, the live range of a PseudoRegister excluded its defining node.
    // The new backend however requires them to be live at the beginning as well.
    // The problem comes from situations where a PseudoRegister is used over a sequence of
    // nodes that all belong to the same byteCodeIndex. In general this has been solved in
    // the backend by killing PseudoRegisters only at byteCodeIndex boundaries. However, with inlining
    // turned on, this became more difficult: resultPRs are defined and used over
    // byteCodeIndex boundaries (from callee to caller), but when actually testing for liveness,
    // the test sees only the range in the caller, which is all in the same byteCodeIndex.
    // By extending the live range so that it includes its definition this is not
    // a problem anymore.
    //
    // Note: the isLiveAt methods are only used by the new backend (gri 3/27/96).
    if ( ss == _scope ) {
        // live range = [startByteCodeIndex, endByteCodeIndex]			// originally: ]startByteCodeIndex, endByteCodeIndex]
        st_assert( ( _begByteCodeIndex == bc ) or ( ss == creationScope() and _creationStartByteCodeIndex == bc ), "oops" );
        return byteCodeIndexLE( _begByteCodeIndex, bs ) and byteCodeIndexLE( bs, _endByteCodeIndex );    // originally: byteCodeIndexLT(_begByteCodeIndex, bs) and byteCodeIndexLE(bs, _endByteCodeIndex);
    } else {
        // live range = [bc, end of scope]			// originally: ]bc, end of scope]
        return byteCodeIndexLE( bc, bs );                // originally: byteCodeIndexLT(bc, bs);
    }
}


bool PseudoRegister::isCPEquivalent( PseudoRegister *r ) const {

    // is receiver in same register as argument?
    if ( this == r )
        return true;

    // try receiver's copy-propagation info
    for ( CopyPropagationInfo *i = _copyPropagationInfo; i and i->_register; i = i->_register->_copyPropagationInfo ) {
        if ( i->_register == r )
            return true;
    }

    // now try the other way
    for ( CopyPropagationInfo *i = r->_copyPropagationInfo; i and i->_register; i = i->_register->_copyPropagationInfo ) {
        if ( i->_register == this )
            return true;
    }
    return false;
}


// all the nameNode() functions translate the PseudoRegister info into debugging info for
// the scopeDescRecorder

NameNode *PseudoRegister::locNameNode( bool mustBeLegal ) const {
    st_assert( not _location.isTemporaryRegister() or not mustBeLegal, "shouldn't be in temp reg" );
    if ( _location.isTemporaryRegister() and not _debug ) {
        return new IllegalName;
    } else {
        // debug-visible PseudoRegisters may have temp regs if they're only visible
        // from uncommon branches
        return new LocationName( _location );
    }
}


InlinedScope *BlockPseudoRegister::parent() const {
    return _closure->parent_scope();
}


NameNode *BlockPseudoRegister::locNameNode( bool mustBeLegal ) const {
    static_cast<void>(mustBeLegal); // unused

    st_assert( not _location.isTemporaryRegister(), "shouldn't be in temp reg" );
    // for now, always use MemoizedName to describe block (even if always created)
    // makes debugging info easier to read (can see which locs must be blocks)
    return new MemoizedName( _location, closure()->method(), closure()->parent_scope()->getScopeInfo() );
}


NameNode *PseudoRegister::nameNode( bool mustBeLegal ) const {
    PseudoRegister *r = cpseudoRegister();
    if ( not( r->_location.equals( Location::UNALLOCATED_LOCATION ) ) ) {
        return r->locNameNode( mustBeLegal );
    } else if ( r->isConstPseudoRegister() ) {
        return r->nameNode( mustBeLegal );
    } else if ( r->isBlockPseudoRegister() ) {
        CompileTimeClosure *c = ( (BlockPseudoRegister *) r )->closure();
        return new BlockValueName( c->method(), c->parent_scope()->getScopeInfo() );
    } else {
        // hack: initial nilling of locals isn't represented yet
        // fix this -- should at least check if it's a local
        // also, when entire scopes are removed (e.g. when deleting branches of
        // a type test) the scope should be marked so its debugging info isn't written
        // currently, the contents of such scopes would break several assertions
        return newValueName( nilObject );
#ifdef how_it_should_be
        assert(not debug, "couldn't recover debug info");
        return new IllegalName;
#endif
    }
}


NameNode *ConstPseudoRegister::nameNode( bool mustBeLegal ) const {
    static_cast<void>(mustBeLegal); // unused
    return newValueName( constant );
}


NameNode *NoResultPseudoRegister::nameNode( bool mustBeLegal ) const {
    static_cast<void>(mustBeLegal); // unused
    return new IllegalName;
}


PseudoRegister *PseudoRegister::cpseudoRegister() const {
    // assert(not cpInfo or loc.equals(UNALLOCATED_LOCATION), "allocated regs shouldn't have cpInfo");
    // NB: the above assertion looks tempting but can be wrong: some unused PseudoRegisters may still
    // retain their definition because the defining node cannot be eliminated (because it might fail
    // or have other side effects)
    // e.g.: result of ArrayAt operation
    if ( _copyPropagationInfo == nullptr ) {
        return (PseudoRegister *) this;
    } else {
        PseudoRegister            *r;
        for ( CopyPropagationInfo *i = _copyPropagationInfo; i; r = i->_register, i = r->_copyPropagationInfo );
        return r == dummyPR ? (PseudoRegister *) this : r;
    }
}


void BlockPseudoRegister::memoize() {
    _memoized = true;
}


void BlockPseudoRegister::markEscaped() {
    if ( not _escapes ) {
        _escapes = true;
        if ( CompilerDebug )
            cout( PrintExposed )->print( "*exposing %s\n", name() );
        if ( MemoizeBlocks )
            memoize();
    }
}


void BlockPseudoRegister::markEscaped( Node *n ) {
    markEscaped();
    if ( _escapeNodes == nullptr )
        _escapeNodes = new GrowableArray<Node *>( 5 );
    if ( not _escapeNodes->contains( n ) )
        _escapeNodes->append( n );
}


// A helper class for BlockPseudoRegisters to compute their uplevel accesses

// Note: Uplevel accesses in all branches are taken into account,
// even if it is known at compile-time that a branch of an ifTrue/ifFalse
// is never generated due to a constant condition
// (the same holds of course for whileTrue/whileFalse and failure blocks of primitive calls).
// -> conservative computation of uplevel accesses

class UplevelComputer : public SpecializedMethodClosure {

public:
    BlockPseudoRegister             *_r;                // the block whose accesses we're computing
    InlinedScope                    *_scope;            // r's scope (i.e., scope creating the block)
    GrowableArray<PseudoRegister *> *_read;             // list of rscope's temps read by r
    GrowableArray<PseudoRegister *> *_written;          // same for written temps
    std::int32_t                    _nestingLevel;      // nesting level (0 = block itself, 1 = block within block, etc)
    std::int32_t                    _enclosingDepth;    // depth to which we're nested within outer method
    GrowableArray<Scope *>          *_enclosingScopes;  // 0 = scope immediately enclosing block (= this->scope), etc.
    MethodOop                       _methodOop;         // the method currently being scanned for uplevel-accesses; either r's block method or a nested block method

    UplevelComputer( BlockPseudoRegister *reg ) :
        _r{ nullptr },
        _scope{ nullptr },
        _read{ nullptr },
        _written{ nullptr },
        _nestingLevel{ 0 },
        _enclosingDepth{ 0 },
        _enclosingScopes{ nullptr },
        _methodOop{ nullptr } {
        _r               = reg;
        _scope           = _r->scope();
        _read            = new GrowableArray<PseudoRegister *>( 10 );
        _written         = new GrowableArray<PseudoRegister *>( 10 );
        _nestingLevel    = 0;
        _methodOop       = _r->closure()->method();
        _enclosingDepth  = 0;
        _enclosingScopes = new GrowableArray<Scope *>( 5 );
        for ( Scope *s   = _scope; s not_eq nullptr; s = s->parent(), _enclosingDepth++ )
            _enclosingScopes->push( s );
    }


    UplevelComputer() = default;
    virtual ~UplevelComputer() = default;
    UplevelComputer( const UplevelComputer & ) = default;
    UplevelComputer &operator=( const UplevelComputer & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    void record_temporary( bool reading, std::int32_t no, std::int32_t ctx ) {
        // distance is the lexical nesting distance in source-level terms (i.e., regardless of what the interpreter
        // does or whether the intermediate scopes have contexts or not) between r's scope and the scope
        // resolving the access; e.g., 1 --> the scope creating r
        std::int32_t distance = _methodOop->lexicalDistance( ctx ) - _nestingLevel;
        if ( distance < 1 )
            return;                // access is resolved in some nested block
        Scope *s = _enclosingScopes->at( distance - 1 );    // -1 because 0th element is enclosing scope, i.e., at distance 1
        if ( s->isInlinedScope() ) {
            // temporary is defined in this NativeMethod
            InlinedScope *target = (InlinedScope *) s;
            st_assert( target->allocatesInterpretedContext(), "find_scope returned bad scope" );
            PseudoRegister *reg = target->contextTemporary( no )->pseudoRegister();
            if ( CompilerDebug ) {
                cout( PrintExposed )->print( "*adding %s to uplevel-%s of block %s\n", reg->name(), reading ? "read" : "written", _r->name() );
            }
            GrowableArray<PseudoRegister *> *list = reading ? _read : _written;
            list->append( reg );
        } else {
            // uplevel access goes to another NativeMethod
        }
    }


    void push_temporary( std::int32_t no, std::int32_t context ) {
        record_temporary( true, no, context );
    }


    void store_temporary( std::int32_t no, std::int32_t context ) {
        record_temporary( false, no, context );
    }


    void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) {
        static_cast<void>(type); // unused
        static_cast<void>(nofArgs); // unused

        // recursively search nested blocks
        _nestingLevel++;
        MethodOop savedMethod = _methodOop;
        _methodOop = meth;
        MethodIterator iter( meth, this );
        _methodOop = savedMethod;
        _nestingLevel--;
    }
};


void BlockPseudoRegister::computeUplevelAccesses() {
    // compute _uplevelRead/_uplevelWritten
    st_assert( escapes(), "should escape" );
    if ( _uplevelRead )
        return;    // already computed

    UplevelComputer c( this );
    MethodIterator  iter( _closure->method(), &c );
    st_assert( not _uplevelWritten, "shouldn't be there" );
    _uplevelRead    = c._read;
    _uplevelWritten = c._written;
    for ( std::int32_t i = _uplevelRead->length() - 1; i >= 0; i-- ) {
        _uplevelRead->at( i )->addUplevelAccessor( this, true, false );
    }

    for ( std::int32_t i = _uplevelWritten->length() - 1; i >= 0; i-- ) {
        _uplevelWritten->at( i )->addUplevelAccessor( this, false, true );
    }

}


const char *PseudoRegister::safeName() const {
    return name();
//    if ( this == nullptr ) {
//        return "(null)";     // for safer debugging
//    } else {
//        return name();
//    }
}


const char *PseudoRegister::name() const {
    char *n = new_resource_array<char>( 25 );
    if ( _location.equals( Location::UNALLOCATED_LOCATION ) ) {
        sprintf( n, "%s%d%s%s%s", prefix(), id(), uplevelR() or uplevelW() ? "^" : "", uplevelR() ? "R" : "", uplevelW() ? "W" : "" );
    } else {
        sprintf( n, "%s%d(%s)%s%s%s", prefix(), id(), _location.name(), uplevelR() or uplevelW() ? "^" : "", uplevelR() ? "R" : "", uplevelW() ? "W" : "" );
    }

    return n;
}


void PseudoRegister::print() {
    SPDLOG_INFO( "%s: ", name() );
    printDefsAndUses( &_dus );
    SPDLOG_INFO( "" );
}


void BlockPseudoRegister::print() {
    print_short();
    SPDLOG_INFO( ": " );
    printDefsAndUses( &_dus );
    if ( _uplevelRead ) {
        SPDLOG_INFO( "; uplevel-read: " );
        _uplevelRead->print();
    }
    if ( _uplevelWritten ) {
        SPDLOG_INFO( "; uplevel-written: " );
        _uplevelWritten->print();
    }
    if ( _escapeNodes ) {
        SPDLOG_INFO( "; escapes at: " );
        for ( std::int32_t i = 0; i < _escapeNodes->length(); i++ )
            SPDLOG_INFO( "N%d ", _escapeNodes->at( i )->id() );
    }
    SPDLOG_INFO( "" );
}


const char *BlockPseudoRegister::name() const {
    char *n = new_resource_array<char>( 25 );
    sprintf( n, "%s <0x%0x}>%s", PseudoRegister::name(), PrintHexAddresses ? this : 0, _memoized ? "#" : "" );
    return n;
}


const char *ConstPseudoRegister::name() const {
    char *n = new_resource_array<char>( 25 );
    sprintf( n, "%s <0x%0x>", PseudoRegister::name(), constant );
    return n;
}


bool PseudoRegister::verify() const {
    bool ok = true;
    if ( _id < 0 or _id >= currentNo ) {
        ok = false;
        error( "PseudoRegister 0x{0:x} %s: invalid ID %ld", this, name(), _id );
    }
    std::int32_t uses = 0, definitions = 0;

    for ( std::int32_t i = 0; i < _dus.length(); i++ ) {
        PseudoRegisterBasicBlockIndex *index = _dus.at( i );
        DefinitionUsageInfo           *info  = index->_basicBlock->duInfo.info->at( index->_index );
        definitions += info->_definitions.length();
        uses += info->_usages.length();
    }
    if ( definitions not_eq _definitionCount and not incorrectD() and not isConstPseudoRegister() ) {
        // ConstPseudoRegisters have fake def
        ok = false;
        error( "PseudoRegister 0x{0:x} %s: wrong def count (%ld instead of %ld)", this, name(), _definitionCount, definitions );
    }
    if ( uses not_eq _usageCount and not incorrectU() ) {
        ok = false;
        error( "PseudoRegister 0x{0:x} %s: wrong use count (%ld instead of %ld)", this, name(), _usageCount, uses );
    }
    if ( not incorrectD() and _definitionCount == 0 and _usageCount > 0 ) {
        ok = false;
        error( "PseudoRegister 0x{0:x} %s: used but not defined", this, name() );
    }

#ifdef FIXME  // fix this - may still be needed
    if ( debug and not incorrectDU() and isTrashedReg( loc ) ) {
        ok = false;
        error( "PseudoRegister 0x{0:x} %s: debug-visible but allocated to temp reg", this, name() );
    }
#endif

    return ok;
}


bool SinglyAssignedPseudoRegister::verify() const {
    bool ok = PseudoRegister::verify();
    if ( ok ) {
        if ( _begByteCodeIndex == IllegalByteCodeIndex ) {
            if ( _creationStartByteCodeIndex not_eq IllegalByteCodeIndex or _endByteCodeIndex not_eq IllegalByteCodeIndex ) {
                ok = false;
                error( "SinglyAssignedPseudoRegister 0x{0:x} %s: live range only partially set", this, name() );
            }
        } else if ( _scope->isInlinedScope() ) {
            std::int32_t ncodes = scope()->nofBytes();
            if ( _creationStartByteCodeIndex < PrologueByteCodeIndex or _creationStartByteCodeIndex > creationScope()->nofBytes() ) {
                ok = false;
                error( "SinglyAssignedPseudoRegister 0x{0:x} %s: invalid _creationStartByteCodeIndex %ld", this, name(), _creationStartByteCodeIndex );
            }
            if ( _begByteCodeIndex < PrologueByteCodeIndex or _begByteCodeIndex > ncodes ) {
                ok = false;
                error( "SinglyAssignedPseudoRegister 0x{0:x} %s: invalid startByteCodeIndex %ld", this, name(), _begByteCodeIndex );
            }
            if ( _endByteCodeIndex < PrologueByteCodeIndex or ( _endByteCodeIndex > ncodes and _endByteCodeIndex not_eq EpilogueByteCodeIndex ) ) {
                ok = false;
                error( "SinglyAssignedPseudoRegister 0x{0:x} %s: invalid endByteCodeIndex %ld", this, name(), _endByteCodeIndex );
            }
        }
    }
    return ok;
}


bool BlockPseudoRegister::verify() const {
    bool ok = SinglyAssignedPseudoRegister::verify() and _closure->verify();
    // check uplevel-accessed vars: if they are blocks, they must be exposed
    if ( _uplevelRead ) {
        for ( std::int32_t i = 0; i < _uplevelRead->length(); i++ ) {
            if ( _uplevelRead->at( i )->isBlockPseudoRegister() ) {
                BlockPseudoRegister *blk = (BlockPseudoRegister *) _uplevelRead->at( i );
                if ( not blk->escapes() ) {
                    error( "BlockPseudoRegister 0x{0:x} is uplevel-accessed by escaping BlockPseudoRegister 0x{0:x} but isn't marked escaping itself", blk, this );
                    ok = false;
                }
            }
        }
        for ( std::int32_t i = 0; i < _uplevelWritten->length(); i++ ) {
            if ( _uplevelWritten->at( i )->isBlockPseudoRegister() ) {
                BlockPseudoRegister *blk = (BlockPseudoRegister *) _uplevelRead->at( i );
                error( "BlockPseudoRegister 0x{0:x} is uplevel-written by escaping BlockPseudoRegister 0x{0:x}, but BlockPseudoRegisters should never be assigned", blk, this );
                ok                       = false;
            }
        }
    }
    return ok;
}


bool NoResultPseudoRegister::verify() const {
    if ( _usageCount not_eq 0 ) {
        error( "NoResultPseudoRegister 0x{0:x}: has uses", this );
        return false;
    } else {
        return true;
    }
}


bool ConstPseudoRegister::verify() const {
    bool ok = ( PseudoRegister::verify() and constant->is_klass() ) or constant->verify();
//    if ( std::int32_t( constant ) < maxImmediate and std::int32_t( constant ) > -maxImmediate and _location not_eq UnAllocated ) {
//        error( "ConstPseudoRegister 0x{0:x}: could use load immediate to load Oop 0x{0:x}", this, constant );
//        ok = false;
//    }
    if ( not( _location.equals( Location::UNALLOCATED_LOCATION ) ) and not _location.isRegisterLocation() ) {
        error( "ConstPseudoRegister 0x{0:x}: was allocated to stack", this );
        ok = false;
    }
    if ( not( _location.equals( Location::UNALLOCATED_LOCATION ) ) and _location.isTrashedRegister() ) {
        error( "ConstPseudoRegister 0x{0:x}: was allocated to trashed reg", this );
        ok = false;
    }
    return ok;
}
