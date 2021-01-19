//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/Expression.hpp"
#include "vm/compiler/NodeBuilder.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/oops/BlockClosureKlass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"


const int UnknownExpression::UnlikelyBit        = 1;
const int MergeExpression::SplittableBit        = 2;
const int MergeExpression::UnknownSetBit        = 4;
const int MergeExpression::ContainingUnknownBit = 8;

const int MaxMergeExprSize = 5;    // max. # exprs in a merge expression

Expression::Expression( PseudoRegister *p, Node *n ) {
    st_assert( p, "must have PseudoRegister" );
    _pseudoRegister = p;
    _node           = n;
    next            = nullptr;
    flags           = 0;
    st_assert( p->scope()->isInlinedScope(), "should be InlinedScope" );
    st_assert( n not_eq NodeBuilder::EndOfCode, "should be a real node" );
}


MergeExpression::MergeExpression( Expression *e1, Expression *e2, PseudoRegister *p, Node *nod ) :
        Expression( p, nod ) {
    initialize();
    if ( not p )
        _pseudoRegister = e1->preg();
    mergeInto( e1, nod );
    mergeInto( e2, nod );
    verify();
}


MergeExpression::MergeExpression( PseudoRegister *p, Node *nod ) :
        Expression( p, nod ) {
    initialize();
}


void MergeExpression::initialize() {
    exprs = new GrowableArray<Expression *>( MaxMergeExprSize ); // NB: won't grow beyond MaxMergeExprSize
    setSplittable( _node not_eq nullptr );
}


NoResultExpression::NoResultExpression( Node *n ) :
        Expression( new NoResultPseudoRegister( n ? n->scope() : theCompiler->currentScope() ), n ) {
}


ContextExpression::ContextExpression( PseudoRegister *r ) :
        Expression( r, nullptr ) {
}


KlassExpression::KlassExpression( KlassOop k, PseudoRegister *p, Node *n ) :
        Expression( p, n ) {
    _klass = k;
    st_assert( k, "must have klass" );
}


BlockExpression::BlockExpression( BlockPseudoRegister *p, Node *n ) :
        KlassExpression( BlockClosureKlass::blockKlassFor( p->closure()->nofArgs() ), p, n ) {
    st_assert( n, "must have a node" );
    _blockScope = p->scope();
}


Expression *Expression::asReceiver() const {
    // the receiver is the Expression* for a newly created InlinedScope; return the Expression that
    // should be used as the new scope's receiver
    st_assert( hasKlass(), "must have klass" );
    return (Expression *) this;
}


Expression *MergeExpression::asReceiver() const {
    return new KlassExpression( klass(), _pseudoRegister, _node );
}

// equals: do two expression denote the same type information?

bool_t UnknownExpression::equals( Expression *other ) const {
    return other->isUnknownExpression();
}


bool_t NoResultExpression::equals( Expression *other ) const {
    return other->isNoResultExpression();
}


bool_t KlassExpression::equals( Expression *other ) const {
    return ( other->isKlassExpression() or other->isConstantExpression() ) and other->klass() == klass();
}


bool_t BlockExpression::equals( Expression *other ) const {
    return other->isBlockExpression() and other->klass() == klass();
}


bool_t ConstantExpression::equals( Expression *other ) const {
    return ( other->isConstantExpression() and other->constant() == constant() ) or ( other->isKlassExpression() and other->klass() == klass() );
}


bool_t MergeExpression::equals( Expression *other ) const {
    return false; // for now -- fix this later
}


// mergeWith: return receiver merged with arg; functional (does not modify receiver or arg expr)
Expression *UnknownExpression::mergeWith( Expression *other, Node *n ) {
    if ( other->isNoResultExpression() )
        return this;
    if ( other->isUnknownExpression() ) {
        if ( n and node() and other->node() ) {
            // preserve splitting info
            MergeExpression *e = new MergeExpression( this, other, preg(), n );
            st_assert( e->isSplittable(), "wasted effort" );
            return e;
        } else {
            _node = nullptr;    // prevent future splitting
            return this;
        }
    } else {
        PseudoRegister *r = _pseudoRegister == other->preg() ? _pseudoRegister : nullptr;
        return new MergeExpression( this, other, r, n );
    }
}


Expression *NoResultExpression::mergeWith( Expression *other, Node *n ) {
    return other;
}


Expression *KlassExpression::mergeWith( Expression *other, Node *n ) {
    if ( other->isNoResultExpression() )
        return this;
    if ( ( other->isKlassExpression() or other->isConstantExpression() ) and other->klass() == klass() ) {
        // generalize klass + constant in same clone family --> klass
        _node = nullptr;    // prevent future splitting
        return this;
    } else {
        PseudoRegister *r = _pseudoRegister == other->preg() ? _pseudoRegister : nullptr;
        return new MergeExpression( this, other, r, n );
    }
}


Expression *BlockExpression::mergeWith( Expression *other, Node *n ) {
    if ( other->isNoResultExpression() )
        return this;
    if ( equals( other ) ) {
        if ( n and node() and other->node() ) {
            // preserve splitting info
            MergeExpression *e = new MergeExpression( this, other, preg(), n );
            st_assert( e->isSplittable(), "wasted effort" );
            return e;
        } else {
            _node = nullptr;    // prevent future splitting
            return this;
        }
    } else {
        PseudoRegister *r = _pseudoRegister == other->preg() ? _pseudoRegister : nullptr;
        return new MergeExpression( this, other, r, n );
    }
}


Expression *ConstantExpression::mergeWith( Expression *other, Node *n ) {
    // NB: be careful not to turn true & false into klasses
    if ( other->isNoResultExpression() )
        return this;
    if ( other->isConstantExpression() and other->constant() == constant() ) {
        if ( n and node() and other->node() ) {
            // preserve splitting info
            MergeExpression *e = new MergeExpression( this, other, preg(), n );
            st_assert( e->isSplittable(), "wasted effort" );
            return e;
        } else {
            _node = nullptr;    // prevent future splitting
            return this;
        }
    } else if ( other->isKlassExpression() ) {
        return other->mergeWith( this, n );
    } else {
        PseudoRegister *r = _pseudoRegister == other->preg() ? _pseudoRegister : nullptr;
        return new MergeExpression( this, other, r, n );
    }
}


Expression *MergeExpression::mergeWith( Expression *other, Node *n ) {
    st_assert( _pseudoRegister == other->preg() or other->isNoResultExpression(), "mismatched PRegs" );
    return new MergeExpression( this, other, _pseudoRegister, n );
}


// mergeInto: merge other expr into receiver; modifies receiver
void MergeExpression::mergeInto( Expression *other, Node *n ) {
    if ( other->isNoResultExpression() )
        return;
    setUnknownSet( false );
    if ( n == nullptr )
        setSplittable( false );
    _node = n;
    if ( other->isMergeExpression() ) {
        MergeExpression *o = other->asMergeExpression();
        if ( o->isSplittable() and not isSplittable() ) {
            std::size_t i = 0;
        }
        for ( int       i  = 0; i < o->exprs->length(); i++ ) {
            // must be careful when adding splittable exprs (e->next not_eq nullptr)
            // to avoid creating loops in the ->next chain
            Expression *e = o->exprs->at( i );
            Expression *nexte;
            for ( ; e; e = nexte ) {
                nexte = e->next;
                e->next = nullptr;
                add( e );
            }
        }
    } else {
        add( other );
    }

    int       len = exprs->length();
    for ( std::size_t i   = 0; i < len; i++ ) {
        Expression *e = exprs->at( i );
        for ( std::size_t j = i + 1; j < len; j++ ) {
            Expression *e2 = exprs->at( j );
            st_assert( not e->equals( e2 ), "duplicate expr" );
            st_assert( not( e->hasKlass() and e2->hasKlass() and e->klass() == e2->klass() ), "duplicate klasses" );
        }
    }

    st_assert( not isSplittable() or _node, "splittable mergeExpression must have a node" );
}


// add a new expression to the receiver
void MergeExpression::add( Expression *e ) {
    st_assert( e->next == nullptr, "shouldn't be set" );
    setUnknownSet( false );
    if ( exprs->isFull() ) {
        setSplittable( false );
        return;
    }
    if ( not e->node() )
        setSplittable( false );
    for ( std::size_t i = 0; i < exprs->length(); i++ ) {
        Expression *e1 = exprs->at( i );
        if ( ( e->hasKlass() and e1->hasKlass() and ( e->klass() == e1->klass() ) ) or e->equals( e1 ) ) {
            // an equivalent expression is already in our list
            // if unsplittable we don't need to do anything except
            // if e is a klass and the expr we already have is a constant
            // (otherwise: might later make unknown unlikely and rely on constant value)
            if ( not isSplittable() and not e1->isConstantExpression() )
                return;

            // even though the klass is already in our list, we care about
            // the new entry because we might have to copy the nodes between
            // it and the split send
            // Therefore, we keep lists of equivalent Exprs (linked via the "next" field).
            Node *n = e->node();
            if ( n ) {
                for ( Expression *e2 = exprs->at( i ); e2; e2 = e2->next ) {
                    if ( n == e2->node() ) {
                        // node already in list; this can happen if we're merging an expression
                        // with itself (e.g. we inlined 2 cases, both return the same argument)
                        // can't treat as splittable anymore
                        setSplittable( false );
                        break;
                    }
                }
            }

            // generalize different constants to klasses
            if ( e->isConstantExpression() and e1->isConstantExpression() and e->constant() == e1->constant() ) {
                // leave them as constants
            } else {
                if ( e->isConstantExpression() ) {
                    e = e->convertToKlass( e->preg(), e->node() );
                }
                if ( e1->isConstantExpression() ) {
                    // convertToKlass e1 and replace it in receiver
                    Expression *ee = e1->convertToKlass( e1->preg(), e1->node() );
                    ee->next = e1->next;
                    exprs->at_put( i, ee );
                }
            }
            if ( not isSplittable() )
                return;
            // append e at end of e1's next chain
            for ( e1 = exprs->at( i ); e1->next; e1 = e1->next );
            e1->next = e;
            return;
        }
    }
    if ( exprs->length() == MaxMergeExprSize ) {
        // our capacity overflows, so make sure we've got at least one Unknown
        // type in our set
        if ( findUnknown() == nullptr )
            exprs->append( new UnknownExpression( e->preg(), nullptr ) );
    } else {
        exprs->append( e );
    }
}


int MergeExpression::nklasses() const {
    int       n = 0;
    for ( std::size_t i = 0; i < exprs->length(); i++ ) {
        n += exprs->at( i )->nklasses();
    }
    return n;
}


// copyWithout: return receiver w/o the argument expression

Expression *KlassExpression::copyWithout( Expression *e ) const {
    st_assert( e->klass() == klass(), "don't have this klass" );
    return new NoResultExpression( node() );
}


Expression *ConstantExpression::copyWithout( Expression *e ) const {
    st_assert( e->constant() == constant(), "don't have this constant" );
    return new NoResultExpression( node() );
}


Expression *MergeExpression::copyWithout( Expression *e ) const {
    MergeExpression *res = (MergeExpression *) shallowCopy( preg(), node() );
    res->exprs->remove( e );
    return res;
}


bool_t MergeExpression::really_hasKlass( InlinedScope *s ) const {
    // Check if receiver really has only one klass.  Specifically, if we're
    // at the place that made the receiver's unknown part unlikely, the
    // receiver should *not* be considered a KlassExpression because the unknown
    // part hasn't been tested yet.
    return hasKlass() and not( s == _unlikelyScope and s->byteCodeIndex() <= _unlikelyByteCodeIndex );
}


bool_t MergeExpression::hasKlass() const {
    // treat a merge expr like a single klass if it contains only one klass and
    // possibly an unlikely unknown
    if ( exprs->length() > 2 )
        return false;
    Expression *e1 = exprs->at( 0 );
    bool_t haveKlass1 = e1->hasKlass();
    if ( exprs->length() == 1 )
        return haveKlass1;    // only one expr
    UnknownExpression *u1 = e1->findUnknown();
    if ( u1 and not u1->isUnlikely() )
        return false;  // 1st = likely unknown
    Expression *e2 = exprs->at( 1 );
    bool_t haveKlass2 = e2->hasKlass();
    UnknownExpression *u2 = e2->findUnknown();
    if ( u2 and not u2->isUnlikely() )
        return false;  // 2nd = likely unknown
    if ( haveKlass1 and haveKlass2 )
        return false;    // 2 klasses
    // success!  one expr may have klass, one is unlikely unknown
    return haveKlass1 or haveKlass2;
}


KlassExpression *MergeExpression::asKlassExpression() const {
    st_assert( hasKlass(), "don't have a klass" );
    Expression *e = exprs->at( 0 );
    return e->hasKlass() ? e->asKlassExpression() : exprs->at( 1 )->asKlassExpression();
}


KlassOop MergeExpression::klass() const {
    st_assert( hasKlass(), "don't have a klass" );
    Expression *e = exprs->at( 0 );
    return e->hasKlass() ? e->klass() : exprs->at( 1 )->klass();
}


bool_t MergeExpression::hasConstant() const {
    // see hasKlass()...also, must be careful about different constants with
    // same klass (i.e. expr->next is set)
    return false;
}


Oop MergeExpression::constant() const {
    ShouldNotCallThis();
    return nullptr;
}


Expression *ConstantExpression::convertToKlass( PseudoRegister *p, Node *n ) const {
    return new KlassExpression( _c->klass(), p, n );
}


KlassExpression *ConstantExpression::asKlassExpression() const {
    return new KlassExpression( _c->klass(), _pseudoRegister, _node );
}


Expression *MergeExpression::convertToKlass( PseudoRegister *p, Node *n ) const {
    MergeExpression *e = new MergeExpression( p, n );
    for ( std::size_t i = 0; i < exprs->length(); i++ ) {
        Expression *expr = exprs->at( i )->convertToKlass( p, n );
        e->add( expr );
    }
    // result is non-splittable because nodes aren't correct and expr->next
    // is ignored for the components of the receiver
    e->setSplittable( false );
    return e;
}


bool_t MergeExpression::containsUnknown() {
    if ( isUnknownSet() ) {
        st_assert ( ( findUnknown() == nullptr ) not_eq isContainingUnknown(), "isContainingUnknown wrong" );
        return isContainingUnknown();
    }
    setUnknownSet( true );
    for ( std::size_t i = 0; i < exprs->length(); i++ ) {
        if ( exprs->at( i )->isUnknownExpression() ) {
            setContainingUnknown( true );
            return true;
        }
    }
    setContainingUnknown( false );
    return false;
}


UnknownExpression *MergeExpression::findUnknown() const {
    for ( std::size_t i = 0; i < exprs->length(); i++ ) {
        if ( exprs->at( i )->isUnknownExpression() )
            return (UnknownExpression *) exprs->at( i );
    }
    return nullptr;
}


Expression *MergeExpression::findKlass( KlassOop klass ) const {
    for ( std::size_t i = 0; i < exprs->length(); i++ ) {
        Expression *e = exprs->at( i );
        if ( e->hasKlass() and e->klass() == klass )
            return e;
    }
    return nullptr;
}


Expression *UnknownExpression::makeUnknownUnlikely( InlinedScope *s ) {
    st_assert( DeferUncommonBranches, "shouldn't make unlikely" );
    // called on an UnknownExpression itself, this is a no-op; works only
    // with merge exprs
    return this;
}


Expression *MergeExpression::makeUnknownUnlikely( InlinedScope *s ) {
    st_assert( DeferUncommonBranches, "shouldn't make unlikely" );
    _unlikelyScope         = s;
    _unlikelyByteCodeIndex = s->byteCodeIndex();
    for ( std::size_t i = 0; i < exprs->length(); i++ ) {
        Expression *e;
        if ( ( e = exprs->at( i ) )->isUnknownExpression() ) {
            if ( not( (UnknownExpression *) e )->isUnlikely() ) {
                // must make a copy to avoid side-effecting e.g. incoming arg
                UnknownExpression *u     = (UnknownExpression *) e;
                UnknownExpression *new_u = new UnknownExpression( u->preg(), u->node(), true );
                exprs->at_put( i, new_u );
                for ( u = (UnknownExpression *) u->next; u; u = (UnknownExpression *) u->next, new_u = (UnknownExpression *) new_u->next ) {
                    new_u->next = new UnknownExpression( u->preg(), u->node(), true );
                }
            }
            return this;
        }
    }
    ShouldNotReachHere(); // contains no unknown expression
    return nullptr;
}


Expression *ConstantExpression::findKlass( KlassOop m ) const {
    return klass() == m ? (Expression *) this : nullptr;
}


// needsStoreCheck: when storing the expr into the heap, do we need a GC store check?
bool_t KlassExpression::needsStoreCheck() const {
    return _klass not_eq smiKlassObj;
}


bool_t ConstantExpression::needsStoreCheck() const {
    // don't need a check if either
    // - it's a smi_t, or
    // - it's an old object (old objects never become young again)
    return not( _c->is_smi() or _c->is_old() );
}


Expression *UnknownExpression::shallowCopy( PseudoRegister *p, Node *n ) const {
    return new UnknownExpression( p, n, isUnlikely() );
}


Expression *NoResultExpression::shallowCopy( PseudoRegister *p, Node *n ) const {
    return new NoResultExpression();
}


Expression *KlassExpression::shallowCopy( PseudoRegister *p, Node *n ) const {
    return new KlassExpression( _klass, p, n );
}


Expression *BlockExpression::shallowCopy( PseudoRegister *p, Node *n ) const {
    if ( p->isBlockPseudoRegister() ) {
        return new BlockExpression( (BlockPseudoRegister *) p, n );
    } else {
        // remove block info (might be a performance bug -- should keep
        // around info so can inline (but: would have to check for non-LIFO
        // cases, e.g. when non-clean block is returned from a method)
        return new UnknownExpression( p, n );
    }
}


Expression *ConstantExpression::shallowCopy( PseudoRegister *p, Node *n ) const {
    return new ConstantExpression( _c, p, n );
}


Expression *MergeExpression::shallowCopy( PseudoRegister *p, Node *n ) const {
    MergeExpression *e = new MergeExpression( p, n );
    e->exprs = exprs->copy();
    e->setSplittable( isSplittable() );
    return e;
}


InlinedScope *Expression::scope() const {
    st_assert( _pseudoRegister->scope()->isInlinedScope(), "oops" );
    return (InlinedScope *) _pseudoRegister->scope();
}


NameNode *Expression::nameNode( bool_t mustBeLegal ) const {
    return preg()->nameNode( mustBeLegal );
}


NameNode *ConstantExpression::nameNode( bool_t mustBeLegal ) const {
//c    return newValueName(constant()); }
    return 0;
}


void Expression::print_helper( const char *type ) {
    lprintf( " (Node %#lx)", node() );
    if ( next )
        lprintf( " (next %#lx)", next );
    lprintf( "    ((%s*)%#x)\n", type, this );
}


void UnknownExpression::print() {
    lprintf( "UnknownExpression %s", isUnlikely() ? "unlikely" : "" );
    Expression::print_helper( "UnknownExpression" );
}


void NoResultExpression::print() {
    lprintf( "NoResultExpression " );
    Expression::print_helper( "NoResultExpression" );
}


void ContextExpression::print() {
    lprintf( "ContextExpression %s", preg()->safeName() );
    Expression::print_helper( "ContextExpression" );
}


void ConstantExpression::print() {
    lprintf( "ConstantExpression %s", constant()->print_value_string() );
    Expression::print_helper( "ConstantExpression" );
}


void KlassExpression::print() {
    lprintf( "KlassExpression %s", klass()->print_value_string() );
    Expression::print_helper( "KlassExpression" );
}


void BlockExpression::print() {
    lprintf( "BlockExpression %s", preg()->name() );
    Expression::print_helper( "BlockExpression" );
}


void MergeExpression::print() {
    lprintf( "MergeExpression %s(\n", isSplittable() ? "splittable " : "" );
    for ( std::size_t i = 0; i < exprs->length(); i++ ) {
        lprintf( "\t%#lx%s ", exprs->at( i ), exprs->at( i )->next ? "*" : "" );
        exprs->at( i )->print();
    }
    lprintf( ")" );
    Expression::print_helper( "MergeExpression" );
}


void Expression::verify() const {
    if ( _pseudoRegister == nullptr ) {
        error( "Expression %#lx: no preg", this );
    } else {
        _pseudoRegister->verify();
    }
    //if (_node) _node->verify();
    //  if (unlikelyScope) unlikelyScope->verify();
}


void KlassExpression::verify() const {
    Expression::verify();
    _klass->verify();
    if ( not _klass->is_klass() )
        error( "KlassExpression %#lx: _klass %#lx isn't a klass", this, _klass );
}


void BlockExpression::verify() const {
    Expression::verify();
    if ( _blockScope not_eq preg()->creationScope() )
        error( "BlockExpression %#lx: inconsistent parent scope", this, _blockScope, preg()->creationScope() );
}


void ConstantExpression::verify() const {
    Expression::verify();
    _c->verify();
}


void MergeExpression::verify() const {
    GrowableArray<Node *> nodes( 10 );
    for ( int             i = 0; i < exprs->length(); i++ ) {
        Expression *e = exprs->at( i );
        e->verify();
        if ( e->isMergeExpression() )
            error( "MergeExpression %#lx contains nested MergeExpression %#lx", this, e );
        Node *n = e->node();
        if ( n ) {
            if ( nodes.contains( n ) )
                error( "MergeExpression %#lx contains 2 expressions with same node %#lx", this, n );
            nodes.append( n );
        }
    }
    Expression::verify();
}


void ContextExpression::verify() const {
    Expression::verify();
}


ExpressionStack::ExpressionStack( InlinedScope *scope, int size ) :
        GrowableArray<Expression *>( size ) {
    _scope = scope;
}


void ExpressionStack::push( Expression *expr, InlinedScope *currentScope, int byteCodeIndex ) {
    st_assert( not expr->isContextExpression(), "shouldn't push contexts" );
    // Register expression e for byteCodeIndex
    currentScope->setExprForByteCodeIndex( byteCodeIndex, expr );
    // Set r's startByteCodeIndex if it is an expr stack entry and not already set,
    // currentScope is the scope doing the push.
    PseudoRegister *r = expr->preg();
    if ( r->isSinglyAssignedPseudoRegister() ) {
        SinglyAssignedPseudoRegister *sr = (SinglyAssignedPseudoRegister *) r;
        if ( sr->scope() == _scope ) {
            if ( sr->begByteCodeIndex() == IllegalByteCodeIndex )
                sr->_begByteCodeIndex = sr->creationStartByteCodeIndex = _scope->byteCodeIndex();
        } else {
            st_assert( sr->scope()->isSenderOf( _scope ), "preg scope too low" );
        }
    }
    GrowableArray<Expression *>::push( expr );
}


void ExpressionStack::push2nd( Expression *expr, InlinedScope *currentScope, int byteCodeIndex ) {
    st_assert( not expr->isContextExpression(), "shouldn't push contexts" );
    // Register expression e for current ByteCodeIndex.
    currentScope->set2ndExprForByteCodeIndex( byteCodeIndex, expr );
    // Set r's startByteCodeIndex if it is an expr stack entry and not already set,
    // currentScope is the scope doing the push.
    PseudoRegister *r = expr->preg();
    if ( r->isSinglyAssignedPseudoRegister() ) {
        SinglyAssignedPseudoRegister *sr = (SinglyAssignedPseudoRegister *) r;
        if ( sr->scope() == _scope ) {
            if ( sr->begByteCodeIndex() == IllegalByteCodeIndex )
                sr->_begByteCodeIndex = sr->creationStartByteCodeIndex = _scope->byteCodeIndex();
        } else {
            st_assert( sr->scope()->isSenderOf( _scope ), "preg scope too low" );
        }
    }
    GrowableArray<Expression *>::push( expr );
}


void ExpressionStack::assign_top( Expression *expr ) {
    st_assert( not expr->isContextExpression(), "shouldn't push contexts" );
    GrowableArray<Expression *>::at_put( _length - 1, expr );
}


Expression *ExpressionStack::pop() {
    Expression     *e = GrowableArray<Expression *>::pop();
    PseudoRegister *r = e->preg();
    if ( r->isSinglyAssignedPseudoRegister() ) {
        SinglyAssignedPseudoRegister *sr = (SinglyAssignedPseudoRegister *) r;
        if ( sr->scope() == _scope ) {
            // endByteCodeIndex may be assigned several times
            int newByteCodeIndex = _scope->byteCodeIndex() == EpilogueByteCodeIndex ? _scope->nofBytes() - 1 : _scope->byteCodeIndex();
            if ( byteCodeIndexLT( sr->endByteCodeIndex(), newByteCodeIndex ) )
                sr->_endByteCodeIndex = newByteCodeIndex;
        } else {
            st_assert( sr->scope()->isSenderOf( _scope ), "preg scope too low" );
        }
    }
    return e;
}


void ExpressionStack::pop( int nofExprsToPop ) {
    for ( std::size_t i = 0; i < nofExprsToPop; i++ )
        pop();
}


void ExpressionStack::print() {
    const int len = length();
    for ( std::size_t i   = 0; i < len; i++ ) {
        lprintf( "[TOS - %2d]:  ", len - i - 1 );
        at( i )->print();
    }
}
