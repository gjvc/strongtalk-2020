//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/PrimitiveInliner.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/compiler/Inliner.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"


bool_t equal( const char * s, const char * t ) {
    return strcmp( s, t ) == 0;
}


void PrimitiveInliner::assert_failure_block() {
    st_assert( _failure_block not_eq nullptr, "primitive must have a failure block" );
}


void PrimitiveInliner::assert_no_failure_block() {
    st_assert( _failure_block == nullptr, "primitive doesn't have a failure block" );
}


void PrimitiveInliner::assert_receiver() {
    // %hack: this assertion fails
    // assert(parameter(0) == _scope->self(), "first parameter must be self");
}


int PrimitiveInliner::log2( int x ) const {
    int i = -1;
    int p = 1;
    while ( p not_eq 0 and p <= x ) {
        // p = 2^(i+1) and p <= x (i.e., 2^(i+1) <= x)
        i++;
        p *= 2;
    }
    // p = 2^(i+1) and x < p (i.e., 2^i <= x < 2^(i+1))
    // (if p = 0 then overflow occured and i = 31)
    return i;
}


Expression * PrimitiveInliner::tryConstantFold() {
    // Returns the result if the primitive call has been constant folded
    // successfully; returns nullptr otherwise.
    // Note: The result may be a marked Oop - which has to be unmarked
    // before using it - and which indicates that the primitive will fail
    // always.
    if ( not _primitiveDescriptor->can_be_constant_folded() ) {
        // check for Symbol>>at: before declaring failure
        if ( ( equal( _primitiveDescriptor->name(), "primitiveIndexedByteAt:ifFail:" ) or equal( _primitiveDescriptor->name(), "primitiveIndexedByteCharacterAt:ifFail:" ) ) and parameter( 0 )->hasKlass() and parameter( 0 )->klass() == Universe::symbolKlassObj() ) {
            // the at: primitive can be constant-folded for symbols
            // what if the receiver is a constant string? unfortunately, Smalltalk has
            // "assignable constants" like Fortran...
        } else {
            return nullptr;
        }
    }
    // get parameters
    int i = number_of_parameters();
    Oop * args = new_resource_array <Oop>( i );
    while ( i > 0 ) {
        i--;
        Expression * arg = parameter( i );
        if ( not arg->isConstantExpression() )
            return nullptr;
        args[ i ] = arg->constant();
    }
    // all parameters are constants, so call primitive
    Oop res = _primitiveDescriptor->eval( args );
    if ( res->is_mark() ) {
        // primitive failed
        return primitiveFailure( unmarkSymbol( res ) );
    } else if ( res->is_mem() and not res->is_old() ) {
        // must tenure result because nativeMethods aren't scavenged
        if ( res->is_double() ) {
            res = oopFactory::clone_double_to_oldspace( DoubleOop( res ) );
        } else {
            // don't know how to tenure non-doubles
            warning( "const folding: primitive %s is returning non-tenured object", _primitiveDescriptor->name() );
            return nullptr;
        }
    }
    ConstPseudoRegister          * constResult = new_ConstPReg( _scope, res );
    SinglyAssignedPseudoRegister * result      = new SinglyAssignedPseudoRegister( _scope );
    _gen->append( NodeFactory::AssignNode( constResult, result ) );
    if ( CompilerDebug )
        cout( PrintInlining )->print( "%*sconstant-folding %s --> %#x\n", _scope->depth + 2, "", _primitiveDescriptor->name(), res );
    st_assert( not constResult->constant->is_mark(), "result must not be marked" );
    return new ConstantExpression( res, constResult, _gen->current() );
}


Expression * PrimitiveInliner::tryTypeCheck() {

    // Check if we have enough type information to prove that the primitive is going to fail;
    // if so, directly compile failure block (important for mixed-mode arithmetic).
    // Returns the failure result if the primitive call has been proven to fail; returns nullptr otherwise.
    //
    // Should extend code to do general type compatibility test (including MergeExprs, e.g. for booleans) -- fix this later.  -Urs 11/95

    int num = number_of_parameters();

    for ( int i = 0; i < num; i++ ) {
        Expression * a = parameter( i );
        if ( a->hasKlass() ) {
            Expression * primArgType = _primitiveDescriptor->parameter_klass( i, a->preg(), nullptr );
            if ( primArgType and primArgType->hasKlass() and ( a->klass() not_eq primArgType->klass() ) ) {
                // types differ -> primitive must fail
                return primitiveFailure( failureSymbolForArg( i ) );
            }
        }
    }

    return nullptr;
}


SymbolOop PrimitiveInliner::failureSymbolForArg( int i ) {
    st_assert( i >= 0 and i < number_of_parameters(), "bad index" );
    switch ( i ) {
        case 0:
            return vmSymbols::receiver_has_wrong_type();
        case 1:
            return vmSymbols::first_argument_has_wrong_type();
        case 2:
            return vmSymbols::second_argument_has_wrong_type();
        case 3:
            return vmSymbols::third_argument_has_wrong_type();
        case 4:
            return vmSymbols::fourth_argument_has_wrong_type();
        case 5:
            return vmSymbols::fifth_argument_has_wrong_type();
        case 6:
            return vmSymbols::sixth_argument_has_wrong_type();
        case 7:
            return vmSymbols::seventh_argument_has_wrong_type();
        case 8:
            return vmSymbols::eighth_argument_has_wrong_type();
        case 9:
            return vmSymbols::ninth_argument_has_wrong_type();
        case 10:
            return vmSymbols::tenth_argument_has_wrong_type();
        default:
            return vmSymbols::argument_has_wrong_type();
    }
}


// Helper class to find the first assignment to a temporary in a failure
// block and the remaining method interval (without the assignment).
class AssignmentFinder : public SpecializedMethodClosure {
    public:
        int tempNo;    // the temporary to which the assignment took place
        MethodInterval * block;    // the block over which to iterate
        MethodInterval * interval;    // the rest of the block without the assignment

        AssignmentFinder( MethodInterval * b ) :
            SpecializedMethodClosure() {
            block    = b;
            interval = nullptr;
            MethodIterator iter( block, this );
        }


        void instruction() {
            abort();
        }


        void store_temporary( int no ) {
            tempNo   = no;
            interval = MethodIterator::factory->new_MethodInterval( method(), nullptr, next_byteCodeIndex(), block->end_byteCodeIndex() );
        }
};


Expression * PrimitiveInliner::primitiveFailure( SymbolOop failureCode ) {
    // compile a failing primitive
    if ( CompilerDebug )
        cout( PrintInlining )->print( "%*sprimitive %s will fail: %s\n", _scope->depth + 2, "", _primitiveDescriptor->name(), failureCode->as_string() );
    if ( _failure_block == nullptr ) {
        error( "\nPotential compiler bug: compiler believes primitive %s will fail\nwith error %s, but primitive has no failure block.\n", _primitiveDescriptor->name(), failureCode->as_string() );
        return nullptr;
    }
    // The byte code compiler stores the primitive result in a temporary (= the parameter of
    // the inlined failure block).  Thus, we store res in the corresponding temp Expression so that
    // its value isn't forgotten (the default Expression for temps is UnknownExpression) and restore the
    // temp's original value afterwards. This is safe because within the failure block that
    // temporary is treated as a parameter and therefore is read-only.
    AssignmentFinder closure( _failure_block );
    st_assert( closure.interval not_eq nullptr, "no assignment found" );
    Expression * old_temp = _scope->temporary( closure.tempNo );    // save temp
    Expression * res      = new ConstantExpression( failureCode, new_ConstPReg( _scope, failureCode ), _gen->current() );
    _scope->set_temporary( closure.tempNo, res );
    closure.set_primitive_failure( false );        // allow inlining sends in failure branch
    _gen->generate_subinterval( closure.interval, true );
    _scope->set_temporary( closure.tempNo, old_temp );
    return _gen->exprStack()->pop();        // restore temp
}


Expression * PrimitiveInliner::merge_failure_block( Node * ok_exit, Expression * ok_result, Node * failure_exit, Expression * failure_code, bool_t ok_result_is_read_only ) {
    st_assert( failure_code->hasKlass() and failure_code->klass() == Universe::symbolKlassObj(), "failure code must be a symbol" );
    _cannotFail = false;
    // push failure code
    _gen->setCurrent( failure_exit );
    _gen->exprStack()->push( failure_code, _scope, failure_exit->byteCodeIndex() );
    // code for failure block
    _gen->generate_subinterval( _failure_block, true );
    Expression * failure_block_result = _gen->exprStack()->pop();
    if ( failure_block_result->isNoResultExpression() ) {
        // failure doesn't reach here (e.g., uncommon branch)
        st_assert( not _gen->current() or _gen->current() == NodeBuilder::EndOfCode, "shouldn't reach here" );
        _gen->setCurrent( ok_exit );
        return ok_result;
    } else {
        if ( ok_result_is_read_only ) {
            // cannot assign to ok_result directly => introduce extra PseudoRegister & extra assignment
            // (was bug detected with new backend: assignment to ok_result is not legal if it
            // refers to parameter/receiver of the method calling the primitive - gri 6/29/96)
            PseudoRegister * resPReg       = new SinglyAssignedPseudoRegister( _scope );
            Node           * node          = NodeFactory::AssignNode( ok_result->preg(), resPReg );
            Expression     * resExpression = ok_result->shallowCopy( resPReg, node );
            ok_exit->append( node );
            ok_exit   = node;
            ok_result = resExpression;
        }
        _gen->append( NodeFactory::AssignNode( failure_block_result->preg(), ok_result->preg() ) );
        // merge after failure block
        MergeNode * prim_exit = NodeFactory::MergeNode( _failure_block->end_byteCodeIndex() );
        _gen->append( prim_exit );
        ok_exit->append( prim_exit );
        _gen->setCurrent( prim_exit );
        return new MergeExpression( ok_result, failure_block_result, ok_result->preg(), prim_exit );
    }
}


class PrimitiveSendFinder : public SpecializedMethodClosure {

    public:
        RecompilationScope * _recompilationScope;
        bool_t _wasExecuted;


        PrimitiveSendFinder( RecompilationScope * rs ) {
            this->_recompilationScope = rs;
            _wasExecuted = false;
            rs->extend();
        }


        virtual void normal_send( InterpretedInlineCache * ic ) {
            check_send( ic );
        }


        virtual void self_send( InterpretedInlineCache * ic ) {
            check_send( ic );
        }


        virtual void super_send( InterpretedInlineCache * ic ) {
            check_send( ic );
        }


        void check_send( InterpretedInlineCache * ic ) {
            GrowableArray <RecompilationScope *> * sub = _recompilationScope->subScopes( byteCodeIndex() );
            if ( sub->length() == 1 and sub->first()->isUntakenScope() ) {
                // this send was never taken
            } else {
                _wasExecuted = true;            // this send was taken in recompilee
            }
        }
};


bool_t PrimitiveInliner::basic_shouldUseUncommonTrap() const {
    ResourceMark resourceMark;
    if ( not theCompiler->useUncommonTraps )
        return false;
    // first check if recompilee has a taken uncommon branch
    // NOTE: the uncommon branch should be at _failure_block->begin_byteCodeIndex(), not _byteCodeIndex, but
    // deoptimization currently breaks if that's so -- please fix this, Lars
    // (search for new_UncommonNode below when fixing it)
    if ( _scope->rscope->isNotUncommonAt( _byteCodeIndex ) )
        return false;
    //if (_scope->rscope->isNotUncommonAt(_failure_block->begin_byteCodeIndex())) return false;
    if ( _scope->rscope->isUncommonAt( _byteCodeIndex ) )
        return true;
    //if (_scope->rscope->isUncommonAt(_failure_block->begin_byteCodeIndex())) return true;
    // now check if recompilee had code for the failure block, and any send in it was taken
    if ( not _scope->rscope->get_nativeMethod() ) {
        return true;      // if it's not compiled, assume that using uncommon branch is ok
    }
#ifdef broken
    // This code doesn't really do the right thing; what's needed is something that
    // can determine whether the failure was taken in the previous version of compiled code.
    // If there's a send that's easy (ic dirty flag), but if everything in the failure block
    // was inlined...?  Could put a "removable trap" there to determine this.  fix this later
    // -Urs 8/96
    PrimitiveSendFinder p(_scope->rscope);
    MethodIterator it(_failure_block, &p);
    return not p.was_executed;
#endif
    // for now, try the aggressive approach: no proof that it was taken, so guess that it's uncommon
    return true;
}


bool_t PrimitiveInliner::shouldUseUncommonTrap() {
    // remember result of call in _usingUncommonTrap
    _usingUncommonTrap = basic_shouldUseUncommonTrap();
    return _usingUncommonTrap;
}


Expression * PrimitiveInliner::smi_ArithmeticOp( ArithOpCode op, Expression * arg1, Expression * arg2 ) {
    assert_failure_block();
    assert_receiver();

    // parameters & result registers
    bool_t intArg1 = arg1->is_smi();
    bool_t intArg2 = arg2->is_smi();
    bool_t intBoth = intArg1 and intArg2;            // if true, tag error cannot occur
    SinglyAssignedPseudoRegister * resPReg      = new SinglyAssignedPseudoRegister( _scope );            // holds the result if primitive didn't fail
    SinglyAssignedPseudoRegister * errPReg      = new SinglyAssignedPseudoRegister( _scope );            // holds the error message if primitive failed
    MergeNode                    * failureStart = NodeFactory::MergeNode( _failure_block->begin_byteCodeIndex() );

    // n1: operation & treatment of tag error
    Node               * n1;
    AssignNode         * n1Err;
    ConstantExpression * n1Expression           = nullptr;
    if ( intBoth ) {
        // tag error cannot occur
        n1 = NodeFactory::ArithRRNode( resPReg, arg1->preg(), op, arg2->preg() );
    } else {
        // tag error can occur
        n1 = NodeFactory::TArithRRNode( resPReg, arg1->preg(), op, arg2->preg(), intArg1, intArg2 );
        if ( shouldUseUncommonTrap() ) {
            // simply jump to uncommon branch code
            n1->append( 1, failureStart );
        } else {
            ConstPseudoRegister * n1PReg = new_ConstPReg( _scope, vmSymbols::first_argument_has_wrong_type() );
            n1Err                        = NodeFactory::AssignNode( n1PReg, errPReg );
            n1Expression                 = new ConstantExpression( n1PReg->constant, errPReg, n1Err );
            n1->append( 1, n1Err );
            n1Err->append( failureStart );
        }
    }
    _gen->append( n1 );
    Expression * result = new KlassExpression( smiKlassObj, resPReg, n1 );

    // n2: overflow check & treatment of overflow
    const bool_t taken_is_uncommon = true;
    BranchNode         * n2           = NodeFactory::BranchNode( BranchOpCode::VSBranchOp, taken_is_uncommon );
    AssignNode         * n2Err;
    ConstantExpression * n2Expression = nullptr;
    if ( shouldUseUncommonTrap() ) {
        // simply jump to uncommon branch code
        n2->append( 1, failureStart );
    } else {
        ConstPseudoRegister * n2PReg = new_ConstPReg( _scope, vmSymbols::smi_overflow() );
        n2Err        = NodeFactory::AssignNode( n2PReg, errPReg );
        n2Expression = new ConstantExpression( n2PReg->constant, errPReg, n2Err );
        n2->append( 1, n2Err );
        n2Err->append( failureStart );
    }
    _gen->append( n2 );

    // continuation
    if ( shouldUseUncommonTrap() ) {
        // uncommon branch instead of failure code
        failureStart->append( NodeFactory::UncommonNode( _gen->copyCurrentExprStack(), _byteCodeIndex /*_failure_block->begin_byteCodeIndex()*/) );
    } else {
        st_assert( n2Expression not_eq nullptr, "no error message defined" );
        Expression * error;
        if ( n1Expression not_eq nullptr ) {
            error = new MergeExpression( n1Expression, n2Expression, errPReg, failureStart );
        } else {
            error = n2Expression;
        }
        result = merge_failure_block( n2, result, failureStart, error, false );
    }
    return result;
}


Expression * PrimitiveInliner::smi_BitOp( ArithOpCode op, Expression * arg1, Expression * arg2 ) {
    assert_failure_block();
    assert_receiver();

    // parameters & result registers
    bool_t intArg1 = arg1->is_smi();
    bool_t intArg2 = arg2->is_smi();
    bool_t intBoth = intArg1 and intArg2;    // if true, tag error cannot occur
    SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the result if primitive didn't fail
    SinglyAssignedPseudoRegister * errPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the error message if primitive failed

    // n1: operation & treatment of tag error
    Node               * n1;
    AssignNode         * n1Err;
    ConstantExpression * n1Expression;
    if ( intBoth ) {
        // tag error cannot occur
        n1 = NodeFactory::ArithRRNode( resPReg, arg1->preg(), op, arg2->preg() );
    } else {
        // tag error can occur
        n1                           = NodeFactory::TArithRRNode( resPReg, arg1->preg(), op, arg2->preg(), intArg1, intArg2 );
        ConstPseudoRegister * n1PReg = new_ConstPReg( _scope, vmSymbols::first_argument_has_wrong_type() );
        n1Err        = NodeFactory::AssignNode( n1PReg, errPReg );
        n1Expression = new ConstantExpression( n1PReg->constant, errPReg, n1Err );
        n1->append( 1, n1Err );
    }
    _gen->append( n1 );

    // continuation
    Expression * result = new KlassExpression( smiKlassObj, resPReg, n1 );
    if ( not intBoth ) {
        // failure can occur
        if ( shouldUseUncommonTrap() ) {
            // simply do an uncommon trap
            n1Err->append( NodeFactory::UncommonNode( _gen->copyCurrentExprStack(), _byteCodeIndex /*_failure_block->begin_byteCodeIndex()*/) );
        } else {
            // treat failure case
            result = merge_failure_block( n1, result, n1Err, n1Expression, false );
        }
    }
    return result;
}


Expression * PrimitiveInliner::smi_Div( Expression * x, Expression * y ) {
    if ( y->preg()->isConstPseudoRegister() ) {
        st_assert( y->is_smi(), "type check should have failed" );
        int d = SMIOop( ( ( ConstPseudoRegister * ) y->preg() )->constant )->value();
        if ( is_power_of_2( d ) ) {
            // replace it with shift
            ConstPseudoRegister * preg = new_ConstPReg( _scope, smiOopFromValue( -log2( d ) ) );
            return smi_BitOp( ArithOpCode::tShiftArithOp, x, new ConstantExpression( preg->constant, preg, _gen->_current ) );
        }
    }
    // otherwise leave it alone
    return nullptr;
}


Expression * PrimitiveInliner::smi_Mod( Expression * x, Expression * y ) {
    if ( y->preg()->isConstPseudoRegister() ) {
        st_assert( y->is_smi(), "type check should have failed" );
        int d = SMIOop( ( ( ConstPseudoRegister * ) y->preg() )->constant )->value();
        if ( is_power_of_2( d ) ) {
            // replace it with mask
            ConstPseudoRegister * preg = new_ConstPReg( _scope, smiOopFromValue( d - 1 ) );
            return smi_BitOp( ArithOpCode::tAndArithOp, x, new ConstantExpression( preg->constant, preg, _gen->_current ) );
        }
    }
    // otherwise leave it alone
    return nullptr;
}


Expression * PrimitiveInliner::smi_Shift( Expression * arg1, Expression * arg2 ) {
    if ( parameter( 1 )->preg()->isConstPseudoRegister() ) {
        // inline if the shift count is a constant
        st_assert( arg2->is_smi(), "type check should have failed" );
        return smi_BitOp( ArithOpCode::tShiftArithOp, arg1, arg2 );
    }
    // otherwise leave it alone
    return nullptr;
}


static BranchOpCode Not( BranchOpCode cond ) {
    switch ( cond ) {
        case BranchOpCode::EQBranchOp:
            return BranchOpCode::NEBranchOp;
        case BranchOpCode::NEBranchOp:
            return BranchOpCode::EQBranchOp;
        case BranchOpCode::LTBranchOp:
            return BranchOpCode::GEBranchOp;
        case BranchOpCode::LEBranchOp:
            return BranchOpCode::GTBranchOp;
        case BranchOpCode::GTBranchOp:
            return BranchOpCode::LEBranchOp;
        case BranchOpCode::GEBranchOp:
            return BranchOpCode::LTBranchOp;
        case BranchOpCode::LTUBranchOp:
            return BranchOpCode::GEUBranchOp;
        case BranchOpCode::LEUBranchOp:
            return BranchOpCode::GTUBranchOp;
        case BranchOpCode::GTUBranchOp:
            return BranchOpCode::LEUBranchOp;
        case BranchOpCode::GEUBranchOp:
            return BranchOpCode::LTUBranchOp;
        case BranchOpCode::VSBranchOp:
            return BranchOpCode::VCBranchOp;
        case BranchOpCode::VCBranchOp:
            return BranchOpCode::VSBranchOp;
    }
    ShouldNotReachHere();
    return BranchOpCode::EQBranchOp;
}


Expression * PrimitiveInliner::generate_cond( BranchOpCode cond, NodeBuilder * gen, PseudoRegister * resPReg ) {
    // generate code loading true or false depending on current condition code
    // Note: condition is negated in order to provoke a certain code order
    //       when compiling whileTrue loops - gri 6/26/96

    // n2: conditional branch
    BranchNode * n2 = NodeFactory::BranchNode( Not( cond ) );
    gen->append( n2 );

    // tAsgn: true branch
    ConstPseudoRegister * tPReg       = new_ConstPReg( gen->scope(), trueObj );
    AssignNode          * tAsgn       = NodeFactory::AssignNode( tPReg, resPReg );
    ConstantExpression  * tExpression = new ConstantExpression( trueObj, resPReg, tAsgn );
    n2->append( 0, tAsgn );

    // fAsgn: false branch
    ConstPseudoRegister * fPReg       = new_ConstPReg( gen->scope(), falseObj );
    AssignNode          * fAsgn       = NodeFactory::AssignNode( fPReg, resPReg );
    ConstantExpression  * fExpression = new ConstantExpression( falseObj, resPReg, fAsgn );
    n2->append( 1, fAsgn );

    // ftm: false & true branch merger
    MergeNode  * ftm    = NodeFactory::MergeNode( fAsgn, tAsgn );
    Expression * result = new MergeExpression( fExpression, tExpression, resPReg, ftm );
    gen->setCurrent( ftm );
    return result;
}


Expression * PrimitiveInliner::smi_Comparison( BranchOpCode cond, Expression * arg1, Expression * arg2 ) {
    assert_failure_block();
    assert_receiver();

    // parameters & result registers
    bool_t intArg1 = arg1->is_smi();
    bool_t intArg2 = arg2->is_smi();
    bool_t intBoth = intArg1 and intArg2;    // if true, tag error cannot occur
    SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the result if primitive didn't fail
    SinglyAssignedPseudoRegister * errPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the error message if primitive failed

    // n1: comparison & treatment of tag error
    Node                * n1;
    ConstPseudoRegister * n1PReg;
    AssignNode          * n1Err;
    ConstantExpression  * n1Expression;
    if ( intBoth ) {
        // tag error cannot occur
        n1 = NodeFactory::ArithRRNode( new NoResultPseudoRegister( _scope ), arg1->preg(), ArithOpCode::tCmpArithOp, arg2->preg() );
    } else {
        // tag error can occur
        n1           = NodeFactory::TArithRRNode( new NoResultPseudoRegister( _scope ), arg1->preg(), ArithOpCode::tCmpArithOp, arg2->preg(), intArg1, intArg2 );
        n1PReg       = new_ConstPReg( _scope, vmSymbols::first_argument_has_wrong_type() );
        n1Err        = NodeFactory::AssignNode( n1PReg, errPReg );
        n1Expression = new ConstantExpression( n1PReg->constant, errPReg, n1Err );
        n1->append( 1, n1Err );
    }
    _gen->append( n1 );

    Expression * result = generate_cond( cond, _gen, resPReg );

    // continuation
    if ( not intBoth ) {
        // failure can occur
        if ( shouldUseUncommonTrap() ) {
            // simply do an uncommon trap
            n1Err->append( NodeFactory::UncommonNode( _gen->copyCurrentExprStack(), _byteCodeIndex /*_failure_block->begin_byteCodeIndex()*/) );
        } else {
            // treat failure case
            result = merge_failure_block( result->node(), result, n1Err, n1Expression, false );
        }
    }
    return result;
}


Expression * PrimitiveInliner::array_size() {
    st_assert( _failure_block == nullptr, "primitive must have no failure block" );
    assert_receiver();

    // parameters & result registers
    Expression * array = parameter( 0 );
    Klass      * klass = array->klass()->klass_part();
    int lenOffs = klass->non_indexable_size();

    // get size
    SinglyAssignedPseudoRegister * res = new SinglyAssignedPseudoRegister( _scope );
    _gen->append( NodeFactory::LoadOffsetNode( res, array->preg(), lenOffs, true ) );
    return new KlassExpression( smiKlassObj, res, _gen->_current );
}


Expression * PrimitiveInliner::array_at_ifFail( ArrayAtNode::AccessType access_type ) {
    assert_failure_block();
    assert_receiver();

    // parameters & result registers
    Expression                   * array   = parameter( 0 );
    Expression                   * index   = parameter( 1 );
    SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the result if primitive didn't fail
    SinglyAssignedPseudoRegister * errPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the error message if primitive failed
    Klass                        * klass   = array->klass()->klass_part();
    int lenOffs = klass->non_indexable_size();
    int arrOffs = lenOffs + 1;

    // at node
    ArrayAtNode * at = NodeFactory::ArrayAtNode( access_type, array->preg(), index->preg(), index->is_smi(), resPReg, errPReg, arrOffs, lenOffs );
    _gen->append( at );

    // continuation
    Expression * resExpression;
    switch ( access_type ) {
        case ArrayAtNode::byte_at        : // fall through
        case ArrayAtNode::double_byte_at:
            resExpression = new KlassExpression( Universe::smiKlassObj(), resPReg, at );
            break;
        case ArrayAtNode::character_at:
            resExpression = new KlassExpression( Universe::characterKlassObj(), resPReg, at );
            break;
        case ArrayAtNode::object_at:
            resExpression = new UnknownExpression( resPReg, at );
            break;
        default: ShouldNotReachHere();
    }
    if ( at->canFail() ) {
        if ( shouldUseUncommonTrap() ) {
            // uncommon branch instead of failure code
            at->append( 1, NodeFactory::UncommonNode( _gen->copyCurrentExprStack(), _byteCodeIndex /*_failure_block->begin_byteCodeIndex()*/) );
        } else {
            // append failure code
            NopNode * err = NodeFactory::NopNode();
            at->append( 1, err );
            Expression * errExpression = new KlassExpression( Universe::symbolKlassObj(), errPReg, at );
            resExpression              = merge_failure_block( at, resExpression, err, errExpression, false );
        }
    }
    return resExpression;
}


Expression * PrimitiveInliner::array_at_put_ifFail( ArrayAtPutNode::AccessType access_type ) {
    assert_failure_block();
    assert_receiver();

    // parameters & result registers
    Expression                   * array   = parameter( 0 );
    Expression                   * index   = parameter( 1 );
    Expression                   * element = parameter( 2 );
    SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the result if primitive didn't fail
    SinglyAssignedPseudoRegister * errPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the error message if primitive failed
    Klass                        * klass   = array->klass()->klass_part();
    int    lenOffs    = klass->non_indexable_size();
    int    arrOffs    = lenOffs + 1;
    bool_t storeCheck = ( access_type == ArrayAtPutNode::object_at_put ) and element->needsStoreCheck();

    if ( access_type == ArrayAtPutNode::object_at_put ) {
        // make sure blocks stored into array are created
        _gen->materialize( element->preg(), nullptr );
    }

    // atPut node
    ArrayAtPutNode * atPut = NodeFactory::ArrayAtPutNode( access_type, array->preg(), index->preg(), index->is_smi(), element->preg(), element->is_smi(), nullptr, errPReg, arrOffs, lenOffs, storeCheck );
    _gen->append( atPut );

    // continuation
    Expression * resExpression = array;
    if ( atPut->canFail() ) {
        if ( shouldUseUncommonTrap() ) {
            // uncommon branch instead of failure code
            atPut->append( 1, NodeFactory::UncommonNode( _gen->copyCurrentExprStack(), _byteCodeIndex /*_failure_block->begin_byteCodeIndex()*/) );
        } else {
            // append failure code
            NopNode * err = NodeFactory::NopNode();
            atPut->append( 1, err );
            Expression * errExpression = new KlassExpression( Universe::symbolKlassObj(), errPReg, atPut );
            resExpression = merge_failure_block( atPut, resExpression, err, errExpression, true );
        }
    }
    return resExpression;
}


Expression * PrimitiveInliner::obj_new() {
    // replace generic allocation primitive by size-specific primitive, if possible
    Expression * receiver = parameter( 0 );
    if ( not receiver->isConstantExpression() or not receiver->constant()->is_klass() )
        return nullptr;
    Klass * klass = KlassOop( receiver->constant() )->klass_part();    // class being instantiated
    if ( klass->oop_is_indexable() )
        return nullptr;            // would fail (extremely unlikely)
    int size = klass->non_indexable_size();            // size in words

    if ( klass->can_inline_allocation() ) {
        // These special compiler primitives only work for MemOop klasses
        int number_of_instance_variables = size - MemOopDescriptor::header_size();
        switch ( number_of_instance_variables ) {
            case 0:
                _primitiveDescriptor = Primitives::new0();
                break;
            case 1:
                _primitiveDescriptor = Primitives::new1();
                break;
            case 2:
                _primitiveDescriptor = Primitives::new2();
                break;
            case 3:
                _primitiveDescriptor = Primitives::new3();
                break;
            case 4:
                _primitiveDescriptor = Primitives::new4();
                break;
            case 5:
                _primitiveDescriptor = Primitives::new5();
                break;
            case 6:
                _primitiveDescriptor = Primitives::new6();
                break;
            case 7:
                _primitiveDescriptor = Primitives::new7();
                break;
            case 8:
                _primitiveDescriptor = Primitives::new8();
                break;
            case 9:
                _primitiveDescriptor = Primitives::new9();
                break;
            default:; // use generic primitives
        }
    }
    Expression * u = genCall( true );
    return new KlassExpression( klass->as_klassOop(), u->preg(), u->node() );
}


Expression * PrimitiveInliner::obj_shallowCopy() {
    // temporary hack, fix when prims have type info
    // Fix this Robert, 10/24-95, Lars
    Expression * u = genCall( false );
    st_assert( u->isUnknownExpression(), "oops" );
    return new KlassExpression( parameter( 0 )->klass(), u->preg(), u->node() );
}


Expression * PrimitiveInliner::obj_equal() {
    assert_no_failure_block();

    // parameters & result registers
    PseudoRegister * arg1 = parameter( 0 )->preg();
    PseudoRegister * arg2 = parameter( 1 )->preg();
    // comparison
    _gen->append( NodeFactory::ArithRRNode( new NoResultPseudoRegister( _scope ), arg1, ArithOpCode::CmpArithOp, arg2 ) );
    SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );
    return generate_cond( BranchOpCode::EQBranchOp, _gen, resPReg );
}


Expression * PrimitiveInliner::obj_class( bool_t has_receiver ) {
    assert_no_failure_block();
    if ( has_receiver )
        assert_receiver();

    Expression * obj = parameter( 0 );
    if ( obj->hasKlass() ) {
        // constant-fold it
        KlassOop k = obj->klass();
        return new ConstantExpression( k, new_ConstPReg( _scope, k ), nullptr );
    } else {
        SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );
        InlinedPrimitiveNode         * n       = NodeFactory::InlinedPrimitiveNode( InlinedPrimitiveNode::Operation::obj_klass, resPReg, nullptr, obj->preg() );
        _gen->append( n );
        // don't know exactly what it is - just use PolymorphicInlineCache info
        return new UnknownExpression( resPReg, n );
    }
}


Expression * PrimitiveInliner::obj_hash( bool_t has_receiver ) {
    assert_no_failure_block();
    if ( has_receiver )
        assert_receiver();

    Expression * obj = parameter( 0 );
    if ( obj->is_smi() ) {
        // hash value = self (no code necessary)
        return obj;
    } else {
        return nullptr;
        //
        // code in x86_node.cpp not yet implemented - fix this at some point
        //
        // has value taken from header field
        SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );
        InlinedPrimitiveNode         * n       = NodeFactory::InlinedPrimitiveNode( InlinedPrimitiveNode::Operation::obj_hash, resPReg, nullptr, obj->preg() );
        _gen->append( n );
        return new KlassExpression( Universe::smiKlassObj(), resPReg, n );
    }
}


Expression * PrimitiveInliner::proxy_byte_at() {
    assert_failure_block();
    assert_receiver();

    // parameters & result registers
    Expression                   * proxy   = parameter( 0 );
    Expression                   * index   = parameter( 1 );
    SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the result if primitive didn't fail
    SinglyAssignedPseudoRegister * errPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the error message if primitive failed

    // at node
    InlinedPrimitiveNode * at = NodeFactory::InlinedPrimitiveNode( InlinedPrimitiveNode::Operation::proxy_byte_at, resPReg, errPReg, proxy->preg(), index->preg(), index->is_smi() );
    _gen->append( at );

    // continuation
    Expression * resExpression = new KlassExpression( Universe::smiKlassObj(), resPReg, at );
    if ( at->canFail() ) {
        if ( shouldUseUncommonTrap() ) {
            // uncommon branch instead of failure code
            at->append( 1, NodeFactory::UncommonNode( _gen->copyCurrentExprStack(), _byteCodeIndex /*_failure_block->begin_byteCodeIndex()*/) );
        } else {
            // append failure code
            NopNode * err = NodeFactory::NopNode();
            at->append( 1, err );
            Expression * errExpression = new KlassExpression( Universe::symbolKlassObj(), errPReg, at );
            resExpression = merge_failure_block( at, resExpression, err, errExpression, false );
        }
    }
    return resExpression;
}


Expression * PrimitiveInliner::proxy_byte_at_put() {
    assert_failure_block();
    assert_receiver();

    // parameters & result registers
    Expression                   * proxy   = parameter( 0 );
    Expression                   * index   = parameter( 1 );
    Expression                   * value   = parameter( 2 );
    SinglyAssignedPseudoRegister * errPReg = new SinglyAssignedPseudoRegister( _scope );    // holds the error message if primitive failed

    // atPut node
    InlinedPrimitiveNode * atPut = NodeFactory::InlinedPrimitiveNode( InlinedPrimitiveNode::Operation::proxy_byte_at_put, nullptr, errPReg, proxy->preg(), index->preg(), index->is_smi(), value->preg(), value->is_smi() );
    _gen->append( atPut );

    // continuation
    Expression * resExpression = proxy;
    if ( atPut->canFail() ) {
        if ( shouldUseUncommonTrap() ) {
            // uncommon branch instead of failure code
            atPut->append( 1, NodeFactory::UncommonNode( _gen->copyCurrentExprStack(), _byteCodeIndex /*_failure_block->begin_byteCodeIndex()*/) );
        } else {
            // append failure code
            NopNode * err = NodeFactory::NopNode();
            atPut->append( 1, err );
            Expression * errExpression = new KlassExpression( Universe::symbolKlassObj(), errPReg, atPut );
            resExpression = merge_failure_block( atPut, resExpression, err, errExpression, true );
        }
    }
    return resExpression;
}


Expression * PrimitiveInliner::block_primitiveValue() {
    PseudoRegister * r = parameter( 0 )->preg();
    if ( r->isBlockPseudoRegister() ) {
        // we know the identity of the block -- inline it if possible
        Inliner    * inliner = _gen->inliner();
        SendInfo   * info    = new SendInfo( _scope, parameter( 0 ), _primitiveDescriptor->selector() );
        Expression * res     = inliner->inlineBlockInvocation( info );
        if ( res )
            return res;
    }
    // could at least inline block invocation -- fix this later
    return nullptr;
}


Expression * PrimitiveInliner::tryInline() {

    // Returns the failure result or the result of the primitive (if the primitive can't fail) if the primitive has been inlined; returns nullptr otherwise.
    // If the primitive has been inlined but can't fail, the byteCodeIndex in the MethodDecoder is set to the first instruction after the failure block.
    // NB: The comparisons below should be replaced with pointer comparisons comparing with the appropriate vmSymbol. Should fix this at some point.

    const char * name = _primitiveDescriptor->name();
    Expression * res  = nullptr;
    switch ( _primitiveDescriptor->group() ) {
        case PrimitiveGroup::IntArithmeticPrimitive:
            if ( number_of_parameters() == 2 ) {
                Expression * x = parameter( 0 );
                Expression * y = parameter( 1 );
                if ( equal( name, "primitiveAdd:ifFail:" ) ) {
                    res = smi_ArithmeticOp( ArithOpCode::tAddArithOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveSubtract:ifFail:" ) ) {
                    res = smi_ArithmeticOp( ArithOpCode::tSubArithOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveMultiply:ifFail:" ) ) {
                    res = smi_ArithmeticOp( ArithOpCode::tMulArithOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveDiv:ifFail:" ) ) {
                    res = smi_Div( x, y );
                    break;
                }
                if ( equal( name, "primitiveMod:ifFail:" ) ) {
                    res = smi_Mod( x, y );
                    break;
                }
                if ( equal( name, "primitiveBitAnd:ifFail:" ) ) {
                    res = smi_BitOp( ArithOpCode::tAndArithOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveBitOr:ifFail:" ) ) {
                    res = smi_BitOp( ArithOpCode::tOrArithOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveBitXor:ifFail:" ) ) {
                    res = smi_BitOp( ArithOpCode::tXOrArithOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveRawBitShift:ifFail:" ) ) {
                    res = smi_Shift( x, y );
                    break;
                }
            }
            break;
        case PrimitiveGroup::IntComparisonPrimitive:
            if ( number_of_parameters() == 2 ) {
                Expression * x = parameter( 0 );
                Expression * y = parameter( 1 );
                if ( equal( name, "primitiveSmallIntegerEqual:ifFail:" ) ) {
                    res = smi_Comparison( BranchOpCode::EQBranchOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveSmallIntegerNotEqual:ifFail:" ) ) {
                    res = smi_Comparison( BranchOpCode::NEBranchOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveLessThan:ifFail:" ) ) {
                    res = smi_Comparison( BranchOpCode::LTBranchOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveLessThanOrEqual:ifFail:" ) ) {
                    res = smi_Comparison( BranchOpCode::LEBranchOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveGreaterThan:ifFail:" ) ) {
                    res = smi_Comparison( BranchOpCode::GTBranchOp, x, y );
                    break;
                }
                if ( equal( name, "primitiveGreaterThanOrEqual:ifFail:" ) ) {
                    res = smi_Comparison( BranchOpCode::GEBranchOp, x, y );
                    break;
                }
            }
            break;
        case PrimitiveGroup::FloatArithmeticPrimitive:
            break;
        case PrimitiveGroup::FloatComparisonPrimitive:
            break;
        case PrimitiveGroup::ObjArrayPrimitive:
            if ( equal( name, "primitiveIndexedObjectSize" ) ) {
                res = array_size();
                break;
            }
            if ( equal( name, "primitiveIndexedObjectAt:ifFail:" ) ) {
                res = array_at_ifFail( ArrayAtNode::object_at );
                break;
            }
            if ( equal( name, "primitiveIndexedObjectAt:put:ifFail:" ) ) {
                res = array_at_put_ifFail( ArrayAtPutNode::object_at_put );
                break;
            }
            break;
        case PrimitiveGroup::ByteArrayPrimitive:
            if ( equal( name, "primitiveIndexedByteSize" ) ) {
                res = array_size();
                break;
            }
            if ( equal( name, "primitiveIndexedByteAt:ifFail:" ) ) {
                res = array_at_ifFail( ArrayAtNode::byte_at );
                break;
            }
            if ( equal( name, "primitiveIndexedByteAt:put:ifFail:" ) ) {
                res = array_at_put_ifFail( ArrayAtPutNode::byte_at_put );
                break;
            }
            break;
        case PrimitiveGroup::DoubleByteArrayPrimitive:
            if ( equal( name, "primitiveIndexedDoubleByteSize" ) ) {
                res = array_size();
                break;
            }
            if ( equal( name, "primitiveIndexedDoubleByteAt:ifFail:" ) ) {
                res = array_at_ifFail( ArrayAtNode::double_byte_at );
                break;
            }
            if ( equal( name, "primitiveIndexedDoubleByteCharacterAt:ifFail:" ) ) {
                res = array_at_ifFail( ArrayAtNode::character_at );
                break;
            }
            if ( equal( name, "primitiveIndexedDoubleByteAt:put:ifFail:" ) ) {
                res = array_at_put_ifFail( ArrayAtPutNode::double_byte_at_put );
                break;
            }
            break;
        case PrimitiveGroup::BlockPrimitive:
            if ( strncmp( name, "primitiveValue", 14 ) == 0 ) {
                res = block_primitiveValue();
                break;
            }
            break;
        case PrimitiveGroup::NormalPrimitive:
            if ( strncmp( name, "primitiveNew", 12 ) == 0 ) {
                res = obj_new();
                break;
            }
            if ( equal( name, "primitiveShallowCopyIfFail:ifFail:" ) ) {
                res = obj_shallowCopy();
                break;
            }
            if ( equal( name, "primitiveEqual:" ) ) {
                res = obj_equal();
                break;
            }
            if ( equal( name, "primitiveClass" ) ) {
                res = obj_class( true );
                break;
            }
            if ( equal( name, "primitiveClassOf:" ) ) {
                res = obj_class( false );
                break;
            }
            if ( equal( name, "primitiveHash" ) ) {
                res = obj_hash( true );
                break;
            }
            if ( equal( name, "primitiveHashOf:" ) ) {
                res = obj_hash( false );
                break;
            }
            if ( equal( name, "primitiveProxyByteAt:ifFail:" ) ) {
                res = proxy_byte_at();
                break;
            }
            if ( equal( name, "primitiveProxyByteAt:put:ifFail:" ) ) {
                res = proxy_byte_at_put();
                break;
            }
            break;
        default: st_fatal1( "bad primitive group %d", _primitiveDescriptor->group() );
            break;
    }

    if ( CompilerDebug ) {
        cout( PrintInlining and ( res not_eq nullptr ) )->print( "%*sinlining %s %s\n", _scope->depth + 2, "", _primitiveDescriptor->name(), _usingUncommonTrap ? "(unc. failure)" : ( _cannotFail ? "(cannot fail)" : "" ) );
    }
    if ( not _usingUncommonTrap and not _cannotFail )
        theCompiler->reporter->report_primitive_failure( _primitiveDescriptor );
    return res;
}


PrimitiveInliner::PrimitiveInliner( NodeBuilder * gen, PrimitiveDescriptor * pdesc, MethodInterval * failure_block ) {
    st_assert( pdesc, "must have a primitive desc to inline" );
    _gen                 = gen;
    _primitiveDescriptor = pdesc;
    _failure_block       = failure_block;

    _scope             = gen->_scope;
    _byteCodeIndex     = gen->byteCodeIndex();      // byteCodeIndex of primitive call
    _expressionStack   = gen->_expressionStack;
    _cannotFail        = true;
    _params            = new GrowableArray <Expression *>( number_of_parameters(), number_of_parameters(), nullptr );
    _usingUncommonTrap = false;

    // get parameters
    int i     = number_of_parameters();
    int first = _expressionStack->length() - i;
    while ( i-- > 0 ) {
        _params->at_put( i, _expressionStack->at( first + i ) );
    }

    // %hack: assertion fails
    st_assert( _primitiveDescriptor->can_fail() == ( failure_block not_eq nullptr ), "primitive desc & primitive usage inconsistent" );
}


void PrimitiveInliner::generate() {
    Expression * res = nullptr;
    if ( InlinePrims ) {
        if ( ConstantFoldPrims ) {
            res = tryConstantFold();
        }
        if ( res == nullptr ) {
            res = tryTypeCheck();
        }
        if ( res == nullptr ) {
            res = tryInline();
        }
    }
    if ( res == nullptr ) {
        // primitive has not been constant-folded nor inlined
        // -> must call primitive at run-time
        res = genCall( _primitiveDescriptor->can_fail() );
    }
    st_assert( res, "must have result expr" );
    _expressionStack->pop( _primitiveDescriptor->number_of_parameters() );    // pop parameters
    _expressionStack->push2nd( res, _scope, _byteCodeIndex );        // push primitive result
}


Expression * PrimitiveInliner::genCall( bool_t canFail ) {

    // standard primitive call
    //
    // Note: If a primitive fails, it will return a marked symbol. One has to make sure that
    //       this marked symbol is *never* stored on the stack but only kept in registers
    //       (otherwise GC won't work correctly). Here this is established by using the dst()
    //       of a pcall only which is pre-allocated to the result_reg.

    MergeNode * nlrTestPoint = _primitiveDescriptor->can_perform_NonLocalReturn() ? _scope->nlrTestPoint() : nullptr;
    GrowableArray <PseudoRegister *> * args      = _gen->pass_arguments( nullptr, _primitiveDescriptor->number_of_parameters() );
    GrowableArray <PseudoRegister *> * exprStack = _primitiveDescriptor->can_walk_stack() ? _gen->copyCurrentExprStack() : nullptr;
    PrimitiveNode * pcall = NodeFactory::PrimitiveNode( _primitiveDescriptor, nlrTestPoint, args, exprStack );

    _gen->append( pcall );

    BranchNode * branch = nullptr;
    if ( _failure_block not_eq nullptr and canFail ) {
        // primitive with failure block; failed if MARK_TAG_BIT is set in result -> add a branch node here
        _gen->append( NodeFactory::ArithRCNode( new NoResultPseudoRegister( _scope ), pcall->dest(), ArithOpCode::TestArithOp, MARK_TAG_BIT ) );
        branch = NodeFactory::BranchNode( BranchOpCode::NEBranchOp );
        _gen->append( branch );
    }

    // primitive didn't fail (with or without failure block) -> assign to resPReg & determine its expression
    SinglyAssignedPseudoRegister * resPReg = new SinglyAssignedPseudoRegister( _scope );
    _gen->append( NodeFactory::AssignNode( pcall->dst(), resPReg ) );
    Node       * ok_exit       = _gen->_current;
    Expression * resExpression = _primitiveDescriptor->return_klass( resPReg, ok_exit );
    if ( resExpression == nullptr )
        resExpression = new UnknownExpression( resPReg, ok_exit );

    if ( branch not_eq nullptr ) {
        // add failure block if primitive can fail -> reset MARK_TAG_BIT first
        SinglyAssignedPseudoRegister * errPReg      = new SinglyAssignedPseudoRegister( _scope );
        ArithRCNode                  * failure_exit = NodeFactory::ArithRCNode( errPReg, pcall->dst(), ArithOpCode::AndArithOp, ~MARK_TAG_BIT );
        branch->append( 1, failure_exit );
        resExpression = merge_failure_block( ok_exit, resExpression, failure_exit, new KlassExpression( Universe::symbolKlassObj(), errPReg, failure_exit ), false );
    }

    return resExpression;
}


void PrimitiveInliner::print() {
    _console->print_cr( "a PrimitiveInliner" );
}
