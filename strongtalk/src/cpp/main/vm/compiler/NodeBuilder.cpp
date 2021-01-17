//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/compiler/NodeBuilder.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/compiler/Inliner.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/compiler/PrimitiveInliner.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


// Class variables

Node * NodeBuilder::EndOfCode = ( Node * ) -1;


// Initialization

NodeBuilder::NodeBuilder() {
    _expressionStack = nullptr;
    _inliner         = nullptr;
    _scope           = nullptr;
    _current         = nullptr;
}


void NodeBuilder::initialize( InlinedScope * scope ) {
    _expressionStack = new ExpressionStack( scope, 8 );
    _inliner         = new Inliner( scope );
    _scope           = scope;
    _current         = nullptr;
}


// Helper functions for node creation/concatenation

void NodeBuilder::append( Node * node ) {
    if ( node->isExitNode() )
        warning( "should use append_exit for consistency" );

    if ( aborting() or _current == EndOfCode ) {
        if ( node->nPredecessors() == 0 ) {
            // ignore this code
            // (sometimes, the bytecode compiler generates dead code after returns)
        } else {
            // node is already connected to graph (merge node), so restart code generation
            _current = node;
        }
    } else {
        _current = _current->append( node );
    }
}


void NodeBuilder::append_exit( Node * node ) {
    st_assert( node->isExitNode(), "not an exit node" );
    if ( _current not_eq EndOfCode )
        _current->append( node );
    _current = nullptr;
}


void NodeBuilder::branch( MergeNode * target ) {
    // connect current with target
    if ( _current not_eq nullptr and _current not_eq EndOfCode and _current not_eq target ) {
        _current->append( target );
    }
    _current = target;
}


void NodeBuilder::comment( const char * s ) {
    if ( CompilerDebug )
        append( NodeFactory::CommentNode( s ) );
}


GrowableArray <PseudoRegister *> * NodeBuilder::copyCurrentExprStack() {

    int l = exprStack()->length();
    auto * es = new GrowableArray <PseudoRegister *>( l );

    for ( int i = 0; i < l; i++ ) {
        es->push( exprStack()->at( i )->preg() );
    }

    return es;
}


// Node creation

void NodeBuilder::abort() {
    MethodClosure::abort();
    _current = EndOfCode;
}


bool_t NodeBuilder::abortIfDead( Expression * e ) {
    if ( e->isNoResultExpression() ) {
        // dead code
        abort();
        return true;
    } else {
        return false;
    }
}


// Caution: do not use MethodIterator directly, always go through this function.
// Otherwise, things will break in the presence of dead code.
// Also, after calling generate_subinterval, make sure you set current (since
// the subinterval may have ended dead, leaving current at EndOfCode).
void NodeBuilder::generate_subinterval( MethodInterval * m, bool_t produces_result ) {
    st_assert( not aborting(), "shouldn't generate when already aborting" );
    int            savedLen = exprStack()->length();
    MethodIterator mi( m, this );
    if ( aborting() ) {
        // the subinterval ended with dead code
        Expression * res = exprStack()->isEmpty() ? nullptr : exprStack()->top();
        st_assert( res == nullptr or res->isNoResultExpression(), "expected no result" );
        un_abort();      // abort will be done by caller (if needed)
        int diff = exprStack()->length() - savedLen;
        if ( produces_result )
            diff--;
        if ( diff > 0 ) {
            // adjust expression stack top pop extra stuff
            exprStack()->pop( diff );
        }
        if ( produces_result ) {
            if ( res == nullptr ) {
                // subinterval should have produced a result but didn't
                // This is legal: e.g., in "someCond or: [ otherCond ifTrue: [^ false ]. true]" the subinterval
                // of or: doesn't return a result if otherCond is known to be true.   So, just fix it here.
                // Urs 9/6/96
                res = new NoResultExpression();
                exprStack()->push( res, scope(), m->end_byteCodeIndex() );
            } else {
                exprStack()->assign_top( res );
            }
        }
    }
}


void NodeBuilder::constant_if_node( IfNode * node, ConstantExpression * condition ) {
    // if statement with constant condition
    Oop c                   = condition->constant();
    int resultByteCodeIndex = node->begin_byteCodeIndex();
    if ( c == trueObj or c == falseObj ) {
        if ( node->is_ifTrue() == ( c == trueObj ) ) {
            // then branch is always taken
            generate_subinterval( node->then_code(), node->produces_result() );
        } else if ( node->else_code() not_eq nullptr ) {
            // else branch always taken
            generate_subinterval( node->else_code(), node->produces_result() );
        } else {
            // branch is never executed -> ignore
        }
        if ( node->produces_result() ) {
            Expression * res = exprStack()->top();
            // materialize(res, nullptr);
            // NB: need not materialize block because it isn't merged with else branch
            // (BlockPseudoRegister will be materialized when assigned)
            scope()->setExprForByteCodeIndex( resultByteCodeIndex, res );      // for debugging info
            st_assert( not res->isNoResultExpression() or is_in_dead_code(), "no result should imply in_dead_code" );
            abortIfDead( res );                  // constant if ends dead, so method ends dead
        } else {
            if ( is_in_dead_code() )
                abort();          // constant if ends dead, so method ends dead
        }
    } else {
        // non-boolean condition -> fails always
        append( NodeFactory::UncommonNode( copyCurrentExprStack(), resultByteCodeIndex ) );
    }
}


TypeTestNode * NodeBuilder::makeTestNode( bool_t cond, PseudoRegister * r ) {
    GrowableArray <KlassOop> * list = new GrowableArray <KlassOop>( 2 );
    if ( cond ) {
        list->append( trueObj->klass() );
        list->append( falseObj->klass() );
    } else {
        list->append( falseObj->klass() );
        list->append( trueObj->klass() );
    }
    TypeTestNode * test = NodeFactory::TypeTestNode( r, list, true );
    test->append( 0, NodeFactory::UncommonNode( copyCurrentExprStack(), byteCodeIndex() ) );
    return test;
}


void NodeBuilder::if_node( IfNode * node ) {

    Expression * cond = exprStack()->pop();
    int resultByteCodeIndex = node->begin_byteCodeIndex();
    if ( abortIfDead( cond ) ) {
        if ( node->produces_result() )
            exprStack()->push( cond, scope(), resultByteCodeIndex );
        return;
    }

    if ( cond->isConstantExpression() ) {
        constant_if_node( node, ( ConstantExpression * ) cond );
    } else {
        // non-constant condition
        TypeTestNode * test = makeTestNode( node->is_ifTrue(), cond->preg() );
        append( test );
        if ( node->else_code() not_eq nullptr ) {
            // with else branch
            Expression                   * ifResult   = nullptr;
            Expression                   * elseResult = nullptr;
            SinglyAssignedPseudoRegister * resultReg  = new SinglyAssignedPseudoRegister( scope() );
            MergeNode                    * ifBranch   = NodeFactory::MergeNode( node->then_code()->begin_byteCodeIndex() );
            MergeNode                    * elseBranch = NodeFactory::MergeNode( node->else_code()->begin_byteCodeIndex() );
            MergeNode                    * endOfIf    = NodeFactory::MergeNode( node->end_byteCodeIndex() );
            test->append( 1, ifBranch );
            test->append( 2, elseBranch );
            splitMergeExpression( cond, test );
            // then branch
            setCurrent( ifBranch );
            generate_subinterval( node->then_code(), node->produces_result() );
            bool_t ifEndsDead = is_in_dead_code();
            if ( node->produces_result() ) {
                ifResult = exprStack()->pop();
                if ( not ifResult->isNoResultExpression() ) {
                    // NB: must materialize block because it is merged with result of else branch
                    // (was bug 7/4/96 -Urs)
                    materialize( ifResult->preg(), nullptr );
                    append( NodeFactory::AssignNode( ifResult->preg(), resultReg ) );
                } else {
                    st_assert( ifEndsDead, "oops" );
                }
            }
            append( endOfIf );
            // else branch
            setCurrent( elseBranch );
            generate_subinterval( node->else_code(), node->produces_result() );
            bool_t elseEndsDead = is_in_dead_code();
            if ( node->produces_result() ) {
                elseResult = exprStack()->pop();
                if ( not elseResult->isNoResultExpression() ) {
                    materialize( elseResult->preg(), nullptr );
                    append( NodeFactory::AssignNode( elseResult->preg(), resultReg ) );
                } else {
                    st_assert( elseEndsDead, "oops" );
                }
            }
            append( endOfIf );
            setCurrent( endOfIf );
            // end
            if ( ifEndsDead and elseEndsDead ) {
                abort();      // both branches end dead, so containing interval ends dead
            } else {
                if ( node->produces_result() ) {
                    exprStack()->push2nd( new MergeExpression( ifResult, elseResult, resultReg, current() ), scope(), resultByteCodeIndex );
                }
            }
        } else {
            // no else branch
            st_assert( not node->produces_result(), "inconsistency - else branch required" );
            MergeNode * ifBranch = NodeFactory::MergeNode( node->then_code()->begin_byteCodeIndex() );
            MergeNode * endOfIf  = NodeFactory::MergeNode( node->end_byteCodeIndex() );
            test->append( 1, ifBranch );
            test->append( 2, endOfIf );
            splitMergeExpression( cond, test );
            // then branch
            setCurrent( ifBranch );
            generate_subinterval( node->then_code(), false );
            append( endOfIf );
            setCurrent( endOfIf );
            if ( node->produces_result() ) {
                // materialize(exprStack()->top(), nullptr);
                // noo need to materialize result (see commment in constant_if_code)
                scope()->setExprForByteCodeIndex( resultByteCodeIndex, exprStack()->top() );      // for debugging info
            }
        }
        comment( "end of if" );
    }
}


void NodeBuilder::cond_node( CondNode * node ) {
    Expression * cond = exprStack()->pop();
    int resultByteCodeIndex = node->begin_byteCodeIndex();
    if ( abortIfDead( cond ) ) {
        exprStack()->push( cond, scope(), resultByteCodeIndex );
        return;        // condition ends dead, so rest of code does, too
    }
    if ( cond->isConstantExpression() ) {
        // constant condition
        Oop c = cond->asConstantExpression()->constant();
        if ( c == trueObj or c == falseObj ) {
            if ( node->is_and() and c == trueObj or node->is_or() and c == falseObj ) {
                generate_subinterval( node->expr_code(), true );
                // result of and:/or: is result of 2nd expression
                Expression * res = exprStack()->top();
                scope()->setExprForByteCodeIndex( resultByteCodeIndex, res );      // for debugging info
                if ( abortIfDead( res ) )
                    return;          // 2nd expr never returns, so rest is dead
            } else {
                // don't need to evaluate 2nd expression, result is cond
                exprStack()->push( cond, scope(), resultByteCodeIndex );
            }
        } else {
            // non-boolean condition -> fails always
            append( NodeFactory::UncommonNode( copyCurrentExprStack(), resultByteCodeIndex ) );
        }
    } else {
        // non-constant condition
        SinglyAssignedPseudoRegister * resultReg = new SinglyAssignedPseudoRegister( scope() );
        TypeTestNode                 * test      = makeTestNode( node->is_and(), cond->preg() );
        append( test );
        MergeNode * condExpression = NodeFactory::MergeNode( node->expr_code()->begin_byteCodeIndex() );
        MergeNode * otherwise      = NodeFactory::MergeNode( node->expr_code()->begin_byteCodeIndex() );
        MergeNode * endOfCond      = NodeFactory::MergeNode( node->end_byteCodeIndex() );
        test->append( 1, condExpression );
        test->append( 2, otherwise );
        setCurrent( otherwise );
        append( NodeFactory::AssignNode( cond->preg(), resultReg ) );
        append( endOfCond );
        splitMergeExpression( cond, test );        // split on result of first expression
        // evaluate second conditional expression
        setCurrent( condExpression );
        generate_subinterval( node->expr_code(), true );
        Expression * condResult = exprStack()->pop();
        if ( condResult->isNoResultExpression() ) {
            exprStack()->push2nd( new NoResultExpression, scope(), resultByteCodeIndex );    // dead code
            setCurrent( endOfCond );
        } else {
            append( NodeFactory::AssignNode( condResult->preg(), resultReg ) );
            append( endOfCond );
            setCurrent( endOfCond );
            // end
            // The result of the conditional is *one* branch of the first expression (cond) plus
            // the result of the second expression.  (Was splitting bug -- Urs 4/96)
            Expression * exclude = cond->findKlass( node->is_or() ? falseObj->klass() : trueObj->klass() );
            Expression * first;
            if ( exclude ) {
                first = cond->copyWithout( exclude );
            } else {
                first = cond;
            }
            exprStack()->push2nd( new MergeExpression( first, condResult, resultReg, current() ), scope(), resultByteCodeIndex );
            comment( "end of cond" );
        }
    }
}


void NodeBuilder::while_node( WhileNode * node ) {
    int loop_byteCodeIndex = node->body_code() not_eq nullptr ? node->body_code()->begin_byteCodeIndex() : node->expr_code()->begin_byteCodeIndex();
    CompiledLoop   * wloop  = _scope->addLoop();
    LoopHeaderNode * header = NodeFactory::LoopHeaderNode();
    append( header );
    wloop->set_startOfLoop( header );
    MergeNode * loop  = NodeFactory::MergeNode( loop_byteCodeIndex );
    MergeNode * exit  = NodeFactory::MergeNode( node->end_byteCodeIndex() );
    MergeNode * entry = nullptr;    // entry point into loop (start of condition)
    exit->_isLoopEnd = true;
    if ( node->body_code() not_eq nullptr ) {
        // set up entry point
        entry = NodeFactory::MergeNode( node->expr_code()->begin_byteCodeIndex() );
        append( entry );
        entry->_isLoopStart = true;
    } else {
        // no body code -> start with conditional expression
        append( loop );
        loop->_isLoopStart = true;
    }
    wloop->set_startOfCond( current() );
    generate_subinterval( node->expr_code(), true );
    Expression * cond = exprStack()->pop();
    if ( abortIfDead( cond ) ) {
        st_assert( false, "to allow a breakpoint" );
        return;      // condition ends dead --> loop is never executed
    }
    if ( false and cond->isConstantExpression() ) {
        wloop->set_endOfCond( current() );
        // ^^^ TURNED OFF - doesn't work for endless loops yet - FIX THIS
        // constant condition
        Oop c = cond->asConstantExpression()->constant();
        if ( c == trueObj or c == falseObj ) {
            if ( node->is_whileTrue() == ( c == trueObj ) ) {
                append( loop );
                setCurrent( exit );
            }
        } else {
            // non-boolean condition -> fails always
            append( NodeFactory::UncommonNode( copyCurrentExprStack(), byteCodeIndex() ) );
        }
    } else {
        // non-constant condition
        TypeTestNode * test = makeTestNode( node->is_whileTrue(), cond->preg() );
        append( test );
        wloop->set_endOfCond( test );
        test->append( 1, loop );
        test->append( 2, exit );
        setCurrent( exit );
        splitMergeExpression( cond, test );
        comment( "end of while" );
    }

    if ( node->body_code() not_eq nullptr ) {
        // generate loop body
        // NB: *must* be done after generating condition, because Node order must correspond to
        // byteCodeIndex order, otherwise copy propagation breaks  -Urs 10/95
        Node * curr = current();
        setCurrent( loop );
        generate_subinterval( node->body_code(), false );
        wloop->set_startOfBody( loop->next() );   // don't use loop (MergeNode) as start since it's been
        // created too early (before loop cond) --> node id range is off
        if ( theCompiler->is_uncommon_compile() ) {
            // Make sure the invocation counter is incremented at least once per iteration; otherwise uncommon
            // nativeMethods containing loops but no sends won't be recompiled early enough.
            append( NodeFactory::FixedCodeNode( FixedCodeNode::FixedCodeKind::inc_counter ) );
        }
        wloop->set_endOfBody( current() );
        append( entry );
        setCurrent( curr );
    }
    wloop->set_endOfLoop( exit );
}


void NodeBuilder::primitive_call_node( PrimitiveCallNode * node ) {
    if ( node->pdesc() == nullptr ) {
        error( "calling unknown primitive" );
        exprStack()->pop();
        return;
    }
    PrimitiveInliner * p = new PrimitiveInliner( this, node->pdesc(), node->failure_code() );
    p->generate();
}


void NodeBuilder::dll_call_node( DLLCallNode * node ) {
    GrowableArray <PseudoRegister *> * args = pass_arguments( nullptr, node->nofArgs() );
    st_assert( node->failure_code() == nullptr, "compiler cannot handle DLL calls with failure blocks yet" );
    DLLNode * dcall = NodeFactory::DLLNode( node->dll_name(), node->function_name(), node->function(), node->async(), scope()->nlrTestPoint(), args, copyCurrentExprStack() );
    append( dcall );
    exprStack()->pop( node->nofArgs() );
    // a proxy object has been pushed before the arguments, assign result
    // to its _pointer field - the proxy is also the result of the dll call
    // (since the result is NOT a Delta pointer, no store check is needed)
    append( NodeFactory::StoreOffsetNode( dcall->dest(), exprStack()->top()->preg(), pointer_no, false ) );
}


void NodeBuilder::allocate_temporaries( int nofTemps ) {
    MethodOop m = _scope->method();
    st_assert( 1 + nofTemps == m->number_of_stack_temporaries(), "no. of stack variables inconsistent" );
    // temporaries are allocated in the beginning (InlinedScope::createTemporaries)
}


void NodeBuilder::push_self() {
    exprStack()->push( scope()->self(), scope(), scope()->byteCodeIndex() );
}


void NodeBuilder::push_tos() {
    exprStack()->push( exprStack()->top(), scope(), scope()->byteCodeIndex() );
}


void NodeBuilder::push_literal( Oop obj ) {
    exprStack()->push( new ConstantExpression( obj, new_ConstPReg( _scope, obj ), nullptr ), scope(), scope()->byteCodeIndex() );
}


void NodeBuilder::push_argument( int no ) {
    // arguments are non-assignable, so no assignment to SinglyAssignedPseudoRegister necessary
    st_assert( ( 0 <= no ) and ( no < _scope->nofArguments() ), "illegal argument no" );
    exprStack()->push( scope()->argument( no ), scope(), scope()->byteCodeIndex() );
}


void NodeBuilder::push_temporary( int no ) {
    // must assign non-parameters to temporary because exprStack entries must be singly-assigned
    // and because source (i.e., temporary location) could be assigned between now and the send
    // that actually consumes the value
    st_assert( ( 0 <= no ) and ( no < _scope->nofTemporaries() ), "illegal temporary no" );
    Expression                   * temp = _scope->temporary( no );
    PseudoRegister               * src  = temp->preg();
    SinglyAssignedPseudoRegister * dst  = new SinglyAssignedPseudoRegister( _scope );
    append( NodeFactory::AssignNode( src, dst ) );
    exprStack()->push( temp, scope(), scope()->byteCodeIndex() );
}


void NodeBuilder::access_temporary( int no, int context, bool_t push ) {
    // generates code to access temporary no in (logical, i.e., interpreter) context
    // context numbering starts a 0
    st_assert( _scope->allocatesInterpretedContext() == ( _scope->contextInitializer() not_eq nullptr ), "context must exist already if used (find_scope)" );
    st_assert( context >= 0, "context must be >= 0" );
    int nofIndirections;
    OutlinedScope * out = nullptr;
    InlinedScope  * s   = _scope->find_scope( context, nofIndirections, out );
    if ( nofIndirections < 0 ) {
        // the accessed variable is in the same stack frame, in scope s
        if ( push ) {
            Expression     * contextTemp = s->contextTemporary( no );
            PseudoRegister * src         = contextTemp->preg();
            Expression     * res;
            if ( src->isBlockPseudoRegister() ) {
                // don't copy around blocks; see ContextInitializeNode et al.
                res = contextTemp;
            } else {
                SinglyAssignedPseudoRegister * dst = new SinglyAssignedPseudoRegister( _scope );
                append( NodeFactory::AssignNode( src, dst ) );
                res = s->contextTemporary( no )->shallowCopy( dst, _current );
            }
            exprStack()->push( res, scope(), scope()->byteCodeIndex() );
        } else {
            // store
            PseudoRegister * src = exprStack()->top()->preg();
            materialize( src, nullptr );
            PseudoRegister * dst = s->contextTemporary( no )->preg();
            append( NodeFactory::AssignNode( src, dst ) );
        }
    } else {
        // the accessed variable is in higher stack frames
        st_assert( out, "must be set" );
        st_assert( out->scope()->allocates_compiled_context(), "must allocate context" );
        NameDescriptor * nd = out->scope()->contextTemporary( no );
        Location loc = nd->location();    // location of temp in compiled context
        st_assert( loc.isContextLocation(), "must be in context" );
        int tempNo = loc.tempNo();        // compiled offset
        if ( tempNo not_eq no ) {
            compiler_warning( "first time this happens: compiled context offset not_eq interpreted context offset" );
        }
        if ( push ) {
            SinglyAssignedPseudoRegister * dst = new SinglyAssignedPseudoRegister( _scope );
            append( NodeFactory::LoadUplevelNode( dst, s->context(), nofIndirections, ContextOopDescriptor::temp0_word_offset() + tempNo, nullptr ) );
            exprStack()->push( new UnknownExpression( dst, _current ), scope(), scope()->byteCodeIndex() );
        } else {
            // store
            Expression     * srcExpression = exprStack()->top();
            PseudoRegister * src           = srcExpression->preg();
            materialize( src, nullptr );
            append( NodeFactory::StoreUplevelNode( src, s->context(), nofIndirections, ContextOopDescriptor::temp0_word_offset() + tempNo, nullptr, srcExpression->needsStoreCheck() ) );
        }
    }
}


void NodeBuilder::push_temporary( int no, int context ) {
    access_temporary( no, context, true );
}


void NodeBuilder::push_instVar( int offset ) {
    st_assert( offset >= 0, "offset must be positive" );
    SinglyAssignedPseudoRegister * dst  = new SinglyAssignedPseudoRegister( _scope );
    PseudoRegister               * base = _scope->self()->preg();
    append( NodeFactory::LoadOffsetNode( dst, base, offset, false ) );
    exprStack()->push( new UnknownExpression( dst, _current ), scope(), scope()->byteCodeIndex() );
}


void NodeBuilder::push_global( AssociationOop associationObj ) {
    SinglyAssignedPseudoRegister * dst = new SinglyAssignedPseudoRegister( _scope );
    if ( associationObj->is_constant() ) {
        // constant association -- can inline the constant
        Oop c = associationObj->value();
        ConstPseudoRegister * r = new_ConstPReg( _scope, c );
        exprStack()->push( new ConstantExpression( c, r, nullptr ), _scope, scope()->byteCodeIndex() );
    } else {
        // Removed by Lars Bak, 4-22-96
        // if (associationObj->value()->is_klass())
        //    compiler_warning("potential performance bug: non-constant association containing klassOop");
        ConstPseudoRegister * base = new_ConstPReg( _scope, associationObj );
        append( NodeFactory::LoadOffsetNode( dst, base, AssociationOopDescriptor::value_offset(), false ) );
        exprStack()->push( new UnknownExpression( dst, _current ), _scope, scope()->byteCodeIndex() );
    }
}


void NodeBuilder::store_temporary( int no ) {
    PseudoRegister * src = exprStack()->top()->preg();
    // should check here whether src is memoized block pushed in preceding byte code;
    // if so, un-memoize it  -- fix this
    materialize( src, nullptr );
    PseudoRegister * dst = _scope->temporary( no )->preg();
    if ( src not_eq dst )
        append( NodeFactory::AssignNode( src, dst ) );
}


void NodeBuilder::store_temporary( int no, int context ) {
    access_temporary( no, context, false );
}


void NodeBuilder::store_instVar( int offset ) {
    st_assert( offset >= 0, "offset must be positive" );
    Expression     * srcExpression = exprStack()->top();
    PseudoRegister * src           = srcExpression->preg();
    materialize( src, nullptr );
    PseudoRegister * base = _scope->self()->preg();
    append( NodeFactory::StoreOffsetNode( src, base, offset, srcExpression->needsStoreCheck() ) );
}


void NodeBuilder::store_global( AssociationOop associationObj ) {
    Expression     * srcExpression = exprStack()->top();
    PseudoRegister * src           = srcExpression->preg();
    materialize( src, nullptr );
    ConstPseudoRegister * base = new_ConstPReg( _scope, associationObj );
    append( NodeFactory::StoreOffsetNode( src, base, AssociationOopDescriptor::value_offset(), srcExpression->needsStoreCheck() ) );
}


void NodeBuilder::pop() {
    exprStack()->pop();
}


// always call materialize before storing or passing a run-time value that could be a block
void NodeBuilder::materialize( PseudoRegister * r, GrowableArray <BlockPseudoRegister *> * materialized ) {

    if ( r == nullptr )
        return;
    if ( materialized == nullptr )
        return;

    bool_t isBlockPseudoRegister = r->isBlockPseudoRegister();
    bool_t contains    = materialized->contains( ( BlockPseudoRegister * ) r );

    // make sure the block (and all parent blocks / uplevel-accessed blocks) exist
    // materialized is a list of blocks already materialized (nullptr if none)
    if ( r->isBlockPseudoRegister() and ( materialized == nullptr or not materialized->contains( ( BlockPseudoRegister * ) r ) ) ) {
        BlockPseudoRegister * blk = ( BlockPseudoRegister * ) r;
        append( NodeFactory::BlockMaterializeNode( blk, copyCurrentExprStack() ) );
        if ( materialized == nullptr )
            materialized = new GrowableArray <BlockPseudoRegister *>( 5 );
        materialized->append( blk );
        GrowableArray <PseudoRegister *> * reads = blk->uplevelRead();
        if ( reads ) {
            for ( int i = reads->length() - 1; i >= 0; i-- )
                materialize( reads->at( i ), materialized );
        }
    }
}


GrowableArray <PseudoRegister *> * NodeBuilder::pass_arguments( PseudoRegister * receiver, int nofArgs ) {
    // Generate code for argument passing (move all args into the right locations).
    // If the arguments are passed on the stack (including the receiver) they should be assigned in the order of textual appearance.
    // If the receiver is passed in a register that should happen at the end to allow this register to be used as int32_t as possible.
    int                              nofFormals = ( receiver == nullptr ) ? nofArgs : nofArgs + 1;
    GrowableArray <PseudoRegister *> * formals  = new GrowableArray <PseudoRegister *>( nofFormals );

    // setup formal receiver and pass if passed on the stack
    SinglyAssignedPseudoRegister * formalReceiver;
    GrowableArray <BlockPseudoRegister *> blocks( 32 );
    if ( receiver not_eq nullptr ) {
        materialize( receiver, &blocks );
        formalReceiver = new SinglyAssignedPseudoRegister( _scope, receiverLoc, false, false, byteCodeIndex(), byteCodeIndex() );
        formals->append( formalReceiver );
        if ( receiverLoc.isStackLocation() ) {
            // receiver is passed on stack, must happen before the arguments are passed
            append( NodeFactory::AssignNode( receiver, formalReceiver ) );
        }
    }

    // argument range
    const int first_arg = exprStack()->length() - nofArgs;
    const int limit_arg = exprStack()->length();
    int       sp;

    // materialize blocks
    for ( sp = first_arg; sp < limit_arg; sp++ ) {
        ExpressionStack * temp1 = exprStack();
        Expression      * temp2 = temp1->at( sp );
        PseudoRegister  * pReg  = temp2->preg();

        PseudoRegister * actual = exprStack()->at( sp )->preg();
        materialize( actual, &blocks );
    }

    // pass arguments
    for ( sp = first_arg; sp < limit_arg; sp++ ) {
        PseudoRegister               * actual = exprStack()->at( sp )->preg();
        SinglyAssignedPseudoRegister * formal = new SinglyAssignedPseudoRegister( _scope, Mapping::outgoingArg( sp - first_arg, nofArgs ), false, false, byteCodeIndex(), byteCodeIndex() );
        formals->append( formal );
        append( NodeFactory::AssignNode( actual, formal ) );
    }

    // pass receiver if not passed already
    if ( ( receiver not_eq nullptr ) and not receiverLoc.isStackLocation() ) {
        append( NodeFactory::AssignNode( receiver, formalReceiver ) );
    }
    return formals;
}


void NodeBuilder::gen_normal_send( SendInfo * info, int nofArgs, SinglyAssignedPseudoRegister * result ) {
    GrowableArray <PseudoRegister *> * args = pass_arguments( exprStack()->at( exprStack()->length() - nofArgs - 1 )->preg(), nofArgs );
    SendNode * send = NodeFactory::SendNode( info->_lookupKey, scope()->nlrTestPoint(), args, copyCurrentExprStack(), false, info );
    append( send );
    append( NodeFactory::AssignNode( send->dest(), result ) );
}


void NodeBuilder::gen_self_send( SendInfo * info, int nofArgs, SinglyAssignedPseudoRegister * result ) {
    GrowableArray <PseudoRegister *> * args = pass_arguments( _scope->self()->preg(), nofArgs );
    SendNode * send = NodeFactory::SendNode( info->_lookupKey, scope()->nlrTestPoint(), args, copyCurrentExprStack(), false, info );
    append( send );
    append( NodeFactory::AssignNode( send->dest(), result ) );
}


void NodeBuilder::gen_super_send( SendInfo * info, int nofArgs, SinglyAssignedPseudoRegister * result ) {
    GrowableArray <PseudoRegister *> * args = pass_arguments( _scope->self()->preg(), nofArgs );
    SendNode * send = NodeFactory::SendNode( info->_lookupKey, scope()->nlrTestPoint(), args, copyCurrentExprStack(), true, info );
    append( send );
    append( NodeFactory::AssignNode( send->dest(), result ) );
}


void NodeBuilder::normal_send( InterpretedInlineCache * ic ) {
    int nofArgs = ic->selector()->number_of_arguments();
    LookupKey  * key      = LookupKey::allocate( nullptr, ic->selector() );
    Expression * receiver = exprStack()->at( exprStack()->length() - nofArgs - 1 );
    SendInfo   * info     = new SendInfo( _scope, key, receiver );
    Expression * result   = _inliner->inlineNormalSend( info );
    exprStack()->pop( nofArgs + 1 ); // pop arguments & receiver
    exprStack()->push( result, _scope, scope()->byteCodeIndex() );
    abortIfDead( result );
}


void NodeBuilder::self_send( InterpretedInlineCache * ic ) {
    int nofArgs = ic->selector()->number_of_arguments();
    LookupKey  * key    = LookupKey::allocate( _scope->selfKlass(), ic->selector() );
    SendInfo   * info   = new SendInfo( _scope, key, _scope->self() );
    Expression * result = _inliner->inlineSelfSend( info );
    exprStack()->pop( nofArgs );    // receiver has not been pushed
    exprStack()->push( result, _scope, scope()->byteCodeIndex() );
    abortIfDead( result );
}


void NodeBuilder::super_send( InterpretedInlineCache * ic ) {
    int      nofArgs = ic->selector()->number_of_arguments();
    //LookupKey* key = ic->lookupKey(0);
    KlassOop klass   = _scope->selfKlass()->klass_part()->superKlass();
    LookupKey  * key    = LookupKey::allocate( klass, LookupCache::method_lookup( klass, ic->selector() ) );
    SendInfo   * info   = new SendInfo( _scope, key, _scope->self() );
    Expression * result = _inliner->inlineSuperSend( info );
    exprStack()->pop( nofArgs );    // receiver has not been pushed
    exprStack()->push( result, _scope, scope()->byteCodeIndex() );
    abortIfDead( result );
}


void NodeBuilder::double_equal() {
    PrimitiveInliner * p = new PrimitiveInliner( this, Primitives::equal(), nullptr );
    p->generate();
}


void NodeBuilder::double_not_equal() {
    PrimitiveInliner * p = new PrimitiveInliner( this, Primitives::not_equal(), nullptr );
    p->generate();
}


void NodeBuilder::method_return( int nofArgs ) {
    // assign result & return
    Expression * result = exprStack()->pop();
    if ( _current == EndOfCode ) {
        // scope ends with dead code, i.e., there's no method return
        // (e.g., because there is a preceding NonLocalReturn)
        _scope->addResult( new NoResultExpression );
        return;
    }

    if ( result->isNoResultExpression() ) {
        // scope will never return normally (i.e., is always left via a NonLocalReturn)
        _scope->addResult( result );    // make sure scope->result not_eq nullptr
    } else {
        // return TOS
        PseudoRegister * src = result->preg();
        materialize( src, nullptr );
        if ( _scope->isTop() ) {
            // real return from NativeMethod
            append( NodeFactory::AssignNode( src, _scope->resultPR ) );
            _scope->addResult( result->shallowCopy( _scope->resultPR, _current ) );
        } else {
            // inlined return
            if ( UseNewBackend ) {
                // The new backend doesn't support InlinedReturnNodes any more but has
                // an explicit ContextZapNode that is used together with an AssignNode.
                //
                // Note: We don't know if context zapping is really needed before the block optimizations, therefore we introduce it eagerly if the scope seems to need it.
                // However, the backend will check again if it is still necessary (the optimizations can only remove the need, not create it).

                // Turned off for now - problem with ScopeDescriptor - FIX THIS
                // if (_scope->needsContextZapping()) append(NodeFactory::ContextZapNode(_scope->context()));

                append( NodeFactory::AssignNode( src, _scope->resultPR ) );
            } else {
                if ( _scope->needsContextZapping() ) {
                    // keep inlined return node (no context zap node yet)
                    append( NodeFactory::InlinedReturnNode( byteCodeIndex(), src, _scope->resultPR ) );
                } else {
                    // only assignment required
                    append( NodeFactory::AssignNode( src, _scope->resultPR ) );
                }
            }
            _scope->addResult( result->shallowCopy( _scope->resultPR, _current ) );
        }
    }

    append( scope()->returnPoint() );    // connect to return code
    // The byte code compiler might generate a pushNil to adjust the stack;
    // make sure that code is discarded.
    _current = EndOfCode;
}


void NodeBuilder::nonlocal_return( int nofArgs ) {
    // assign result & return
    Expression     * resultExpression = exprStack()->pop();
    PseudoRegister * src              = resultExpression->preg();
    materialize( src, nullptr );
    if ( scope()->isTop() ) {
        // real NonLocalReturn from NativeMethod
        append( NodeFactory::AssignNode( src, scope()->resultPR ) );
        append( scope()->nlrPoint() );
    } else {
        // NonLocalReturn from inlined scope
        Scope * home = scope()->home();
        if ( home->isInlinedScope() ) {
            // the NonLocalReturn target is in the same NativeMethod
            InlinedScope * h = ( InlinedScope * ) home;
            append( NodeFactory::AssignNode( src, h->resultPR ) );
            h->addResult( resultExpression->shallowCopy( h->resultPR, _current ) );
            // jump to home method's return point
            // BUG: should zap contexts along the way -- fix this
            append( h->returnPoint() );
        } else {
            // NonLocalReturn target is in a different NativeMethod -- unwind all sender frames and perform a real NonLocalReturn
            // BUG: should zap contexts along the way -- fix this

            // now assign to result register and jump to NonLocalReturn setup code to set up remaining NonLocalReturn regs
            // NB: each scope needs its own setup node because the home fp/id is different
            int    endByteCodeIndex = scope()->nlrPoint()->byteCodeIndex();
            bool_t haveSetupNode    = scope()->nlrPoint()->next() not_eq nullptr;
            st_assert( not haveSetupNode or scope()->nlrPoint()->next()->isNonLocalReturnSetupNode(), "expected setup node" );
            PseudoRegister * res = haveSetupNode ? ( ( NonTrivialNode * ) scope()->nlrPoint()->next() )->src() : new SinglyAssignedPseudoRegister( scope(), NonLocalReturnResultLoc, true, true, byteCodeIndex(), endByteCodeIndex );
            append( NodeFactory::AssignNode( src, res ) );
            append( scope()->nlrPoint() );
            if ( not haveSetupNode ) {
                // lazily create setup code
                append_exit( NodeFactory::NonLocalReturnSetupNode( res, endByteCodeIndex ) );
            }
        }
    }
    // (the byte code compiler might generate a pushNil to adjust the stack)
    _current = EndOfCode;
}


GrowableArray <NonTrivialNode *> * NodeBuilder::nodesBetween( Node * from, Node * to ) {
    // collect assignments along the path from --> to (for splitting)
    GrowableArray <NonTrivialNode *> * nodes = new GrowableArray <NonTrivialNode *>( 5 );
    Node * n = from;
    while ( n not_eq to ) {
        if ( not n->hasSingleSuccessor() )
            return nullptr;   // can't copy both paths
        bool_t shouldCopy = n->shouldCopyWhenSplitting();
        bool_t ok         = ( n == from ) or shouldCopy or n->isTrivial() or n->isMergeNode();
        if ( not ok )
            return nullptr;                  // can't copy this node
        if ( shouldCopy and n not_eq from )
            nodes->append( ( NonTrivialNode * ) n );
        n = n->next();
    }
    return nodes;
}


MergeNode * NodeBuilder::insertMergeBefore( Node * n ) {
    // insert a MergeNode before n
    st_assert( n->hasSinglePredecessor(), "should have only one predecessor" );
    MergeNode * merge = NodeFactory::MergeNode( n->byteCodeIndex() );
    Node      * prev  = n->firstPrev();
    prev->moveNext( n, merge );
    merge->setPrev( prev );
    n->removePrev( prev );
    merge->append( n );
    return merge;
}


static int split_count = 0; // for conditional breakpoints (debugging)

void NodeBuilder::splitMergeExpression( Expression * expr, TypeTestNode * test ) {
    // Split MergeExpressions (currently works only for booleans).
    // expr is the expression (generated somewhere upstream), test is the (only) node
    // testing this expression.  If there's nothing "complicated" inbetween, split the
    // merge to avoid the type test.
    split_count++;
    GrowableArray <Expression *> * exprsToSplit = splittablePaths( expr, test );
    if ( not exprsToSplit )
        return;

    for ( int i = exprsToSplit->length() - 1; i >= 0; i-- ) {
        Expression * e     = exprsToSplit->at( i );
        Node       * start = e->node();
        GrowableArray <NonTrivialNode *> * nodesToCopy = nodesBetween( start, test );
        st_assert( nodesToCopy not_eq nullptr, "should have worked" );

        // find corresponding 'to' node in type test
        KlassOop c = e->asConstantExpression()->klass();
        int      j = test->classes()->length();
        while ( j-- > 0 and test->classes()->at( j ) not_eq c );
        st_assert( j >= 0, "didn't find klass in type test" );
        Node * to = test->next( j + 1 );    // +1 because next(i) is branch for class i-1
        if ( CompilerDebug )
            cout( PrintSplitting )->print( "%*s*splitting merge expr: from N%d to N%d (split #%d)\n", _scope->depth * 2, "", start->id(), to->id(), split_count );
        if ( not to->isMergeNode() ) {
            to = insertMergeBefore( to );    // insert a MergeNode inbetween
        }

        // disconnect defining node from its current successor
        start->removeNext( start->next() );
        // append copies of all assignments
        Node * current = start;
        bool_t found = nodesToCopy->length() == 0;    // hard to test if no nodes to copy, so assume it's ok

        for ( int i = 0; i < nodesToCopy->length(); i++ ) {
            NonTrivialNode * orig = nodesToCopy->at( i );
            Node           * copy = orig->copy( nullptr, nullptr );
            if ( CompilerDebug )
                cout( PrintSplitting )->print( "%*s*  (copying node N%d along the path -> N%d)\n", _scope->depth * 2, "", orig->id(), copy->id() );
            current   = current->append( copy );
            if ( orig->dest() == test->src() )
                found = true;
        }

        if ( not found ) {
            // no assignment to tested PseudoRegister found along path; must be earlier
            // check only leftmost path (must be assigned along any path)
            for ( Node * n = start; n and not found; n = n->firstPrev() ) {
                if ( n->hasDest() and ( ( NonTrivialNode * ) n )->dest() == test->src() ) {
                    found = true;
                }
            }
            st_assert( found, "assignment to tested PseudoRegister not found" );
        }

        // connect to 'to' node
        current->append( to );
    }
}


GrowableArray <Expression *> * NodeBuilder::splittablePaths( const Expression * expr, const TypeTestNode * test ) const {
    // test if expr can be split
    if ( not Splitting )
        return nullptr;
    if ( not expr->isMergeExpression() )
        return nullptr;
    MergeExpression * m = expr->asMergeExpression();
    if ( not m->isSplittable() )
        return nullptr;

    GrowableArray <Node *>       * exprNodes = new GrowableArray <Node *>( 10 ); // start nodes of all paths
    GrowableArray <Expression *> * okExprs   = new GrowableArray <Expression *>( 10 );  // those who are splittable

    // collect all paths that look splittable
    int i = m->exprs->length() - 1;
    for ( ; i >= 0; i-- ) {
        for ( Expression * x = m->exprs->at( i ); x; x = x->next ) {
            Node * start = x->node();
            exprNodes->append( start );
            if ( not x->isConstantExpression() )
                continue;
            // check if there is a 'simple' path between defining node and type test node -> split if possible
            const Node * n = start;
            while ( n not_eq test ) {
                if ( not n->hasSingleSuccessor() )
                    goto nextExpression;   // can't copy both paths
                bool_t ok = ( n == start ) or n->isTrivial() or n->isAssignNode() or n->isMergeNode();
                if ( not ok )
                    goto nextExpression;                  // can't copy this node
                n = n->next();
            }
            // ok, the path from this node is splittable
            okExprs->append( x );
        }
nextExpression:;
    }

    // check that no exprNode is along the path from any other exprNode to the test node
    // this should be #ifdef ASSERT but for now always check to make sure there are no lurking bugs  -Urs 4/27/96
    for ( int i = okExprs->length() - 1; i >= 0; i-- ) {
        Node       * start = okExprs->at( i )->node()->next();
        for ( Node * n     = start; n not_eq ( Node * ) test; n = n->next() ) {
            if ( exprNodes->contains( n ) ) {
                lprintf( "error in splittable boolean expression:\n" );
                m->print();
                okExprs->at( i )->print();
                printNodes( okExprs->at( i )->node() );
                for ( int j = 0; j < exprNodes->length(); j++ ) {
                    exprNodes->at( j )->print();
                    lprintf( "\n" );
                }
                st_fatal( "compiler error" );
            }
        }
    }

    return okExprs;
}


void NodeBuilder::allocate_closure( AllocationType type, int nofArgs, MethodOop method ) {
    PseudoRegister * context;
    if ( type == tos_as_scope ) {
        context = exprStack()->pop()->preg();
    } else {
        context = _scope->context();
    }
    CompileTimeClosure  * closure = new CompileTimeClosure( _scope, method, context, nofArgs );
    BlockPseudoRegister * block   = new BlockPseudoRegister( _scope, closure, byteCodeIndex(), EpilogueByteCodeIndex );
    append( NodeFactory::BlockCreateNode( block, copyCurrentExprStack() ) );
    exprStack()->push( new BlockExpression( block, _current ), _scope, scope()->byteCodeIndex() );
}


static MethodOopDescriptor::Block_Info incoming_info( MethodOop m ) {
    // this function is here for compatibility reasons only
    // should go away at some point: block_info replaced
    // incoming_info, but is only defined for blocks
    if ( m->is_blockMethod() ) {
        return m->block_info();
    } else {
        return MethodOopDescriptor::expects_self;
    }
}


void NodeBuilder::allocate_context( int nofTemps, bool_t forMethod ) {
    _scope->createContextTemporaries( nofTemps );
    st_assert( not scope()->contextInitializer(), "should not already have a contextInitializer" );
    PseudoRegister * parent;    // previous context in the context chain
    if ( forMethod ) {
        // method, not block context points to current stack frame
        parent = new SinglyAssignedPseudoRegister( _scope, frameLoc, true, true, PrologueByteCodeIndex, EpilogueByteCodeIndex );
    } else {
        // context points to previous (incoming) context, if there
        st_assert( incoming_info( _scope->method() ) not_eq MethodOopDescriptor::expects_self, "fix this" );
        st_assert( incoming_info( _scope->method() ) not_eq MethodOopDescriptor::expects_parameter, "fix this" );
        if ( _scope->isTop() ) {

            // incoming context is passed in self's location (interpreter invariant); fix this to use different PseudoRegister
            // parent should never be used, set to 0 for debugging (note: the interpreter sets parent always to self)
            if ( incoming_info( _scope->method() ) == MethodOopDescriptor::expects_context ) {
                parent = _scope->self()->preg();
            } else {
                parent = nullptr;
            }

        } else {
            st_assert( _scope->isBlockScope(), "must be a block scope" );
            // create new context PseudoRegister (_scope->context() is the context passed in by the caller)
            parent = _scope->context();
            _scope->setContext( new SinglyAssignedPseudoRegister( _scope, PrologueByteCodeIndex, EpilogueByteCodeIndex ) );
        }
    }
    ContextCreateNode * creator = NodeFactory::ContextCreateNode( parent, _scope->context(), nofTemps, copyCurrentExprStack() );
    append( creator );
    // append context initializer and initialize with nil
    scope()->set_contextInitializer( NodeFactory::ContextInitNode( creator ) );
    append( scope()->contextInitializer() );
    ConstantExpression * nil = new ConstantExpression( nilObj, new_ConstPReg( _scope, nilObj ), nullptr );
    for ( int i = 0; i < nofTemps; i++ )
        scope()->contextInitializer()->initialize( i, nil );
}


void NodeBuilder::removeContextCreation() {
    st_assert( scope()->contextInitializer() not_eq nullptr, "must have context" );
    ContextCreateNode * creator = scope()->contextInitializer()->creator();
    creator->eliminate( creator->bb(), nullptr, true, false );        // delete creator node
    scope()->contextInitializer()->notifyNoContext();        // let initializer know about it
}


void NodeBuilder::set_self_via_context() {
    if ( Inline ) {
        Scope * s = _scope->parent();
        if ( s->isInlinedScope() ) {
            // self doesn't need to be set -- can directly use the Expression of the enclosing scope
            st_assert( _scope->self() == ( ( InlinedScope * ) s )->self(), "should have identical selves" );
            return;
        }
    }

    // prevent multiple initializations (see BlockScope::initializeSelf)
    if ( _scope->is_self_initialized() )
        return;
    _scope->set_self_initialized();

    // must load self at runtime; first compute home context no
    int contextNo = _scope->homeContext();
    if ( _scope->allocatesInterpretedContext() ) {
        // correct contextNo: the set_self_via_context works on the incoming context
        // -> subtract 1 (homeContext() already counts the context allocated in this
        // method (if there is one)
        st_assert( _scope->contextInitializer() not_eq nullptr, "should have a _contextInitializer already" );
        contextNo--;
    }

    // do uplevel access and store into self
    // Note: this should use the same mechanism as access_temporary(...). However,
    // access_temporary relies on the fact that a possible local context is already
    // allocated. Thus, for the time being, explicitly generate
    // the uplevel access node. Note: the incoming context is in the recv location!
    const int self_no = 0; // self is always the first entry in the top context, if there
    PseudoRegister * reg = _scope->self()->preg();
    append( NodeFactory::LoadUplevelNode( reg, reg, contextNo, ContextOopDescriptor::temp0_word_offset() + self_no, nullptr ) );
}


Expression * NodeBuilder::copy_into_context( Expression * e, int no ) {
    if ( e->isBlockExpression() ) {
        // A block must be stored into a context (e.g. it's passed in as an arg and the arg
        // is uplevel-accessed).
        // Must materialize it here to make sure it's created; this BlockMaterializeNode should
        // be deleted if the context is eliminated, so register it with the context.
        BlockPseudoRegister  * block = ( BlockPseudoRegister * ) e->preg();
        BlockMaterializeNode * n     = NodeFactory::BlockMaterializeNode( block, copyCurrentExprStack() );
        scope()->contextInitializer()->addBlockMaterializer( n );
        append( n );
        // Remember the context location so we can later retrieve the corresponding context.
        // (We're just using a Location as a convenient way to denote a particular location;
        // this doesn't have anything to do with register / context allocation per se, so don't
        // use the location for anything else.)
        Location * loc = Mapping::new_contextTemporary( no, no, scope()->scopeID() );
        block->addContextCopy( loc );
        // don't copy the block so it can still be inlined (handled specially in context initializer if
        // context is created)
        return e;
    } else {
        return e->shallowCopy( new SinglyAssignedPseudoRegister( scope(), PrologueByteCodeIndex, EpilogueByteCodeIndex, true ), nullptr );
    }
}


void NodeBuilder::copy_self_into_context() {
    const int self_no = 0; // self is always the first temporary in a context, if there
    // caution: must create new expr/preg for self in context because the two locations must be different
    Expression * self_expr_in_context = copy_into_context( scope()->self(), self_no );
    scope()->contextTemporariesAtPut( self_no, self_expr_in_context );
    scope()->contextInitializer()->initialize( self_no, scope()->self() );
}


void NodeBuilder::copy_argument_into_context( int argNo, int no ) {
    // caution: must create new expr/preg for arg in context because the two locations must be different
    // i.e., arg is on stack, arg in context may be on heap
    Expression * arg_expr_in_context = copy_into_context( scope()->argument( argNo ), no );
    scope()->contextTemporariesAtPut( no, arg_expr_in_context );
    scope()->contextInitializer()->initialize( no, scope()->argument( argNo ) );
}


void NodeBuilder::zap_scope() {
    st_assert( scope()->isMethodScope(), "blocks cannot be target of a NonLocalReturn, no zap required" );
    st_assert( scope()->contextInitializer() not_eq nullptr, "should have a context allocated" );
    // no explicit node generated
}


void NodeBuilder::predict_primitive_call( PrimitiveDescriptor * pdesc, int failure_start ) {
    // ignored
}


PseudoRegister * NodeBuilder::float_at( int fno ) {
    if ( UseFPUStack ) {
        if ( fno < scope()->nofFloatTemporaries() ) {
            // fno refers to a float temporary
            return scope()->floatTemporary( fno )->preg();
        } else {
            // fno refers to a float on the float expression stack,
            // it must refer to the top since code is generated for
            // a stack machine
            return new SinglyAssignedPseudoRegister( _scope, topOfFloatStack, true, true, byteCodeIndex(), byteCodeIndex() );
        }
    } else {
        // intermediate expressions are treated as temporaries
        return scope()->floatTemporary( fno )->preg();
    }
}


void NodeBuilder::float_allocate( int nofFloatTemps, int nofFloatExprs ) {
    // nofFloats 64bit floats are allocated and initialized to NaN
    // in the following set of operations float(fno) (fno = float no.)
    // refers to one of these floats within the range [0..nofFloats[.
    //
    // Example code for a method containing the following code
    // (strictly using the stack-machine paradigm)
    //
    // | a <not Float>			; mapped to 0
    //   b <not Float>			; mapped to 1
    //   c <not Float>			; mapped to 2
    // |
    //
    // a := 3.0.
    // b := 4.0.
    // c := a + b.
    // ^ c
    //
    // float_allocate     5		; two more for expression stack temporaries (mapped to 3 & 4)
    // float_set          3, 3.0		; a := 3.0
    // float_move         0, 3
    // float_set          3, 4.0		; b := 4.0
    // float_move         1, 3
    // float_move         3, 0		; push a
    // float_move         4, 1		; push b
    // float_binaryOp     3, add		; add
    // float_move         2, 3		; store to c
    // float_move         3, 2		; push c
    // float_unaryOpToOop 3, oopify	; push c converted to Oop
    // return_tos				; return tos
    int size = nofFloatTemps + nofFloatExprs;
    st_assert( size == method()->total_number_of_floats(), "inconsistency" );
    // floatTemporaries are allocated in InlinedScope::genCode()
}


void NodeBuilder::float_floatify( Floats::Function f, int fno ) {
    // top of stack must be a boxed float, it is unboxed and stored at float(fno).
    Expression * t = _expressionStack->pop();
    if ( t->hasKlass() and t->klass() == doubleKlassObj ) {
        // no type test needed
    } else {
        // put in a type test - fix this!
    }
    append( NodeFactory::FloatUnaryArithNode( float_at( fno ), t->preg(), ArithOpCode::f2FloatArithOp ) );
}


void NodeBuilder::float_move( int to, int from ) {
    // float(to) := float(from)
    append( NodeFactory::AssignNode( float_at( from ), float_at( to ) ) );
}


void NodeBuilder::float_set( int to, DoubleOop value ) {
    // float(to) := value
    ConstPseudoRegister * val = new_ConstPReg( _scope, value );
    append( NodeFactory::AssignNode( val, float_at( to ) ) );
}


void NodeBuilder::float_nullary( Floats::Function f, int to ) {
    // float(to) := f()
    // f refers to one of the functions in Floats
    switch ( f ) {
        case Floats::Function::zero:
            float_set( to, oopFactory::new_double( 0.0 ) );
            break;
        case Floats::Function::one:
            float_set( to, oopFactory::new_double( 1.0 ) );
            break;
        default        : st_fatal1( "bad float nullary code %d", f );
    }
}


void NodeBuilder::float_unary( Floats::Function f, int fno ) {
    // float(fno) := f(float(fno))
    // f refers to one of the functions in Floats
    ArithOpCode op;
    switch ( f ) {
        case Floats::Function::abs:
            op = ArithOpCode::fAbsArithOp;
            break;
        case Floats::Function::negated:
            op = ArithOpCode::fNegArithOp;
            break;
        case Floats::Function::squared:
            op = ArithOpCode::fSqrArithOp;
            break;
        case Floats::Function::sqrt    : Unimplemented();
        case Floats::Function::sin    : Unimplemented();
        case Floats::Function::cos    : Unimplemented();
        case Floats::Function::tan    : Unimplemented();
        case Floats::Function::exp    : Unimplemented();
        case Floats::Function::ln    : Unimplemented();
        default        : st_fatal1( "bad float unary code %d", f );
    }
    PseudoRegister * preg = float_at( fno );
    append( NodeFactory::FloatUnaryArithNode( preg, preg, op ) );
}


void NodeBuilder::float_binary( Floats::Function f, int fno ) {
    // float(fno) := f(float(fno), float(fno+1))
    // f refers to one of the functions in Floats
    ArithOpCode op;
    switch ( f ) {
        case Floats::Function::add:
            op = ArithOpCode::fAddArithOp;
            break;
        case Floats::Function::subtract:
            op = ArithOpCode::fSubArithOp;
            break;
        case Floats::Function::multiply:
            op = ArithOpCode::fMulArithOp;
            break;
        case Floats::Function::divide:
            op = ArithOpCode::fDivArithOp;
            break;
        case Floats::Function::modulo:
            op = ArithOpCode::fModArithOp;
            break;
        default         : st_fatal1( "bad float binary code %d", f );
    }
    PseudoRegister * op1 = float_at( fno );
    PseudoRegister * op2 = float_at( fno + 1 );
    append( NodeFactory::FloatArithRRNode( op1, op1, op, op2 ) );
}


void NodeBuilder::float_unaryToOop( Floats::Function f, int fno ) {
    // push f(float(fno)) on top of (Oop) expression stack, result is an Oop
    // f refers to one of the functions in Floats
    PseudoRegister               * src = float_at( fno );
    SinglyAssignedPseudoRegister * res = new SinglyAssignedPseudoRegister( _scope );
    switch ( f ) {
        case Floats::Function::is_zero: // fall through
        case Floats::Function::is_not_zero: {
            ConstPseudoRegister * zero = new_ConstPReg( _scope, oopFactory::new_double( 0.0 ) );
            NodeFactory::FloatArithRRNode( new NoResultPseudoRegister( _scope ), src, ArithOpCode::fCmpArithOp, zero );
            BranchOpCode cond = f == Floats::Function::is_zero ? BranchOpCode::EQBranchOp : BranchOpCode::NEBranchOp;
            _expressionStack->push( PrimitiveInliner::generate_cond( cond, this, res ), scope(), scope()->byteCodeIndex() );
        }
            break;
        case Floats::Function::oopify: {
            append( NodeFactory::FloatUnaryArithNode( res, src, ArithOpCode::f2OopArithOp ) );
            Expression * result = new KlassExpression( doubleKlassObj, res, current() );
            _expressionStack->push( result, scope(), scope()->byteCodeIndex() );
        }
            break;
        default: st_fatal1( "bad float unaryToOop code %d", f );
    }
}


void NodeBuilder::float_binaryToOop( Floats::Function f, int fno ) {
    // push f(float(fno), float(fno+1)) on top of (Oop) expression stack, result is an Oop
    // f refers to one of the functions in Floats
    Assembler::Condition cc1;
    switch ( f ) {
        case Floats::Function::is_equal:
            cc1 = Assembler::Condition::equal;
            break;
        case Floats::Function::is_not_equal:
            cc1 = Assembler::Condition::notEqual;
            break;
        case Floats::Function::is_less:
            cc1 = Assembler::Condition::less;
            break;
        case Floats::Function::is_less_equal:
            cc1 = Assembler::Condition::lessEqual;
            break;
        case Floats::Function::is_greater:
            cc1 = Assembler::Condition::greater;
            break;
        case Floats::Function::is_greater_equal:
            cc1 = Assembler::Condition::greaterEqual;
            break;
        default             : st_fatal1( "bad float comparison code %d", f );
    }
    int                  mask;
    Assembler::Condition cond;
    MacroAssembler::fpu_mask_and_cond_for( cc1, mask, cond );
    PseudoRegister               * op1        = float_at( fno );
    PseudoRegister               * op2        = float_at( fno + 1 );
    SinglyAssignedPseudoRegister * fpu_status = new SinglyAssignedPseudoRegister( _scope, Mapping::asLocation( eax ), false, false, byteCodeIndex(), byteCodeIndex() );
    append( NodeFactory::FloatArithRRNode( fpu_status, op1, ArithOpCode::fCmpArithOp, op2 ) );
    append( NodeFactory::ArithRCNode( new NoResultPseudoRegister( _scope ), fpu_status, ArithOpCode::TestArithOp, mask ) );
    BranchOpCode cc2;
    switch ( cond ) {
        case Assembler::Condition::zero:
            cc2 = BranchOpCode::EQBranchOp;
            break;
        case Assembler::Condition::notZero:
            cc2 = BranchOpCode::NEBranchOp;
            break;
        default                : ShouldNotReachHere();
    }
    SinglyAssignedPseudoRegister * res = new SinglyAssignedPseudoRegister( _scope );
    _expressionStack->push( PrimitiveInliner::generate_cond( cc2, this, res ), scope(), scope()->byteCodeIndex() );
}
