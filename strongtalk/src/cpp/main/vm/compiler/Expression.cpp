
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


const std::int32_t UnknownExpression::UnlikelyBit        = 1;
const std::int32_t MergeExpression::SplittableBit        = 2;
const std::int32_t MergeExpression::UnknownSetBit        = 4;
const std::int32_t MergeExpression::ContainingUnknownBit = 8;

const std::int32_t MaxMergeExprSize = 5;    // max. # exprs in a merge expression


Expression::Expression( PseudoRegister *p, Node *n ) :
    _unlikelyByteCodeIndex{ 0 },
    _unlikelyScope{ nullptr },
    _pseudoRegister{ p },
    _node{ n },
    next{ nullptr },
    flags{ 0 } {

    //
    st_assert( p, "must have PseudoRegister" );
    st_assert( p->scope()->isInlinedScope(), "should be InlinedScope" );
    st_assert( n not_eq NodeBuilder::EndOfCode, "should be a real node" );

}


MergeExpression::MergeExpression( Expression *e1, Expression *e2, PseudoRegister *p, Node *nod ) :
    Expression( p, nod ),
    exprs{ nullptr } {

    initialize();
    if ( not p ) {
        _pseudoRegister = e1->pseudoRegister();
    }
    mergeInto( e1, nod );
    mergeInto( e2, nod );

    verify();
}


MergeExpression::MergeExpression( PseudoRegister *p, Node *nod ) :
    Expression( p, nod ),
    exprs{ nullptr } {
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
    Expression( p, n ),
    _klass{ k } {
    st_assert( k, "must have klass" );
}


BlockExpression::BlockExpression( BlockPseudoRegister *p, Node *n ) :
    KlassExpression( BlockClosureKlass::blockKlassFor( p->closure()->nofArgs() ), p, n ),
    _blockScope{ nullptr } {
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

bool UnknownExpression::equals( Expression *other ) const {
    return other->isUnknownExpression();
}


bool NoResultExpression::equals( Expression *other ) const {
    return other->isNoResultExpression();
}


bool KlassExpression::equals( Expression *other ) const {
    return ( other->isKlassExpression() or other->isConstantExpression() ) and other->klass() == klass();
}


bool BlockExpression::equals( Expression *other ) const {
    return other->isBlockExpression() and other->klass() == klass();
}


bool ConstantExpression::equals( Expression *other ) const {
    return ( other->isConstantExpression() and other->constant() == constant() ) or ( other->isKlassExpression() and other->klass() == klass() );
}


bool MergeExpression::equals( Expression *other ) const {
    static_cast<void>(other); // unused
    return false; // for now -- fix this later
}


// mergeWith: return receiver merged with arg; functional (does not modify receiver or arg expr)
Expression *UnknownExpression::mergeWith( Expression *other, Node *n ) {
    if ( other->isNoResultExpression() )
        return this;
    if ( other->isUnknownExpression() ) {
        if ( n and node() and other->node() ) {
            // preserve splitting info
            MergeExpression *e = new MergeExpression( this, other, pseudoRegister(), n );
            st_assert( e->isSplittable(), "wasted effort" );
            return e;
        } else {
            _node = nullptr;    // prevent future splitting
            return this;
        }
    } else {
        PseudoRegister *r = _pseudoRegister == other->pseudoRegister() ? _pseudoRegister : nullptr;
        return new MergeExpression( this, other, r, n );
    }
}


Expression *NoResultExpression::mergeWith( Expression *other, Node *n ) {
    static_cast<void>(other); // unused
    static_cast<void>(n); // unused
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
        PseudoRegister *r = _pseudoRegister == other->pseudoRegister() ? _pseudoRegister : nullptr;
        return new MergeExpression( this, other, r, n );
    }
}


Expression *BlockExpression::mergeWith( Expression *other, Node *n ) {
    if ( other->isNoResultExpression() )
        return this;
    if ( equals( other ) ) {
        if ( n and node() and other->node() ) {
            // preserve splitting info
            MergeExpression *e = new MergeExpression( this, other, pseudoRegister(), n );
            st_assert( e->isSplittable(), "wasted effort" );
            return e;
        } else {
            _node = nullptr;    // prevent future splitting
            return this;
        }
    } else {
        PseudoRegister *r = _pseudoRegister == other->pseudoRegister() ? _pseudoRegister : nullptr;
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
            MergeExpression *e = new MergeExpression( this, other, pseudoRegister(), n );
            st_assert( e->isSplittable(), "wasted effort" );
            return e;
        } else {
            _node = nullptr;    // prevent future splitting
            return this;
        }
    } else if ( other->isKlassExpression() ) {
        return other->mergeWith( this, n );
    } else {
        PseudoRegister *r = _pseudoRegister == other->pseudoRegister() ? _pseudoRegister : nullptr;
        return new MergeExpression( this, other, r, n );
    }
}


Expression *MergeExpression::mergeWith( Expression *other, Node *n ) {
    st_assert( _pseudoRegister == other->pseudoRegister() or other->isNoResultExpression(), "mismatched PseudoRegisters" );
    return new MergeExpression( this, other, _pseudoRegister, n );
}


// mergeInto: merge other expr into receiver; modifies receiver
void MergeExpression::mergeInto( Expression *other, Node *n ) {
    if ( other->isNoResultExpression() ) {
        return;
    }

    setUnknownSet( false );
    if ( n == nullptr ) {
        setSplittable( false );
    }
    _node = n;
    if ( other->isMergeExpression() ) {

        MergeExpression *o = other->asMergeExpression();
        if ( o->isSplittable() and not isSplittable() ) {
//            std::int32_t i = 0;
        }

        for ( std::int32_t i = 0; i < o->exprs->length(); i++ ) {
            // must be careful when adding splittable exprs (e->next not_eq nullptr) to avoid creating loops in the ->next chain
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

    std::int32_t       len = exprs->length();
    for ( std::int32_t i   = 0; i < len; i++ ) {
        Expression         *e = exprs->at( i );
        for ( std::int32_t j  = i + 1; j < len; j++ ) {
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
    for ( std::int32_t i = 0; i < exprs->length(); i++ ) {
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
                    e = e->convertToKlass( e->pseudoRegister(), e->node() );
                }
                if ( e1->isConstantExpression() ) {
                    // convertToKlass e1 and replace it in receiver
                    Expression *ee = e1->convertToKlass( e1->pseudoRegister(), e1->node() );
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
            exprs->append( new UnknownExpression( e->pseudoRegister(), nullptr ) );
    } else {
        exprs->append( e );
    }
}


std::int32_t MergeExpression::nklasses() const {
    std::int32_t       n = 0;
    for ( std::int32_t i = 0; i < exprs->length(); i++ ) {
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
    MergeExpression *res = (MergeExpression *) shallowCopy( pseudoRegister(), node() );
    res->exprs->remove( e );
    return res;
}


bool MergeExpression::really_hasKlass( InlinedScope *s ) const {
    // Check if receiver really has only one klass.  Specifically, if we're
    // at the place that made the receiver's unknown part unlikely, the
    // receiver should *not* be considered a KlassExpression because the unknown
    // part hasn't been tested yet.
    return hasKlass() and not( s == _unlikelyScope and s->byteCodeIndex() <= _unlikelyByteCodeIndex );
}


bool MergeExpression::hasKlass() const {
    // treat a merge expr like a single klass if it contains only one klass and
    // possibly an unlikely unknown
    if ( exprs->length() > 2 )
        return false;
    Expression *e1        = exprs->at( 0 );
    bool       haveKlass1 = e1->hasKlass();
    if ( exprs->length() == 1 )
        return haveKlass1;    // only one expr
    UnknownExpression *u1 = e1->findUnknown();
    if ( u1 and not u1->isUnlikely() )
        return false;  // 1st = likely unknown
    Expression        *e2        = exprs->at( 1 );
    bool              haveKlass2 = e2->hasKlass();
    UnknownExpression *u2        = e2->findUnknown();
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


bool MergeExpression::hasConstant() const {
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
    MergeExpression    *e = new MergeExpression( p, n );
    for ( std::int32_t i  = 0; i < exprs->length(); i++ ) {
        Expression *expr = exprs->at( i )->convertToKlass( p, n );
        e->add( expr );
    }
    // result is non-splittable because nodes aren't correct and expr->next
    // is ignored for the components of the receiver
    e->setSplittable( false );
    return e;
}


bool MergeExpression::containsUnknown() {
    if ( isUnknownSet() ) {
        st_assert ( ( findUnknown() == nullptr ) not_eq isContainingUnknown(), "isContainingUnknown wrong" );
        return isContainingUnknown();
    }
    setUnknownSet( true );
    for ( std::int32_t i = 0; i < exprs->length(); i++ ) {
        if ( exprs->at( i )->isUnknownExpression() ) {
            setContainingUnknown( true );
            return true;
        }
    }
    setContainingUnknown( false );
    return false;
}


UnknownExpression *MergeExpression::findUnknown() const {
    for ( std::int32_t i = 0; i < exprs->length(); i++ ) {
        if ( exprs->at( i )->isUnknownExpression() )
            return (UnknownExpression *) exprs->at( i );
    }
    return nullptr;
}


Expression *MergeExpression::findKlass( KlassOop klass ) const {
    for ( std::int32_t i = 0; i < exprs->length(); i++ ) {
        Expression *e = exprs->at( i );
        if ( e->hasKlass() and e->klass() == klass )
            return e;
    }
    return nullptr;
}


Expression *UnknownExpression::makeUnknownUnlikely( InlinedScope *s ) {
    static_cast<void>(s); // unused
    st_assert( DeferUncommonBranches, "shouldn't make unlikely" );
    // called on an UnknownExpression itself, this is a no-op; works only
    // with merge exprs
    return this;
}


Expression *MergeExpression::makeUnknownUnlikely( InlinedScope *s ) {
    st_assert( DeferUncommonBranches, "shouldn't make unlikely" );
    _unlikelyScope         = s;
    _unlikelyByteCodeIndex = s->byteCodeIndex();
    for ( std::int32_t i = 0; i < exprs->length(); i++ ) {
        Expression *e;
        if ( ( e = exprs->at( i ) )->isUnknownExpression() ) {
            if ( not( (UnknownExpression *) e )->isUnlikely() ) {
                // must make a copy to avoid side-effecting e.g. incoming arg
                UnknownExpression *u     = (UnknownExpression *) e;
                UnknownExpression *new_u = new UnknownExpression( u->pseudoRegister(), u->node(), true );
                exprs->at_put( i, new_u );
                for ( u = (UnknownExpression *) u->next; u; u = (UnknownExpression *) u->next, new_u = (UnknownExpression *) new_u->next ) {
                    new_u->next = new UnknownExpression( u->pseudoRegister(), u->node(), true );
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
bool KlassExpression::needsStoreCheck() const {
    return _klass not_eq smiKlassObject;
}


bool ConstantExpression::needsStoreCheck() const {
    // don't need a check if either
    // - it's a small_int_t, or
    // - it's an old object (old objects never become young again)
    return not( _c->isSmallIntegerOop() or _c->is_old() );
}


Expression *UnknownExpression::shallowCopy( PseudoRegister *p, Node *n ) const {
    return new UnknownExpression( p, n, isUnlikely() );
}


Expression *NoResultExpression::shallowCopy( PseudoRegister *p, Node *n ) const {
    static_cast<void>(p); // unused
    static_cast<void>(n); // unused
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


NameNode *Expression::nameNode( bool mustBeLegal ) const {
    return pseudoRegister()->nameNode( mustBeLegal );
}


NameNode *ConstantExpression::nameNode( bool mustBeLegal ) const {
    static_cast<void>(mustBeLegal); // unused

    // return newValueName(constant()); }
    return 0;
}


void Expression::print_helper( const char *type ) {
    SPDLOG_INFO( " (Node 0x{0:x})", static_cast<void *>( node() ) );
    if ( next ) {
        SPDLOG_INFO( " (next 0x{0:x})", static_cast<void *>( next ) );
    }
    SPDLOG_INFO( "    ((%s*)0x{0:x})", type, static_cast<void *>( this ) );
}


void UnknownExpression::print() {
    SPDLOG_INFO( "UnknownExpression %s", isUnlikely() ? "unlikely" : "" );
    Expression::print_helper( "UnknownExpression" );
}


void NoResultExpression::print() {
    SPDLOG_INFO( "NoResultExpression " );
    Expression::print_helper( "NoResultExpression" );
}


void ContextExpression::print() {
    SPDLOG_INFO( "ContextExpression %s", pseudoRegister()->safeName() );
    Expression::print_helper( "ContextExpression" );
}


void ConstantExpression::print() {
    SPDLOG_INFO( "ConstantExpression %s", constant()->print_value_string() );
    Expression::print_helper( "ConstantExpression" );
}


void KlassExpression::print() {
    SPDLOG_INFO( "KlassExpression %s", klass()->print_value_string() );
    Expression::print_helper( "KlassExpression" );
}


void BlockExpression::print() {
    SPDLOG_INFO( "BlockExpression %s", pseudoRegister()->name() );
    Expression::print_helper( "BlockExpression" );
}


void MergeExpression::print() {
    SPDLOG_INFO( "MergeExpression %s(", isSplittable() ? "splittable " : "" );
    for ( std::int32_t i = 0; i < exprs->length(); i++ ) {
        SPDLOG_INFO( "\t0x{0:x}%s ", static_cast<void *>( exprs->at( i ) ), exprs->at( i )->next ? "*" : "" );
        exprs->at( i )->print();
    }
    SPDLOG_INFO( ")" );
    Expression::print_helper( "MergeExpression" );
}


void Expression::verify() const {
    if ( _pseudoRegister == nullptr ) {
        error( "Expression 0x{0:x}: no pseudoRegister", this );
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
        error( "KlassExpression 0x{0:x}: _klass 0x{0:x} isn't a klass", this, _klass );
}


void BlockExpression::verify() const {
    Expression::verify();
    if ( _blockScope not_eq pseudoRegister()->creationScope() )
        error( "BlockExpression 0x{0:x}: inconsistent parent scope", this, _blockScope, pseudoRegister()->creationScope() );
}


void ConstantExpression::verify() const {
    Expression::verify();
    _c->verify();
}


void MergeExpression::verify() const {
    GrowableArray<Node *> nodes( 10 );
    for ( std::int32_t    i = 0; i < exprs->length(); i++ ) {
        Expression *e = exprs->at( i );
        e->verify();
        if ( e->isMergeExpression() )
            error( "MergeExpression 0x{0:x} contains nested MergeExpression 0x{0:x}", this, e );
        Node *n = e->node();
        if ( n ) {
            if ( nodes.contains( n ) )
                error( "MergeExpression 0x{0:x} contains 2 expressions with same node 0x{0:x}", this, n );
            nodes.append( n );
        }
    }
    Expression::verify();
}


void ContextExpression::verify() const {
    Expression::verify();
}


ExpressionStack::ExpressionStack( InlinedScope *scope, std::int32_t size ) :
    GrowableArray<Expression *>( size ),
    _scope{ scope } {
}


void ExpressionStack::push( Expression *expr, InlinedScope *currentScope, std::int32_t byteCodeIndex ) {
    st_assert( not expr->isContextExpression(), "shouldn't push contexts" );
    // Register expression e for byteCodeIndex
    currentScope->setExprForByteCodeIndex( byteCodeIndex, expr );
    // Set r's startByteCodeIndex if it is an expr stack entry and not already set,
    // currentScope is the scope doing the push.
    PseudoRegister *r = expr->pseudoRegister();
    if ( r->isSinglyAssignedPseudoRegister() ) {
        SinglyAssignedPseudoRegister *sr = (SinglyAssignedPseudoRegister *) r;
        if ( sr->scope() == _scope ) {
            if ( sr->begByteCodeIndex() == IllegalByteCodeIndex )
                sr->_begByteCodeIndex = sr->_creationStartByteCodeIndex = _scope->byteCodeIndex();
        } else {
            st_assert( sr->scope()->isSenderOf( _scope ), "pseudoRegister scope too low" );
        }
    }
    GrowableArray<Expression *>::push( expr );
}


void ExpressionStack::push2nd( Expression *expr, InlinedScope *currentScope, std::int32_t byteCodeIndex ) {
    st_assert( not expr->isContextExpression(), "shouldn't push contexts" );
    // Register expression e for current ByteCodeIndex.
    currentScope->set2ndExprForByteCodeIndex( byteCodeIndex, expr );
    // Set r's startByteCodeIndex if it is an expr stack entry and not already set,
    // currentScope is the scope doing the push.
    PseudoRegister *r = expr->pseudoRegister();
    if ( r->isSinglyAssignedPseudoRegister() ) {
        SinglyAssignedPseudoRegister *sr = (SinglyAssignedPseudoRegister *) r;
        if ( sr->scope() == _scope ) {
            if ( sr->begByteCodeIndex() == IllegalByteCodeIndex )
                sr->_begByteCodeIndex = sr->_creationStartByteCodeIndex = _scope->byteCodeIndex();
        } else {
            st_assert( sr->scope()->isSenderOf( _scope ), "pseudoRegister scope too low" );
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
    PseudoRegister *r = e->pseudoRegister();
    if ( r->isSinglyAssignedPseudoRegister() ) {
        SinglyAssignedPseudoRegister *sr = (SinglyAssignedPseudoRegister *) r;
        if ( sr->scope() == _scope ) {
            // endByteCodeIndex may be assigned several times
            std::int32_t newByteCodeIndex = _scope->byteCodeIndex() == EpilogueByteCodeIndex ? _scope->nofBytes() - 1 : _scope->byteCodeIndex();
            if ( byteCodeIndexLT( sr->endByteCodeIndex(), newByteCodeIndex ) )
                sr->_endByteCodeIndex = newByteCodeIndex;
        } else {
            st_assert( sr->scope()->isSenderOf( _scope ), "pseudoRegister scope too low" );
        }
    }
    return e;
}


void ExpressionStack::pop( std::int32_t nofExprsToPop ) {
    for ( std::int32_t i = 0; i < nofExprsToPop; i++ )
        pop();
}


void ExpressionStack::print() {
    const std::int32_t len = length();
    for ( std::int32_t i   = 0; i < len; i++ ) {
        SPDLOG_INFO( "[TOS - %2d]:  ", len - i - 1 );
        at( i )->print();
    }
}
