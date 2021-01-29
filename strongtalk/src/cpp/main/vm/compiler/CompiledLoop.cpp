//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"


GrowableArray<BasicBlock *> *CompiledLoop::_bbs;


CompiledLoop::CompiledLoop() {
    _bbs            = nullptr;
    _startOfLoop    = nullptr;
    _endOfLoop      = nullptr;
    _startOfBody    = nullptr;
    _endOfBody      = nullptr;
    _startOfCond    = nullptr;
    _endOfCond      = nullptr;
    _loopVar        = nullptr;
    _lowerBound     = nullptr;
    _upperBound     = nullptr;
    _isIntegerLoop  = false;
    _hoistableTests = nullptr;
    _loopBranch     = nullptr;
    _isCountingUp   = true;         // initial guess
    _scope          = nullptr;      // in case loop creation is aborted
}


void CompiledLoop::set_startOfLoop( LoopHeaderNode *current ) {
    st_assert( _startOfLoop == nullptr, "already set" );
    st_assert( current->isLoopHeaderNode(), "expected loop header" );
    _startOfLoop = _loopHeader = current;
    _beforeLoop  = current->firstPrev();
}


void CompiledLoop::set_endOfLoop( Node *end ) {
    st_assert( _endOfLoop == nullptr, "already set" );
    _endOfLoop   = end;
    _startOfLoop = _startOfLoop->next();      // because original start is node *before* loop
    _firstNodeID = _startOfLoop->id() + 1;
    _lastNodeID  = BasicNode::currentID - 1;  // -1 because currentID is id of next node to be allocated
    _scope       = _startOfLoop->scope();
}


void CompiledLoop::set_startOfBody( Node *current ) {
    st_assert( _startOfBody == nullptr, "already set" );
    _startOfBody = current;
}


void CompiledLoop::set_endOfBody( Node *current ) {
    st_assert( _endOfBody == nullptr, "already set" );
    _endOfBody       = current;
    // correct startOfBody -- merge node is created before cond
    if ( isInLoopCond( _startOfBody ) )
        _startOfBody = _startOfBody->next();
    st_assert( not isInLoopCond( _startOfBody ), "oops!" );
}


void CompiledLoop::set_startOfCond( Node *current ) {
    st_assert( _startOfCond == nullptr, "already set" );
    _startOfCond = current;
}


void CompiledLoop::set_endOfCond( Node *current ) {
    st_assert( _endOfCond == nullptr, "already set" );
    _endOfCond = current;
}


bool CompiledLoop::isInLoop( Node *n ) const {
    return _firstNodeID <= n->id() and n->id() <= _lastNodeID;
}


bool CompiledLoop::isInLoopCond( Node *n ) const {
    return _startOfCond->id() <= n->id() and n->id() <= _endOfCond->id();
}


bool CompiledLoop::isInLoopBody( Node *n ) const {
    return _startOfBody->id() <= n->id() and n->id() <= _endOfBody->id();
}


const char *CompiledLoop::recognize() {
    // Recognize integer loop.
    // We're looking for a loop where the loopVariable is initialized just before
    // the loop starts, is defined only once in the loop (increment/decrement),
    // and the loop condition is a comparison against a loop invariant.
    discoverLoopNesting();
    if ( not OptimizeIntegerLoops )
        return "not OptimizeIntegerLoops";
    const char *msg;
    if ( ( msg     = findUpperBound() ) not_eq nullptr )
        return msg;
    if ( ( msg     = checkLoopVar() ) not_eq nullptr )
        return msg;
    if ( ( msg     = checkUpperBound() ) not_eq nullptr )
        return msg;
    _isIntegerLoop = true;
    findLowerBound();    // don't need to know to optimize loop; result may be non-nullptr
    return nullptr;
}


void CompiledLoop::discoverLoopNesting() {
    // discover enclosing loop (if any) and set up loop header links
    for ( InlinedScope *s = _scope; s not_eq nullptr; s = s->sender() ) {
        GrowableArray<CompiledLoop *> *loops = s->loops();
        for ( std::int32_t            i      = loops->length() - 1; i >= 0; i-- ) {
            CompiledLoop *l = loops->at( i );
            if ( l->isInLoop( _loopHeader ) ) {
                // this is out enclosing loop
                _loopHeader->set_enclosingLoop( l->loopHeader() );
                l->loopHeader()->addNestedLoop( _loopHeader );
                return;
            }
        }
    }
}


const char *CompiledLoop::findLowerBound() {

    // find lower bound; is assigned to loop var (temp) before loop
    // NB: throughout this code, "lower" and "upper" bound are used to denote the
    // starting and ending values of the loop; in the case of a downward-counting loop,
    // lowerBound will actually be the highest value.
    // search for assignment to loopVar that reaches loop head
    Node *defNode;
    for ( defNode = _beforeLoop; defNode and defNode->nPredecessors() < 2 and ( not defNode->hasDest() or ( (NonTrivialNode *) defNode )->dest() not_eq _loopVar ); defNode = defNode->firstPrev() );
    if ( defNode and defNode->isAssignNode() ) {
        // ok, loopVar is assigned here
        _lowerBound = ( (NonTrivialNode *) defNode )->src();
    }
    return "lower bound not found";
}


// Helper class to find the last send preceding a certain byteCodeIndex
class SendFinder : public SpecializedMethodClosure {
public:
    std::int32_t theByteCodeIndex, lastSendByteCodeIndex;


    SendFinder( MethodOop m, std::int32_t byteCodeIndex ) :
            SpecializedMethodClosure() {
        theByteCodeIndex      = byteCodeIndex;
        lastSendByteCodeIndex = IllegalByteCodeIndex;
        MethodIterator iter( m, this );
    }


    void send() {
        std::int32_t b = byteCodeIndex();
        if ( b <= theByteCodeIndex and b > lastSendByteCodeIndex )
            lastSendByteCodeIndex = b;
    }


    virtual void normal_send( InterpretedInlineCache *ic ) {
        send();
    }


    virtual void self_send( InterpretedInlineCache *ic ) {
        send();
    }


    virtual void super_send( InterpretedInlineCache *ic ) {
        send();
    }
};


std::int32_t CompiledLoop::findStartOfSend( std::int32_t byteCodeIndex ) {
    // find send preceding byteCodeIndex; slow but safe
    SendFinder f( _scope->method(), byteCodeIndex );
    return f.lastSendByteCodeIndex;
}


const char *CompiledLoop::findUpperBound() {
    // find upper bound and loop variable
    std::int32_t condByteCodeIndex = _endOfCond ? findStartOfSend( _endOfCond->byteCodeIndex() - InterpretedInlineCache::size ) : IllegalByteCodeIndex;
    if ( condByteCodeIndex == IllegalByteCodeIndex )
        return "loop condition: no send found";
    // first find comparison in loop condition
    Expression *loopCond = _scope->exprStackElems()->at( condByteCodeIndex );
    if ( loopCond == nullptr )
        return "loop condition: no expr (possible compiler bug)";
    if ( loopCond->isMergeExpression() ) {
        MergeExpression *cond = (MergeExpression *) loopCond;
        if ( cond->isSplittable() ) {
            Node *first = cond->exprs->first()->node();
            if ( first->nPredecessors() == 1 ) {
                Node *prev = first->firstPrev();
                if ( prev->isBranchNode() )
                    _loopBranch = (BranchNode *) prev;
            }
        }
    }
    if ( not _loopBranch )
        return "loop condition: conditional branch not found";
    NonTrivialNode *n = (NonTrivialNode *) _loopBranch->firstPrev();
    PseudoRegister *operand;
    if ( n->isTArithNode() ) {
        operand = ( (TArithRRNode *) n )->operand();
    } else if ( n->isArithNode() ) {
        operand = ( (ArithRRNode *) n )->operand();
    } else {
        return "loop condition: comparison not found";
    }
    BranchOpCode op = _loopBranch->op();

    // Now see if it's a simple comparison involving the loop variable
    // and make an initial guess about the loop variable.
    // NB: code generator inverts loop condition, i.e., branch leaves loop
    if ( op == BranchOpCode::LTBranchOp or op == BranchOpCode::LEBranchOp ) {
        // upperBound > loopVar
        _loopVar    = operand;
        _upperBound = n->src();
    } else if ( op == BranchOpCode::GTBranchOp or op == BranchOpCode::GEBranchOp ) {
        // loopVar < upperBound
        _loopVar    = n->src();
        _upperBound = operand;
    } else {
        return "loop condition: not oneof(<, <=, >, >=)";
    }

    // The above code assumes a loop counting up; check the loopVar now and switch
    // directions if it doesn't work.
    if ( checkLoopVar() ) {
        // the reverse guess may be wrong too, but it doesn't hurt to try
        PseudoRegister *temp = _loopVar;
        _loopVar    = _upperBound;
        _upperBound = temp;
    }
    return nullptr;
}


// closure for finding definitions in a loop
class LoopClosure : public Closure<Definition *> {
public:
    NonTrivialNode *defNode;
    CompiledLoop   *theLoop;


    LoopClosure( CompiledLoop *l ) {
        defNode = nullptr;
        theLoop = l;
    }
};


// count all definitions in loop
class LoopDefCounter : public LoopClosure {
public:
    std::int32_t defCount;


    LoopDefCounter( CompiledLoop *l ) :
            LoopClosure( l ) {
        defCount = 0;
    }


    void do_it( Definition *d ) {
        if ( theLoop->isInLoop( d->_node ) ) {
            defCount++;
            defNode = (NonTrivialNode *) d->_node;
        }
    }
};


std::int32_t CompiledLoop::defsInLoop( PseudoRegister *r, NonTrivialNode **defNode ) {
    // returns the number of definitions of r within the loop
    // also sets defNode to the last definition
    // BUG: won't work if loop has sends -- will ignore possible definitions to inst vars etc.
    LoopDefCounter ldc( this );
    r->forAllDefsDo( &ldc );
    if ( defNode )
        *defNode = ldc.defNode;
    return ldc.defCount;
}


class LoopDefFinder : public LoopClosure {
public:
    LoopDefFinder( CompiledLoop *l ) :
            LoopClosure( l ) {
    }      // don't ask why this is necessary... (MS C++ 4.0)
    void do_it( Definition *d ) {
        if ( theLoop->isInLoop( d->_node ) )
            defNode = d->_node;
    }
};


NonTrivialNode *CompiledLoop::findDefInLoop( PseudoRegister *r ) {
    st_assert( defsInLoop( r ) == 1, "must have single definition in loop" );
    LoopDefFinder ldf( this );
    r->forAllDefsDo( &ldf );
    return ldf.defNode;
}


const char *CompiledLoop::checkLoopVar() {
#ifdef unnecessary
    // first check if loop var is initialized to lower bound
    BasicBlock* beforeLoopBasicBlock = _beforeLoop->bb();
    // search for assignment preceeding loop
    for (Node* n = _beforeLoop;
         n and n->bb() == beforeLoopBasicBlock and (not n->isAssignNode() or ((AssignNode*)n)->dest() not_eq loopVar);
         n = n->firstPrev()) ;
    if (n and n->bb() == beforeLoopBasicBlock) {
      // ok, found the assignment
      assert(n->isAssignNode() and ((AssignNode*)n)->dest() == loopVar, "oops");
    } else {
      return "no lower-bound assignment to loop var before loop";
    }
#endif
    // check usage of loop var within loop

    // quick check: must have at least 2 definitions (initialization plus increment)
    if ( _loopVar->isSinglyAssigned() )
        return "loopVar has single definition globally";

    // loop variable must have exactly one def in loop, and def must be result of an addition
    // usual code pattern: SAPn = add(loopVar, const); ...; loopVar := SAPn
    if ( defsInLoop( _loopVar ) not_eq 1 )
        return "loopVar has not_eq 1 definitions in loop";

    NonTrivialNode *n1 = findDefInLoop( _loopVar );
    if ( defsInLoop( n1->src() ) not_eq 1 )
        return "loopVar has not_eq 1 definitions in loop (2)";

    NonTrivialNode *n2 = findDefInLoop( n1->src() );
    PseudoRegister *operand;
    ArithOpCode    op;
    if ( n2->isTArithNode() ) {
        operand = ( (TArithRRNode *) n2 )->operand();
        op      = ( (TArithRRNode *) n2 )->op();
    } else if ( n2->isArithNode() ) {
        operand = ( (ArithRRNode *) n2 )->operand();
        op      = ( (ArithRRNode *) n2 )->op();
    } else {
        return "loopVar def not an arithmetic operation";
    }
    _incNode           = n2;
    if ( op not_eq ArithOpCode::tAddArithOp and op not_eq ArithOpCode::tSubArithOp )
        return "loopVar def isn't an add/sub";
    if ( _incNode->src() == _loopVar ) {
        if ( not isIncrement( operand, op ) )
            return "loopVar isn't incremented by constant/loop-invariant";
    } else if ( operand == _loopVar ) {
        if ( not isIncrement( _incNode->src(), op ) )
            return "loopVar isn't incremented by constant/loop-invariant";
    } else {
        return "loopVar def isn't an increment";
    }

    // at this point, we finally know for sure whether the loop is counting up or down
    // check that loop is bounded at all
    BranchOpCode   branchOp          = _loopBranch->op();
    bool           loopVarMustBeLeft = ( branchOp == BranchOpCode::GTBranchOp or branchOp == BranchOpCode::GEBranchOp ) ^not _isCountingUp;
    NonTrivialNode *compare          = (NonTrivialNode *) _loopBranch->firstPrev();
    if ( loopVarMustBeLeft not_eq ( compare->src() == _loopVar ) ) {
        return "loopVar is on wrong side of comparison (loop not bounded)";
    }

    return nullptr;
}


bool CompiledLoop::isIncrement( PseudoRegister *p, ArithOpCode op ) {
    // is p a suitable increment (i.e., a positive constant or loop-invariant variable)?
    _increment = p;
    if ( p->isConstPseudoRegister() ) {
        Oop c         = ( (ConstPseudoRegister *) p )->constant;
        if ( not c->is_smi() )
            return false;
        _isCountingUp = ( SMIOop( c )->value() > 0 ) ^ ( op == ArithOpCode::tSubArithOp );
        return true;
    } else {
        // fix this: need to check sign in loop header
        return defsInLoop( p ) == 0;
    }
}


const char *CompiledLoop::checkUpperBound() {
    // upper bound must not be defined in loop (loop invariant)
    _loopArray    = nullptr;
    _loopSizeLoad = nullptr;
    std::int32_t ndefs = defsInLoop( _upperBound, nullptr );
    if ( ndefs > 0 )
        return "upper bound isn't loop-invariant";
    // ok, no assignments in loop; check if upper bound is size of an array
    // search for assignment that reaches loop head
    Node *defNode;
    for ( defNode = _beforeLoop; defNode and defNode->nPredecessors() < 2 and ( not defNode->hasDest() or ( (NonTrivialNode *) defNode )->dest() not_eq _upperBound ); defNode = defNode->firstPrev() );
    if ( defNode and defNode->isArraySizeLoad() ) {
        // ok, upper bound is an array; can optimize array accesses if it is loop-invariant
        if ( defsInLoop( _loopArray ) == 0 ) {
            _loopSizeLoad = (LoadOffsetNode *) defNode;
            _loopArray    = _loopSizeLoad->base();
        }
    }
    return nullptr;
}


void CompiledLoop::optimizeIntegerLoop() {
    // need general loop opts as well (for array type checks)
    if ( not OptimizeLoops ) st_fatal( "if OptimizeIntegerLoops is set, OptimizeLoops must be set too" );
    if ( not isIntegerLoop() )
        return;

    // activate loop header (will check upper bound for smi_t-ness etc.)
    PseudoRegister *u = _loopSizeLoad ? nullptr : _upperBound;
    _loopHeader->activate( _loopVar, _lowerBound, u, _loopSizeLoad );
    _loopHeader->_integerLoop = true;
    removeTagChecks();
    checkForArraysDefinedInLoop();
    if ( _loopArray ) {
        // upper bound is loopArray's size --> counter can't overflow
        removeLoopVarOverflow();
        // also, array accesses need no bounds check
        removeBoundsChecks( _loopArray, _loopVar );
        // potential performance bug: should do bounds check for all array accesses with loop-invariant index
    } else {
        // fix this: must check upper bound against maxInt
        removeLoopVarOverflow();
    }
}


// closure for untagging definitions/uses of loop var
class UntagClosure : public Closure<Usage *> {

public:
    CompiledLoop            *theLoop;
    PseudoRegister          *theLoopPReg;
    GrowableArray<KlassOop> *smi_type;


    UntagClosure( CompiledLoop *l, PseudoRegister *r ) {
        theLoop     = l;
        theLoopPReg = r;
        smi_type    = new GrowableArray<KlassOop>( 1 );
        smi_type->append( smiKlassObject );
    }


    void do_it( Usage *u ) {
        if ( theLoop->isInLoop( u->_node ) ) {
            u->_node->assert_preg_type( theLoopPReg, smi_type, theLoop->loopHeader() );
        }
    }
};


void CompiledLoop::removeTagChecks() {
    // Eliminate all tag checks for loop variable and upper bound
    // As a side effect, this will add all arrays indexed by the loop variable to the LoopHeaderNode
    UntagClosure uc( this, _loopVar );
    _loopVar->forAllUsesDo( &uc );
    if ( _lowerBound ) {
        uc.theLoopPReg = _lowerBound;
        _lowerBound->forAllUsesDo( &uc );
    }
    uc.theLoopPReg = _upperBound;
    _upperBound->forAllUsesDo( &uc );
}


void CompiledLoop::removeLoopVarOverflow() {

    // bug: should remove overflow only if increment is constant and not too large -- fix this
    Node *n = _incNode->next();
    st_assert( n->isBranchNode(), "must be branch" );
    BranchNode *overflowCheck = (BranchNode *) n;
    st_assert( overflowCheck->op() == BranchOpCode::VSBranchOp, "should be overflow check" );
    if ( CompilerDebug or PrintLoopOpts ) {
        cout( PrintLoopOpts )->print( "*removing overflow check at node N%d\n", overflowCheck->id() );
    }
    Node *taken = overflowCheck->next( 1 );      // overflow handling code
    taken->removeUpToMerge();
    overflowCheck->removeNext( taken );      // so overflowCheck can be eliminated
    overflowCheck->eliminate( overflowCheck->bb(), nullptr, true, false );

    // since increment cannot fail anymore, directly increment loop counter if possible
    // need to search for assignment of incremented value to loop var
    Node *a = overflowCheck->next();
    while ( 1 ) {
        if ( a->nPredecessors() > 1 )
            break;      // can't search beyond merge
        if ( a->nSuccessors() > 1 )
            break;      // can't search beyond branches
        if ( a->isAssignNode() ) {
            if ( not a->_deleted ) {
                AssignNode *assign = (AssignNode *) a;
                if ( assign->src() == _incNode->dest() and assign->dest() == _loopVar ) {
                    if ( CompilerDebug or PrintLoopOpts ) {
                        cout( PrintLoopOpts )->print( "*optimizing loopVar increment at N%d\n", _incNode->id() );
                    }
                    _incNode->setDest( _incNode->bb(), _loopVar );
                    assign->eliminate( assign->bb(), nullptr, true, false );
                }
            }
        } else if ( not a->isTrivial() ) {
            compiler_warning( "unexpected nontrivial node N%d after loopVar increment\n", a->id() );
        }
        a = a->next();
    }
}


void CompiledLoop::checkForArraysDefinedInLoop() {
    // remove all arrays from loopHeader's list which are defined in the loop
    GrowableArray<AbstractArrayAtNode *> arraysToRemove( 10 );
    std::int32_t                         len = _loopHeader->_arrayAccesses->length();

    for ( std::int32_t i = 0; i < len; i++ ) {
        AbstractArrayAtNode *n = _loopHeader->_arrayAccesses->at( i );
        if ( defsInLoop( n->src() ) )
            arraysToRemove.append( n );
    }
    while ( arraysToRemove.nonEmpty() ) {
        AbstractArrayAtNode *n = arraysToRemove.pop();
        _loopHeader->_arrayAccesses->remove( n );
    }
}


void CompiledLoop::optimize() {
    if ( not _scope )
        return;
    // general loop optimizations
    hoistTypeTests();
    findRegCandidates();
}


class TTHoister : public Closure<InlinedScope *> {

public:
    GrowableArray<HoistedTypeTest *> *hoistableTests;
    CompiledLoop                     *theLoop;


    TTHoister( CompiledLoop *l, GrowableArray<HoistedTypeTest *> *h ) {
        hoistableTests = h;
        theLoop        = l;
    }


    void do_it( InlinedScope *s ) {
        GrowableArray<NonTrivialNode *> *tests = s->typeTests();
        std::int32_t                    len    = tests->length();

        for ( std::int32_t i = 0; i < len; i++ ) {

            NonTrivialNode *n = tests->at( i );
            st_assert( n->doesTypeTests(), "shouldn't be in list" );
            if ( n->_deleted )
                continue;
            if ( n->hasUnknownCode() )
                continue;      // can't optimize - expects other klasses, so would get uncommon trap at run-time
            if ( not theLoop->isInLoop( n ) )
                continue;      // not in this loop

            GrowableArray<PseudoRegister *>          regs( 4 );
            GrowableArray<GrowableArray<KlassOop> *> klasses( 4 );
            n->collectTypeTests( regs, klasses );
            for ( std::int32_t j = 0; j < regs.length(); j++ ) {
                PseudoRegister *r = regs.at( j );
                if ( theLoop->defsInLoop( r ) == 0 ) {
                    // this test can be hoisted
                    if ( CompilerDebug or PrintLoopOpts )
                        cout( PrintLoopOpts )->print( "*moving type test of %s at N%d out of loop\n", r->name(), n->id() );
                    hoistableTests->append( new HoistedTypeTest( n, r, klasses.at( j ) ) );
                }
            }
        }
    }
};


void CompiledLoop::hoistTypeTests() {

    // collect all type tests that can be hoisted out of the loop
    _loopHeader->_tests = _hoistableTests = new GrowableArray<HoistedTypeTest *>( 10 );
    TTHoister tth( this, _hoistableTests );
    _scope->subScopesDo( &tth );

    // add type tests to loop header for testing (avoid duplicates)
    // (currently quadratic algorithm but there should be very few)

    GrowableArray<HoistedTypeTest *> *headerTests = new GrowableArray<HoistedTypeTest *>( _hoistableTests->length() );

    for ( std::int32_t i = _hoistableTests->length() - 1; i >= 0; i-- ) {

        HoistedTypeTest *t      = _hoistableTests->at( i );
        PseudoRegister  *tested = t->_testedPR;

        for ( std::int32_t j = headerTests->length() - 1; j >= 0; j-- ) {

            if ( headerTests->at( j )->_testedPR == tested ) {
                // already testing this PseudoRegister
                if ( isEquivalentType( headerTests->at( j )->_klasses, t->_klasses ) ) {
                    // nothing to do
                } else {
                    // Whoa!  The same PseudoRegister is tested for different types in different places.
                    // Possible but rare.
                    headerTests->at( j )->_invalid = t->_invalid = true;
                    if ( WizardMode ) {
                        compiler_warning( "CompiledLoop::hoistTypeTests: PseudoRegister tested for different types\n" );
                        t->print();
                        headerTests->at( j )->print();
                    }
                }
                tested = nullptr;    // don't add it to list
                break;
            }
        }
        if ( tested )
            headerTests->append( t );
    }

    // now delete all hoisted type tests from loop body
    for ( std::int32_t i = _hoistableTests->length() - 1; i >= 0; i-- ) {
        HoistedTypeTest *t = _hoistableTests->at( i );
        if ( not t->_invalid ) {
            t->_node->assert_preg_type( t->_testedPR, t->_klasses, _loopHeader );
        }
    }
    if ( not _loopHeader->isActivated() )
        _loopHeader->activate();
}


bool CompiledLoop::isEquivalentType( GrowableArray<KlassOop> *klasses1, GrowableArray<KlassOop> *klasses2 ) {
    // are the two lists klasses1 and klasses2 equivalent (i.e., contain the same set of klasses)?
    if ( klasses1->length() not_eq klasses2->length() )
        return false;
    for ( std::int32_t i = klasses2->length() - 1; i >= 0; i-- ) {
        if ( klasses1->at( i ) not_eq klasses2->at( i )    // quick check
             and ( not klasses1->contains( klasses2->at( i ) ) or not klasses2->contains( klasses1->at( i ) ) ) ) { // slow check
            return false;
        }
    }
    return true;
}


class BoundsCheckRemover : public Closure<Usage *> {

public:
    CompiledLoop                         *theLoop;
    PseudoRegister                       *theLoopPReg;
    GrowableArray<AbstractArrayAtNode *> *theArrayList;


    BoundsCheckRemover( CompiledLoop *l, PseudoRegister *r, GrowableArray<AbstractArrayAtNode *> *arrays ) {
        theLoop      = l;
        theLoopPReg  = r;
        theArrayList = arrays;
    }


    void do_it( Usage *u ) {
        if ( theLoop->isInLoop( u->_node ) and
             // the cast to AbstractArrayAtNode* isn't correct (u->node may be something else),
             // but it's safe since we're only searching in the array list using pointer identity
             theArrayList->contains( (AbstractArrayAtNode *) u->_node ) ) {
            u->_node->assert_in_bounds( theLoopPReg, theLoop->loopHeader() );
        }
    }
};


void CompiledLoop::removeBoundsChecks( PseudoRegister *array, PseudoRegister *var ) {
    // Remove all bounds checks for nodes where var indexes into array.
    // This means that the arrays must be type- and bounds-checked in the loop header.
    // Thus, only do it for arrays that aren't defined within the loop, i.e.,
    // those in the loop header's list of arrays.
    BoundsCheckRemover bcr( this, var, _loopHeader->_arrayAccesses );
    array->forAllUsesDo( &bcr );
}


void CompiledLoop::findRegCandidates() {


    // Find the PseudoRegisters with the highest number of definitions & uses in this loop.
    // Problem: we don't have a list of all PseudoRegisters used in the loop; in fact, we don't even have a list of all nodes.

    // Solution: iterate through all BBs between start and end of loop (in code generation order) and collect the definitions and uses.
    // The BasicBlock ordering algorithm should make sure that the BBs of the loop are consecutive.


    if ( _bbs == nullptr )
        _bbs = bbIterator->code_generation_order();


    GrowableArray<LoopRegCandidate *> candidates( PseudoRegister::currentNo, PseudoRegister::currentNo, nullptr );

    const std::int32_t len              = _bbs->length();
    const BasicBlock   *startBasicBlock = _startOfLoop->bb();

    std::int32_t i;
    for ( i = 0; _bbs->at( i ) not_eq startBasicBlock; i++ );    // search for first BasicBlock

    const BasicBlock *endBasicBlock = _endOfLoop->bb();

    std::int32_t ncalls = 0;

    // iterate through all BBs in the loop
    for ( BasicBlock *bb = _bbs->at( i ); bb not_eq endBasicBlock; i++, bb = _bbs->at( i ) ) {
        const std::int32_t n = bb->duInfo.info->length();
        if ( bb->_last->isCallNode() )
            ncalls++;
        for ( std::int32_t j = 0; j < n; j++ ) {
            DefinitionUsageInfo *info = bb->duInfo.info->at( j );
            PseudoRegister      *r    = info->_pseudoRegister;
            if ( candidates.at( r->id() ) == nullptr )
                candidates.at_put( r->id(), new LoopRegCandidate( r ) );
            LoopRegCandidate *c = candidates.at( r->id() );
            c->incDUs( info->_usages.length(), info->_definitions.length() );
        }
    }
    loopHeader()->set_nofCallsInLoop( ncalls );

    // find the top 2 candidates...
    LoopRegCandidate *first  = new LoopRegCandidate( nullptr );
    LoopRegCandidate *second = new LoopRegCandidate( nullptr );

    for ( std::int32_t j = candidates.length() - 1; j >= 0; j-- ) {
        LoopRegCandidate *c = candidates.at( j );
        if ( c == nullptr )
            continue;
        if ( c->weight() > first->weight() ) {
            second = first;
            first  = c;
        } else if ( c->weight() > second->weight() ) {
            second = c;
        }
    }

    // ...and add them to the loop header
    if ( first->preg() not_eq nullptr ) {
        loopHeader()->addRegisterCandidate( first );
        if ( second->preg() not_eq nullptr ) {
            loopHeader()->addRegisterCandidate( second );
        }
    } else {
        st_assert( second->preg() == nullptr, "bad sorting" );
    }
}


void CompiledLoop::print() {
    spdlog::info( "((CompiledLoop*)0x{0:x}) = [N{}..N{}], cond = [N{}..N%d], body = [N%d..N%d] (byteCodeIndex %d..%d)", static_cast<const void *>(this), _firstNodeID, _lastNodeID, _startOfCond->id(), _endOfCond->id(), _startOfBody->id(), _endOfBody->id(), _startOfLoop->byteCodeIndex(), _endOfLoop->byteCodeIndex() );
    spdlog::info( "\tloopVar=%s, lower=%s, upper=%s", _loopVar->safeName(), _lowerBound->safeName(), _upperBound->safeName() );
}


HoistedTypeTest::HoistedTypeTest( NonTrivialNode *node, PseudoRegister *testedPR, GrowableArray<KlassOop> *klasses ) {
    _node     = node;
    _testedPR = testedPR;
    _klasses  = klasses;
    _invalid = false;
}


void HoistedTypeTest::print_test_on( ConsoleOutputStream *s ) {
    s->print( "%s = {", _testedPR->name() );

    std::int32_t len = _klasses->length();

    for ( std::int32_t j = 0; j < len; j++ ) {
        KlassOop m = _klasses->at( j );
        m->print_value_on( s );
        if ( j < len - 1 )
            s->print( "," );
    }
    s->print( "}" );
}


void HoistedTypeTest::print() {
    _console->print( "((HoistedTypeTest*)0x{0:x}): ", static_cast<const void *>(this) );
    print_test_on( _console );
    _console->cr();
}


void LoopRegCandidate::print() {
    spdlog::info( "((LoopRegCandidate*)0x{0:x}): %s, {} uses, {} definitions", static_cast<const void *>(this), _pseudoRegister->name(), _nuses, _ndefs );
}
