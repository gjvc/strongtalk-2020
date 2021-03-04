
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/primitive/primitives.hpp"
#include "vm/primitive/BehaviorPrimitives.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/primitive/PrimitivesGenerator.hpp"
#include "vm/primitive/SmallIntegerOopPrimitives.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/NodeFactory.hpp"
#include "vm/utility/StringOutputStream.hpp"


std::int32_t        BasicNode::currentID;
std::int32_t        BasicNode::currentCommentID;
std::int32_t        BasicNode::lastByteCodeIndex;
ScopeInfo           BasicNode::lastScopeInfo;
PrimitiveDescriptor *InterruptCheckNode::_intrCheck;


void initNodes() {
    Node::currentID              = Node::currentCommentID = 0;
    Node::lastScopeInfo          = (ScopeInfo) -1;
    Node::lastByteCodeIndex      = IllegalByteCodeIndex;
    NodeFactory::_cumulativeCost = 0;
}


void BasicNode::setScope( InlinedScope *s ) {
    _scope         = s;
    _byteCodeIndex = s ? s->byteCodeIndex() : IllegalByteCodeIndex;
    st_assert( not s or not s->isInlinedScope() or s->byteCodeIndex() <= ( (InlinedScope *) s )->nofBytes(), "illegal byteCodeIndex" );
}


BasicNode::BasicNode() :
    _id{ currentID++ },
    _num{ -1 },
    _byteCodeIndex{},
    _basicBlock{ nullptr },
    _scope{},
    _pseudoRegisterMapping{ nullptr },
    _label{},
    _dontEliminate{ false },
    _deleted{ false } {
    setScope( theCompiler->currentScope() );

}


PseudoRegisterMapping *BasicNode::mapping() const {
    return new PseudoRegisterMapping( _pseudoRegisterMapping );
}


void BasicNode::setMapping( PseudoRegisterMapping *mapping ) {
    st_assert( not hasMapping(), "cannot be assigned twice" );
    _pseudoRegisterMapping = new PseudoRegisterMapping( mapping );
}


NonTrivialNode::NonTrivialNode() :
    _dest{ nullptr },
    _src{ nullptr },
    _srcUse{ nullptr },
    _destDef{ nullptr } {

}


LoadUplevelNode::LoadUplevelNode( PseudoRegister *dst, PseudoRegister *context0, std::int32_t nofLevels, std::int32_t offset, SymbolOop name ) :
    LoadNode( dst ),
    _context0Use{ nullptr },
    _context0{ context0 },
    _nofLevels{ nofLevels },
    _offset{ offset },
    _name{ name } {
    st_assert( context0 not_eq nullptr, "context0 is nullptr" );
    st_assert( nofLevels >= 0, "nofLevels must be >= 0" );
    st_assert( offset >= 0, "offset must be >= 0" );
}


StoreUplevelNode::StoreUplevelNode( PseudoRegister *src, PseudoRegister *context0, std::int32_t nofLevels, std::int32_t offset, SymbolOop name, bool needsStoreCheck ) :
    StoreNode( src ),
    _context0Use{ nullptr },
    _context0{ context0 },
    _nofLevels{ nofLevels },
    _offset{ offset },
    _needsStoreCheck{ needsStoreCheck },
    _name{ name } {
    st_assert( context0 not_eq nullptr, "context0 is nullptr" );
    st_assert( nofLevels >= 0, "nofLevels must be >= 0" );
    st_assert( offset >= 0, "offset must be >= 0" );
}


AssignNode::AssignNode( PseudoRegister *s, PseudoRegister *d ) :
    StoreNode( s ) {
    _dest = d;
    st_assert( d, "dest is nullptr" );
    // Fix this Lars assert(not s->isNoPseudoRegister(), "source must be a real PseudoRegister");
    st_assert( s not_eq d, "creating dummy assignment" );
}


CommentNode::CommentNode( const char *s ) :
    _comment{ s } {
    // give all comments negative ids (don't disturb node numbers by turning CompilerDebug off and on)
    _id = --currentCommentID;
    currentID--;
}


ArrayAtNode::ArrayAtNode( AccessType access_type, PseudoRegister *array, PseudoRegister *index, bool smiIndex, PseudoRegister *result, PseudoRegister *error, std::int32_t data_offset, std::int32_t length_offset ) :
    AbstractArrayAtNode( array, index, smiIndex, result, error, data_offset, length_offset ),
    _access_type{ access_type } {
}


ArrayAtPutNode::ArrayAtPutNode( AccessType access_type, PseudoRegister *array, PseudoRegister *index, bool smi_index, PseudoRegister *element, bool smi_element, PseudoRegister *result, PseudoRegister *error, std::int32_t data_offset, std::int32_t length_offset, bool needs_store_check ) :
    AbstractArrayAtPutNode( array, index, smi_index, element, result, error, data_offset, length_offset ),
    _access_type{ access_type },
    _needs_store_check{ needs_store_check },
    _smi_element{ smi_element },
    _needs_element_range_check{ false } {
    _needs_element_range_check = ( access_type == byte_at_put or access_type == double_byte_at_put );
}


TypeTestNode::TypeTestNode( PseudoRegister *rr, GrowableArray<KlassOop> *classes, bool hasUnknown ) :
//_src{rr},
    _classes{ classes },
    _hasUnknown{ hasUnknown } {

    _src        = rr;
    _classes    = classes;
    _hasUnknown = hasUnknown;
    std::int32_t len = classes->length();
    st_assert( len > 0, "should have at least one class to test" );

    // The assertion below has been replaced by a warning since sometimes Inliner::inlineMerge(...) creates such a TypeTestNode.
    // FIX THIS
    // st_assert( (len > 1) or hasUnknown, "TypeTestNode is not necessary" );
    if ( ( len == 1 ) and not hasUnknown ) {
        SPDLOG_WARN( "TypeTestNode with only one klass & no uncommon case => performance bug" );
    }

    //
    for ( std::size_t i = 0; i < len; i++ ) {
        for ( std::int32_t j = i + 1; j < len; j++ ) {
            st_assert( classes->at( i ) not_eq classes->at( j ), "duplicate class" );
        }
    }

}


RegisterRegisterArithmeticNode::RegisterRegisterArithmeticNode( ArithOpCode op, PseudoRegister *arg1, PseudoRegister *arg2, PseudoRegister *dst ) :
    ArithmeticNode( op, arg1, dst ),
    _oper{ arg2 },
    _operUse{ nullptr } {

    if ( _src->isConstPseudoRegister() and ArithOpIsCommutative[ static_cast<std::int32_t>( _op ) ] ) { // is this _op or op ? XXX ???
        // make sure that if there's a constant argument, it's the 2nd one
        PseudoRegister *t1 = _src;
        _src  = _oper;
        _oper = t1;
    }
}


TArithRRNode::TArithRRNode( ArithOpCode op, PseudoRegister *arg1, PseudoRegister *arg2, PseudoRegister *dst, bool arg1IsInt, bool arg2IsInt ) :

    _op{ op },
    _oper{ arg2 },
    _operUse{ nullptr },
    _arg1IsInt{ arg1IsInt },
    _arg2IsInt{ arg2IsInt },
    _constResult{ nullptr } {

    if ( arg1->isConstPseudoRegister() and ArithOpIsCommutative[ static_cast<std::int32_t>( op ) ] ) {
        // make sure that if there's a constant argument, it's the 2nd one
        PseudoRegister *t1 = arg1;
        arg1 = arg2;
        arg2 = t1;
        bool t2 = arg1IsInt;
        arg1IsInt = arg2IsInt;
        arg2IsInt = t2;
    }

    _src           = arg1;
    _dest          = dst;
    _dontEliminate = true;

    // don't eliminate even if result unused because primitive might fail
}


PseudoRegister *NonTrivialNode::dest() const {
    if ( not hasDest() ) st_fatal( "has no dest" );
    return _dest;
}


void NonTrivialNode::setDest( BasicBlock *bb, PseudoRegister *d ) {
    // bb == nullptr means don't update definitions
    if ( not hasDest() ) {
        st_fatal( "has no dest" );
    }

    st_assert( bb or not _destDef, "shouldn't have a def" );
    if ( _destDef ) {
        _dest->removeDef( bb, _destDef );
    }

    _dest = d;
    if ( bb ) {
        _destDef = _dest->addDef( bb, (NonTrivialNode *) this );
    }
}


PseudoRegister *NonTrivialNode::src() const {
    if ( not hasSrc() ) st_fatal( "has no src" );
    return _src;
}


bool AssignNode::isAccessingFloats() const {
    // After building the node data structure, float pseudoRegisters have a float location but
    // later during compilation, this location is transformed into a stack location,
    // therefore the two tests. This should change at some point; it would be cleaner
    // to have a special FloatPseudoRegisters (or a flag in the PseudoRegister, respectively).
    return _src->_location.isFloatLocation() or _src->_location == Location::TOP_OF_FLOAT_STACK or Mapping::isFloatTemporary( _src->_location ) or _dest->_location.isFloatLocation() or _dest->_location == Location::TOP_OF_FLOAT_STACK or Mapping::isFloatTemporary( _dest->_location );
}


Oop AssignNode::constantSrc() const {
    st_assert( hasConstantSrc(), "no constant src" );
    return ( (ConstPseudoRegister *) _src )->constant;
}


bool AssignNode::canBeEliminated() const {
    return not( _src->_location.isTopOfStack() or _dest->_location.isTopOfStack() );
}


bool Node::endsBasicBlock() const {
    return next() == nullptr or next()->newBasicBlock();
}


void Node::removeMe() {
    st_assert( hasSingleSuccessor() and hasSinglePredecessor(), "subclass" );
    if ( _prev )
        _prev->moveNext( this, _next );
    if ( _next )
        _next->movePrev( this, _prev );
    _prev = _next = nullptr;
}


void Node::removePrev( Node *n ) {
    /* cut the _prev link between this and n	*/
    st_assert( hasSinglePredecessor(), "subclass" );
    st_assert( _prev == n, "should be same" );
    _prev = nullptr;
}


void Node::removeNext( Node *n ) {
    /* cut the next link between this and n */
    st_assert( hasSingleSuccessor(), "subclass" );
    st_assert( _next == n, "should be same" );
    n->removePrev( this );
    _next = nullptr;
}


Node *Node::endOfList() const {

    if ( _next == nullptr )
        return const_cast<Node *> (this);

    Node *n = _next;
    for ( ; n->_next; n = n->_next ) {
        st_assert( n->hasSingleSuccessor(), ">1 successors" );
    }

    return n;
}


void AbstractMergeNode::removeMe() {
    if ( hasSinglePredecessor() ) {
        _prev = firstPrev();
        TrivialNode::removeMe();
    } else {
        st_fatal( "not implemented yet" );
    }
}


void AbstractMergeNode::movePrev( Node *from, Node *to ) {
    for ( std::size_t i = _prevs->length() - 1; i >= 0; i-- ) {
        if ( _prevs->at( i ) == from ) {
            _prevs->at_put( i, to );
            return;
        }
    }
    st_fatal( "from not found" );
}


bool AbstractMergeNode::isPredecessor( const Node *n ) const {
    for ( std::size_t i = _prevs->length() - 1; i >= 0; i-- ) {
        if ( _prevs->at( i ) == n )
            return true;
    }
    return false;
}


void AbstractBranchNode::removeMe() {
    if ( hasSingleSuccessor() ) {
        if ( not _next and _nxt->nonEmpty() ) {
            _next = next1();
            _nxt->pop();
        }
        NonTrivialNode::removeMe();
    } else {
        st_fatal( "not implemented yet" );
    }
}


void AbstractBranchNode::removeNext( Node *n ) {
    /* cut the next link between this and n */
    if ( n == _next ) {
        n->removePrev( this );
        _next = nullptr;
    } else {
        std::int32_t i = 0;
        for ( ; i < _nxt->length() and _nxt->at( i ) not_eq n; i++ );
        st_assert( i < _nxt->length(), "not found" );
        n->removePrev( this );
        for ( ; i < _nxt->length() - 1; i++ )
            _nxt->at_put( i, _nxt->at( i + 1 ) );
        _nxt->pop();
    }
}


void AbstractBranchNode::setNext( std::int32_t i, Node *n ) {
    if ( i == 0 ) {
        setNext( n );
    } else {
        st_assert( _nxt->length() == i - 1, "wrong index" );
        _nxt->append( n );
    }
}


void AbstractBranchNode::moveNext( Node *from, Node *to ) {
    if ( _next == from ) {
        _next = to;
    } else {
        std::int32_t i = 0;
        for ( ; i < _nxt->length() and _nxt->at( i ) not_eq from; i++ );
        st_assert( i < _nxt->length(), "not found" );
        _nxt->at_put( i, to );
    }
}


bool AbstractBranchNode::isSuccessor( const Node *n ) const {
    if ( _next == n ) {
        return true;
    } else {
        std::int32_t i = 0;
        for ( ; i < _nxt->length() and _nxt->at( i ) not_eq n; i++ );
        return i < _nxt->length();
    }
}


BasicBlock *BasicNode::newBasicBlock() {
    if ( _basicBlock )
        return _basicBlock;

    std::int32_t len = 0;
    _basicBlock = new BasicBlock( (Node *)
                                      this, (Node *)
                                      this, 1 );

    Node *n{ nullptr };

    for ( n = (Node *) this; not n->endsBasicBlock() and n->next() not_eq nullptr;
          n = n->next() ) {
        n->_num        = len++;
        n->_basicBlock = _basicBlock;
    }

    n->_num                 = len++;
    n->_basicBlock          = _basicBlock;
    _basicBlock->_last      = n;
    _basicBlock->_nodeCount = len;

    return _basicBlock;
}


MergeNode::MergeNode( Node *prev1, Node *prev2 ) :
    AbstractMergeNode( prev1, prev2 ),
    _isLoopStart{ false },
    _isLoopEnd{ false },
    _didStartBasicBlock{ false } {
    _byteCodeIndex = max( prev1->byteCodeIndex(), prev2->byteCodeIndex() );
}


MergeNode::MergeNode( std::int32_t byteCodeIndex ) :
//    _byteCodeIndex{byteCodeIndex},
    _isLoopStart{ false },
    _isLoopEnd{ false },
    _didStartBasicBlock{ false } {

    _byteCodeIndex = byteCodeIndex;

}


BasicBlock *MergeNode::newBasicBlock() {
    if ( _basicBlock != nullptr ) {
        return _basicBlock;
    }

    // receiver starts a new BasicBlock
    _didStartBasicBlock = true;
    _basicBlock         = Node::newBasicBlock();

    return _basicBlock;
}


ReturnNode::ReturnNode( PseudoRegister *res, std::int32_t byteCodeIndex ) :
    AbstractReturnNode( byteCodeIndex, res, new TemporaryPseudoRegister( theCompiler->currentScope(), resultLoc, true, true ) ),
    _resultUse{ nullptr } {
    st_assert( res->_location == resultLoc, "must be in special location" );
}


NonLocalReturnSetupNode::NonLocalReturnSetupNode( PseudoRegister *result, std::int32_t byteCodeIndex ) :
    AbstractReturnNode( byteCodeIndex, result, new TemporaryPseudoRegister( theCompiler->currentScope(), resultLoc, true, true ) ),
    _resultUse{ nullptr },
    _contextUse{ nullptr } {
    st_assert( result->_location == NonLocalReturnResultLoc, "must be in special location" );
}


MergeNode *CallNode::nlrTestPoint() const {
    if ( nSuccessors() > 1 ) {
        st_assert( next1()->isMergeNode(), "should be a merge" );
        return (MergeNode *) next1();
    } else {
        return nullptr;
    }
}


CallNode::CallNode( MergeNode *n, GrowableArray<PseudoRegister *> *a, GrowableArray<PseudoRegister *> *e ) :

    exprStack{ e },
    argUses{ nullptr },
    uplevelUsed{ nullptr },
    uplevelDefd{ nullptr },
    uplevelUses{ nullptr },
    uplevelDefs{ nullptr },
    args{ a },
    nblocks{ 0 } {

    //
    if ( n not_eq nullptr ) {
        append1( n );
    }

    _dest   = new SinglyAssignedPseudoRegister( scope(), resultLoc, false, false, _byteCodeIndex, _byteCodeIndex );
    nblocks = theCompiler->blockClosures->length();
}


SendNode::SendNode( LookupKey *key, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack, bool superSend, SendInfo *info ) :
    CallNode( nlrTestPoint, args, expr_stack ),
    _key{ key },
    _superSend{ superSend },
    _info{ info } {

    //
    st_assert( exprStack, "should have expr stack" );
    // Fix this when compiler is more flexible not a fatal because it could happen for super sends that fail (no super method found)

    if ( _superSend and not UseNewBackend ) {
        SPDLOG_WARN( "We cannot yet have super sends in nativeMethods" );
    }

}


ContextCreateNode::ContextCreateNode( PseudoRegister *parent, PseudoRegister *context, std::int32_t nofTemps, GrowableArray<PseudoRegister *> *expr_stack ) :
    PrimitiveNode( Primitives::context_allocate(), nullptr, nullptr, expr_stack ),
    _nofTemps{ nofTemps },
    _contextSize{ 0 },
    _contextNo{ 0 },
    _parentContexts{ nullptr },
    _parentContextUses{ nullptr } {

    _src  = parent;
    _dest = context;

    Scope          *p           = _scope->parent();
    PseudoRegister *prevContext = parent;
    // collect all parent contexts

    while ( p and p->isInlinedScope() and ( (InlinedScope *) p )->context() ) {
        PseudoRegister *c = ( (InlinedScope *) p )->context();
        if ( c not_eq prevContext ) {
            if ( not _parentContexts ) {
                _parentContexts = new GrowableArray<PseudoRegister *>( 5 );
            }
            _parentContexts->append( c );
            prevContext = c;
        }

        p = p->parent();
    }
}


ContextCreateNode::ContextCreateNode( PseudoRegister *b, const ContextCreateNode *n, GrowableArray<PseudoRegister *> *expr_stack ) :
    PrimitiveNode( Primitives::context_allocate(), nullptr, nullptr, expr_stack ),
//    _dest{ nullptr },
    _nofTemps{ n->_nofTemps },
    _contextSize{ 0 },
    _contextNo{ 0 },
    _parentContexts{ nullptr },
    _parentContextUses{ nullptr } {

    SPDLOG_WARN( "check this implementation" );
    Unimplemented();
    // Urs, don't we need a source here?
    // I've added hasSrc() (= true) to ContextCreateNode) - should double check this
    // What is this constructor good for? Cloning only? src should be taken care of as well, I guess.
    // This constructor would be called only for splitting (when copying the node) -- won't happen for now.
    _dest              = b;
    _nofTemps          = n->_nofTemps;
    _parentContextUses = nullptr;
}


ContextInitNode::ContextInitNode( ContextCreateNode *creator ) :
    _initializers{ nullptr },
    _contentDefs{ nullptr },
    _initializerUses{ nullptr },
    _materializedBlocks{ nullptr } {

    std::int32_t nofTemps = creator->nofTemps();
    _src = creator->context();
    st_assert( _src, "must have context" );
    _initializers = new GrowableArray<Expression *>( nofTemps, nofTemps, nullptr );    // holds initializer for each element (or nullptr)
}


ContextInitNode::ContextInitNode( PseudoRegister *b, const ContextInitNode *node ) :
    _initializers{ node->_initializers },
    _contentDefs{ nullptr },
    _initializerUses{ nullptr },
    _materializedBlocks{ nullptr } {
    _src = b;
    st_assert( _src, "must have context" );
    st_assert( ( node->_contentDefs == nullptr ) and ( node->_initializerUses == nullptr ), "shouldn't copy after uses have been built" );
}


BlockCreateNode::BlockCreateNode( BlockPseudoRegister *b, GrowableArray<PseudoRegister *> *expr_stack ) :
    PrimitiveNode( Primitives::block_allocate(), nullptr, nullptr, expr_stack ),
    _context{ nullptr },
    _contextUse{ nullptr } {

    _src  = nullptr;
    _dest = b;

    switch ( b->method()->block_info() ) {
        case MethodOopDescriptor::expects_nil:        // no context needed
            _context = nullptr;
            break;
        case MethodOopDescriptor::expects_self:
            _context = b->scope()->self()->pseudoRegister();
            break;
        case MethodOopDescriptor::expects_parameter:    // fix this -- should find which
            _context = nullptr;
            break;
        case MethodOopDescriptor::expects_context:
            _context = b->scope()->context();
            break;
        default: st_fatal( "unexpected incoming info" );
    };
}


std::int32_t ContextInitNode::positionOfContextTemp( std::int32_t n ) const {
    // return position of ith context temp in compiled (physical) context
    std::int32_t pos = 0;

    for ( std::size_t i = 0; i < n; i++ ) {
        PseudoRegister *p = contents()->at( i )->pseudoRegister();
        if ( p->_location.isContextLocation() )
            pos++;
    }
    return pos;
}


void ContextInitNode::initialize( std::int32_t no, Expression *expr ) {
    st_assert( ( _initializers->at( no ) == nullptr ) or ( _initializers->at( no )->constant() == nilObject ), "already initialized this context element" );
    _initializers->at_put( no, expr );
}


ContextCreateNode *ContextInitNode::creator() const {
    // returns the corresponding context creation node
    Node *n = _prev;
    st_assert( n->isContextCreateNode(), "must be creator node" );
    return (ContextCreateNode *) n;
}


void ContextInitNode::addBlockMaterializer( BlockMaterializeNode *n ) {
    if ( not _materializedBlocks )
        _materializedBlocks = new GrowableArray<BlockMaterializeNode *>( 5 );
    _materializedBlocks->append( n );
}


void ContextInitNode::notifyNoContext() {
    // our context has been optimized away, i.e., all context contents
    // will be stack-allocated like normal PseudoRegisters
    // remove the context use (otherwise the contextPR has 1 use and no definitions)
    _src->removeUse( bb(), _srcUse );
    _src = nullptr;
    if ( _materializedBlocks ) {
        for ( std::size_t i = _materializedBlocks->length() - 1; i >= 0; i-- ) {
            // remove the block materialization node
            BlockMaterializeNode *n = _materializedBlocks->at( i );
            n->eliminate( n->bb(), nullptr, true, false );
            PseudoRegister *blk = n->src();
            st_assert( blk->isBlockPseudoRegister(), "must be a block" );

            // remove use of block
            for ( std::int32_t j = _initializers->length() - 1; j >= 0; j-- ) {
                if ( _initializers->at( j )->pseudoRegister() == blk ) {
                    blk->removeUse( _basicBlock, _initializerUses->at( j ) );
                    break;
                }
            }

            // try to eliminate the block, too
            if ( blk->hasNoUses() ) {
                // eliminate the block
                blk->eliminate( false );
            }
        }
    }
}


PrimitiveNode::PrimitiveNode( PrimitiveDescriptor *pdesc, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack ) :
    CallNode( nlrTestPoint, args, expr_stack ),
    _pdesc{ pdesc } {

    st_assert( _pdesc->can_perform_NonLocalReturn() or ( nlrTestPoint == nullptr ), "no NonLocalReturn target needed" );
    if ( pdesc->can_invoke_delta() ) {
        st_assert( expr_stack not_eq nullptr, "should have expr stack" );
    } else {
        // the expression stack is only needed if the primitive can walk the
        // stack (then the elements will be debug-visible) or if the primitive
        // can scavenge (then the elems must be allocated to GCable regs)
        exprStack = nullptr;
    }
}


InlinedPrimitiveNode::InlinedPrimitiveNode( Operation op, PseudoRegister *result, PseudoRegister *error, PseudoRegister *recv, PseudoRegister *arg1, bool arg1_is_SmallInteger, PseudoRegister *arg2, bool arg2_is_SmallInteger ) :
//    _dest{ result },
//    _src{ recv },
    _arg1{ arg1 },
    _arg2{ arg2 },
    _error{ error },
    _arg1_use{ nullptr },
    _arg2_use{ nullptr },
    _error_def{ nullptr },
    _arg1_is_SmallInteger{ arg1_is_SmallInteger },
    _arg2_is_SmallInteger{ arg2_is_SmallInteger },
    _operation{ op } {

    _dest = result;
    _src  = recv;

}


bool InlinedPrimitiveNode::canFail() const {

    switch ( op() ) {
        case Operation::OBJ_KLASS:
            return false;
        case Operation::OBJ_HASH:
            return false;
        case Operation::PROXY_BYTE_AT:
            return not arg1_is_SmallInteger();
        case Operation::PROXY_BYTE_AT_PUT:
            return not arg1_is_SmallInteger() or not arg2_is_SmallInteger();
        default:
            return false;
    };

    ShouldNotReachHere();
    return false;
}


bool InlinedPrimitiveNode::canBeEliminated() const {

    switch ( op() ) {
        case Operation::OBJ_KLASS:
            return true;
        case Operation::OBJ_HASH:
            return true;
        case Operation::PROXY_BYTE_AT:
            return not canFail();
        case Operation::PROXY_BYTE_AT_PUT:
            return false;
        default:
            return false;
    };

    ShouldNotReachHere();
    return false;
}


UncommonNode::UncommonNode( GrowableArray<PseudoRegister *> *e, std::int32_t byteCodeIndex ) :
    exprStack{ e } {
    _byteCodeIndex = byteCodeIndex;
}


UncommonSendNode::UncommonSendNode( GrowableArray<PseudoRegister *> *e, std::int32_t byteCodeIndex, std::int32_t argCount ) :
    UncommonNode( e, byteCodeIndex ),
    _argCount{ argCount } {
}


Node *UncommonSendNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    return NodeFactory::UncommonSendNode( this->expressionStack()->copy(), byteCodeIndex(), _argCount );
}


void UncommonSendNode::makeUses( BasicBlock *bb ) {
    std::int32_t expressionCount = expressionStack()->length();

    for ( std::int32_t pos = expressionCount - _argCount; pos < expressionCount; pos++ ) {
        bb->addUse( this, expressionStack()->at( pos ) );
    }
}


void UncommonSendNode::verify() const {
    if ( _argCount > expressionStack()->length() )
        error( "Too few expressions on stack for 0x{0:x}: required %d, but got %d", this, _argCount, expressionStack()->length() );
    UncommonNode::verify();
}


bool PrimitiveNode::canBeEliminated() const {
    if ( not Node::canBeEliminated() )
        return false;
    if ( _pdesc->can_be_constant_folded() and not canFail() )
        return true;
    // temporary hack -- fix this
    // should test arg types to make sure prim won't fail
    // for now, treat cloning etc. special
    if ( _pdesc->can_be_constant_folded() )
        return true;    // not safe!

    //%TODO these references get replaced with generated versions
    // so these tests will fail - fix this
    if ( _pdesc->fn() == primitiveFunctionType( &BehaviorPrimitives::allocate ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew0 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew1 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew2 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew3 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew4 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew5 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew6 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew7 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew8 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew9 ) ) {
        return true;
    }

    return false;
}


bool PrimitiveNode::canInvokeDelta() const {
    return _pdesc->can_invoke_delta();
}


bool PrimitiveNode::canFail() const {
    return _pdesc->can_fail();
}


DLLNode::DLLNode( SymbolOop dll_name, SymbolOop function_name, dll_func_ptr_t function, bool async, MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack ) :
    CallNode( nlrTestPoint, args, expr_stack ),
    _dll_name{ dll_name },
    _function_name{ function_name },
    _function{ function },
    _async{ async } {

}


bool DLLNode::canInvokeDelta() const {
    return true;        // user-defined DLL code can do anything
}


NonLocalReturnTestNode::NonLocalReturnTestNode( std::int32_t byteCodeIndex ) {
    static_cast<void>(byteCodeIndex); // unused
}


void NonLocalReturnTestNode::fixup() {
    // connect next() and next1()
    if ( scope()->isTop() ) {
        theCompiler->enterScope( scope() ); // so that nodes get right scope
        // Non-inlined scope: continuation just continues NonLocalReturn
        append( 0, NodeFactory::NonLocalReturnContinuationNode( byteCodeIndex() ) );
        // return point returns the NonLocalReturn result
        PseudoRegister *nlr_result = new TemporaryPseudoRegister( scope(), Location::RESULT_OF_NON_LOCAL_RETURN, true, true );
        Node           *n          = NodeFactory::createAndRegisterNode<AssignNode>( nlr_result, scope()->resultPR );
        append( 1, n );
        n->append( scope()->returnPoint() );
        theCompiler->exitScope( scope() );
    } else {
        // Inlined scope: for continuation, follow caller chain until a nlrTestPoint is found
        // (the top scope is guaranteed to have a nlrTestPoint, so loop below will terminate correctly)
        // introduce an extra assignment to satisfy new backend, will be optimized away
        InlinedScope *s = scope()->sender();
        while ( not s->has_nlrTestPoint() )
            s = s->sender();
        append( 0, s->nlrTestPoint() );
        // now connect to return point and return the NonLocalReturn value
        // s may not have a return point, so search for one
        s     = scope();
        while ( s->returnPoint() == nullptr )
            s = s->sender();
        theCompiler->enterScope( s ); // so that node gets right scope
        PseudoRegister *nlr_result = new TemporaryPseudoRegister( scope(), Location::RESULT_OF_NON_LOCAL_RETURN, true, true );
        Node           *n          = NodeFactory::createAndRegisterNode<AssignNode>( nlr_result, s->resultPR );
        theCompiler->exitScope( s );
        append( 1, n );
        n->append( s->returnPoint() );
    }
}


bool SendNode::isCounting() const {
    return _info->_counting;
}


bool SendNode::isUninlinable() const {
    return _info->uninlinable;
}


bool SendNode::staticReceiver() const {
    return _info->_receiverStatic;
}


PseudoRegister *SendNode::recv() const {
    std::int32_t i = args->length() - 1;
    while ( i >= 0 and args->at( i )->_location not_eq receiverLoc ) {
        i--;
    }
    st_assert( i >= 0, "must have a receiver" );
    return args->at( i );
}


// ==================================================================================
// cloning: copy the node during splitting; returning nullptr means node is a de facto
// no-op only need to copy the basic state; definitions, uses etc haven't yet been computed
// ==================================================================================

// general splitting hasn't been implemented yet; only nodes that shouldCopyWhenSplitting()
// are currently copied  -Urs 5/96

Node *BasicNode::copy( PseudoRegister *from, PseudoRegister *to ) const {
    Node *c = clone( from, to );
    if ( c ) {
        c->_scope         = _scope;
        c->_byteCodeIndex = _byteCodeIndex;
    }
    return c;
}


#define SHOULD_NOT_CLONE    { ShouldNotCallThis(); return nullptr; }
#define TRANSLATE( s )        ((s == from) ? to : s)


Node *PrologueNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    SHOULD_NOT_CLONE
}


Node *NonLocalReturnSetupNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    SHOULD_NOT_CLONE
}


Node *NonLocalReturnContinuationNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    SHOULD_NOT_CLONE
}


Node *ReturnNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    SHOULD_NOT_CLONE
}


Node *BranchNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    SHOULD_NOT_CLONE
}


Node *TypeTestNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    SHOULD_NOT_CLONE
}


Node *FixedCodeNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    SHOULD_NOT_CLONE
}


Node *LoadOffsetNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<LoadOffsetNode>( TRANSLATE( _dest ), _src, _offset, _isArraySize );
}


Node *LoadIntNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<LoadIntNode>( TRANSLATE( _dest ), _value );
}


Node *LoadUplevelNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<LoadUplevelNode>( TRANSLATE( _dest ), TRANSLATE( _context0 ), _nofLevels, _offset, _name );
}


Node *StoreOffsetNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<StoreOffsetNode>( TRANSLATE( _src ), _base, _offset, _needsStoreCheck );
}


Node *StoreUplevelNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<StoreUplevelNode>( TRANSLATE( _src ), TRANSLATE( _context0 ), _nofLevels, _offset, _name, _needsStoreCheck );
}


Node *AssignNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<AssignNode>( TRANSLATE( _src ), TRANSLATE( _dest ) );
}


Node *RegisterRegisterArithmeticNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<RegisterRegisterArithmeticNode>( _op, TRANSLATE( _src ), _oper, TRANSLATE( _dest ) );
}


Node *TArithRRNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<TArithRRNode>( _op, TRANSLATE( _src ), _oper, TRANSLATE( _dest ), _arg1IsInt, _arg2IsInt );
}


Node *ArithRCNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    return NodeFactory::createAndRegisterNode<ArithRCNode>( _op, TRANSLATE( _src ), _operand, TRANSLATE( _dest ) );
}


Node *SendNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    // NB: use current split signature, not the receiver's sig!
    SendNode *n = NodeFactory::SendNode( _key, nlrTestPoint(), args, exprStack, _superSend, _info );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node *PrimitiveNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(to); // unused
    // NB: use scope's current sig, not the receiver's sig!
    PrimitiveNode *n = NodeFactory::PrimitiveNode( _pdesc, nlrTestPoint(), args, exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node *DLLNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(to); // unused
    // NB: use scope's current sig, not the receiver's sig!
    DLLNode *n = NodeFactory::DLLNode( _dll_name, _function_name, _function, _async, nlrTestPoint(), args, exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node *InterruptCheckNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(to); // unused
    // NB: use scope's current sig, not the receiver's sig!
    InterruptCheckNode *n = NodeFactory::createAndRegisterNode<InterruptCheckNode>( exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node *BlockCreateNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    // NB: use scope's current sig, not the receiver's sig!
    BlockCreateNode *n = NodeFactory::createAndRegisterNode<BlockCreateNode>( (BlockPseudoRegister *) TRANSLATE( block() ), exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node *BlockMaterializeNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    // NB: use scope's current sig, not the receiver's sig!
    BlockMaterializeNode *n = NodeFactory::createAndRegisterNode<BlockMaterializeNode>( (BlockPseudoRegister *) TRANSLATE( block() ), exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node *ContextCreateNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    return NodeFactory::createAndRegisterNode<ContextCreateNode>( TRANSLATE( _dest ), this, exprStack );
}


Node *ContextInitNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    return NodeFactory::createAndRegisterNode<ContextInitNode>( TRANSLATE( _src ), this );
}


Node *ContextZapNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    return NodeFactory::createAndRegisterNode<ContextZapNode>( TRANSLATE( _src ) );
}


Node *NonLocalReturnTestNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    Unimplemented();
    return nullptr;
}


Node *ArrayAtNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    return NodeFactory::createAndRegisterNode<ArrayAtNode>( _access_type, TRANSLATE( _src ), TRANSLATE( _arg ), _intArg, TRANSLATE( _dest ), TRANSLATE( _error ), _dataOffset, _sizeOffset );
}


Node *ArrayAtPutNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    return NodeFactory::createAndRegisterNode<ArrayAtPutNode>( _access_type, TRANSLATE( _src ), TRANSLATE( _arg ), _intArg, TRANSLATE( elem ), _smi_element, TRANSLATE( _dest ), TRANSLATE( _error ), _dataOffset, _sizeOffset, _needs_store_check );
}


Node *UncommonNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    return NodeFactory::UncommonNode( exprStack, _byteCodeIndex );
}


Node *InlinedReturnNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    return NodeFactory::createAndRegisterNode<InlinedReturnNode>( byteCodeIndex(), TRANSLATE( src() ), TRANSLATE( dest() ) );
}


#define NO_NEED_TO_COPY    { return nullptr; }


Node *MergeNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    NO_NEED_TO_COPY
}


Node *NopNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    NO_NEED_TO_COPY
}


Node *CommentNode::clone( PseudoRegister *from, PseudoRegister *to ) const {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    NO_NEED_TO_COPY
}


// ==================================================================================
// makeUses: construct all uses and definitions
// ==================================================================================

void PrologueNode::makeUses( BasicBlock *bb ) {
    InlinedScope *s = scope();

    // build initial def for self and context (for blocks only)
    bb->addDef( this, s->self()->pseudoRegister() );
    if ( s->isBlockScope() ) {
        bb->addDef( this, s->context() );
    }
    // build initial definitions for incoming args

    for ( std::size_t i = 0; i < _nofArgs; i++ ) {
        Expression *a = s->argument( i );
        if ( a )
            bb->addDef( this, a->pseudoRegister() );
    }

    // build initial definitions for locals (initalization to nil)
    for ( std::size_t i = 0; i < _nofTemps; i++ ) {
        Expression *t = s->temporary( i );
        if ( t )
            bb->addDef( this, t->pseudoRegister() );
    }
}


void NonTrivialNode::makeUses( BasicBlock *bb ) {
    _basicBlock = bb;
    st_assert( not hasSrc() or _srcUse, "should have srcUse" );
    st_assert( not hasDest() or _destDef or _dest->isNoPseudoRegister(), "should have destDef" );
}


void LoadNode::makeUses( BasicBlock *basicBlock ) {
    _destDef = basicBlock->addDef( this, _dest );
    NonTrivialNode::makeUses( basicBlock );
}


void LoadOffsetNode::makeUses( BasicBlock *bb ) {
    _srcUse = bb->addUse( this, _src );
    LoadNode::makeUses( bb );
}


void LoadUplevelNode::makeUses( BasicBlock *bb ) {
    _context0Use = bb->addUse( this, _context0 );
    LoadNode::makeUses( bb );
}


void StoreNode::makeUses( BasicBlock *bb ) {
    _srcUse = bb->addUse( this, _src );
    NonTrivialNode::makeUses( bb );
}


void StoreOffsetNode::makeUses( BasicBlock *bb ) {
    _baseUse = bb->addUse( this, _base );
    StoreNode::makeUses( bb );
}


void StoreUplevelNode::makeUses( BasicBlock *bb ) {
    _context0Use = bb->addUse( this, _context0 );
    StoreNode::makeUses( bb );
}


void AssignNode::makeUses( BasicBlock *bb ) {
    _destDef = bb->addDef( this, _dest );
    StoreNode::makeUses( bb );
}


void AbstractReturnNode::makeUses( BasicBlock *bb ) {
    _srcUse  = bb->addUse( this, _src );
    _destDef = bb->addDef( this, _dest );
    NonTrivialNode::makeUses( bb );
}


void ReturnNode::makeUses( BasicBlock *bb ) {
    // _resultUse models the value's use in the caller
    _resultUse = bb->addUse( this, _dest );
    AbstractReturnNode::makeUses( bb );
}


void NonLocalReturnSetupNode::makeUses( BasicBlock *bb ) {
    // has no src or dest -- uses hardwired NonLocalReturn register but track them anyway for consistency
    // _resultUse models the value's use in the caller
    _resultUse  = bb->addUse( this, _dest );
    _contextUse = bb->addUse( this, _scope->context() );
    AbstractReturnNode::makeUses( bb );
}


void NonLocalReturnTestNode::makeUses( BasicBlock *bb ) {
    AbstractBranchNode::makeUses( bb );
}


void NonLocalReturnContinuationNode::makeUses( BasicBlock *bb ) {
    // has no src or dest -- uses hardwired NonLocalReturn register but track them anyway for consistency
    AbstractReturnNode::makeUses( bb );
}


void ArithmeticNode::makeUses( BasicBlock *bb ) {
    _srcUse  = bb->addUse( this, _src );
    _destDef = bb->addDef( this, _dest );
    NonTrivialNode::makeUses( bb );
}


void RegisterRegisterArithmeticNode::makeUses( BasicBlock *bb ) {
    _operUse = bb->addUse( this, _oper );
    ArithmeticNode::makeUses( bb );
}


void TArithRRNode::makeUses( BasicBlock *bb ) {
    _operUse = bb->addUse( this, _oper );
    _srcUse  = bb->addUse( this, _src );
    _destDef = bb->addDef( this, _dest );
    NonTrivialNode::makeUses( bb );
}


void CallNode::makeUses( BasicBlock *bb ) {

    //
    _destDef = bb->addDef( this, _dest );
    if ( args ) {
        std::int32_t len = args->length();
        argUses          = new GrowableArray<Usage *>( len );
        for ( std::size_t i = 0; i < len; i++ ) {
            argUses->append( bb->addUse( this, args->at( i ) ) );
        }
    }
    NonTrivialNode::makeUses( bb );

    //
    if ( not canInvokeDelta() ) return;

    // add definitions/uses for all PseudoRegisters uplevel-accessed by live blocks
    const std::int32_t InitialSize = 5;
    uplevelUses = new GrowableArray<Usage *>( InitialSize );
    uplevelDefs = new GrowableArray<Definition *>( InitialSize );
    uplevelUsed = new GrowableArray<PseudoRegister *>( InitialSize );
    uplevelDefd = new GrowableArray<PseudoRegister *>( InitialSize );
    GrowableArray<BlockPseudoRegister *> *blks = theCompiler->blockClosures;
    for ( std::int32_t                   i1    = 0; i1 < nblocks; i1++ ) {
        BlockPseudoRegister *blk = blks->at( i1 );
        if ( !blk->escapes() ) continue;

        // check if block's home is on current call stack; if not, we don't care about the block's
        // uplevel accesses since the block is non-LIFO and we won't access its context anyway
        Scope *home = blk->scope()->home();
        if ( !home->isSenderOrSame( scope() ) ) continue;

        // ok, this block is live
        GrowableArray<PseudoRegister *> *uplevelRead    = blk->uplevelRead();
        std::int32_t                    j               = uplevelRead->length() - 1;
        for ( ; j >= 0; j-- ) {
            PseudoRegister *r = uplevelRead->at( j );
            uplevelUses->append( bb->addUse( this, r ) );
            uplevelUsed->append( r );
        }
        GrowableArray<PseudoRegister *> *uplevelWritten = blk->uplevelWritten();
        for ( j = uplevelWritten->length() - 1; j >= 0; j-- ) {
            PseudoRegister *r = uplevelWritten->at( j );
            uplevelDefs->append( bb->addDef( this, r ) );
            uplevelDefd->append( r );
        }
    }
}


void BlockCreateNode::makeUses( BasicBlock *bb ) {
    if ( _context and not isMemoized() ) {
        _contextUse = bb->addUse( this, _context );
    }
    _destDef = bb->addDef( this, _dest );
    NonTrivialNode::makeUses( bb );
}


void BlockMaterializeNode::makeUses( BasicBlock *bb ) {
    // without memoization, BlockMaterializeNode is a no-op
    if ( isMemoized() ) {
        _srcUse         = bb->addUse( this, _src );
        if ( _context )
            _contextUse = bb->addUse( this, _context );
        BlockCreateNode::makeUses( bb );
    }
}


void ContextCreateNode::makeUses( BasicBlock *bb ) {
    if ( _src )
        _srcUse = bb->addUse( this, _src ); // no src if there's no incoming context
    _destDef    = bb->addDef( this, _dest );
    if ( _parentContexts ) {
        std::int32_t len   = _parentContexts->length();
        _parentContextUses = new GrowableArray<Usage *>( len, len, nullptr );
        for ( std::size_t i = _parentContexts->length() - 1; i >= 0; i-- ) {
            Usage *u = bb->addUse( this, _parentContexts->at( i ) );
            _parentContextUses->at_put( i, u );
        }
    }
    NonTrivialNode::makeUses( bb );
}


void ContextInitNode::makeUses( BasicBlock *bb ) {
    _srcUse = bb->addUse( this, _src );
    std::int32_t i = nofTemps();
    _contentDefs     = new GrowableArray<Definition *>( i, i, nullptr );
    _initializerUses = new GrowableArray<Usage *>( i, i, nullptr );
    while ( i-- > 0 ) {
        PseudoRegister *r = contents()->at( i )->pseudoRegister();
        if ( r->isBlockPseudoRegister() ) {
            _contentDefs->at_put( i, nullptr );      // there is no assignment to the block
        } else {
            _contentDefs->at_put( i, bb->addDef( this, r ) );
        }
        _initializerUses->at_put( i, bb->addUse( this, _initializers->at( i )->pseudoRegister() ) );
    }
    NonTrivialNode::makeUses( bb );
}


void ContextZapNode::makeUses( BasicBlock *bb ) {
    _srcUse = bb->addUse( this, _src );
}


void TypeTestNode::makeUses( BasicBlock *bb ) {
    _srcUse = bb->addUse( this, _src );
    AbstractBranchNode::makeUses( bb );
}


void AbstractArrayAtNode::makeUses( BasicBlock *bb ) {
    _srcUse   = bb->addUse( this, _src );
    _destDef  = _dest ? bb->addDef( this, _dest ) : nullptr;
    _argUse   = bb->addUse( this, _arg );
    _errorDef = ( _error and canFail() ) ? bb->addDef( this, _error ) : nullptr;
    AbstractBranchNode::makeUses( bb );
}


void AbstractArrayAtPutNode::makeUses( BasicBlock *bb ) {
    elemUse = bb->addUse( this, elem );
    AbstractArrayAtNode::makeUses( bb );
}


void InlinedPrimitiveNode::makeUses( BasicBlock *bb ) {
    _srcUse    = _src ? bb->addUse( this, _src ) : nullptr;
    _arg1_use  = _arg1 ? bb->addUse( this, _arg1 ) : nullptr;
    _arg2_use  = _arg2 ? bb->addUse( this, _arg2 ) : nullptr;
    _destDef   = _dest ? bb->addDef( this, _dest ) : nullptr;
    _error_def = ( _error and canFail() ) ? bb->addDef( this, _error ) : nullptr;
    AbstractBranchNode::makeUses( bb );
}

// ==================================================================================
// removeUses: remove all uses and definitions (receiver node has been eliminated)
// ==================================================================================

// removeUses: remove all uses and definitions
void NonTrivialNode::removeUses( BasicBlock *bb ) {
    st_assert( _basicBlock == bb, "wrong BasicBlock" );
}


void LoadNode::removeUses( BasicBlock *bb ) {
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void LoadOffsetNode::removeUses( BasicBlock *bb ) {
    _src->removeUse( bb, _srcUse );
    LoadNode::removeUses( bb );
}


void LoadUplevelNode::removeUses( BasicBlock *bb ) {
    _context0->removeUse( bb, _context0Use );
    LoadNode::removeUses( bb );
}


void StoreNode::removeUses( BasicBlock *bb ) {
    _src->removeUse( bb, _srcUse );
    NonTrivialNode::removeUses( bb );
}


void StoreOffsetNode::removeUses( BasicBlock *bb ) {
    _base->removeUse( bb, _baseUse );
    StoreNode::removeUses( bb );
}


void StoreUplevelNode::removeUses( BasicBlock *bb ) {
    _context0->removeUse( bb, _context0Use );
    StoreNode::removeUses( bb );
}


void AssignNode::removeUses( BasicBlock *bb ) {
    _dest->removeDef( bb, _destDef );
    StoreNode::removeUses( bb );
}


void AbstractReturnNode::removeUses( BasicBlock *bb ) {
    _src->removeUse( bb, _srcUse );
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void ReturnNode::removeUses( BasicBlock *bb ) {
    _dest->removeUse( bb, _resultUse );
    AbstractReturnNode::removeUses( bb );
}


void NonLocalReturnSetupNode::removeUses( BasicBlock *bb ) {
    _dest->removeUse( bb, _resultUse );
    _scope->context()->removeUse( bb, _contextUse );
    AbstractReturnNode::removeUses( bb );
}


void NonLocalReturnTestNode::removeUses( BasicBlock *bb ) {
    AbstractBranchNode::removeUses( bb );
}


void NonLocalReturnContinuationNode::removeUses( BasicBlock *bb ) {
    AbstractReturnNode::removeUses( bb );
}


void ArithmeticNode::removeUses( BasicBlock *bb ) {
    _src->removeUse( bb, _srcUse );
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void RegisterRegisterArithmeticNode::removeUses( BasicBlock *bb ) {
    _oper->removeUse( bb, _operUse );
    ArithmeticNode::removeUses( bb );
}


void TArithRRNode::removeUses( BasicBlock *bb ) {
    _oper->removeUse( bb, _operUse );
    _src->removeUse( bb, _srcUse );
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void CallNode::removeUses( BasicBlock *bb ) {
    _dest->removeDef( bb, _destDef );
    if ( argUses ) {
        std::int32_t       len = args->length();
        for ( std::int32_t i   = 0; i < len; i++ ) {
            args->at( i )->removeUse( bb, argUses->at( i ) );
        }
    }
    if ( uplevelUses ) {
        std::int32_t i = uplevelUses->length() - 1;
        for ( ; i >= 0; i-- )
            uplevelUsed->at( i )->removeUse( bb, uplevelUses->at( i ) );
        for ( std::size_t i = uplevelDefs->length() - 1; i >= 0; i-- )
            uplevelDefd->at( i )->removeDef( bb, uplevelDefs->at( i ) );
    }
    NonTrivialNode::removeUses( bb );
}


void BlockCreateNode::removeUses( BasicBlock *bb ) {
    if ( _contextUse )
        _context->removeUse( bb, _contextUse );
    if ( _src )
        _src->removeUse( bb, _srcUse );
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void BlockMaterializeNode::removeUses( BasicBlock *bb ) {
    // without memoization, BlockMaterializeNode is a no-op
    if ( isMemoized() )
        BlockCreateNode::removeUses( bb );
}


void ContextCreateNode::removeUses( BasicBlock *bb ) {
    if ( _src )
        _src->removeUse( bb, _srcUse ); // no src if there's no incoming context
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void ContextInitNode::removeUses( BasicBlock *bb ) {
    std::int32_t i = nofTemps();
    while ( i-- > 0 ) {
        contents()->at( i )->pseudoRegister()->removeDef( bb, _contentDefs->at( i ) );
        _initializers->at( i )->pseudoRegister()->removeUse( bb, _initializerUses->at( i ) );
    }
    _src->removeUse( bb, _srcUse );
    NonTrivialNode::removeUses( bb );
}


void ContextZapNode::removeUses( BasicBlock *bb ) {
    _src->removeUse( bb, _srcUse );
    NonTrivialNode::removeUses( bb );
}


void TypeTestNode::removeUses( BasicBlock *bb ) {
    _src->removeUse( bb, _srcUse );
    AbstractBranchNode::removeUses( bb );
}


void AbstractArrayAtNode::removeUses( BasicBlock *bb ) {
    _src->removeUse( bb, _srcUse );
    if ( _dest )
        _dest->removeDef( bb, _destDef );
    _arg->removeUse( bb, _argUse );
    if ( _errorDef )
        _error->removeDef( bb, _errorDef );
    AbstractBranchNode::removeUses( bb );
}


void AbstractArrayAtPutNode::removeUses( BasicBlock *bb ) {
    elem->removeUse( bb, elemUse );
    AbstractArrayAtNode::removeUses( bb );
}


void InlinedPrimitiveNode::removeUses( BasicBlock *bb ) {
    if ( _srcUse )
        _src->removeUse( bb, _srcUse );
    if ( _arg1_use )
        _arg1->removeUse( bb, _arg1_use );
    if ( _arg2_use )
        _arg2->removeUse( bb, _arg2_use );
    if ( _destDef )
        _dest->removeDef( bb, _destDef );
    if ( _error_def )
        _error->removeDef( bb, _error_def );
    AbstractBranchNode::removeUses( bb );
}

// ==================================================================================
// eliminate: the receiver has just been removed because its
// result was not used.  Propagate this to all nodes whose use count
// has now become zero, too.  The register passed in is the one
// causing the elimination (passed in to catch recursive elimination
// of the same register).
// It is an error to call eliminate() if canBeEliminated() is false.
// If cp is true, the node is eliminated because of copy propagation.
// If removing is true, we're removing dead code (e.g. a branch of a type
// case that will never be executed)
// ==================================================================================

void Node::eliminate( BasicBlock *bb, PseudoRegister *r, bool removing, bool cp ) {
    static_cast<void>(bb); // unused
    static_cast<void>(r); // unused
    static_cast<void>(cp); // unused

    st_assert( not _deleted, "already deleted this node" );
    if ( CompilerDebug ) {
        char buf[1024];
        cout( PrintEliminateUnnededNodes )->print( "*%s node N%ld: %s\n", removing ? "removing" : "eliminating", id(), toString( buf, PrintHexAddresses ) );
    }
    st_assert( not _dontEliminate or removing, "shouldn't eliminate this node" );
    bb->remove( this );
}


#define CHECK( pseudoRegister, r )                              \
  if (pseudoRegister not_eq r and pseudoRegister->isOnlySoftUsed()) pseudoRegister->eliminate(false);


void LoadNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    st_assert( canBeEliminated() or rem, "cannot be eliminated" );
    NonTrivialNode::eliminate( bb, r, rem, cp );
    CHECK( _dest, r );
}


void LoadOffsetNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    LoadNode::eliminate( bb, r, rem, cp );
    CHECK( _src, r );
}


void LoadUplevelNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    LoadNode::eliminate( bb, r, rem, cp );
    CHECK( _context0, r );
}


void StoreNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    NonTrivialNode::eliminate( bb, r, rem, cp );
    CHECK( _src, r );
}


void StoreOffsetNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    StoreNode::eliminate( bb, r, rem, cp );
    CHECK( _base, r );
}


void StoreUplevelNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    StoreNode::eliminate( bb, r, rem, cp );
    CHECK( _context0, r );
}


void AssignNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    StoreNode::eliminate( bb, r, rem, cp );
    CHECK( _dest, r );
}


void ReturnNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    st_assert( rem, "should not delete except during dead-code elimination" );
    AbstractReturnNode::eliminate( bb, r, rem, cp );
    CHECK( _src, r );
    CHECK( _dest, r );
    // don't need to check resultPR
}


void ArithmeticNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    NonTrivialNode::eliminate( bb, r, rem, cp );
    CHECK( _src, r );
    CHECK( _dest, r );
}


void RegisterRegisterArithmeticNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    ArithmeticNode::eliminate( bb, r, rem, cp );
    CHECK( _oper, r );
}


void BranchNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool removing, bool cp ) {
    if ( _deleted )
        return;
    if ( removing and nSuccessors() <= 1 ) {
        NonTrivialNode::eliminate( bb, r, removing, cp );
    } else {
        // caller has to handle this
        st_fatal( "removing branch node with > 1 successor" );
    }
}


void BlockMaterializeNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    BlockCreateNode::eliminate( bb, r, rem, cp );
}


void BasicNode::removeUpToMerge() {
    BasicBlock *thisBasicBlock = _basicBlock;
    Node       *n              = (Node *)
        this;

    for ( ; n and n->hasSinglePredecessor(); ) {
        while ( n->nSuccessors() > 1 ) {
            std::int32_t i     = n->nSuccessors() - 1;
            Node         *succ = n->next( i );
            succ->removeUpToMerge();
            /* Must removeNext after removeUpToMerge to avoid false removal of MergeNode with 2 predecessors. SLR 08/08 */
            n->removeNext( succ );
        }

        Node *nextn = n->next();
        if ( not n->_deleted )
            n->eliminate( thisBasicBlock, nullptr, true, false );
        if ( nextn ) {
            BasicBlock *nextBasicBlock = nextn->bb();
            if ( nextBasicBlock not_eq thisBasicBlock ) {
                if ( nextn->nPredecessors() >= 2 ) {
                    // also remove n's successor so that we can delete past merges
                    // if all incoming branches of the merges are deleted
                    n->removeNext( nextn );
                    // nextn had at least 2 predecessors, so must stop deleting here
                    // NB: must break here -- if was 2 successors, will now be one
                    // and loop would continue (was a bug)  -Urs 8/94
                    break;
                }
                thisBasicBlock = nextBasicBlock;
            }
        }

        n = nextn;
    }

//    BasicBlock *nextBB = n ? n->bb() : nullptr;
}


void PrimitiveNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    st_assert( rem or canBeEliminated(), "shouldn't call" );
    if ( nlrTestPoint() ) {
        // remove all unused nodes along NonLocalReturn branch
        // should be exactly 2 nodes (see IRGenerator::makeNonLocalReturnPoint())
        MergeNode *n1 = nlrTestPoint();
        Node      *n2 = n1->next();
        st_assert( n2->isNonLocalReturnTestNode(), "unexpected node sequence" );
        _nxt->pop();        // remove nlrTestPoint
        st_assert( nlrTestPoint() == nullptr, "should be nullptr now" );
        n1->eliminate( n1->bb(), nullptr, true, false );
        n2->eliminate( n2->bb(), nullptr, true, false );
    }
    NonTrivialNode::eliminate( bb, r, rem, cp );
    CHECK( _dest, r );
    if ( args ) {
        // check if any arg became unused
        std::int32_t       len = args->length();
        for ( std::int32_t i   = 0; i < len; i++ ) {
            PseudoRegister *arg = args->at( i );
            if ( arg->_location.isTopOfStack() ) {
                // must delete this push, but node won't be eliminated with TOP_OF_STACK because of side effect
                // so reset location first
                arg->_location = Location::UNALLOCATED_LOCATION;
            } else {
                st_fatal( "internal compiler error" );
                //fatal("Urs thinks all args should be TOP_OF_STACK");
            }
            CHECK( arg, r );
        }
    }
}


void TypeTestNode::eliminate( BasicBlock *bb, PseudoRegister *rr, bool rem, bool cp ) {
    static_cast<void>(rem); // unused
    static_cast<void>(cp); // unused

    // completely eliminate receiver and all successors
    if ( _deleted )
        return;

    eliminate( bb, rr, (ConstPseudoRegister *)
        nullptr, (KlassOop) MarkOopDescriptor::bad() );
}


void TypeTestNode::eliminate( BasicBlock *bb, PseudoRegister *r, ConstPseudoRegister *c, KlassOop theKlass ) {
    // remove node and all successor branches (except for one if receiver is known)
    if ( _deleted )
        return;
    GrowableArray<Node *> *successors = _nxt;
    _nxt = new GrowableArray<Node *>( 1 );
    Oop  constant = c ? c->constant : 0;
    Node *keep    = nullptr;
    if ( CompilerDebug ) {
        cout( PrintEliminateUnnededNodes )->print( "*eliminating tt node 0x{0:x} const 0x{0:x} klass 0x{0:x}\n", PrintHexAddresses ? this : 0, constant, theKlass );
    }
    Node               *un = _next;        // save unknown branch
    // remove all successor nodes
    for ( std::int32_t i   = 0; i < successors->length(); i++ ) {
        Node     *succ = successors->at( i );
        KlassOop m     = _classes->at( i );
        if ( constant == m or theKlass == m ) {
            st_assert( keep == nullptr, "shouldn't have more than one match" );
            keep = succ;
        } else {
            _next = succ;
            _next->removeUpToMerge();
        }
        succ->removePrev( this );
    }

    if ( keep or theKlass == KlassOop( MarkOopDescriptor::bad() ) ) {
        // found correct prediction, so can delete unknown branch, or
        // delete everything (theKlass == MarkOopDescriptor::bad())
        _next = un;
        un->removeUpToMerge();    // delete unknown branch
        _next = nullptr;
    } else {
        // the type tests didn't predict for theKlass
        // (performance bug: should inline correct case since it's known now;
        // also, unknown branch may be uncommon)
        if ( WizardMode ) {
            SPDLOG_WARN( "Compiler: typetest didn't predict klass 0x{0:x}", static_cast<const void *>(theKlass) );
            SPDLOG_INFO( "predicted klasses: " );
            _classes->print();
        }
        _next = un;        // keep unknown branch
    }
    st_assert( this == bb->_last, "should end my BasicBlock" );

    if ( keep ) {
        // append remaining case as fall-through
        append( keep );
    }
    NonTrivialNode::eliminate( bb, r, true, false );
    CHECK( _dest, r );
}


void TypeTestNode::eliminateUnnecessary( KlassOop m ) {
    // eliminate unnecessary type test: receiver is known to have map m
    eliminate( bb(), nullptr, nullptr, m );
}


void AbstractArrayAtNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    st_assert( rem, "shouldn't eliminate because of side effects (errors)" );
    if ( _deleted )
        return;
    // remove fail branch nodes first
    Node *fail = next1();
    if ( fail ) {
        fail->removeUpToMerge();
        fail->removePrev( this );
    }
    AbstractBranchNode::eliminate( bb, r, rem, cp );
}


void InlinedPrimitiveNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool rem, bool cp ) {
    if ( _deleted )
        return;
    AbstractBranchNode::eliminate( bb, r, rem, cp );
    if ( _arg1 )
        CHECK( _arg1, r );
    if ( _arg2 )
        CHECK( _arg2, r );
    if ( _dest )
        CHECK( _dest, r );
    if ( _error )
        CHECK( _error, r );
}


void ContextCreateNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool removing, bool cp ) {
    if ( _deleted )
        return;
    PrimitiveNode::eliminate( bb, r, removing, cp );
}


void ContextInitNode::eliminate( BasicBlock *bb, PseudoRegister *r, bool removing, bool cp ) {
    st_assert( removing, "should only remove when removing unreachable code" );
    if ( _deleted )
        return;
    NonTrivialNode::eliminate( bb, r, removing, cp );
}


void BranchNode::eliminateBranch( std::int32_t op1, std::int32_t op2, std::int32_t res ) {
    static_cast<void>(res); // unused

    // the receiver can be eliminated because the result it is testing is a constant (res)

    bool ok{ false };

    switch ( _op ) {
        case BranchOpCode::EQBranchOp:
            ok = op1 == op2;
            break;
        case BranchOpCode::NEBranchOp:
            ok = op1 not_eq op2;
            break;
        case BranchOpCode::LTBranchOp:
            ok = op1 < op2;
            break;
        case BranchOpCode::LEBranchOp:
            ok = op1 <= op2;
            break;
        case BranchOpCode::GTBranchOp:
            ok = op1 > op2;
            break;
        case BranchOpCode::GEBranchOp:
            ok = op1 >= op2;
            break;
        case BranchOpCode::LTUBranchOp:
            ok = static_cast<std::uint32_t>(op1) < static_cast<std::uint32_t>(op2);
            break;
        case BranchOpCode::LEUBranchOp:
            ok = static_cast<std::uint32_t>(op1) <= static_cast<std::uint32_t>(op2);
            break;
        case BranchOpCode::GTUBranchOp:
            ok = static_cast<std::uint32_t>(op1) > static_cast<std::uint32_t>(op2);
            break;
        case BranchOpCode::GEUBranchOp:
            ok = static_cast<std::uint32_t>(op1) >= static_cast<std::uint32_t>(op2);
            break;
        case BranchOpCode::VSBranchOp:
            // can't handle yet
            return;
        case BranchOpCode::VCBranchOp:
            // can't handle yet
            return;
        default: st_fatal( "unexpected branch type" );
    }

    std::int32_t nodeToRemove;
    if ( ok ) {
        nodeToRemove = 0; // branch is taken
    } else {
        nodeToRemove = 1;
    }

    // discard one successor and make the remaining one the fall-thru case
    Node *discard = next( nodeToRemove );
    discard->removeUpToMerge();
    removeNext( discard );
    Node *successor = next( 1 - nodeToRemove );
    removeNext( successor );
    append( successor );
    bb()->remove( this );    // delete the branch
}

// ==================================================================================
// likelySuccessor: answers the most likely successor node (or nullptr). Used for better
// code positioning (determines traversal order for BBs).
// ==================================================================================

Node *Node::likelySuccessor() const {
    st_assert( hasSingleSuccessor(), "should override likelySuccessor()" );
    return next();
}


Node *TArithRRNode::likelySuccessor() const {
    return next();                // predict success
}


Node *CallNode::likelySuccessor() const {
    return next();                // predict normal return, not NonLocalReturn
}


Node *NonLocalReturnTestNode::likelySuccessor() const {
    return next();                // predict home not found
}


Node *BranchNode::likelySuccessor() const {
    return next();                // predict untaken
}


Node *TypeTestNode::likelySuccessor() const {
    if ( _deleted )
        return next();
    st_assert( classes()->length() > 0, "no TypeTestNode needed" );
    return next( hasUnknown() ? classes()->length() : classes()->length() - 1 );
}


Node *AbstractArrayAtNode::likelySuccessor() const {
    return next();                // predict success
}


Node *InlinedPrimitiveNode::likelySuccessor() const {
    return next();                // predict success
}


// ==================================================================================
// uncommonSuccessor: answers the most uncommon successor node (or nullptr). Used for
// better code positioning (determines traversal order for BBs).
// ==================================================================================

Node *Node::uncommonSuccessor() const {
    st_assert( hasSingleSuccessor(), "should override uncommonSuccessor()" );
    return nullptr;                    // no uncommon case
}


Node *TArithRRNode::uncommonSuccessor() const {
    return ( _deleted or not canFail() ) ? nullptr : next( 1 );        // failure case is uncommon
}


Node *CallNode::uncommonSuccessor() const {
    return nullptr;                    // no uncommon case (NonLocalReturn is not uncommon)
}


Node *NonLocalReturnTestNode::uncommonSuccessor() const {
    return nullptr;                    // no uncommon case (both exits are common)
}


Node *BranchNode::uncommonSuccessor() const {
    return ( not _deleted and _taken_is_uncommon ) ? next( 1 ) : nullptr;
}


Node *TypeTestNode::uncommonSuccessor() const {
    if ( not _deleted and hasUnknown() ) {
//          [[fallthrough]];
        st_assert( next() not_eq nullptr, "just checking" );
        return next();
    } else {
        // no unknown case, all cases are common
        return nullptr;
    }
}


Node *AbstractArrayAtNode::uncommonSuccessor() const {
    return ( _deleted or not canFail() ) ? nullptr : next( 1 );    // failure case is uncommon
}


Node *InlinedPrimitiveNode::uncommonSuccessor() const {
    return nSuccessors() == 2 ? next( 1 ) : nullptr;    // failure case is uncommon if there
}


// ==================================================================================
// copy propagation: replace a use by another use; return false if unsuccessful
// ==================================================================================

#define CP_HELPER( _src, srcUse, return )                      \
  /* live range must be correct - otherwise reg. allocation breaks   */     \
  /* (even if doing just local CP - could fix this by checking for   */     \
  /* local conflicts when allocating PseudoRegisters, i.e. keep BasicBlock alloc info) */     \
  if (replace or (not d->_location.isTopOfStack() and d->extendLiveRange(this))) {    \
  _src->removeUse(bb, srcUse);                          \
  _src = d;                                      \
  srcUse = _src->addUse(bb, this);                          \
  return true;                                  \
  } else {                                      \
  return false;                                  \
  }


// if node can't fail anymore, remove failure branch (if not already removed)
void AbstractBranchNode::removeFailureIfPossible() {
    Node *taken = next1();
    if ( not canFail() and taken not_eq nullptr and taken->bb()->isPredecessor( bb() ) ) {
        taken->removeUpToMerge();
        removeNext( taken );
    }
}


bool BasicNode::canCopyPropagate( Node *fromNode ) const {
    // current restriction: cannot copy-propagate into a loop
    // reason: copy-propagated PseudoRegister needs its live range extended to cover the entire loop,
    // not just the stretch between fromNode and this node
    return canCopyPropagate() and fromNode->bb()->loopDepth() == _basicBlock->loopDepth();
}


bool NonTrivialNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    st_assert( canCopyPropagate(), "can't copy-propagate" );
    st_assert( hasSrc(), "has no src" );
    if ( _srcUse == u ) {
        CP_HELPER( _src, _srcUse, return );
    } else {
        st_fatal( "copyPropagate: not the source use" );
    }
    return false;
}


bool LoadOffsetNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( u == _srcUse ) {
        // minor performance bug: prev. node should probably be deleted
        // (loads base reg) - eliminateUnneeded doesn't catch it - fix this
        // (happens esp. if d is a constant)
        CP_HELPER( _src, _srcUse, return );
    } else {
        return LoadNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool LoadUplevelNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( u == _context0Use ) {
        CP_HELPER( _context0, _context0Use, return );
    } else {
        return LoadNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool StoreOffsetNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( u == _baseUse ) {
        CP_HELPER( _base, _baseUse, return );
    } else {
        return StoreNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool StoreUplevelNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( u == _context0Use ) {
        CP_HELPER( _context0, _context0Use, return );
    } else {
        return StoreNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool CallNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    static_cast<void>(bb); // unused
    static_cast<void>(u); // unused
    static_cast<void>(d); // unused
    static_cast<void>(replace); // unused
    //SPDLOG_WARN("fix this -- propagate args somewhere");
    return false;
}


bool RegisterRegisterArithmeticNode::operIsConst() const {
    return _oper->isConstPseudoRegister();
}


std::int32_t RegisterRegisterArithmeticNode::operConst() const {
    st_assert( operIsConst(), "not a constant" );
    return _oper->isConstPseudoRegister();
}


bool ArithmeticNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    bool success = doCopyPropagate( bb, u, d, replace );
    if ( _src->isConstPseudoRegister() and operIsConst() ) {
        st_assert( success, "CP must have worked" );
        // can constant-fold this operation
        std::int32_t c1 = (std::int32_t) ( (ConstPseudoRegister *) _src )->constant;
        std::int32_t c2 = (std::int32_t) operConst();
        std::int32_t res;
        switch ( _op ) {
            case ArithOpCode::AddArithOp:
                res = c1 + c2;
                break;

            case ArithOpCode::SubArithOp:
                res = c1 - c2;
                break;

            case ArithOpCode::AndArithOp:
                res = c1 & c2;
                break;

            case ArithOpCode::OrArithOp:
                res = c1 | c2;
                break;

            case ArithOpCode::XOrArithOp:
                res = c1 ^ c2;
                break;

            default:
                return success;        // can't constant-fold
        }

        _constResult   = new_ConstPseudoRegister( scope(), (Oop) res );
        // enable further constant propagation of the result
        _dontEliminate = false;
        _src->removeUse( bb, _srcUse );
        _src    = _constResult;
        _srcUse = bb->addUse( this, _src );

        // condition codes set -- see if there's a branch we can eliminate
        Node *branch = next();
        if ( branch->isBranchNode() ) {
            ( (BranchNode *) branch )->eliminateBranch( c1, c2, res );
        }
    }
    return success;
}


bool ArithmeticNode::doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    return NonTrivialNode::copyPropagate( bb, u, d, replace );
}


bool RegisterRegisterArithmeticNode::doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( u == _operUse ) {
        CP_HELPER( _oper, _operUse, return );
    } else {
        return ArithmeticNode::doCopyPropagate( bb, u, d, replace );
    }
    return false;
}


bool TArithRRNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    bool res = doCopyPropagate( bb, u, d, replace );
    if ( _src->isConstPseudoRegister() and _oper->isConstPseudoRegister() ) {
        st_assert( res, "CP must have worked" );
        // can constant-fold this operation
        Oop c1 = ( (ConstPseudoRegister *) _src )->constant;
        Oop c2 = ( (ConstPseudoRegister *) _oper )->constant;
        Oop result{};
        switch ( _op ) {
            case ArithOpCode::tAddArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_add( c1, c2 );
                break;
            case ArithOpCode::tSubArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_subtract( c1, c2 );
                break;
            case ArithOpCode::tMulArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_multiply( c1, c2 );
                break;
            case ArithOpCode::tDivArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_div( c1, c2 );
                break;
            case ArithOpCode::tModArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_mod( c1, c2 );
                break;
            case ArithOpCode::tAndArithOp:
                result = SmallIntegerOopPrimitives::bitAnd( c1, c2 );
                break;
            case ArithOpCode::tOrArithOp:
                result = SmallIntegerOopPrimitives::bitOr( c1, c2 );
                break;
            case ArithOpCode::tXOrArithOp:
                result = SmallIntegerOopPrimitives::bitXor( c1, c2 );
                break;
            case ArithOpCode::tShiftArithOp:
                SPDLOG_WARN( "possible performance bug: constant folding of ArithOpCode::tShiftArithOp not implemented" );
                return false;
            case ArithOpCode::tCmpArithOp:
                SPDLOG_WARN( "possible performance bug: constant folding of ArithOpCode::tCmpArithOp not implemented" );
                return false;
            default           : st_fatal1( "unknown tagged opcode %ld", _op );
        }
        bool ok = not result->isMarkOop();
        if ( ok ) {
            // constant-fold this operation
            if ( CompilerDebug ) {
                cout( PrintCopyPropagation )->print( "*constant-folding N%d --> 0x{0:x}\n", _id, result );
            }
            _constResult  = new_ConstPseudoRegister( scope(), result );
            // first, discard the error branch (if there)
            Node *discard = next1();
            if ( discard not_eq nullptr ) {
                discard->bb()->remove( discard );// SLR should this be removeNext(discard)? and should it be after removeUpToMerge()?
                discard->removeUpToMerge();
                bb->verify();
                ( (BasicBlock *) bb->next() )->verify();
            }
            // now, discard the overflow check
            discard = next();
            st_assert( discard->isBranchNode(), "must be a cond. branch" );
            st_assert( ( (BranchNode *) discard )->op() == BranchOpCode::VSBranchOp, "expected an overflow check" );
            discard->bb()->remove( discard );// SLR should this be removeNext(discard)? and should it be after removeUpToMerge()?
            // and the "overflow taken" code
            discard = discard->next1();
            discard->bb()->remove( discard );// SLR ditto this?
            discard->removeUpToMerge();
            bb->verify();
            ( (BasicBlock *) bb->next() )->verify();
            // enable further constant propagation of the result
            _dontEliminate = false;
            _src->removeUse( bb, _srcUse );
            _src    = _constResult;
            _srcUse = bb->addUse( this, _src );
        } else {
            // for now, can't constant-fold failure - can't use marks in ConstPseudoRegister
            // (and GC can't handle markOops embedded in compiled code)
        }
    }
    removeFailureIfPossible();
    return res;
}


bool TArithRRNode::doCopyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {

    bool res{ false };

    if ( u == _srcUse ) {
        if ( d->isConstPseudoRegister() and ( (ConstPseudoRegister *) d )->constant->isSmallIntegerOop() )
            _arg1IsInt = true;
        CP_HELPER( _src, _srcUse, res = );

    } else if ( u == _operUse ) {
        if ( d->isConstPseudoRegister() and ( (ConstPseudoRegister *) d )->constant->isSmallIntegerOop() )
            _arg2IsInt = true;
        CP_HELPER( _oper, _operUse, res = );

    } else {
        st_fatal( "copyPropagate: not the source use" );
    }

    removeFailureIfPossible();
    return res;
}


bool FloatArithRRNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( d->isConstPseudoRegister() and not( (ConstPseudoRegister *) d )->constant->isDouble() ) {
        // can't handle non-float arguments (don't optimize guaranteed failure)
        return false;
    }
    bool res = RegisterRegisterArithmeticNode::copyPropagate( bb, u, d, replace );
    // should check for constant folding opportunity here -- fix this
    return res;
}


bool FloatUnaryArithNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( d->isConstPseudoRegister() and not( (ConstPseudoRegister *) d )->constant->isDouble() ) {
        // can't handle non-float arguments (don't optimize guaranteed failure)
        return false;
    }
    bool res = ArithmeticNode::copyPropagate( bb, u, d, replace );
    // should check for constant folding opportunity here -- fix this
    return res;
}


bool TypeTestNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( u == _srcUse ) {
        if ( d->isConstPseudoRegister() ) {
            // we know the receiver - the type test is unnecessary!
            ConstPseudoRegister *c = (ConstPseudoRegister *) d;
            // The code below has been disabled by Lars Bak 4-19-96 per request by Urs
            // Cause:
            //   Something may go wrong when deleting the non-integer branch messing up
            //   the node graph.
            if ( true ) {
                eliminate( bb, nullptr, c, c->constant->klass() );
                return true;
            }
            return false;
        } else {
            CP_HELPER( _src, _srcUse, return );
        }
    } else {
        st_fatal( "don't have this use" );
    }
    return false;
}


bool AbstractArrayAtNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    if ( u == _argUse ) {
        bool res;
        CP_HELPER( _arg, _argUse, res = );
        removeFailureIfPossible();
        return res;
    } else {
        return AbstractBranchNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool AbstractArrayAtPutNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    bool res;
    if ( u == _argUse ) {
        CP_HELPER( _arg, _argUse, res = );
    } else if ( u == elemUse ) {
        CP_HELPER( elem, elemUse, res = );
    } else {
        return AbstractBranchNode::copyPropagate( bb, u, d, replace );
    }
    removeFailureIfPossible();
    return res;
}


bool InlinedPrimitiveNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    static_cast<void>(bb); // unused
    static_cast<void>(u); // unused
    static_cast<void>(d); // unused
    static_cast<void>(replace); // unused

    // copyPropagate should be fairly easy to put in, right now it is doing nothing.
    return false;
}


bool ContextInitNode::copyPropagate( BasicBlock *bb, Usage *u, PseudoRegister *d, bool replace ) {
    for ( std::size_t i = nofTemps() - 1; i >= 0; i-- ) {
        if ( _initializerUses->at( i ) == u ) {
            Expression     *initExpression = _initializers->at( i );
            PseudoRegister *initPR         = initExpression->pseudoRegister();
            Usage          *newUse         = u;
            bool           ok;
            CP_HELPER( initPR, newUse, ok = );
            if ( ok ) {
                st_assert( newUse not_eq u, "must have new use" );
                _initializers->at_put( i, initExpression->shallowCopy( d, initExpression->node() ) );
                _initializerUses->at_put( i, newUse );
            }
            return ok;
        }
    }
    return NonTrivialNode::copyPropagate( bb, u, d, replace );
}

// ==================================================================================
// markAllocated: for all PseudoRegisters used/defined by the node, increment the
// corresponding counters if the PseudoRegister is already allocated
// ==================================================================================

#define U_CHECK( r ) if (r->_location.isRegisterLocation()) use_count[r->_location.number()]++
#define D_CHECK( r ) if (r->_location.isRegisterLocation()) def_count[r->_location.number()]++


void LoadNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    static_cast<void>(use_count); // unused
    static_cast<void>(def_count); // unused
    D_CHECK( _dest );
}


void LoadOffsetNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _src );
    LoadNode::markAllocated( use_count, def_count );
}


void LoadUplevelNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _context0 );
    LoadNode::markAllocated( use_count, def_count );
}


void StoreNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    static_cast<void>(use_count); // unused
    static_cast<void>(def_count); // unused
    U_CHECK( _src );
}


void StoreOffsetNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _base );
    StoreNode::markAllocated( use_count, def_count );
}


void StoreUplevelNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _context0 );
    StoreNode::markAllocated( use_count, def_count );
}


void AssignNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _src );
    D_CHECK( _dest );
}


void ReturnNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _src );
    D_CHECK( _dest );
    AbstractReturnNode::markAllocated( use_count, def_count );
}


void ArithmeticNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _src );
    D_CHECK( _dest );
}


void RegisterRegisterArithmeticNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _oper );
    ArithmeticNode::markAllocated( use_count, def_count );
}


void TArithRRNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    U_CHECK( _src );
    D_CHECK( _dest );
    U_CHECK( _oper );
}


void CallNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    D_CHECK( _dest );
    // CallNode trashes all regs
    for ( std::size_t i = 0; i < REGISTER_COUNT; i++ ) {
        use_count[ i ]++;
        def_count[ i ]++;
    }
}


void BlockCreateNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    if ( _src )
        U_CHECK( _src );
    if ( _context )
        U_CHECK( _context );
    PrimitiveNode::markAllocated( use_count, def_count );
}


void BlockMaterializeNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    if ( isMemoized() )
        BlockCreateNode::markAllocated( use_count, def_count );
}


void ContextCreateNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    if ( _src )
        U_CHECK( _src ); // no src if there's no incoming context
    if ( _parentContexts ) {
        for ( std::size_t i = _parentContexts->length() - 1; i >= 0; i-- ) {
            U_CHECK( _parentContexts->at( i ) );
        }
    }
    PrimitiveNode::markAllocated( use_count, def_count );
}


void ContextInitNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    static_cast<void>(use_count); // unused
    static_cast<void>(def_count); // unused
    if ( _src )
        U_CHECK( _src );
    std::int32_t i = nofTemps();
    while ( i-- > 0 ) {
        D_CHECK( contents()->at( i )->pseudoRegister() );
    }
}


void ContextZapNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    static_cast<void>(use_count); // unused
    static_cast<void>(def_count); // unused
    U_CHECK( _src );
}


void TypeTestNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    static_cast<void>(use_count); // unused
    static_cast<void>(def_count); // unused
    U_CHECK( _src );
}


void AbstractArrayAtNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    static_cast<void>(use_count); // unused
    static_cast<void>(def_count); // unused
    U_CHECK( _src );
    if ( _dest )
        D_CHECK( _dest );
    U_CHECK( _arg );
    if ( _error )
        D_CHECK( _error );
}


void AbstractArrayAtPutNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    if ( elem )
        U_CHECK( elem );
    AbstractArrayAtNode::markAllocated( use_count, def_count );
}


void InlinedPrimitiveNode::markAllocated( std::int32_t *use_count, std::int32_t *def_count ) {
    static_cast<void>(use_count); // unused
    static_cast<void>(def_count); // unused
    if ( _src )
        U_CHECK( _src );
    if ( _arg1 )
        U_CHECK( _arg1 );
    if ( _arg2 )
        U_CHECK( _arg2 );
    if ( _dest )
        D_CHECK( _dest );
    if ( _error )
        D_CHECK( _error );
}


// trashedMask: return bit mask of trashed registers
SimpleBitVector BasicNode::trashedMask() {
    return SimpleBitVector( 0 );
}


SimpleBitVector CallNode::trashedMask() {
    return SimpleBitVector( -1 );
}

// ==================================================================================
// computeEscapingBlocks: find escaping blocks
// ==================================================================================

void computeEscapingBlocks( Node *n, PseudoRegister *src, GrowableArray<BlockPseudoRegister *> *l, const char *msg ) {
    // helper function for computing exposed blocks
    if ( src->isBlockPseudoRegister() ) {
        BlockPseudoRegister *r = (BlockPseudoRegister *) src;
        r->markEscaped( n );
        if ( not l->contains( r ) )
            l->append( r );
        if ( msg )
            theCompiler->reporter->report_block( n, r, msg );
    }
}


void StoreNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    // Any store is considered to expose a block -- even if it's a store into a local.
    // That's pessimistic, but simple.
    ::computeEscapingBlocks( this, _src, ll, action() );
}


void AbstractReturnNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    // a block returned by a NativeMethod escapes
    if ( _src ) {
        ::computeEscapingBlocks( this, _src, ll, "returned" );
    }
}


void CallNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    static_cast<void>(ll); // unused
    SubclassResponsibility();
}


void SendNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    static_cast<void>(ll); // unused
    // all arguments to a non-inlined call escape
    if ( exprStack and ( args not_eq nullptr ) ) {
        // if the receiver is not pushed on the exprStack (self/super sends),
        // the exprStack is by 1 shorter than the args array
        // (exprStack may be longer than that, so just look at top elems)
        std::int32_t len = exprStack->length();
        std::int32_t i   = min( args->length(), len );
        while ( i-- > 0 ) {
            ::computeEscapingBlocks( this, exprStack->at( --len ), ll, "exposed by a send" );
        }
    }
}


void UncommonSendNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    // all arguments to an uncommon send escape
    if ( expressionStack() and ( _argCount > 0 ) ) {
        std::int32_t len = expressionStack()->length();
        std::int32_t i   = _argCount;
        while ( i-- > 0 ) {
            ::computeEscapingBlocks( this, expressionStack()->at( --len ), ll, "exposed by an uncommon send" );
        }
    }
}


void PrimitiveNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    // assume that all block arguments to a primitive call escape
    if ( exprStack ) {
        std::int32_t len = exprStack->length();
        std::int32_t i   = min( len, _pdesc->number_of_parameters() );
        while ( i-- > 0 ) {
            ::computeEscapingBlocks( this, exprStack->at( --len ), ll, "exposed by a primitive call" );
        }
    }
}


void DLLNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    // assume that all block arguments to a DLL call escape
    if ( exprStack ) {
        std::int32_t len = exprStack->length();
        std::int32_t i   = min( len, nofArguments() );
        while ( i-- > 0 ) {
            ::computeEscapingBlocks( this, exprStack->at( --len ), ll, "exposed by a DLL call" );
        }
    }
}


void ContextInitNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    // all blocks stored into a context escape
    // (later phase will recognize if context isn't created --> block doesn't really escape)
    std::int32_t i = nofTemps();
    while ( i-- > 0 ) {
        Expression *e = _initializers->at( i );
        if ( e )
            ::computeEscapingBlocks( this, e->pseudoRegister(), ll, nullptr );
    }
}


void ArrayAtPutNode::computeEscapingBlocks( GrowableArray<BlockPseudoRegister *> *ll ) {
    // all blocks stored into an array escape
    ::computeEscapingBlocks( this, elem, ll, "stored into an array" );
}


// ==================================================================================
// machine-independent routines for code generation
// ==================================================================================

bool TypeTestNode::needsKlassLoad() const {
    // a test needs a klass load if it tests for any non-small_int_t/bool/nil klass
    const std::int32_t len = _hasUnknown ? _classes->length() : _classes->length() - 1;
    for ( std::int32_t i   = 0; i < len; i++ ) {
        KlassOop klass = _classes->at( i );
        if ( klass not_eq trueObject->klass() and klass not_eq falseObject->klass() and klass not_eq nilObject->klass() and klass not_eq smiKlassObject ) {
            return true;
        }
    }
    return false;
}


static bool hasUnknownCode( Node *n ) {
    while ( n->isTrivial() )
        n = n->next();
    return not n->isUncommonNode();
}


bool TypeTestNode::hasUnknownCode() const {
    if ( not _hasUnknown )
        return false;     // no unknown type
    return ::hasUnknownCode( next() );
}


bool TArithRRNode::hasUnknownCode() const {
    return ::hasUnknownCode( next1() );
}


bool AbstractArrayAtNode::hasUnknownCode() const {
    return ::hasUnknownCode( next1() );
}


Node *TypeTestNode::smiCase() const {
    std::int32_t i = _classes->length();
    while ( i-- > 0 ) {
        if ( _classes->at( i ) == smiKlassObject )
            return next( i + 1 );
    }
    return nullptr;
}

// ==================================================================================
// integer loop optimization
// ==================================================================================

LoopHeaderNode::LoopHeaderNode() :
    _integerLoop{ false },
    _loopVar{ nullptr },
    _lowerBound{ nullptr },
    _upperBound{ nullptr },
    _upperLoad{ nullptr },
    _arrayAccesses{ nullptr },
    _enclosingLoop{ nullptr },
    _tests{ nullptr },
    _nestedLoops{ nullptr },
    _registerCandidates{ nullptr },
    _activated{ false },
    _nofCalls{ 0 } {

}


void LoopHeaderNode::activate( PseudoRegister *loopVar, PseudoRegister *lowerBound, PseudoRegister *upperBound, LoadOffsetNode *loopSizeLoad ) {
    _activated     = true;
    _integerLoop   = true;
    _loopVar       = loopVar;
    _lowerBound    = lowerBound;
    _upperBound    = upperBound;
    _upperLoad     = loopSizeLoad;
    _arrayAccesses = new GrowableArray<AbstractArrayAtNode *>( 10 );
}


void LoopHeaderNode::activate() {
    _activated = true;
    st_assert( _tests, "should have type tests" );
    _loopVar       = _lowerBound = _upperBound = nullptr;
    _upperLoad     = nullptr;
    _arrayAccesses = nullptr;
}


void LoopHeaderNode::addArray( AbstractArrayAtNode *n ) {
    st_assert( _activated, "shouldn't call" );
    _arrayAccesses->append( n );
}


void LoopHeaderNode::set_enclosingLoop( LoopHeaderNode *l ) {
    st_assert( _enclosingLoop == nullptr, "already set" );
    _enclosingLoop = l;
}


void LoopHeaderNode::addNestedLoop( LoopHeaderNode *l ) {
    if ( _nestedLoops == nullptr )
        _nestedLoops = new GrowableArray<LoopHeaderNode *>( 5 );
    _nestedLoops->append( l );
}


void LoopHeaderNode::addRegisterCandidate( LoopPseudoRegisterCandidate *c ) {
    if ( _registerCandidates == nullptr )
        _registerCandidates = new GrowableArray<LoopPseudoRegisterCandidate *>( 2 );
    _registerCandidates->append( c );
}


bool is_SmallInteger_type( GrowableArray<KlassOop> *klasses ) {
    return klasses->length() == 1 and klasses->at( 0 ) == smiKlassObject;
}


GrowableArray<KlassOop> *make_smi_type() {
    GrowableArray<KlassOop> *t = new GrowableArray<KlassOop>( 1 );
    t->append( smiKlassObject );
    return t;
}


void StoreNode::assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n ) {
    static_cast<void>(n); // unused
    if ( is_SmallInteger_type( klasses ) and r == src() ) {
        if ( CompilerDebug )
            cout( PrintLoopOpts )->print( "*removing store check from N%d\n", id() );
        setStoreCheck( false );
    }
}


void AbstractArrayAtNode::assert_in_bounds( PseudoRegister *r, LoopHeaderNode *n ) {
    static_cast<void>(n); // unused
    if ( r == _arg ) {
        if ( CompilerDebug and _needBoundsCheck )
            cout( PrintLoopOpts )->print( "*removing bounds check from N%d\n", id() );
        _needBoundsCheck = false;
        removeFailureIfPossible();
    }
}


void AbstractArrayAtNode::collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *
> &klasses ) const {
// ArrayAt node tests index for small_int_t-ness
    regs.
        append( _arg );
    klasses.
        append( make_smi_type() );
}


void AbstractArrayAtNode::assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n ) {
    if ( is_SmallInteger_type( klasses ) and r == _arg ) {
        if ( CompilerDebug and not _intArg )
            cout( PrintLoopOpts )->print( "*removing index tag check from N%d\n", id() );
        _intArg = true;
        n->addArray( this );
        removeFailureIfPossible();
    } else if ( r not_eq _arg ) {
        st_fatal( "array can't be an integer" );
    }
}


void ArrayAtPutNode::collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *
> &klasses ) const {
// atPut node tests element for small_int_t-ness if character array
    AbstractArrayAtNode::collectTypeTests( regs, klasses
    );
    if (
        stores_smi_elements( _access_type )
        ) {
        regs.
            append( elem );

        st_assert( klasses.first()->first() == smiKlassObject, "must be small_int_t type for index" );

        klasses.
            append( klasses
                        .
                            first()
        );    /* reuse small_int_t type descriptor*/
    }
}


void ArrayAtPutNode::assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n ) {
    if ( is_SmallInteger_type( klasses ) and r == elem ) {
        if ( CompilerDebug and _needs_store_check )
            cout( PrintLoopOpts )->print( "*removing array store check from N%d\n", id() );
        _needs_store_check = false;
        removeFailureIfPossible();
    } else if ( r == _arg ) {
        AbstractArrayAtPutNode::assert_pseudoRegister_type( r, klasses, n );
    }
}


void TArithRRNode::collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *
> &klasses ) const {
// tests receiver and/or arg for small_int_t-ness
    if (
        canFail()
        ) {
        GrowableArray<KlassOop> *t = make_smi_type();
        if ( not _arg1IsInt ) {
            regs.
                append( _src );
            klasses.
                append( t );
        }
        if ( not _arg2IsInt ) {
            regs.
                append( _oper );
            klasses.
                append( t );
        }
    }
}


void TArithRRNode::assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *klasses, LoopHeaderNode *n ) {
    static_cast<void>(n); // unused

    if ( is_SmallInteger_type( klasses ) and r == _src ) {
        if ( CompilerDebug and not _arg1IsInt )
            cout( PrintLoopOpts )->print( "*removing arith arg1 tag check from N%d\n", id() );
        _arg1IsInt = true;
    }

    if ( is_SmallInteger_type( klasses ) and r == _oper ) {
        if ( CompilerDebug and not _arg2IsInt )
            cout( PrintLoopOpts )->print( "*removing arith arg2 tag check from N%d\n", id() );
        _arg2IsInt = true;
    }

    removeFailureIfPossible();
}


void TypeTestNode::collectTypeTests( GrowableArray<PseudoRegister *> &regs, GrowableArray<GrowableArray<KlassOop> *
> &klasses ) const {
    regs.
        append( _src );
    klasses.
        append( _classes );
}


void TypeTestNode::assert_pseudoRegister_type( PseudoRegister *r, GrowableArray<KlassOop> *k, LoopHeaderNode *n ) {
    static_cast<void>(n); // unused
    st_assert( r == src(), "must be source" );
    if ( k->length() == 1 ) {
        // common case: tests for one klass --> can be eliminated
        eliminateUnnecessary( k->at( 0 ) );
    } else {
        // can at least eliminate uncommon unknown case
        st_assert( k->length() <= _classes->length(), "type cannot widen" );
        setUnknown( false );
    }
}

// ==================================================================================
// printing code (for debugging)
// ==================================================================================

const std::int32_t PrintStringLen = 40;    // width of output before printing address

void BasicNode::print_short() {
    char buf[1024];
    SPDLOG_INFO( toString( buf, PrintHexAddresses ) );
}


static std::int32_t id_of( Node *node ) {
    return node == nullptr ? -1 : node->id();
}


const char *PrologueNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "Prologue" );
    if ( printAddress )
        my_sprintf( buf, " ((PrologueNode*)0x{0:x}", this );
    return b;
}


const char *InterruptCheckNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "InterruptCheckNode" );
    if ( printAddress )
        my_sprintf( buf, " ((InterruptCheckNode*)0x{0:x})", this );
    return b;
}


const char *LoadOffsetNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "LoadOffset %s := %s[0x{0:x}]", _dest->safeName(), _src->safeName(), _offset );
    if ( printAddress )
        my_sprintf( buf, " ((LoadOffsetNode*)0x{0:x})", this );
    return b;
}


const char *LoadIntNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "LoadInt %s := 0x{0:x}", _dest->safeName(), _value );
    if ( printAddress )
        my_sprintf( buf, " ((LoadIntNode*)0x{0:x})", this );
    return b;
}


const char *LoadUplevelNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "LoadUpLevel %s := %s^%d[%d]", _dest->safeName(), _context0->safeName(), _nofLevels, _offset );
    if ( printAddress )
        my_sprintf( buf, " ((LoadUplevelNode*)0x{0:x})", this );
    return b;
}


const char *StoreOffsetNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "StoreOffset %s[0x{0:x}] := %s", _base->safeName(), _offset, _src->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((StoreOffsetNode*)0x{0:x})", this );
    return b;
}


const char *StoreUplevelNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "StoreUpLevel %s^%d[%d] := %s", _context0->safeName(), _nofLevels, _offset, _src->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((StoreUplevelNode*)0x{0:x})", this );
    return b;
}


const char *AssignNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s", _dest->safeName(), _src->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((AssignNode*)0x{0:x})", this );
    return b;
}


const char *SendNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "Send %s NonLocalReturn %ld ", _key->toString(), id_of( nlrTestPoint() ) );
    if ( printAddress )
        my_sprintf( buf, " ((SendNode*)0x{0:x})", this );
    return b;
}


const char *PrimitiveNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "PrimCall _%s NonLocalReturn %ld", _pdesc->name(), id_of( nlrTestPoint() ) );
    if ( printAddress )
        my_sprintf( buf, " ((PrimitiveNode*)0x{0:x})", this );
    return b;
}


const char *DLLNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "DLLCall <%s, %s> NonLocalReturn %ld", _dll_name->as_string(), _function_name->as_string(), id_of( nlrTestPoint() ) );
    if ( printAddress )
        my_sprintf( buf, " ((DLLNode*)0x{0:x})", this );
    return b;
}


const char *BlockCreateNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "BlockCreate %s", _dest->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((BlockCreateNode*)0x{0:x})", this );
    return b;
}


const char *BlockMaterializeNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "BlockMaterialize %s", _dest->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((BlockMaterializeNode*)0x{0:x})", this );
    return b;
}


const char *InlinedReturnNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "InlinedReturn %s := %s", _dest->safeName(), _src->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((InlinedReturnNode*)0x{0:x})", this );
    return b;
}


const char *NonLocalReturnSetupNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "NonLocalReturneturn %s := %s", _dest->safeName(), _src->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((NonLocalReturnSetupNode*)0x{0:x})", this );
    return b;
}


const char *NonLocalReturnContinuationNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "NonLocalReturn Continuation" );
    if ( printAddress )
        my_sprintf( buf, " ((NonLocalReturnContinuationNode*)0x{0:x})", this );
    return b;
}


const char *ReturnNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "MethodReturn  %s", _src->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((ReturnNode*)0x{0:x})", this );
    return b;
}


const char *NonLocalReturnTestNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "NonLocalReturnTest  N%ld N%ld", id_of( next1() ), id_of( next() ) );
    if ( printAddress )
        my_sprintf( buf, " ((NonLocalReturnTestNode*)0x{0:x})", this );
    return b;
}


const char *ArithmeticNode::opName() const {
    return ArithOpName[ static_cast<std::int32_t>( _op ) ];
}


const char *RegisterRegisterArithmeticNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s %s", _dest->safeName(), _src->safeName(), opName(), _oper->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((RegisterRegisterArithmeticNode*)0x{0:x})", this );
    return b;
}


const char *FloatArithRRNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s %s", _dest->safeName(), _src->safeName(), opName(), _oper->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((FloatArithRRNode*)0x{0:x})", this );
    return b;
}


const char *FloatUnaryArithNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s", _dest->safeName(), opName(), _src->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((FloatUnaryArithNode*)0x{0:x})", this );
    return b;
}


const char *TArithRRNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s %s   N%d, N%d", _dest->safeName(), _src->safeName(), ArithOpName[ static_cast<std::int32_t>( _op ) ], _oper->safeName(), id_of( next1() ), id_of( next() ) );
    if ( printAddress )
        my_sprintf( buf, " ((TArithRRNode*)0x{0:x})", this );
    return b;
}


const char *ArithRCNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s 0x{0:x}", _dest->safeName(), _src->safeName(), opName(), _operand );
    if ( printAddress )
        my_sprintf( buf, " ((ArithRCNode*)0x{0:x})", this );
    return b;
}


const char *BranchNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s  N%ld N%ld", BranchOpName[ static_cast<std::int32_t>( _op ) ], id_of( next1() ), id_of( next() ) );
    if ( printAddress )
        my_sprintf( buf, " ((BranchNode*)0x{0:x})", this );
    return b;
}


const char *TypeTestNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf( buf, " TypeTest %s, ", _src->safeName() );
    for ( std::size_t i = 1; i <= _classes->length(); i++ ) {
        KlassOop m = _classes->at( i - 1 );
        my_sprintf( buf, m->print_value_string() );
        my_sprintf( buf, " : N%ld; ", ( i < nSuccessors() and next( i ) not_eq nullptr ) ? next( i )->id() : -1 );
    }
    my_sprintf_len( buf, b + PrintStringLen - buf, "N%ld%s", id_of( next() ), _hasUnknown ? "" : "*" );
    if ( printAddress )
        my_sprintf( buf, " ((TypeTestNode*)0x{0:x})", this );
    return b;
}


const char *ArrayAtNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "ArrayAt %s := %s[%s]", _dest->safeName(), _src->safeName(), _arg->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((ArrayAtNode*)0x{0:x})", this );
    return b;
}


const char *ArrayAtPutNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "ArrayAtPut %s[%s] := %s", _src->safeName(), _arg->safeName(), elem->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((ArrayAtPutNode*)0x{0:x})", this );
    return b;
}


const char *FixedCodeNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "DeadEnd" );
    if ( printAddress )
        my_sprintf( buf, " ((FixedCodeNode*)0x{0:x})", this );
    return b;
}


static std::int32_t prevsLen;
static char         *mergePrintBuf;


static void printPrevNodes( Node *n ) {
    my_sprintf( mergePrintBuf, "N%ld%s", id_of( n ), --prevsLen > 0 ? ", " : "" );
}


const char *MergeNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf( buf, " Merge " );
    prevsLen      = _prevs->length();
    mergePrintBuf = buf;
    _prevs->apply( printPrevNodes );
    buf = mergePrintBuf;
    my_sprintf_len( buf, b + PrintStringLen - buf, " " );
    if ( printAddress )
        my_sprintf( buf, " ((MergeNode*)0x{0:x})", this );
    return b;
}


const char *LoopHeaderNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf( buf, " LoopHeader " );
    if ( _activated ) {
        if ( _integerLoop ) {
            my_sprintf( buf, " std::int32_t " );
            my_sprintf( buf, " %s=[%s..%s] ", _loopVar->safeName(), _lowerBound->safeName(), _upperBound ? _upperBound->safeName() : _upperLoad->base()->safeName() );
        }
        if ( _registerCandidates not_eq nullptr ) {
            my_sprintf( buf, " reg vars = " );
            for ( std::size_t i = 0; i < _registerCandidates->length(); i++ )
                my_sprintf( buf, " %s ", _registerCandidates->at( i )->pseudoRegister()->name() );
        }
        if ( _tests not_eq nullptr ) {
            for ( std::size_t i = 0; i < _tests->length(); i++ ) {
                HoistedTypeTest *t = _tests->at( i );
                if ( t->_testedPR->_location not_eq Location::UNALLOCATED_LOCATION ) {
                    StringOutputStream s( 50 );
                    t->print_test_on( &s );
                    my_sprintf( buf, " %s ", s.as_string() );
                }
            }
        }
        my_sprintf_len( buf, PrintStringLen - ( buf - b ), " " );
    } else {
        my_sprintf_len( buf, PrintStringLen - 11, "(inactive)" );
    }
    if ( printAddress )
        my_sprintf( buf, " ((LoopHeaderNode*)0x{0:x})", this );
    return b;
}


const char *ContextCreateNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "Create Context %s", _dest->safeName() );
    if ( printAddress )
        my_sprintf( buf, " ((ContextCreateNode*)0x{0:x})", this );
    return b;
}


const char *ContextInitNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf( buf, " Initialize context " );
    if ( _src == nullptr ) {
        my_sprintf( buf, " (optimized away) " );
    } else {
        my_sprintf( buf, " %s { ", _src->safeName() );
        for ( std::size_t i = 0; i < contents()->length(); i++ ) {
            my_sprintf( buf, "  %s := ", contents()->at( i )->pseudoRegister()->safeName() );
            Expression *e = _initializers->at( i );
            my_sprintf( buf, "  %s; ", e->pseudoRegister()->safeName() );
        }
    }
    my_sprintf_len( buf, b + PrintStringLen - buf, "}" );
    if ( printAddress )
        my_sprintf( buf, " ((ContextInitNode*)0x{0:x})", this );
    return b;
}


const char *ContextZapNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "Zap Context %s", isActive() ? context()->safeName() : "- inactive" );
    if ( printAddress )
        my_sprintf( buf, " ((ContextZapNode*)0x{0:x})", this );
    return b;
}


const char *InlinedPrimitiveNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf( buf, " %s := ", _dest->safeName() );
    const char *op_name;
    switch ( _operation ) {
        case InlinedPrimitiveNode::Operation::OBJ_KLASS:
            op_name = "obj_klass";
            break;
        case InlinedPrimitiveNode::Operation::OBJ_HASH:
            op_name = "obj_hash";
            break;
        case InlinedPrimitiveNode::Operation::PROXY_BYTE_AT:
            op_name = "proxy_byte_at";
            break;
        case InlinedPrimitiveNode::Operation::PROXY_BYTE_AT_PUT:
            op_name = "proxy_byte_at_put";
            break;
        default:
            op_name = "*** unknown primitive ***";
            break;
    }
    my_sprintf( buf, " %s(", op_name );
    my_sprintf( buf, "  %s", _src->safeName() );
    my_sprintf( buf, "  %s", _arg1->safeName() );
    my_sprintf( buf, "  %s", _arg2->safeName() );
    my_sprintf_len( buf, b + PrintStringLen - buf, ")" );
    if ( printAddress )
        my_sprintf( buf, " ((InlinedPrimitiveNode*)0x{0:x})", this );
    return const_cast<char *>( b );
}


const char *UncommonNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "UncommonBranch" );
    if ( printAddress )
        my_sprintf( buf, " ((UncommonNode*)0x{0:x})", this );
    return b;
}


const char *UncommonSendNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "UncommonSend(%d arg%s)", _argCount, _argCount not_eq 1 ? "s" : "" );
    if ( printAddress )
        my_sprintf( buf, " ((UncommonSendNode*)0x{0:x})", this );
    return b;
}


const char *NopNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "Nop" );
    if ( printAddress )
        my_sprintf( buf, " ((NopNode*)0x{0:x})", this );
    return b;
}


const char *CommentNode::toString( char *buf, bool printAddress ) const {
    char *b = buf;
    my_sprintf_len( buf, PrintStringLen, "'%s' ", _comment );
    if ( printAddress )
        my_sprintf( buf, " ((CommentNode*)0x{0:x})", this );
    return b;
}


void BasicNode::printID() const {
    SPDLOG_INFO( "%4ld:%1s %-4s", id(), _deleted ? "D" : " ", " " );
    //c hasSplitSig() ? splitSig()->prefix(buf) : " ");
}


void Node::verify() const {
    if ( _deleted )
        return;
    if ( not firstPrev() and not isPrologueNode() )
        error( "Node 0x{0:x}: no predecessor", this );
    if ( firstPrev() and not firstPrev()->isSuccessor( this ) )
        error( "prev->next not_eq this for Node 0x{0:x}", this );
    if ( _basicBlock and not _basicBlock->contains( this ) )
        error( "BasicBlock doesn't contain Node 0x{0:x}", this );
    if ( next() and not next()->isPredecessor( this ) )
        error( "next->prev not_eq this for Node 0x{0:x}", this );
    if ( bbIterator->_blocksBuilt and _basicBlock == nullptr )
        error( "Node 0x{0:x}: doesn't belong to any BasicBlock", this );
    if ( next() == nullptr and not isExitNode() and not isCommentNode() )   // for the "rest of method omitted (dead)" comment
        error( "Node 0x{0:x} has no successor", this );
    if ( next() not_eq nullptr and isExitNode() ) {
        Node *n = next();
        for ( ; n and ( n->isCommentNode() or n->isDeadEndNode() ); n = n->next() );
        if ( n )
            error( "exit node 0x{0:x} has a successor (0x{0:x})", this, next() );
    }
}


void NonTrivialNode::verify() const {
    if ( _deleted )
        return;
    if ( hasSrc() )
        src()->verify();
    if ( hasDest() ) {
        dest()->verify();
        if ( dest()->isConstPseudoRegister() ) {
            error( "Node 0x{0:x}: dest 0x{0:x} is ConstPR", this, dest() );
        }
    }
    if ( isAssignmentLike() and ( not hasSrc() or not hasDest() ) )
        error( "Node 0x{0:x}: isAssignmentLike() implies hasSrc/Dest", this );
    Node::verify();
}


void LoadOffsetNode::verify() const {
    if ( _deleted )
        return;
    NonTrivialNode::verify();
    base()->verify();
    if ( _offset < 0 )
        error( "Node 0x{0:x}: offset must be >= 0", this );
}


void LoadUplevelNode::verify() const {
    if ( _deleted )
        return;
    if ( _context0 == nullptr )
        error( "Node 0x{0:x}: context0 is nullptr", this );
    if ( _nofLevels < 0 )
        error( "Node 0x{0:x}: nofLevels must be >= 0", this );
    if ( _offset < 0 )
        error( "Node 0x{0:x}: offset must be >= 0", this );
    NonTrivialNode::verify();
    _context0->verify();
}


void StoreOffsetNode::verify() const {
    if ( _deleted )
        return;
    NonTrivialNode::verify();
    base()->verify();
    if ( _offset < 0 )
        error( "Node 0x{0:x}: offset must be >= 0", this );
}


void StoreUplevelNode::verify() const {
    if ( _deleted )
        return;
    if ( _context0 == nullptr )
        error( "Node 0x{0:x}: context0 is nullptr", this );
    if ( _nofLevels < 0 )
        error( "Node 0x{0:x}: nofLevels must be > 0", this );
    if ( _offset < 0 )
        error( "Node 0x{0:x}: offset must be >= 0", this );
    NonTrivialNode::verify();
    _context0->verify();
}


void MergeNode::verify() const {
    if ( _deleted )
        return;
    if ( _isLoopStart and _isLoopEnd )
        error( "MergeNode 0x{0:x}: cannot be both start and end of loop" );
    TrivialNode::verify();
}


void BlockCreateNode::verify() const {
    if ( _deleted )
        return;
    PrimitiveNode::verify();
}


void ReturnNode::verify() const {
    if ( _deleted )
        return;
    AbstractReturnNode::verify();
    if ( next() )
        error( "ReturnNode 0x{0:x} has a successor", this );
}


void NonLocalReturnSetupNode::verify() const {
    if ( _deleted )
        return;
    AbstractReturnNode::verify();
    if ( next() )
        error( "NonLocalReturnSetupNode 0x{0:x} has a successor", this );
}


void NonLocalReturnContinuationNode::verify() const {
    if ( _deleted )
        return;
    AbstractReturnNode::verify();
    if ( next() )
        error( "NonLocalReturnContinuationNode 0x{0:x} has a successor", this );
}


void NonLocalReturnTestNode::verify() const {
    if ( _deleted )
        return;
    AbstractBranchNode::verify( false );
    if ( next() == nullptr )
        error( "NonLocalReturnTestNode 0x{0:x} has no continue-NonLocalReturn node", this );
    if ( next1() == nullptr )
        error( "NonLocalReturnTestNode 0x{0:x} has no end-of-NonLocalReturn node", this );
}


void InlinedReturnNode::verify() const {
    if ( _deleted )
        return;
    AbstractReturnNode::verify();
    if ( not next() ) {
        error( "InlinedReturnNode 0x{0:x} has no successor", this );
    } else {
        Node *nextAfterMerge = next()->next();
        if ( nextAfterMerge->scope() == scope() )
            error( "InlinedReturnNode 0x{0:x}: successor is in same scope", this );
    }
}


void ContextCreateNode::verify() const {
    PrimitiveNode::verify();
}


void ContextInitNode::verify() const {
    if ( _deleted )
        return;
    std::int32_t n = nofTemps();
    if ( ( n not_eq contents()->length() ) or ( n not_eq _initializers->length() ) or ( ( _contentDefs not_eq nullptr ) and ( n not_eq _contentDefs->length() ) ) or ( ( _initializerUses not_eq nullptr ) and ( n not_eq _initializerUses->length() ) ) ) {
        error( "ContextInitNode 0x{0:x}: bad nofTemps %d", this, n );
    }
    std::int32_t i = nofTemps();
    while ( i-- > 0 ) {
        Expression *e = _initializers->at( i );
        if ( e not_eq nullptr )
            e->verify();
        contents()->at( i )->verify();
        PseudoRegister *r = contents()->at( i )->pseudoRegister();
        if ( _src == nullptr and r->_location.isContextLocation() ) {
            ( (ContextInitNode *)
                this )->print();
            scope()->print();
            error( "ContextInitNode 0x{0:x}: context eliminated but temp %d is context location", this, i );
        }
        // isInContext is accessing _isInContext which is never set (and thus always 0 initially)
        // Should check if we're missing something here or if we can remove the code completely.
        // - gri 9/10/96
        /*
    if (r->isSinglyAssignedPseudoRegister() and not r->isBlockPseudoRegister() and not ((SinglyAssignedPseudoRegister*)r)->isInContext()) {
    // I'm not sure what the isInContext() flag is supposed to do....but the error is triggered
    // in the test suite (when running the standard tests).  Could be because of copy propagation.
    // If the assertion does make sense, please also put it in ContextInitNode::initialize().
    // (But please turn the condition above into a method, don't duplicate it.)
    // Urs 9/8/96
    // error("ContextInitNode 0x{0:x}: temp %d is non-context SinglyAssignedPseudoRegister %s", this, i, r->safeName());
    }
    */
    }
}


void ContextZapNode::verify() const {
    if ( _deleted )
        return;
    if ( _src not_eq scope()->context() ) {
        error( "ContextZapNode 0x{0:x}: wrong context 0x{0:x}", this, _src );
    }
    NonTrivialNode::verify();
}


void CallNode::verify() const {
    if ( _deleted )
        return;
    if ( ( exprStack not_eq nullptr ) and ( args not_eq nullptr ) ) {
        if ( exprStack->length() + 1 < args->length() ) {
            error( "CallNode 0x{0:x}: exprStack is too short", this );
        }
    }
}


void RegisterRegisterArithmeticNode::verify() const {
    if ( _deleted )
        return;
    ArithmeticNode::verify();
    _oper->verify();
}


void TArithRRNode::verify() const {
    if ( _deleted )
        return;
    AbstractBranchNode::verify( true );
    if ( ( _op < ArithOpCode::tAddArithOp ) or ( ArithOpCode::tCmpArithOp < _op ) ) {
        error( "TArithRRNode 0x{0:x}: wrong opcode %ld", this, _op );
    }
}


void FloatArithRRNode::verify() const {
    if ( _deleted )
        return;
    RegisterRegisterArithmeticNode::verify();
    // fix this -- check opcode
}


void FloatUnaryArithNode::verify() const {
    if ( _deleted )
        return;
    ArithmeticNode::verify();
    // fix this -- check opcode
}


void AbstractBranchNode::verify( bool verifySuccessors ) const {
    if ( _deleted )
        return;
    NonTrivialNode::verify();
    if ( verifySuccessors and not canFail() and failureBranch() not_eq nullptr ) {
        error( "Node 0x{0:x}: cannot fail, but failure branch is still there", this );
    }
}


void InlinedPrimitiveNode::verify() const {
    if ( _deleted )
        return;
    AbstractBranchNode::verify( true );
    // fix this - check node
}


void UncommonNode::verify() const {
    if ( _deleted )
        return;
    if ( (Node *) this not_eq bb()->_last )
        error( "UncommonNode 0x{0:x}: not last node in BasicBlock", this );
    NonTrivialNode::verify();
}


void TypeTestNode::verify() const {
    if ( _deleted )
        return;
    if ( (Node *) this not_eq bb()->_last )
        error( "TypeTestNode 0x{0:x}: not last node in BasicBlock", this );
    NonTrivialNode::verify();
}


// for debugging
void printNodes( Node *n ) {
    for ( ; n; n = n->next() ) {
        n->printID();
        n->print_short();
        SPDLOG_INFO( "" );
    }
}
