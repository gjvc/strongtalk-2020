
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/primitives/behavior_primitives.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/primitives/PrimitivesGenerator.hpp"
#include "vm/primitives/smi_primitives.hpp"
#include "vm/compiler/CompiledLoop.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/compiler/PseudoRegister.hpp"


int                 BasicNode::currentID;
int                 BasicNode::currentCommentID;
int                 BasicNode::lastByteCodeIndex;
ScopeInfo           BasicNode::lastScopeInfo;
PrimitiveDescriptor * InterruptCheckNode::_intrCheck;

int NodeFactory::_cumulativeCost;


PrologueNode * NodeFactory::PrologueNode( LookupKey * key, int nofArgs, int nofTemps ) {
    class PrologueNode * res = new class PrologueNode( key, nofArgs, nofTemps );
    registerNode( res );
    return res;
}


LoadOffsetNode * NodeFactory::LoadOffsetNode( PseudoRegister * dst, PseudoRegister * base, int offs, bool_t isArray ) {
    class LoadOffsetNode * res = new class LoadOffsetNode( dst, base, offs, isArray );
    registerNode( res );
    return res;
}


LoadUplevelNode * NodeFactory::LoadUplevelNode( PseudoRegister * dst, PseudoRegister * context0, int nofLevels, int offset, SymbolOop name ) {
    class LoadUplevelNode * res = new class LoadUplevelNode( dst, context0, nofLevels, offset, name );
    registerNode( res );
    return res;
}


LoadIntNode * NodeFactory::LoadIntNode( PseudoRegister * dst, int value ) {
    class LoadIntNode * res = new class LoadIntNode( dst, value );
    registerNode( res );
    return res;
}


StoreOffsetNode * NodeFactory::StoreOffsetNode( PseudoRegister * src, PseudoRegister * base, int offs, bool_t needStoreCheck ) {
    class StoreOffsetNode * res = new class StoreOffsetNode( src, base, offs, needStoreCheck );
    registerNode( res );
    return res;
}


StoreUplevelNode * NodeFactory::StoreUplevelNode( PseudoRegister * src, PseudoRegister * context0, int nofLevels, int offset, SymbolOop name, bool_t needStoreCheck ) {
    class StoreUplevelNode * res = new class StoreUplevelNode( src, context0, nofLevels, offset, name, needStoreCheck );
    registerNode( res );
    return res;
}


AssignNode * NodeFactory::AssignNode( PseudoRegister * src, PseudoRegister * dst ) {
    class AssignNode * res = new class AssignNode( src, dst );
    registerNode( res );
    return res;
}


ReturnNode * NodeFactory::ReturnNode( PseudoRegister * result, int byteCodeIndex ) {
    class ReturnNode * res = new class ReturnNode( result, byteCodeIndex );
    registerNode( res );
    return res;
}


InlinedReturnNode * NodeFactory::InlinedReturnNode( int byteCodeIndex, PseudoRegister * src, PseudoRegister * dst ) {
    class InlinedReturnNode * res = new class InlinedReturnNode( byteCodeIndex, src, dst );
    registerNode( res );
    return res;
}


NonLocalReturnSetupNode * NodeFactory::NonLocalReturnSetupNode( PseudoRegister * result, int byteCodeIndex ) {
    class NonLocalReturnSetupNode * res = new class NonLocalReturnSetupNode( result, byteCodeIndex );
    registerNode( res );
    return res;
}


NonLocalReturnContinuationNode * NodeFactory::NonLocalReturnContinuationNode( int byteCodeIndex ) {
    InlinedScope                         * scope = theCompiler->currentScope();
    PseudoRegister                       * reg   = new PseudoRegister( scope, NonLocalReturnResultLoc, false, false );
    class NonLocalReturnContinuationNode * res   = new class NonLocalReturnContinuationNode( byteCodeIndex, reg, reg );
    registerNode( res );
    return res;
}


NonLocalReturnTestNode * NodeFactory::NonLocalReturnTestNode( int byteCodeIndex ) {
    class NonLocalReturnTestNode * res = new class NonLocalReturnTestNode( byteCodeIndex );
    registerNode( res );
    theCompiler->nlrTestPoints->append( res );
    return res;
}


ArithRRNode * NodeFactory::ArithRRNode( PseudoRegister * dst, PseudoRegister * src, ArithOpCode op, PseudoRegister * o2 ) {
    class ArithRRNode * res = new class ArithRRNode( op, src, o2, dst );
    registerNode( res );
    return res;
}


ArithRCNode * NodeFactory::ArithRCNode( PseudoRegister * dst, PseudoRegister * src, ArithOpCode op, int o2 ) {
    class ArithRCNode * res = new class ArithRCNode( op, src, o2, dst );
    registerNode( res );
    return res;
}


TArithRRNode * NodeFactory::TArithRRNode( PseudoRegister * dst, PseudoRegister * src, ArithOpCode op, PseudoRegister * o2, bool_t a1, bool_t a2 ) {
    class TArithRRNode * res = new class TArithRRNode( op, src, o2, dst, a1, a2 );
    registerNode( res );
    return res;
}


FloatArithRRNode * NodeFactory::FloatArithRRNode( PseudoRegister * dst, PseudoRegister * src, ArithOpCode op, PseudoRegister * o2 ) {
    class FloatArithRRNode * res = new class FloatArithRRNode( op, src, o2, dst );
    registerNode( res );
    return res;
}


FloatUnaryArithNode * NodeFactory::FloatUnaryArithNode( PseudoRegister * dst, PseudoRegister * src, ArithOpCode op ) {
    class FloatUnaryArithNode * res = new class FloatUnaryArithNode( op, src, dst );
    registerNode( res );
    return res;
}


MergeNode * NodeFactory::MergeNode( Node * prev1, Node * prev2 ) {
    class MergeNode * res = new class MergeNode( prev1, prev2 );
    registerNode( res );
    return res;
}


MergeNode * NodeFactory::MergeNode( int byteCodeIndex ) {
    class MergeNode * res = new class MergeNode( byteCodeIndex );
    registerNode( res );
    return res;
}


SendNode * NodeFactory::SendNode( LookupKey * key, class MergeNode * nlrTestPoint, GrowableArray <PseudoRegister *> * args, GrowableArray <PseudoRegister *> * expr_stack, bool_t superSend, SendInfo * info ) {
    class SendNode * res = new class SendNode( key, nlrTestPoint, args, expr_stack, superSend, info );
    st_assert( expr_stack, "must have expression stack" );
    res->scope()->addSend( expr_stack, true );  // arguments to call are debug-visible
    registerNode( res );
    return res;
}


PrimitiveNode * NodeFactory::PrimitiveNode( PrimitiveDescriptor * pdesc, class MergeNode * nlrTestPoint, GrowableArray <PseudoRegister *> * args, GrowableArray <PseudoRegister *> * expr_stack ) {
    class PrimitiveNode * res = new class PrimitiveNode( pdesc, nlrTestPoint, args, expr_stack );
    if ( pdesc->can_walk_stack() ) {
        st_assert( expr_stack, "must have expression stack" );
        if ( expr_stack )
            res->scope()->addSend( expr_stack, true );  // arguments to some prim calls are debug-visible
    } else {
        st_assert( expr_stack == nullptr, "should not have expression stack" );
    }
    registerNode( res );
    return res;
}


DLLNode * NodeFactory::DLLNode( SymbolOop dll_name, SymbolOop function_name, dll_func function, bool_t async, class MergeNode * nlrTestPoint, GrowableArray <PseudoRegister *> * args, GrowableArray <PseudoRegister *> * expr_stack ) {
    class DLLNode * res = new class DLLNode( dll_name, function_name, function, async, nlrTestPoint, args, expr_stack );
    res->scope()->addSend( expr_stack, true );  // arguments to DLL call are debug-visible
    registerNode( res );
    return res;
}


InterruptCheckNode * NodeFactory::InterruptCheckNode( GrowableArray <PseudoRegister *> * exprStack ) {
    class InterruptCheckNode * res = new class InterruptCheckNode( exprStack );
    registerNode( res );
    return res;
}


LoopHeaderNode * NodeFactory::LoopHeaderNode() {
    class LoopHeaderNode * res = new class LoopHeaderNode();
    registerNode( res );
    return res;
}


BlockCreateNode * NodeFactory::BlockCreateNode( BlockPseudoRegister * b, GrowableArray <PseudoRegister *> * expr_stack ) {
    class BlockCreateNode * res = new class BlockCreateNode( b, expr_stack );
    registerNode( res );
    return res;
}


BlockMaterializeNode * NodeFactory::BlockMaterializeNode( BlockPseudoRegister * b, GrowableArray <PseudoRegister *> * expr_stack ) {
    class BlockMaterializeNode * res = new class BlockMaterializeNode( b, expr_stack );
    registerNode( res );
    return res;
}


ContextCreateNode * NodeFactory::ContextCreateNode( PseudoRegister * parent, PseudoRegister * context, int nofTemps, GrowableArray <PseudoRegister *> * expr_stack ) {
    class ContextCreateNode * res = new class ContextCreateNode( parent, context, nofTemps, expr_stack );
    registerNode( res );
    return res;
}


ContextCreateNode * NodeFactory::ContextCreateNode( PseudoRegister * b, const class ContextCreateNode * n, GrowableArray <PseudoRegister *> * expr_stack ) {
    class ContextCreateNode * res = new class ContextCreateNode( b, n, expr_stack );
    registerNode( res );
    return res;
}


ContextInitNode * NodeFactory::ContextInitNode( class ContextCreateNode * creator ) {
    class ContextInitNode * res = new class ContextInitNode( creator );
    registerNode( res );
    return res;
}


ContextInitNode * NodeFactory::ContextInitNode( PseudoRegister * b, const class ContextInitNode * n ) {
    class ContextInitNode * res = new class ContextInitNode( b, n );
    registerNode( res );
    return res;
}


ContextZapNode * NodeFactory::ContextZapNode( PseudoRegister * context ) {
    class ContextZapNode * res = new class ContextZapNode( context );
    registerNode( res );
    return res;
}


BranchNode * NodeFactory::BranchNode( BranchOpCode op, bool_t taken_is_uncommon ) {
    class BranchNode * res = new class BranchNode( op, taken_is_uncommon );
    registerNode( res );
    return res;
}


TypeTestNode * NodeFactory::TypeTestNode( PseudoRegister * recv, GrowableArray <KlassOop> * classes, bool_t hasUnknown ) {
    class TypeTestNode * res = new class TypeTestNode( recv, classes, hasUnknown );
    registerNode( res );
    res->scope()->addTypeTest( res );
    return res;
}


ArrayAtNode * NodeFactory::ArrayAtNode( ArrayAtNode::AccessType access_type, PseudoRegister * array, PseudoRegister * index, bool_t smiIndex, PseudoRegister * result, PseudoRegister * error, int data_offset, int length_offset ) {
    class ArrayAtNode * res = new class ArrayAtNode( access_type, array, index, smiIndex, result, error, data_offset, length_offset );
    registerNode( res );
    return res;
}


ArrayAtPutNode * NodeFactory::ArrayAtPutNode( ArrayAtPutNode::AccessType access_type, PseudoRegister * array, PseudoRegister * index, bool_t smi_index, PseudoRegister * element, bool_t smi_element, PseudoRegister * result, PseudoRegister * error, int data_offset, int length_offset, bool_t needs_store_check ) {
    class ArrayAtPutNode * res = new class ArrayAtPutNode( access_type, array, index, smi_index, element, smi_element, result, error, data_offset, length_offset, needs_store_check );
    registerNode( res );
    return res;
}


InlinedPrimitiveNode * NodeFactory::InlinedPrimitiveNode( InlinedPrimitiveNode::Operation op, PseudoRegister * result, PseudoRegister * error, PseudoRegister * recv, PseudoRegister * arg1, bool_t arg1_is_smi, PseudoRegister * arg2, bool_t arg2_is_smi ) {
    class InlinedPrimitiveNode * res = new class InlinedPrimitiveNode( op, result, error, recv, arg1, arg1_is_smi, arg2, arg2_is_smi );
    registerNode( res );
    return res;
}


UncommonNode * NodeFactory::UncommonNode( GrowableArray <PseudoRegister *> * exprStack, int byteCodeIndex ) {
    class UncommonNode * res = new class UncommonNode( exprStack, byteCodeIndex );
    registerNode( res );
    st_assert( exprStack, "must have expr. stack" );
    res->scope()->addSend( exprStack, false );  // current expr stack is debug-visible
    return res;
}


UncommonSendNode * NodeFactory::UncommonSendNode( GrowableArray <PseudoRegister *> * exprStack, int byteCodeIndex, int args ) {
    class UncommonSendNode * res = new class UncommonSendNode( exprStack, byteCodeIndex, args );
    registerNode( res );
    st_assert( exprStack, "must have expr. stack" );
    res->scope()->addSend( exprStack, false );  // current expr stack is debug-visible
    return res;
}


FixedCodeNode * NodeFactory::FixedCodeNode( FixedCodeNode::FixedCodeKind k ) {
    class FixedCodeNode * res = new class FixedCodeNode( k );
    registerNode( res );
    return res;
}


NopNode * NodeFactory::NopNode() {
    class NopNode * res = new class NopNode();
    registerNode( res );
    return res;
}


CommentNode * NodeFactory::CommentNode( const char * comment ) {
    class CommentNode * res = new class CommentNode( comment );
    registerNode( res );
    return res;
}


void initNodes() {
    Node::currentID             = Node::currentCommentID = 0;
    Node::lastScopeInfo         = ( ScopeInfo ) - 1;
    Node::lastByteCodeIndex      = IllegalByteCodeIndex;
    NodeFactory::_cumulativeCost = 0;
}


void BasicNode::setScope( InlinedScope * s ) {
    _scope         = s;
    _byteCodeIndex = s ? s->byteCodeIndex() : IllegalByteCodeIndex;
    st_assert( not s or not s->isInlinedScope() or s->byteCodeIndex() <= ( ( InlinedScope * ) s )->nofBytes(), "illegal byteCodeIndex" );
}


BasicNode::BasicNode() {
    _id         = currentID++;
    _basicBlock = nullptr;
    setScope( theCompiler->currentScope() );
    _num                   = -1;
    _dontEliminate         = _deleted = false;
    _pseudoRegisterMapping = nullptr;
}


PseudoRegisterMapping * BasicNode::mapping() const {
    return new PseudoRegisterMapping( _pseudoRegisterMapping );
}


void BasicNode::setMapping( PseudoRegisterMapping * mapping ) {
    st_assert( not hasMapping(), "cannot be assigned twice" );
    _pseudoRegisterMapping = new PseudoRegisterMapping( mapping );
}


NonTrivialNode::NonTrivialNode() {
    _src     = _dest = nullptr;
    _srcUse  = nullptr;
    _destDef = nullptr;
}


LoadUplevelNode::LoadUplevelNode( PseudoRegister * dst, PseudoRegister * context0, int nofLevels, int offset, SymbolOop name ) :
    LoadNode( dst ) {
    st_assert( context0 not_eq nullptr, "context0 is nullptr" );
    st_assert( nofLevels >= 0, "nofLevels must be >= 0" );
    st_assert( offset >= 0, "offset must be >= 0" );
    _context0    = context0;
    _context0Use = nullptr;
    _nofLevels   = nofLevels;
    _offset      = offset;
    _name        = name;
}


StoreUplevelNode::StoreUplevelNode( PseudoRegister * src, PseudoRegister * context0, int nofLevels, int offset, SymbolOop name, bool_t needsStoreCheck ) :
    StoreNode( src ) {
    st_assert( context0 not_eq nullptr, "context0 is nullptr" );
    st_assert( nofLevels >= 0, "nofLevels must be >= 0" );
    st_assert( offset >= 0, "offset must be >= 0" );
    _context0        = context0;
    _nofLevels       = nofLevels;
    _offset          = offset;
    _needsStoreCheck = needsStoreCheck;
    _name            = name;
}


AssignNode::AssignNode( PseudoRegister * s, PseudoRegister * d ) :
    StoreNode( s ) {
    _dest = d;
    st_assert( d, "dest is nullptr" );
    // Fix this Lars assert(not s->isNoPReg(), "source must be a real PseudoRegister");
    st_assert( s not_eq d, "creating dummy assignment" );
}


CommentNode::CommentNode( const char * s ) {
    comment = s;
    // give all comments negative ids (don't disturb node numbers by turning
    // CompilerDebug off and on)
    _id     = --currentCommentID;
    currentID--;
}


ArrayAtNode::ArrayAtNode( AccessType access_type, PseudoRegister * array, PseudoRegister * index, bool_t smiIndex, PseudoRegister * result, PseudoRegister * error, int data_offset, int length_offset ) :
    AbstractArrayAtNode( array, index, smiIndex, result, error, data_offset, length_offset ) {
    _access_type = access_type;
}


ArrayAtPutNode::ArrayAtPutNode( AccessType access_type, PseudoRegister * array, PseudoRegister * index, bool_t smi_index, PseudoRegister * element, bool_t smi_element, PseudoRegister * result, PseudoRegister * error, int data_offset, int length_offset, bool_t needs_store_check ) :
    AbstractArrayAtPutNode( array, index, smi_index, element, result, error, data_offset, length_offset ) {
    _access_type               = access_type;
    _needs_store_check         = needs_store_check;
    _smi_element               = smi_element;
    _needs_element_range_check = ( access_type == byte_at_put or access_type == double_byte_at_put );
}


TypeTestNode::TypeTestNode( PseudoRegister * rr, GrowableArray <KlassOop> * classes, bool_t hasUnknown ) {
    _src        = rr;
    _classes    = classes;
    _hasUnknown = hasUnknown;
    int len = classes->length();
    st_assert( len > 0, "should have at least one class to test" );
    // The assertion below has been replaced by a warning since
    // sometimes Inliner::inlineMerge(...) creates such a TypeTestNode.
    // FIX THIS
    // st_assert( (len > 1) or hasUnknown, "TypeTestNode is not necessary" );
    if ( ( len == 1 ) and not hasUnknown ) {
        warning( "TypeTestNode with only one klass & no uncommon case => performance bug" );
    }
    for ( int i = 0; i < len; i++ ) {
        for ( int j = i + 1; j < len; j++ ) {
            st_assert( classes->at( i ) not_eq classes->at( j ), "duplicate class" );
        }
    }
}


ArithRRNode::ArithRRNode( ArithOpCode op, PseudoRegister * arg1, PseudoRegister * arg2, PseudoRegister * dst ) :
    ArithNode( op, arg1, dst ) {
    _oper = arg2;
    if ( _src->isConstPReg() and ArithOpIsCommutative[ _op ] ) {
        // make sure that if there's a constant argument, it's the 2nd one
        PseudoRegister * t1 = _src;
        _src                = _oper;
        _oper               = t1;
    }
}


TArithRRNode::TArithRRNode( ArithOpCode op, PseudoRegister * arg1, PseudoRegister * arg2, PseudoRegister * dst, bool_t arg1IsInt, bool_t arg2IsInt ) {
    if ( arg1->isConstPReg() and ArithOpIsCommutative[ op ] ) {
        // make sure that if there's a constant argument, it's the 2nd one
        PseudoRegister * t1 = arg1;
        arg1 = arg2;
        arg2 = t1;
        bool_t t2 = arg1IsInt;
        arg1IsInt = arg2IsInt;
        arg2IsInt = t2;
    }
    _op            = op;
    _src           = arg1;
    _oper          = arg2;
    _dest          = dst;
    _arg1IsInt     = arg1IsInt;
    _arg2IsInt     = arg2IsInt;
    _constResult   = nullptr;
    _dontEliminate = true; // don't eliminate even if result unused because primitive might fail
}


PseudoRegister * NonTrivialNode::dest() const {
    if ( not hasDest() ) fatal( "has no dest" );
    return _dest;
}


void NonTrivialNode::setDest( BasicBlock * bb, PseudoRegister * d ) {
    // bb == nullptr means don't update definitions
    if ( not hasDest() ) fatal( "has no dest" );
    st_assert( bb or not _destDef, "shouldn't have a def" );
    if ( _destDef )
        _dest->removeDef( bb, _destDef );
    _dest = d;
    if ( bb )
        _destDef = _dest->addDef( bb, ( NonTrivialNode * )
    this );
}


PseudoRegister * NonTrivialNode::src() const {
    if ( not hasSrc() ) fatal( "has no src" );
    return _src;
}


bool_t AssignNode::isAccessingFloats() const {
    // After building the node data structure, float pregs have a float location but
    // later during compilation, this location is transformed into a stack location,
    // therefore the two tests. This should change at some point; it would be cleaner
    // to have a special FloatPseudoRegisters (or a flag in the PseudoRegister, respectively).
    return _src->_location.isFloatLocation() or _src->_location == topOfFloatStack or Mapping::isFloatTemporary( _src->_location ) or _dest->_location.isFloatLocation() or _dest->_location == topOfFloatStack or Mapping::isFloatTemporary( _dest->_location );
}


Oop AssignNode::constantSrc() const {
    st_assert( hasConstantSrc(), "no constant src" );
    return ( ( ConstPseudoRegister * ) _src )->constant;
}


bool_t AssignNode::canBeEliminated() const {
    return not( _src->_location.isTopOfStack() or _dest->_location.isTopOfStack() );
}


bool_t Node::endsBasicBlock() const {
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


void Node::removePrev( Node * n ) {
    /* cut the _prev link between this and n	*/
    st_assert( hasSinglePredecessor(), "subclass" );
    st_assert( _prev == n, "should be same" );
    _prev = nullptr;
}


void Node::removeNext( Node * n ) {
    /* cut the next link between this and n */
    st_assert( hasSingleSuccessor(), "subclass" );
    st_assert( _next == n, "should be same" );
    n->removePrev( this );
    _next = nullptr;
}


Node * Node::endOfList() const {
    if ( _next == nullptr )
        return ( Node * )
    this;
    Node * n = _next;
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
        fatal( "not implemented yet" );
    }
}


void AbstractMergeNode::movePrev( Node * from, Node * to ) {
    for ( int i = _prevs->length() - 1; i >= 0; i-- ) {
        if ( _prevs->at( i ) == from ) {
            _prevs->at_put( i, to );
            return;
        }
    }
    fatal( "from not found" );
}


bool_t AbstractMergeNode::isPredecessor( const Node * n ) const {
    for ( int i = _prevs->length() - 1; i >= 0; i-- ) {
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
        fatal( "not implemented yet" );
    }
}


void AbstractBranchNode::removeNext( Node * n ) {
    /* cut the next link between this and n */
    if ( n == _next ) {
        n->removePrev( this );
        _next = nullptr;
    } else {
        int i = 0;
        for ( ; i < _nxt->length() and _nxt->at( i ) not_eq n; i++ );
        st_assert( i < _nxt->length(), "not found" );
        n->removePrev( this );
        for ( ; i < _nxt->length() - 1; i++ )
            _nxt->at_put( i, _nxt->at( i + 1 ) );
        _nxt->pop();
    }
}


void AbstractBranchNode::setNext( int i, Node * n ) {
    if ( i == 0 ) {
        setNext( n );
    } else {
        st_assert( _nxt->length() == i - 1, "wrong index" );
        _nxt->append( n );
    }
}


void AbstractBranchNode::moveNext( Node * from, Node * to ) {
    if ( _next == from ) {
        _next = to;
    } else {
        int i = 0;
        for ( ; i < _nxt->length() and _nxt->at( i ) not_eq from; i++ );
        st_assert( i < _nxt->length(), "not found" );
        _nxt->at_put( i, to );
    }
}


bool_t AbstractBranchNode::isSuccessor( const Node * n ) const {
    if ( _next == n ) {
        return true;
    } else {
        int i = 0;
        for ( ; i < _nxt->length() and _nxt->at( i ) not_eq n; i++ );
        return i < _nxt->length();
    }
}


BasicBlock * BasicNode::newBasicBlock() {
    if ( _basicBlock )
        return _basicBlock;

    int len = 0;
    _basicBlock = new BasicBlock( ( Node * )
    this, ( Node * )
    this, 1 );

    Node * n = ( Node * )
    this;
    for ( ; not n->endsBasicBlock() and n->next() not_eq nullptr; n = n->next() ) {
        n->_num        = len++;
        n->_basicBlock = _basicBlock;
    }

    n->_num                 = len++;
    n->_basicBlock          = _basicBlock;
    _basicBlock->_last      = n;
    _basicBlock->_nodeCount = len;

    return _basicBlock;
}


MergeNode::MergeNode( Node * prev1, Node * prev2 ) :
    AbstractMergeNode( prev1, prev2 ) {
    _byteCodeIndex = max( prev1->byteCodeIndex(), prev2->byteCodeIndex() );
    _isLoopStart   = _isLoopEnd = _didStartBasicBlock = false;
}


MergeNode::MergeNode( int byteCodeIndex ) {
    _byteCodeIndex = byteCodeIndex;
    _isLoopStart   = _isLoopEnd = _didStartBasicBlock = false;
}


BasicBlock * MergeNode::newBasicBlock() {
    if ( _basicBlock != nullptr )
        return _basicBlock;

    // receiver starts a new BasicBlock
    _didStartBasicBlock = true;
    _basicBlock         = Node::newBasicBlock();

    return _basicBlock;
}


ReturnNode::ReturnNode( PseudoRegister * res, int byteCodeIndex ) :
    AbstractReturnNode( byteCodeIndex, res, new TemporaryPseudoRegister( theCompiler->currentScope(), resultLoc, true, true ) ) {
    st_assert( res->_location == resultLoc, "must be in special location" );
}


NonLocalReturnSetupNode::NonLocalReturnSetupNode( PseudoRegister * result, int byteCodeIndex ) :
    AbstractReturnNode( byteCodeIndex, result, new TemporaryPseudoRegister( theCompiler->currentScope(), resultLoc, true, true ) ) {
    _contextUse = _resultUse = nullptr;
    st_assert( result->_location == NonLocalReturnResultLoc, "must be in special location" );
}


MergeNode * CallNode::nlrTestPoint() const {
    if ( nSuccessors() > 1 ) {
        st_assert( next1()->isMergeNode(), "should be a merge" );
        return ( MergeNode * ) next1();
    } else {
        return nullptr;
    }
}


CallNode::CallNode( MergeNode * n, GrowableArray < PseudoRegister * > *a, GrowableArray < PseudoRegister * > *e ) {
    if ( n not_eq nullptr )
        append1( n );
    exprStack   = e;
    args        = a;
    _dest       = new SinglyAssignedPseudoRegister( scope(), resultLoc, false, false, _byteCodeIndex, _byteCodeIndex );
    argUses     = uplevelUses = nullptr;
    uplevelDefs = nullptr;
    uplevelUsed = uplevelDefd = nullptr;
    nblocks     = theCompiler->blockClosures->length();
}


SendNode::SendNode( LookupKey * key, MergeNode * nlrTestPoint, GrowableArray <PseudoRegister *> * args, GrowableArray <PseudoRegister *> * expr_stack, bool_t superSend, SendInfo * info ) :
    CallNode( nlrTestPoint, args, expr_stack ) {
    _key       = key;
    _superSend = superSend;
    _info      = info;
    st_assert( exprStack, "should have expr stack" );
    // Fix this when compiler is more flexible
    // not a fatal because it could happen for super sends that fail (no super method found)
    if ( _superSend and not UseNewBackend )
        warning( "We cannot yet have super sends in nativeMethods" );
}


ContextCreateNode::ContextCreateNode( PseudoRegister * parent, PseudoRegister * context, int nofTemps, GrowableArray <PseudoRegister *> * expr_stack ) :
    PrimitiveNode( Primitives::context_allocate(), nullptr, nullptr, expr_stack ) {
    _src               = parent;
    _dest              = context;
    _nofTemps          = nofTemps;
    _contextSize       = 0;
    _parentContexts    = nullptr;
    _parentContextUses = nullptr;
    Scope          * p           = _scope->parent();
    PseudoRegister * prevContext = parent;
    // collect all parent contexts
    while ( p and p->isInlinedScope() and ( ( InlinedScope * ) p )->context() ) {
        PseudoRegister * c = ( ( InlinedScope * ) p )->context();
        if ( c not_eq prevContext ) {
            if ( not _parentContexts )
                _parentContexts = new GrowableArray <PseudoRegister *>( 5 );
            _parentContexts->append( c );
            prevContext = c;
        }
        p                  = p->parent();
    }
}


ContextCreateNode::ContextCreateNode( PseudoRegister * b, const ContextCreateNode * n, GrowableArray <PseudoRegister *> * expr_stack ) :
    PrimitiveNode( Primitives::context_allocate(), nullptr, nullptr, expr_stack ) {
    warning( "check this implementation" );
    Unimplemented();
    // Urs, don't we need a source here?
    // I've added hasSrc() (= true) to ContextCreateNode) - should double check this
    // What is this constructor good for? Cloning only? src should be taken care of as well, I guess.
    // This constructor would be called only for splitting (when copying the node) -- won't happen for now.
    _dest              = b;
    _nofTemps          = n->_nofTemps;
    _parentContextUses = nullptr;
}


ContextInitNode::ContextInitNode( ContextCreateNode * creator ) {
    int nofTemps = creator->nofTemps();
    _src = creator->context();
    st_assert( _src, "must have context" );
    _initializers       = new GrowableArray <Expression *>( nofTemps, nofTemps, nullptr );    // holds initializer for each element (or nullptr)
    _contentDefs        = nullptr;
    _initializerUses    = nullptr;
    _materializedBlocks = nullptr;
}


ContextInitNode::ContextInitNode( PseudoRegister * b, const ContextInitNode * node ) {
    _src = b;
    st_assert( _src, "must have context" );
    _initializers = node->_initializers;
    st_assert( ( node->_contentDefs == nullptr ) and ( node->_initializerUses == nullptr ), "shouldn't copy after uses have been built" );
    _contentDefs        = nullptr;
    _initializerUses    = nullptr;
    _materializedBlocks = nullptr;
}


BlockCreateNode::BlockCreateNode( BlockPseudoRegister * b, GrowableArray <PseudoRegister *> * expr_stack ) :
    PrimitiveNode( Primitives::block_allocate(), nullptr, nullptr, expr_stack ) {
    _src        = nullptr;
    _dest       = b;
    _contextUse = nullptr;
    switch ( b->method()->block_info() ) {
        case MethodOopDescriptor::expects_nil:        // no context needed
            _context = nullptr;
            break;
        case MethodOopDescriptor::expects_self:
            _context = b->scope()->self()->preg();
            break;
        case MethodOopDescriptor::expects_parameter:    // fix this -- should find which
            _context = nullptr;
            break;
        case MethodOopDescriptor::expects_context:
            _context = b->scope()->context();
            break;
        default:
            fatal( "unexpected incoming info" );
    };
}


int ContextInitNode::positionOfContextTemp( int n ) const {
    // return position of ith context temp in compiled (physical) context
    int       pos = 0;
    for ( int i   = 0; i < n; i++ ) {
        PseudoRegister * p = contents()->at( i )->preg();
        if ( p->_location.isContextLocation() )
            pos++;
    }
    return pos;
}


void ContextInitNode::initialize( int no, Expression * expr ) {
    st_assert( ( _initializers->at( no ) == nullptr ) or ( _initializers->at( no )->constant() == nilObj ), "already initialized this context element" );
    _initializers->at_put( no, expr );
}


ContextCreateNode * ContextInitNode::creator() const {
    // returns the corresponding context creation node
    Node * n = _prev;
    st_assert( n->isContextCreateNode(), "must be creator node" );
    return ( ContextCreateNode * ) n;
}


void ContextInitNode::addBlockMaterializer( BlockMaterializeNode * n ) {
    if ( not _materializedBlocks )
        _materializedBlocks = new GrowableArray <BlockMaterializeNode *>( 5 );
    _materializedBlocks->append( n );
}


void ContextInitNode::notifyNoContext() {
    // our context has been optimized away, i.e., all context contents
    // will be stack-allocated like normal PseudoRegisters
    // remove the context use (otherwise the contextPR has 1 use and no definitions)
    _src->removeUse( bb(), _srcUse );
    _src = nullptr;
    if ( _materializedBlocks ) {
        for ( int i = _materializedBlocks->length() - 1; i >= 0; i-- ) {
            // remove the block materialization node
            BlockMaterializeNode * n = _materializedBlocks->at( i );
            n->eliminate( n->bb(), nullptr, true, false );
            PseudoRegister * blk = n->src();
            st_assert( blk->isBlockPReg(), "must be a block" );

            // remove use of block
            for ( int j = _initializers->length() - 1; j >= 0; j-- ) {
                if ( _initializers->at( j )->preg() == blk ) {
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


PrimitiveNode::PrimitiveNode( PrimitiveDescriptor * pdesc, MergeNode * nlrTestPoint, GrowableArray <PseudoRegister *> * args, GrowableArray <PseudoRegister *> * expr_stack ) :
    CallNode( nlrTestPoint, args, expr_stack ) {
    _pdesc = pdesc;
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


InlinedPrimitiveNode::InlinedPrimitiveNode( Operation op, PseudoRegister * result, PseudoRegister * error, PseudoRegister * recv, PseudoRegister * arg1, bool_t arg1_is_smi, PseudoRegister * arg2, bool_t arg2_is_smi ) {
    _operation = op;
    _dest      = result;
    _error       = error;
    _src         = recv;
    _arg1        = arg1;
    _arg2        = arg2;
    _arg1_is_smi = arg1_is_smi;
    _arg2_is_smi = arg2_is_smi;
}


bool_t InlinedPrimitiveNode::canFail() const {
    switch ( op() ) {
        case Operation::obj_klass:
            return false;
        case Operation::obj_hash:
            return false;
        case Operation::proxy_byte_at:
            return not arg1_is_smi();
        case Operation::proxy_byte_at_put:
            return not arg1_is_smi() or not arg2_is_smi();
    };
    ShouldNotReachHere();
    return false;
}


bool_t InlinedPrimitiveNode::canBeEliminated() const {
    switch ( op() ) {
        case Operation::obj_klass:
            return true;
        case Operation::obj_hash:
            return true;
        case Operation::proxy_byte_at:
            return not canFail();
        case Operation::proxy_byte_at_put:
            return false;
    };
    ShouldNotReachHere();
    return false;
}


UncommonNode::UncommonNode( GrowableArray <PseudoRegister *> * e, int byteCodeIndex ) {
    exprStack      = e;
    _byteCodeIndex = byteCodeIndex;
}


UncommonSendNode::UncommonSendNode( GrowableArray <PseudoRegister *> * e, int byteCodeIndex, int argCount ) :
    UncommonNode( e, byteCodeIndex ) {
    this->_argCount = argCount;
}


Node * UncommonSendNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::UncommonSendNode( this->expressionStack()->copy(), byteCodeIndex(), _argCount );
}


void UncommonSendNode::makeUses( BasicBlock * bb ) {
    int       expressionCount = expressionStack()->length();
    for ( int pos             = expressionCount - _argCount; pos < expressionCount; pos++ )
        bb->addUse( this, expressionStack()->at( pos ) );
}


void UncommonSendNode::verify() const {
    if ( _argCount > expressionStack()->length() )
        error( "Too few expressions on stack for 0x%x: required %d, but got %d", this, _argCount, expressionStack()->length() );
    UncommonNode::verify();
}


bool_t PrimitiveNode::canBeEliminated() const {
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
    if ( _pdesc->fn() == primitiveFunctionType( &behaviorPrimitives::allocate ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew0 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew1 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew2 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew3 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew4 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew5 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew6 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew7 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew8 ) or _pdesc->fn() == primitiveFunctionType( &primitiveNew9 ) ) {
        return true;
    }

    return false;
}


bool_t PrimitiveNode::canInvokeDelta() const {
    return _pdesc->can_invoke_delta();
}


bool_t PrimitiveNode::canFail() const {
    return _pdesc->can_fail();
}


DLLNode::DLLNode( SymbolOop dll_name, SymbolOop function_name, dll_func function, bool_t async, MergeNode * nlrTestPoint, GrowableArray <PseudoRegister *> * args, GrowableArray <PseudoRegister *> * expr_stack ) :
    CallNode( nlrTestPoint, args, expr_stack ) {
    _dll_name      = dll_name;
    _function_name = function_name;
    _function      = function;
    _async         = async;
}


bool_t DLLNode::canInvokeDelta() const {
    return true;        // user-defined DLL code can do anything
}


NonLocalReturnTestNode::NonLocalReturnTestNode( int byteCodeIndex ) {
}


void NonLocalReturnTestNode::fixup() {
    // connect next() and next1()
    if ( scope()->isTop() ) {
        theCompiler->enterScope( scope() ); // so that nodes get right scope
        // Non-inlined scope: continuation just continues NonLocalReturn
        append( 0, NodeFactory::NonLocalReturnContinuationNode( byteCodeIndex() ) );
        // return point returns the NonLocalReturn result
        PseudoRegister * nlr_result = new TemporaryPseudoRegister( scope(), resultOfNonLocalReturn, true, true );
        Node           * n          = NodeFactory::AssignNode( nlr_result, scope()->resultPR );
        append( 1, n );
        n->append( scope()->returnPoint() );
        theCompiler->exitScope( scope() );
    } else {
        // Inlined scope: for continuation, follow caller chain until a nlrTestPoint is found
        // (the top scope is guaranteed to have a nlrTestPoint, so loop below will terminate correctly)
        // introduce an extra assignment to satisfy new backend, will be optimized away
        InlinedScope * s = scope()->sender();
        while ( not s->has_nlrTestPoint() )
            s = s->sender();
        append( 0, s->nlrTestPoint() );
        // now connect to return point and return the NonLocalReturn value
        // s may not have a return point, so search for one
        s     = scope();
        while ( s->returnPoint() == nullptr )
            s = s->sender();
        theCompiler->enterScope( s ); // so that node gets right scope
        PseudoRegister * nlr_result = new TemporaryPseudoRegister( scope(), resultOfNonLocalReturn, true, true );
        Node           * n          = NodeFactory::AssignNode( nlr_result, s->resultPR );
        theCompiler->exitScope( s );
        append( 1, n );
        n->append( s->returnPoint() );
    }
}


bool_t SendNode::isCounting() const {
    return _info->_counting;
}


bool_t SendNode::isUninlinable() const {
    return _info->uninlinable;
}


bool_t SendNode::staticReceiver() const {
    return _info->_receiverStatic;
}


PseudoRegister * SendNode::recv() const {
    int i = args->length() - 1;
    while ( i >= 0 and args->at( i )->_location not_eq receiverLoc )
        i--;
    st_assert( i >= 0, "must have a receiver" );
    return args->at( i );
}


// ==================================================================================
// cloning: copy the node during splitting; returning nullptr means node is a de facto
// no-op only need to copy the basic state; definitions, uses etc haven't yet been computed
// ==================================================================================

// general splitting hasn't been implemented yet; only nodes that shouldCopyWhenSplitting()
// are currently copied  -Urs 5/96

Node * BasicNode::copy( PseudoRegister * from, PseudoRegister * to ) const {
    Node * c = clone( from, to );
    if ( c ) {
        c->_scope         = _scope;
        c->_byteCodeIndex = _byteCodeIndex;
    }
    return c;
}


#define SHOULD_NOT_CLONE    { ShouldNotCallThis(); return nullptr; }
#define TRANSLATE( s )        ((s == from) ? to : s)


Node * PrologueNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    SHOULD_NOT_CLONE
}


Node * NonLocalReturnSetupNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    SHOULD_NOT_CLONE
}


Node * NonLocalReturnContinuationNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    SHOULD_NOT_CLONE
}


Node * ReturnNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    SHOULD_NOT_CLONE
}


Node * BranchNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    SHOULD_NOT_CLONE
}


Node * TypeTestNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    SHOULD_NOT_CLONE
}


Node * FixedCodeNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    SHOULD_NOT_CLONE
}


Node * LoadOffsetNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::LoadOffsetNode( TRANSLATE( _dest ), _src, _offset, _isArraySize );
}


Node * LoadIntNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::LoadIntNode( TRANSLATE( _dest ), _value );
}


Node * LoadUplevelNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::LoadUplevelNode( TRANSLATE( _dest ), TRANSLATE( _context0 ), _nofLevels, _offset, _name );
}


Node * StoreOffsetNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::StoreOffsetNode( TRANSLATE( _src ), _base, _offset, _needsStoreCheck );
}


Node * StoreUplevelNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::StoreUplevelNode( TRANSLATE( _src ), TRANSLATE( _context0 ), _nofLevels, _offset, _name, _needsStoreCheck );
}


Node * AssignNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::AssignNode( TRANSLATE( _src ), TRANSLATE( _dest ) );
}


Node * ArithRRNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::ArithRRNode( TRANSLATE( _dest ), TRANSLATE( _src ), _op, _oper );
}


Node * TArithRRNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::TArithRRNode( TRANSLATE( _dest ), TRANSLATE( _src ), _op, _oper, _arg1IsInt, _arg2IsInt );
}


Node * ArithRCNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::ArithRCNode( TRANSLATE( _dest ), TRANSLATE( _src ), _op, _oper );
}


Node * SendNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    // NB: use current split signature, not the receiver's sig!
    SendNode * n = NodeFactory::SendNode( _key, nlrTestPoint(), args, exprStack, _superSend, _info );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node * PrimitiveNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    // NB: use scope's current sig, not the receiver's sig!
    PrimitiveNode * n = NodeFactory::PrimitiveNode( _pdesc, nlrTestPoint(), args, exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node * DLLNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    // NB: use scope's current sig, not the receiver's sig!
    DLLNode * n = NodeFactory::DLLNode( _dll_name, _function_name, _function, _async, nlrTestPoint(), args, exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node * InterruptCheckNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    // NB: use scope's current sig, not the receiver's sig!
    InterruptCheckNode * n = NodeFactory::InterruptCheckNode( exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node * BlockCreateNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    // NB: use scope's current sig, not the receiver's sig!
    BlockCreateNode * n = NodeFactory::BlockCreateNode( ( BlockPseudoRegister * )TRANSLATE( block() ), exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node * BlockMaterializeNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    // NB: use scope's current sig, not the receiver's sig!
    BlockMaterializeNode * n = NodeFactory::BlockMaterializeNode( ( BlockPseudoRegister * )TRANSLATE( block() ), exprStack );
    st_assert( _dest not_eq from, "shouldn't change dest" );
    n->_dest = _dest;        // don't give it a new dest!
    return n;
}


Node * ContextCreateNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::ContextCreateNode( TRANSLATE( _dest ), this, exprStack );
}


Node * ContextInitNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::ContextInitNode( TRANSLATE( _src ), this );
}


Node * ContextZapNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::ContextZapNode( TRANSLATE( _src ) );
}


Node * NonLocalReturnTestNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    Unimplemented();
    return nullptr;
}


Node * ArrayAtNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::ArrayAtNode( _access_type, TRANSLATE( _src ), TRANSLATE( _arg ), _intArg, TRANSLATE( _dest ), TRANSLATE( _error ), _dataOffset, _sizeOffset );
}


Node * ArrayAtPutNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::ArrayAtPutNode( _access_type, TRANSLATE( _src ), TRANSLATE( _arg ), _intArg, TRANSLATE( elem ), _smi_element, TRANSLATE( _dest ), TRANSLATE( _error ), _dataOffset, _sizeOffset, _needs_store_check );
}


Node * UncommonNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::UncommonNode( exprStack, _byteCodeIndex );
}


Node * InlinedReturnNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    return NodeFactory::InlinedReturnNode( byteCodeIndex(), TRANSLATE( src() ), TRANSLATE( dest() ) );
}


#define NO_NEED_TO_COPY    { return nullptr; }


Node * MergeNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    NO_NEED_TO_COPY
}


Node * NopNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    NO_NEED_TO_COPY
}


Node * CommentNode::clone( PseudoRegister * from, PseudoRegister * to ) const {
    NO_NEED_TO_COPY
}


// ==================================================================================
// makeUses: construct all uses and definitions
// ==================================================================================

void PrologueNode::makeUses( BasicBlock * bb ) {
    InlinedScope * s = scope();

    // build initial def for self and context (for blocks only)
    bb->addDef( this, s->self()->preg() );
    if ( s->isBlockScope() ) {
        bb->addDef( this, s->context() );
    }
    // build initial definitions for incoming args

    for ( int i = 0; i < _nofArgs; i++ ) {
        Expression * a = s->argument( i );
        if ( a )
            bb->addDef( this, a->preg() );
    }

    // build initial definitions for locals (initalization to nil)
    for ( int i = 0; i < _nofTemps; i++ ) {
        Expression * t = s->temporary( i );
        if ( t )
            bb->addDef( this, t->preg() );
    }
}


void NonTrivialNode::makeUses( BasicBlock * bb ) {
    _basicBlock = bb;
    st_assert( not hasSrc() or _srcUse, "should have srcUse" );
    st_assert( not hasDest() or _destDef or _dest->isNoPReg(), "should have destDef" );
}


void LoadNode::makeUses( BasicBlock * basicBlock ) {
    _destDef = basicBlock->addDef( this, _dest );
    NonTrivialNode::makeUses( basicBlock );
}


void LoadOffsetNode::makeUses( BasicBlock * bb ) {
    _srcUse = bb->addUse( this, _src );
    LoadNode::makeUses( bb );
}


void LoadUplevelNode::makeUses( BasicBlock * bb ) {
    _context0Use = bb->addUse( this, _context0 );
    LoadNode::makeUses( bb );
}


void StoreNode::makeUses( BasicBlock * bb ) {
    _srcUse = bb->addUse( this, _src );
    NonTrivialNode::makeUses( bb );
}


void StoreOffsetNode::makeUses( BasicBlock * bb ) {
    _baseUse = bb->addUse( this, _base );
    StoreNode::makeUses( bb );
}


void StoreUplevelNode::makeUses( BasicBlock * bb ) {
    _context0Use = bb->addUse( this, _context0 );
    StoreNode::makeUses( bb );
}


void AssignNode::makeUses( BasicBlock * bb ) {
    _destDef = bb->addDef( this, _dest );
    StoreNode::makeUses( bb );
}


void AbstractReturnNode::makeUses( BasicBlock * bb ) {
    _srcUse  = bb->addUse( this, _src );
    _destDef = bb->addDef( this, _dest );
    NonTrivialNode::makeUses( bb );
}


void ReturnNode::makeUses( BasicBlock * bb ) {
    // _resultUse models the value's use in the caller
    _resultUse = bb->addUse( this, _dest );
    AbstractReturnNode::makeUses( bb );
}


void NonLocalReturnSetupNode::makeUses( BasicBlock * bb ) {
    // has no src or dest -- uses hardwired NonLocalReturn register but track them anyway for consistency
    // _resultUse models the value's use in the caller
    _resultUse  = bb->addUse( this, _dest );
    _contextUse = bb->addUse( this, _scope->context() );
    AbstractReturnNode::makeUses( bb );
}


void NonLocalReturnTestNode::makeUses( BasicBlock * bb ) {
    AbstractBranchNode::makeUses( bb );
}


void NonLocalReturnContinuationNode::makeUses( BasicBlock * bb ) {
    // has no src or dest -- uses hardwired NonLocalReturn register but track them anyway for consistency
    AbstractReturnNode::makeUses( bb );
}


void ArithNode::makeUses( BasicBlock * bb ) {
    _srcUse  = bb->addUse( this, _src );
    _destDef = bb->addDef( this, _dest );
    NonTrivialNode::makeUses( bb );
}


void ArithRRNode::makeUses( BasicBlock * bb ) {
    _operUse = bb->addUse( this, _oper );
    ArithNode::makeUses( bb );
}


void TArithRRNode::makeUses( BasicBlock * bb ) {
    _operUse = bb->addUse( this, _oper );
    _srcUse  = bb->addUse( this, _src );
    _destDef = bb->addDef( this, _dest );
    NonTrivialNode::makeUses( bb );
}


void CallNode::makeUses( BasicBlock * bb ) {

    //
    _destDef = bb->addDef( this, _dest );
    if ( args ) {
        int len = args->length();
        argUses = new GrowableArray <Usage *>( len );
        for ( int i = 0; i < len; i++ ) {
            argUses->append( bb->addUse( this, args->at( i ) ) );
        }
    }
    NonTrivialNode::makeUses( bb );

    //
    if ( not canInvokeDelta() ) return;

    // add definitions/uses for all PseudoRegisters uplevel-accessed by live blocks
    const int InitialSize = 5;
    uplevelUses = new GrowableArray <Usage *>( InitialSize );
    uplevelDefs = new GrowableArray <Definition *>( InitialSize );
    uplevelUsed = new GrowableArray <PseudoRegister *>( InitialSize );
    uplevelDefd = new GrowableArray <PseudoRegister *>( InitialSize );
    GrowableArray < BlockPseudoRegister * > *blks = theCompiler->blockClosures;
    for ( int i1 = 0; i1 < nblocks; i1++ ) {
        BlockPseudoRegister * blk = blks->at( i1 );
        if ( !blk->escapes() ) continue;

        // check if block's home is on current call stack; if not, we don't care about the block's
        // uplevel accesses since the block is non-LIFO and we won't access its context anyway
        Scope * home = blk->scope()->home();
        if ( !home->isSenderOrSame( scope() ) ) continue;

        // ok, this block is live
        GrowableArray < PseudoRegister * > *uplevelRead = blk->uplevelRead();
        int j = uplevelRead->length() - 1;
        for ( ; j >= 0; j-- ) {
            PseudoRegister * r = uplevelRead->at( j );
            uplevelUses->append( bb->addUse( this, r ) );
            uplevelUsed->append( r );
        }
        GrowableArray < PseudoRegister * > *uplevelWritten = blk->uplevelWritten();
        for ( j = uplevelWritten->length() - 1; j >= 0; j-- ) {
            PseudoRegister * r = uplevelWritten->at( j );
            uplevelDefs->append( bb->addDef( this, r ) );
            uplevelDefd->append( r );
        }
    }
}


void BlockCreateNode::makeUses( BasicBlock * bb ) {
    if ( _context and not isMemoized() ) {
        _contextUse = bb->addUse( this, _context );
    }
    _destDef = bb->addDef( this, _dest );
    NonTrivialNode::makeUses( bb );
}


void BlockMaterializeNode::makeUses( BasicBlock * bb ) {
    // without memoization, BlockMaterializeNode is a no-op
    if ( isMemoized() ) {
        _srcUse         = bb->addUse( this, _src );
        if ( _context )
            _contextUse = bb->addUse( this, _context );
        BlockCreateNode::makeUses( bb );
    }
}


void ContextCreateNode::makeUses( BasicBlock * bb ) {
    if ( _src )
        _srcUse = bb->addUse( this, _src ); // no src if there's no incoming context
    _destDef    = bb->addDef( this, _dest );
    if ( _parentContexts ) {
        int len            = _parentContexts->length();
        _parentContextUses = new GrowableArray <Usage *>( len, len, nullptr );
        for ( int i = _parentContexts->length() - 1; i >= 0; i-- ) {
            Usage * u = bb->addUse( this, _parentContexts->at( i ) );
            _parentContextUses->at_put( i, u );
        }
    }
    NonTrivialNode::makeUses( bb );
}


void ContextInitNode::makeUses( BasicBlock * bb ) {
    _srcUse = bb->addUse( this, _src );
    int i = nofTemps();
    _contentDefs     = new GrowableArray <Definition *>( i, i, nullptr );
    _initializerUses = new GrowableArray <Usage *>( i, i, nullptr );
    while ( i-- > 0 ) {
        PseudoRegister * r = contents()->at( i )->preg();
        if ( r->isBlockPReg() ) {
            _contentDefs->at_put( i, nullptr );      // there is no assignment to the block
        } else {
            _contentDefs->at_put( i, bb->addDef( this, r ) );
        }
        _initializerUses->at_put( i, bb->addUse( this, _initializers->at( i )->preg() ) );
    }
    NonTrivialNode::makeUses( bb );
}


void ContextZapNode::makeUses( BasicBlock * bb ) {
    _srcUse = bb->addUse( this, _src );
}


void TypeTestNode::makeUses( BasicBlock * bb ) {
    _srcUse = bb->addUse( this, _src );
    AbstractBranchNode::makeUses( bb );
}


void AbstractArrayAtNode::makeUses( BasicBlock * bb ) {
    _srcUse   = bb->addUse( this, _src );
    _destDef  = _dest ? bb->addDef( this, _dest ) : nullptr;
    _argUse   = bb->addUse( this, _arg );
    _errorDef = ( _error and canFail() ) ? bb->addDef( this, _error ) : nullptr;
    AbstractBranchNode::makeUses( bb );
}


void AbstractArrayAtPutNode::makeUses( BasicBlock * bb ) {
    elemUse = bb->addUse( this, elem );
    AbstractArrayAtNode::makeUses( bb );
}


void InlinedPrimitiveNode::makeUses( BasicBlock * bb ) {
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
void NonTrivialNode::removeUses( BasicBlock * bb ) {
    st_assert( _basicBlock == bb, "wrong BasicBlock" );
}


void LoadNode::removeUses( BasicBlock * bb ) {
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void LoadOffsetNode::removeUses( BasicBlock * bb ) {
    _src->removeUse( bb, _srcUse );
    LoadNode::removeUses( bb );
}


void LoadUplevelNode::removeUses( BasicBlock * bb ) {
    _context0->removeUse( bb, _context0Use );
    LoadNode::removeUses( bb );
}


void StoreNode::removeUses( BasicBlock * bb ) {
    _src->removeUse( bb, _srcUse );
    NonTrivialNode::removeUses( bb );
}


void StoreOffsetNode::removeUses( BasicBlock * bb ) {
    _base->removeUse( bb, _baseUse );
    StoreNode::removeUses( bb );
}


void StoreUplevelNode::removeUses( BasicBlock * bb ) {
    _context0->removeUse( bb, _context0Use );
    StoreNode::removeUses( bb );
}


void AssignNode::removeUses( BasicBlock * bb ) {
    _dest->removeDef( bb, _destDef );
    StoreNode::removeUses( bb );
}


void AbstractReturnNode::removeUses( BasicBlock * bb ) {
    _src->removeUse( bb, _srcUse );
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void ReturnNode::removeUses( BasicBlock * bb ) {
    _dest->removeUse( bb, _resultUse );
    AbstractReturnNode::removeUses( bb );
}


void NonLocalReturnSetupNode::removeUses( BasicBlock * bb ) {
    _dest->removeUse( bb, _resultUse );
    _scope->context()->removeUse( bb, _contextUse );
    AbstractReturnNode::removeUses( bb );
}


void NonLocalReturnTestNode::removeUses( BasicBlock * bb ) {
    AbstractBranchNode::removeUses( bb );
}


void NonLocalReturnContinuationNode::removeUses( BasicBlock * bb ) {
    AbstractReturnNode::removeUses( bb );
}


void ArithNode::removeUses( BasicBlock * bb ) {
    _src->removeUse( bb, _srcUse );
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void ArithRRNode::removeUses( BasicBlock * bb ) {
    _oper->removeUse( bb, _operUse );
    ArithNode::removeUses( bb );
}


void TArithRRNode::removeUses( BasicBlock * bb ) {
    _oper->removeUse( bb, _operUse );
    _src->removeUse( bb, _srcUse );
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void CallNode::removeUses( BasicBlock * bb ) {
    _dest->removeDef( bb, _destDef );
    if ( argUses ) {
        int       len = args->length();
        for ( int i   = 0; i < len; i++ ) {
            args->at( i )->removeUse( bb, argUses->at( i ) );
        }
    }
    if ( uplevelUses ) {
        int i = uplevelUses->length() - 1;
        for ( ; i >= 0; i-- )
            uplevelUsed->at( i )->removeUse( bb, uplevelUses->at( i ) );
        for ( int i = uplevelDefs->length() - 1; i >= 0; i-- )
            uplevelDefd->at( i )->removeDef( bb, uplevelDefs->at( i ) );
    }
    NonTrivialNode::removeUses( bb );
}


void BlockCreateNode::removeUses( BasicBlock * bb ) {
    if ( _contextUse )
        _context->removeUse( bb, _contextUse );
    if ( _src )
        _src->removeUse( bb, _srcUse );
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void BlockMaterializeNode::removeUses( BasicBlock * bb ) {
    // without memoization, BlockMaterializeNode is a no-op
    if ( isMemoized() )
        BlockCreateNode::removeUses( bb );
}


void ContextCreateNode::removeUses( BasicBlock * bb ) {
    if ( _src )
        _src->removeUse( bb, _srcUse ); // no src if there's no incoming context
    _dest->removeDef( bb, _destDef );
    NonTrivialNode::removeUses( bb );
}


void ContextInitNode::removeUses( BasicBlock * bb ) {
    int i = nofTemps();
    while ( i-- > 0 ) {
        contents()->at( i )->preg()->removeDef( bb, _contentDefs->at( i ) );
        _initializers->at( i )->preg()->removeUse( bb, _initializerUses->at( i ) );
    }
    _src->removeUse( bb, _srcUse );
    NonTrivialNode::removeUses( bb );
}


void ContextZapNode::removeUses( BasicBlock * bb ) {
    _src->removeUse( bb, _srcUse );
    NonTrivialNode::removeUses( bb );
}


void TypeTestNode::removeUses( BasicBlock * bb ) {
    _src->removeUse( bb, _srcUse );
    AbstractBranchNode::removeUses( bb );
}


void AbstractArrayAtNode::removeUses( BasicBlock * bb ) {
    _src->removeUse( bb, _srcUse );
    if ( _dest )
        _dest->removeDef( bb, _destDef );
    _arg->removeUse( bb, _argUse );
    if ( _errorDef )
        _error->removeDef( bb, _errorDef );
    AbstractBranchNode::removeUses( bb );
}


void AbstractArrayAtPutNode::removeUses( BasicBlock * bb ) {
    elem->removeUse( bb, elemUse );
    AbstractArrayAtNode::removeUses( bb );
}


void InlinedPrimitiveNode::removeUses( BasicBlock * bb ) {
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

void Node::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t removing, bool_t cp ) {
    st_assert( not _deleted, "already deleted this node" );
    if ( CompilerDebug ) {
        char buf[1024];
        cout( PrintEliminateUnnededNodes )->print( "*%s node N%ld: %s\n", removing ? "removing" : "eliminating", id(), print_string( buf, PrintHexAddresses ) );
    }
    st_assert( not _dontEliminate or removing, "shouldn't eliminate this node" );
    bb->remove( this );
}


#define CHECK( preg, r )                              \
  if (preg not_eq r and preg->isOnlySoftUsed()) preg->eliminate(false);


void LoadNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    st_assert( canBeEliminated() or rem, "cannot be eliminated" );
    NonTrivialNode::eliminate( bb, r, rem, cp );
    CHECK( _dest, r );
}


void LoadOffsetNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    LoadNode::eliminate( bb, r, rem, cp );
    CHECK( _src, r );
}


void LoadUplevelNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    LoadNode::eliminate( bb, r, rem, cp );
    CHECK( _context0, r );
}


void StoreNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    NonTrivialNode::eliminate( bb, r, rem, cp );
    CHECK( _src, r );
}


void StoreOffsetNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    StoreNode::eliminate( bb, r, rem, cp );
    CHECK( _base, r );
}


void StoreUplevelNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    StoreNode::eliminate( bb, r, rem, cp );
    CHECK( _context0, r );
}


void AssignNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    StoreNode::eliminate( bb, r, rem, cp );
    CHECK( _dest, r );
}


void ReturnNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    st_assert( rem, "should not delete except during dead-code elimination" );
    AbstractReturnNode::eliminate( bb, r, rem, cp );
    CHECK( _src, r );
    CHECK( _dest, r );
    // don't need to check resultPR
}


void ArithNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    NonTrivialNode::eliminate( bb, r, rem, cp );
    CHECK( _src, r );
    CHECK( _dest, r );
}


void ArithRRNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    ArithNode::eliminate( bb, r, rem, cp );
    CHECK( _oper, r );
}


void BranchNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t removing, bool_t cp ) {
    if ( _deleted )
        return;
    if ( removing and nSuccessors() <= 1 ) {
        NonTrivialNode::eliminate( bb, r, removing, cp );
    } else {
        // caller has to handle this
        fatal( "removing branch node with > 1 successor" );
    }
}


void BlockMaterializeNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    BlockCreateNode::eliminate( bb, r, rem, cp );
}


void BasicNode::removeUpToMerge() {
    BasicBlock * thisBasicBlock = _basicBlock;
    Node * n            = ( Node * )
    this;
    for ( ; n and n->hasSinglePredecessor(); ) {
        while ( n->nSuccessors() > 1 ) {
            int i = n->nSuccessors() - 1;
            Node * succ = n->next( i );
            succ->removeUpToMerge();
            /* Must removeNext after removeUpToMerge to avoid false removal of MergeNode with 2 predecessors. SLR 08/08 */
            n->removeNext( succ );
        }
        Node * nextn = n->next();
        if ( not n->_deleted )
            n->eliminate( thisBasicBlock, nullptr, true, false );
        if ( nextn ) {
            BasicBlock * nextBasicBlock = nextn->bb();
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
        n            = nextn;
    }
    BasicBlock * nextBB = n ? n->bb() : nullptr;
}


void PrimitiveNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    if ( _deleted )
        return;
    st_assert( rem or canBeEliminated(), "shouldn't call" );
    if ( nlrTestPoint() ) {
        // remove all unused nodes along NonLocalReturn branch
        // should be exactly 2 nodes (see IRGenerator::makeNonLocalReturnPoint())
        MergeNode * n1 = nlrTestPoint();
        Node      * n2 = n1->next();
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
        int       len = args->length();
        for ( int i   = 0; i < len; i++ ) {
            PseudoRegister * arg = args->at( i );
            if ( arg->_location.isTopOfStack() ) {
                // must delete this push, but node won't be eliminated with topOfStack because of side effect
                // so reset location first
                arg->_location = unAllocated;
            } else {
                fatal( "internal compiler error" );
                //fatal("Urs thinks all args should be topOfStack");
            }
            CHECK( arg, r );
        }
    }
}


void TypeTestNode::eliminate( BasicBlock * bb, PseudoRegister * rr, bool_t rem, bool_t cp ) {
    // completely eliminate receiver and all successors
    if ( _deleted )
        return;

    eliminate( bb, rr, ( ConstPseudoRegister * )
    nullptr, ( KlassOop ) badOop );
}


void TypeTestNode::eliminate( BasicBlock * bb, PseudoRegister * r, ConstPseudoRegister * c, KlassOop theKlass ) {
    // remove node and all successor branches (except for one if receiver is known)
    if ( _deleted )
        return;
    GrowableArray < Node * > *successors = _nxt;
    _nxt = new GrowableArray <Node *>( 1 );
    Oop constant = c ? c->constant : 0;
    Node * keep = nullptr;
    if ( CompilerDebug ) {
        cout( PrintEliminateUnnededNodes )->print( "*eliminating tt node %#lx const %#lx klass %#lx\n", PrintHexAddresses ? this : 0, constant, theKlass );
    }
    Node * un = _next;        // save unknown branch
    // remove all successor nodes
    for ( int i = 0; i < successors->length(); i++ ) {
        Node * succ = successors->at( i );
        KlassOop m = _classes->at( i );
        if ( constant == m or theKlass == m ) {
            st_assert( keep == nullptr, "shouldn't have more than one match" );
            keep = succ;
        } else {
            _next = succ;
            _next->removeUpToMerge();
        }
        succ->removePrev( this );
    }

    if ( keep or theKlass == KlassOop( badOop ) ) {
        // found correct prediction, so can delete unknown branch, or
        // delete everything (theKlass == badOop)
        _next = un;
        un->removeUpToMerge();    // delete unknown branch
        _next = nullptr;
    } else {
        // the type tests didn't predict for theKlass
        // (performance bug: should inline correct case since it's known now;
        // also, unknown branch may be uncommon)
        if ( WizardMode ) {
            warning( "Compiler: typetest didn't predict klass %#lx", theKlass );
            lprintf( "predicted klasses: " );
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


void AbstractArrayAtNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
    st_assert( rem, "shouldn't eliminate because of side effects (errors)" );
    if ( _deleted )
        return;
    // remove fail branch nodes first
    Node * fail = next1();
    if ( fail ) {
        fail->removeUpToMerge();
        fail->removePrev( this );
    }
    AbstractBranchNode::eliminate( bb, r, rem, cp );
}


void InlinedPrimitiveNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t rem, bool_t cp ) {
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


void ContextCreateNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t removing, bool_t cp ) {
    if ( _deleted )
        return;
    PrimitiveNode::eliminate( bb, r, removing, cp );
}


void ContextInitNode::eliminate( BasicBlock * bb, PseudoRegister * r, bool_t removing, bool_t cp ) {
    st_assert( removing, "should only remove when removing unreachable code" );
    if ( _deleted )
        return;
    NonTrivialNode::eliminate( bb, r, removing, cp );
}


void BranchNode::eliminateBranch( int op1, int op2, int res ) {
    // the receiver can be eliminated because the result it is testing
    // is a constant (res)
    bool_t ok;
    switch ( _op ) {
        case EQBranchOp:
            ok = op1 == op2;
            break;
        case NEBranchOp:
            ok = op1 not_eq op2;
            break;
        case LTBranchOp:
            ok = op1 < op2;
            break;
        case LEBranchOp:
            ok = op1 <= op2;
            break;
        case GTBranchOp:
            ok = op1 > op2;
            break;
        case GEBranchOp:
            ok = op1 >= op2;
            break;
        case LTUBranchOp:
            ok = ( unsigned ) op1 < ( unsigned ) op2;
            break;
        case LEUBranchOp:
            ok = ( unsigned ) op1 <= ( unsigned ) op2;
            break;
        case GTUBranchOp:
            ok = ( unsigned ) op1 > ( unsigned ) op2;
            break;
        case GEUBranchOp:
            ok = ( unsigned ) op1 >= ( unsigned ) op2;
            break;
        case VSBranchOp:
            return;        // can't handle yet
        case VCBranchOp:
            return;        // can't handle yet
        default:
            fatal( "unexpected branch type" );
    }
    int nodeToRemove;
    if ( ok ) {
        nodeToRemove = 0; // branch is taken
    } else {
        nodeToRemove = 1;
    }

    // discard one successor and make the remaining one the fall-thru case
    Node * discard = next( nodeToRemove );
    discard->removeUpToMerge();
    removeNext( discard );
    Node * succ = next( 1 - nodeToRemove );
    removeNext( succ );
    append( succ );
    bb()->remove( this );    // delete the branch
}

// ==================================================================================
// likelySuccessor: answers the most likely successor node (or nullptr). Used for better
// code positioning (determines traversal order for BBs).
// ==================================================================================

Node * Node::likelySuccessor() const {
    st_assert( hasSingleSuccessor(), "should override likelySuccessor()" );
    return next();
}


Node * TArithRRNode::likelySuccessor() const {
    return next();                // predict success
}


Node * CallNode::likelySuccessor() const {
    return next();                // predict normal return, not NonLocalReturn
}


Node * NonLocalReturnTestNode::likelySuccessor() const {
    return next();                // predict home not found
}


Node * BranchNode::likelySuccessor() const {
    return next();                // predict untaken
}


Node * TypeTestNode::likelySuccessor() const {
    if ( _deleted )
        return next();
    st_assert( classes()->length() > 0, "no TypeTestNode needed" );
    return next( hasUnknown() ? classes()->length() : classes()->length() - 1 );
}


Node * AbstractArrayAtNode::likelySuccessor() const {
    return next();                // predict success
}


Node * InlinedPrimitiveNode::likelySuccessor() const {
    return next();                // predict success
}


// ==================================================================================
// uncommonSuccessor: answers the most uncommon successor node (or nullptr). Used for
// better code positioning (determines traversal order for BBs).
// ==================================================================================

Node * Node::uncommonSuccessor() const {
    st_assert( hasSingleSuccessor(), "should override uncommonSuccessor()" );
    return nullptr;                    // no uncommon case
}


Node * TArithRRNode::uncommonSuccessor() const {
    return ( _deleted or not canFail() ) ? nullptr : next( 1 );        // failure case is uncommon
}


Node * CallNode::uncommonSuccessor() const {
    return nullptr;                    // no uncommon case (NonLocalReturn is not uncommon)
}


Node * NonLocalReturnTestNode::uncommonSuccessor() const {
    return nullptr;                    // no uncommon case (both exits are common)
}


Node * BranchNode::uncommonSuccessor() const {
    return ( not _deleted and _taken_is_uncommon ) ? next( 1 ) : nullptr;
}


Node * TypeTestNode::uncommonSuccessor() const {
    if ( not _deleted and hasUnknown() ) {
        // fall through case treated as uncommon case
        st_assert( next() not_eq nullptr, "just checking" );
        return next();
    } else {
        // no unknown case, all cases are common
        return nullptr;
    }
}


Node * AbstractArrayAtNode::uncommonSuccessor() const {
    return ( _deleted or not canFail() ) ? nullptr : next( 1 );    // failure case is uncommon
}


Node * InlinedPrimitiveNode::uncommonSuccessor() const {
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
    Node * taken = next1();
    if ( not canFail() and taken not_eq nullptr and taken->bb()->isPredecessor( bb() ) ) {
        taken->removeUpToMerge();
        removeNext( taken );
    }
}


bool_t BasicNode::canCopyPropagate( Node * fromNode ) const {
    // current restriction: cannot copy-propagate into a loop
    // reason: copy-propagated PseudoRegister needs its live range extended to cover the entire loop,
    // not just the stretch between fromNode and this node
    return canCopyPropagate() and fromNode->bb()->loopDepth() == _basicBlock->loopDepth();
}


bool_t NonTrivialNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    st_assert( canCopyPropagate(), "can't copy-propagate" );
    st_assert( hasSrc(), "has no src" );
    if ( _srcUse == u ) {
        CP_HELPER( _src, _srcUse, return );
    } else {
        fatal( "copyPropagate: not the source use" );
    }
    return false;
}


bool_t LoadOffsetNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
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


bool_t LoadUplevelNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    if ( u == _context0Use ) {
        CP_HELPER( _context0, _context0Use, return );
    } else {
        return LoadNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool_t StoreOffsetNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    if ( u == _baseUse ) {
        CP_HELPER( _base, _baseUse, return );
    } else {
        return StoreNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool_t StoreUplevelNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    if ( u == _context0Use ) {
        CP_HELPER( _context0, _context0Use, return );
    } else {
        return StoreNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool_t CallNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    //warning("fix this -- propagate args somewhere");
    return false;
}


bool_t ArithRRNode::operIsConst() const {
    return _oper->isConstPReg();
}


int ArithRRNode::operConst() const {
    st_assert( operIsConst(), "not a constant" );
    return _oper->isConstPReg();
}


bool_t ArithNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    bool_t success = doCopyPropagate( bb, u, d, replace );
    if ( _src->isConstPReg() and operIsConst() ) {
        st_assert( success, "CP must have worked" );
        // can constant-fold this operation
        int c1 = ( int ) ( ( ConstPseudoRegister * ) _src )->constant;
        int c2 = ( int ) operConst();
        int res;
        switch ( _op ) {
            case AddArithOp:
                res = c1 + c2;
                break;

            case SubArithOp:
                res = c1 - c2;
                break;

            case AndArithOp:
                res = c1 & c2;
                break;

            case OrArithOp:
                res = c1 | c2;
                break;

            case XOrArithOp:
                res = c1 ^ c2;
                break;

            default:
                return success;        // can't constant-fold
        }

        _constResult   = new_ConstPReg( scope(), ( Oop ) res );
        // enable further constant propagation of the result
        _dontEliminate = false;
        _src->removeUse( bb, _srcUse );
        _src    = _constResult;
        _srcUse = bb->addUse( this, _src );

        // condition codes set -- see if there's a branch we can eliminate
        Node * branch = next();
        if ( branch->isBranchNode() ) {
            ( ( BranchNode * ) branch )->eliminateBranch( c1, c2, res );
        }
    }
    return success;
}


bool_t ArithNode::doCopyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    return NonTrivialNode::copyPropagate( bb, u, d, replace );
}


bool_t ArithRRNode::doCopyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    if ( u == _operUse ) {
        CP_HELPER( _oper, _operUse, return );
    } else {
        return ArithNode::doCopyPropagate( bb, u, d, replace );
    }
    return false;
}


bool_t TArithRRNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    bool_t res = doCopyPropagate( bb, u, d, replace );
    if ( _src->isConstPReg() and _oper->isConstPReg() ) {
        st_assert( res, "CP must have worked" );
        // can constant-fold this operation
        Oop c1 = ( ( ConstPseudoRegister * ) _src )->constant;
        Oop c2 = ( ( ConstPseudoRegister * ) _oper )->constant;
        Oop result;
        switch ( _op ) {
            case tAddArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_add( c1, c2 );
                break;
            case tSubArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_subtract( c1, c2 );
                break;
            case tMulArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_multiply( c1, c2 );
                break;
            case tDivArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_div( c1, c2 );
                break;
            case tModArithOp:
                result = GeneratedPrimitives::smiOopPrimitives_mod( c1, c2 );
                break;
            case tAndArithOp:
                result = smiOopPrimitives::bitAnd( c1, c2 );
                break;
            case tOrArithOp:
                result = smiOopPrimitives::bitOr( c1, c2 );
                break;
            case tXOrArithOp:
                result = smiOopPrimitives::bitXor( c1, c2 );
                break;
            case tShiftArithOp:
                warning( "possible performance bug: constant folding of tShiftArithOp not implemented" );
                return false;
            case tCmpArithOp:
                warning( "possible performance bug: constant folding of tCmpArithOp not implemented" );
                return false;
            default           :
                fatal1( "unknown tagged opcode %ld", _op );
        }
        bool_t ok = not result->is_mark();
        if ( ok ) {
            // constant-fold this operation
            if ( CompilerDebug )
                cout( PrintCopyPropagation )->print( "*constant-folding N%d --> %#x\n", _id, result );
            _constResult   = new_ConstPReg( scope(), result );
            // first, discard the error branch (if there)
            Node * discard = next1();
            if ( discard not_eq nullptr ) {
                discard->bb()->remove( discard );// SLR should this be removeNext(discard)? and should it be after removeUpToMerge()?
                discard->removeUpToMerge();
                bb->verify();
                ( ( BasicBlock * ) bb->next() )->verify();
            }
            // now, discard the overflow check
            discard = next();
            st_assert( discard->isBranchNode(), "must be a cond. branch" );
            st_assert( ( ( BranchNode * ) discard )->op() == VSBranchOp, "expected an overflow check" );
            discard->bb()->remove( discard );// SLR should this be removeNext(discard)? and should it be after removeUpToMerge()?
            // and the "overflow taken" code
            discard = discard->next1();
            discard->bb()->remove( discard );// SLR ditto this?
            discard->removeUpToMerge();
            bb->verify();
            ( ( BasicBlock * ) bb->next() )->verify();
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


bool_t TArithRRNode::doCopyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    bool_t res;
    if ( u == _srcUse ) {
        if ( d->isConstPReg() and ( ( ConstPseudoRegister * ) d )->constant->is_smi() )
            _arg1IsInt = true;
        CP_HELPER( _src, _srcUse, res = );
    } else if ( u == _operUse ) {
        if ( d->isConstPReg() and ( ( ConstPseudoRegister * ) d )->constant->is_smi() )
            _arg2IsInt = true;
        CP_HELPER( _oper, _operUse, res = );
    } else {
        fatal( "copyPropagate: not the source use" );
    }
    removeFailureIfPossible();
    return res;
}


bool_t FloatArithRRNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    if ( d->isConstPReg() and not( ( ConstPseudoRegister * ) d )->constant->is_double() ) {
        // can't handle non-float arguments (don't optimize guaranteed failure)
        return false;
    }
    bool_t res = ArithRRNode::copyPropagate( bb, u, d, replace );
    // should check for constant folding opportunity here -- fix this
    return res;
}


bool_t FloatUnaryArithNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    if ( d->isConstPReg() and not( ( ConstPseudoRegister * ) d )->constant->is_double() ) {
        // can't handle non-float arguments (don't optimize guaranteed failure)
        return false;
    }
    bool_t res = ArithNode::copyPropagate( bb, u, d, replace );
    // should check for constant folding opportunity here -- fix this
    return res;
}


bool_t TypeTestNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    if ( u == _srcUse ) {
        if ( d->isConstPReg() ) {
            // we know the receiver - the type test is unnecessary!
            ConstPseudoRegister * c = ( ConstPseudoRegister * ) d;
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
        fatal( "don't have this use" );
    }
    return false;
}


bool_t AbstractArrayAtNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    if ( u == _argUse ) {
        bool_t res;
        CP_HELPER( _arg, _argUse, res = );
        removeFailureIfPossible();
        return res;
    } else {
        return AbstractBranchNode::copyPropagate( bb, u, d, replace );
    }
    return false;
}


bool_t AbstractArrayAtPutNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    bool_t res;
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


bool_t InlinedPrimitiveNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    // copyPropagate should be fairly easy to put in, right now it is doing nothing.
    return false;
}


bool_t ContextInitNode::copyPropagate( BasicBlock * bb, Usage * u, PseudoRegister * d, bool_t replace ) {
    for ( int i = nofTemps() - 1; i >= 0; i-- ) {
        if ( _initializerUses->at( i ) == u ) {
            Expression     * initExpression = _initializers->at( i );
            PseudoRegister * initPR         = initExpression->preg();
            Usage          * newUse         = u;
            bool_t ok;
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


void LoadNode::markAllocated( int * use_count, int * def_count ) {
    D_CHECK( _dest );
}


void LoadOffsetNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
    LoadNode::markAllocated( use_count, def_count );
}


void LoadUplevelNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _context0 );
    LoadNode::markAllocated( use_count, def_count );
}


void StoreNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
}


void StoreOffsetNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _base );
    StoreNode::markAllocated( use_count, def_count );
}


void StoreUplevelNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _context0 );
    StoreNode::markAllocated( use_count, def_count );
}


void AssignNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
    D_CHECK( _dest );
}


void ReturnNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
    D_CHECK( _dest );
    AbstractReturnNode::markAllocated( use_count, def_count );
}


void ArithNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
    D_CHECK( _dest );
}


void ArithRRNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _oper );
    ArithNode::markAllocated( use_count, def_count );
}


void TArithRRNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
    D_CHECK( _dest );
    U_CHECK( _oper );
}


void CallNode::markAllocated( int * use_count, int * def_count ) {
    D_CHECK( _dest );
    // CallNode trashes all regs
    for ( int i = 0; i < REGISTER_COUNT; i++ ) {
        use_count[ i ]++;
        def_count[ i ]++;
    }
}


void BlockCreateNode::markAllocated( int * use_count, int * def_count ) {
    if ( _src )
        U_CHECK( _src );
    if ( _context )
        U_CHECK( _context );
    PrimitiveNode::markAllocated( use_count, def_count );
}


void BlockMaterializeNode::markAllocated( int * use_count, int * def_count ) {
    if ( isMemoized() )
        BlockCreateNode::markAllocated( use_count, def_count );
}


void ContextCreateNode::markAllocated( int * use_count, int * def_count ) {
    if ( _src )
        U_CHECK( _src ); // no src if there's no incoming context
    if ( _parentContexts ) {
        for ( int i = _parentContexts->length() - 1; i >= 0; i-- ) {
            U_CHECK( _parentContexts->at( i ) );
        }
    }
    PrimitiveNode::markAllocated( use_count, def_count );
}


void ContextInitNode::markAllocated( int * use_count, int * def_count ) {
    if ( _src )
        U_CHECK( _src );
    int i = nofTemps();
    while ( i-- > 0 ) {
        D_CHECK( contents()->at( i )->preg() );
    }
}


void ContextZapNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
}


void TypeTestNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
}


void AbstractArrayAtNode::markAllocated( int * use_count, int * def_count ) {
    U_CHECK( _src );
    if ( _dest )
        D_CHECK( _dest );
    U_CHECK( _arg );
    if ( _error )
        D_CHECK( _error );
}


void AbstractArrayAtPutNode::markAllocated( int * use_count, int * def_count ) {
    if ( elem )
        U_CHECK( elem );
    AbstractArrayAtNode::markAllocated( use_count, def_count );
}


void InlinedPrimitiveNode::markAllocated( int * use_count, int * def_count ) {
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

void computeEscapingBlocks( Node * n, PseudoRegister * src, GrowableArray <BlockPseudoRegister *> * l, const char * msg ) {
    // helper function for computing exposed blocks
    if ( src->isBlockPReg() ) {
        BlockPseudoRegister * r = ( BlockPseudoRegister * ) src;
        r->markEscaped( n );
        if ( not l->contains( r ) )
            l->append( r );
        if ( msg )
            theCompiler->reporter->report_block( n, r, msg );
    }
}


void StoreNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    // Any store is considered to expose a block -- even if it's a store into a local.
    // That's pessimistic, but simple.
    ::computeEscapingBlocks( this, _src, ll, action() );
}


void AbstractReturnNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    // a block returned by a NativeMethod escapes
    if ( _src )
        ::computeEscapingBlocks( this, _src, ll, "returned" );
}


void CallNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    SubclassResponsibility();
}


void SendNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    // all arguments to a non-inlined call escape
    if ( exprStack and ( args not_eq nullptr ) ) {
        // if the receiver is not pushed on the exprStack (self/super sends),
        // the exprStack is by 1 shorter than the args array
        // (exprStack may be longer than that, so just look at top elems)
        int len = exprStack->length();
        int i   = min( args->length(), len );
        while ( i-- > 0 ) {
            ::computeEscapingBlocks( this, exprStack->at( --len ), ll, "exposed by a send" );
        }
    }
}


void UncommonSendNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    // all arguments to an uncommon send escape
    if ( expressionStack() and ( _argCount > 0 ) ) {
        int len = expressionStack()->length();
        int i   = _argCount;
        while ( i-- > 0 ) {
            ::computeEscapingBlocks( this, expressionStack()->at( --len ), ll, "exposed by an uncommon send" );
        }
    }
}


void PrimitiveNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    // assume that all block arguments to a primitive call escape
    if ( exprStack ) {
        int len = exprStack->length();
        int i   = min( len, _pdesc->number_of_parameters() );
        while ( i-- > 0 ) {
            ::computeEscapingBlocks( this, exprStack->at( --len ), ll, "exposed by a primitive call" );
        }
    }
}


void DLLNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    // assume that all block arguments to a DLL call escape
    if ( exprStack ) {
        int len = exprStack->length();
        int i   = min( len, nofArguments() );
        while ( i-- > 0 ) {
            ::computeEscapingBlocks( this, exprStack->at( --len ), ll, "exposed by a DLL call" );
        }
    }
}


void ContextInitNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    // all blocks stored into a context escape
    // (later phase will recognize if context isn't created --> block doesn't really escape)
    int i = nofTemps();
    while ( i-- > 0 ) {
        Expression * e = _initializers->at( i );
        if ( e )
            ::computeEscapingBlocks( this, e->preg(), ll, nullptr );
    }
}


void ArrayAtPutNode::computeEscapingBlocks( GrowableArray <BlockPseudoRegister *> * ll ) {
    // all blocks stored into an array escape
    ::computeEscapingBlocks( this, elem, ll, "stored into an array" );
}


// ==================================================================================
// machine-independent routines for code generation
// ==================================================================================

bool_t TypeTestNode::needsKlassLoad() const {
    // a test needs a klass load if it tests for any non-smi_t/bool_t/nil klass
    const int len = _hasUnknown ? _classes->length() : _classes->length() - 1;
    for ( int i   = 0; i < len; i++ ) {
        KlassOop klass = _classes->at( i );
        if ( klass not_eq trueObj->klass() and klass not_eq falseObj->klass() and klass not_eq nilObj->klass() and klass not_eq smiKlassObj ) {
            return true;
        }
    }
    return false;
}


static bool_t hasUnknownCode( Node * n ) {
    while ( n->isTrivial() )
        n = n->next();
    return not n->isUncommonNode();
}


bool_t TypeTestNode::hasUnknownCode() const {
    if ( not _hasUnknown )
        return false;     // no unknown type
    return ::hasUnknownCode( next() );
}


bool_t TArithRRNode::hasUnknownCode() const {
    return ::hasUnknownCode( next1() );
}


bool_t AbstractArrayAtNode::hasUnknownCode() const {
    return ::hasUnknownCode( next1() );
}


Node * TypeTestNode::smiCase() const {
    int i = _classes->length();
    while ( i-- > 0 ) {
        if ( _classes->at( i ) == smiKlassObj )
            return next( i + 1 );
    }
    return nullptr;
}

// ==================================================================================
// integer loop optimization
// ==================================================================================

LoopHeaderNode::LoopHeaderNode() {
    _activated          = false;
    _integerLoop        = false;
    _tests              = nullptr;
    _enclosingLoop      = nullptr;
    _nestedLoops        = nullptr;
    _nofCalls           = 0;
    _registerCandidates = nullptr;
}


void LoopHeaderNode::activate( PseudoRegister * loopVar, PseudoRegister * lowerBound, PseudoRegister * upperBound, LoadOffsetNode * loopSizeLoad ) {
    _activated     = true;
    _integerLoop   = true;
    _loopVar       = loopVar;
    _lowerBound    = lowerBound;
    _upperBound    = upperBound;
    _upperLoad     = loopSizeLoad;
    _arrayAccesses = new GrowableArray <AbstractArrayAtNode *>( 10 );
}


void LoopHeaderNode::activate() {
    _activated = true;
    st_assert( _tests, "should have type tests" );
    _loopVar       = _lowerBound = _upperBound = nullptr;
    _upperLoad     = nullptr;
    _arrayAccesses = nullptr;
}


void LoopHeaderNode::addArray( AbstractArrayAtNode * n ) {
    st_assert( _activated, "shouldn't call" );
    _arrayAccesses->append( n );
}


void LoopHeaderNode::set_enclosingLoop( LoopHeaderNode * l ) {
    st_assert( _enclosingLoop == nullptr, "already set" );
    _enclosingLoop = l;
}


void LoopHeaderNode::addNestedLoop( LoopHeaderNode * l ) {
    if ( _nestedLoops == nullptr )
        _nestedLoops = new GrowableArray <LoopHeaderNode *>( 5 );
    _nestedLoops->append( l );
}


void LoopHeaderNode::addRegisterCandidate( LoopRegCandidate * c ) {
    if ( _registerCandidates == nullptr )
        _registerCandidates = new GrowableArray <LoopRegCandidate *>( 2 );
    _registerCandidates->append( c );
}


bool_t is_smi_type( GrowableArray <KlassOop> * klasses ) {
    return klasses->length() == 1 and klasses->at( 0 ) == smiKlassObj;
}


GrowableArray <KlassOop> * make_smi_type() {
    GrowableArray <KlassOop> * t = new GrowableArray <KlassOop>( 1 );
    t->append( smiKlassObj );
    return t;
}


void StoreNode::assert_preg_type( PseudoRegister * r, GrowableArray <KlassOop> * klasses, LoopHeaderNode * n ) {
    if ( is_smi_type( klasses ) and r == src() ) {
        if ( CompilerDebug )
            cout( PrintLoopOpts )->print( "*removing store check from N%d\n", id() );
        setStoreCheck( false );
    }
}


void AbstractArrayAtNode::assert_in_bounds( PseudoRegister * r, LoopHeaderNode * n ) {
    if ( r == _arg ) {
        if ( CompilerDebug and _needBoundsCheck )
            cout( PrintLoopOpts )->print( "*removing bounds check from N%d\n", id() );
        _needBoundsCheck = false;
        removeFailureIfPossible();
    }
}


void AbstractArrayAtNode::collectTypeTests( GrowableArray <PseudoRegister *> & regs, GrowableArray <GrowableArray < KlassOop> *
> & klasses ) const {
// ArrayAt node tests index for smi_t-ness
regs.
append( _arg );
klasses.
append ( make_smi_type() );
}


void AbstractArrayAtNode::assert_preg_type( PseudoRegister * r, GrowableArray <KlassOop> * klasses, LoopHeaderNode * n ) {
    if ( is_smi_type( klasses ) and r == _arg ) {
        if ( CompilerDebug and not _intArg )
            cout( PrintLoopOpts )->print( "*removing index tag check from N%d\n", id() );
        _intArg = true;
        n->addArray( this );
        removeFailureIfPossible();
    } else if ( r not_eq _arg ) {
        fatal( "array can't be an integer" );
    }
}


void ArrayAtPutNode::collectTypeTests( GrowableArray <PseudoRegister *> & regs, GrowableArray <GrowableArray < KlassOop> *
> & klasses ) const {
// atPut node tests element for smi_t-ness if character array
AbstractArrayAtNode::collectTypeTests( regs, klasses
);
if (
stores_smi_elements( _access_type )
) {
regs.
append( elem );
st_assert          ( klasses
.first()->first() == smiKlassObj, "must be smi_t type for index" );
klasses.
append( klasses
.
first()
);    // reuse smi_t type descriptor
}
}


void ArrayAtPutNode::assert_preg_type( PseudoRegister * r, GrowableArray <KlassOop> * klasses, LoopHeaderNode * n ) {
    if ( is_smi_type( klasses ) and r == elem ) {
        if ( CompilerDebug and _needs_store_check )
            cout( PrintLoopOpts )->print( "*removing array store check from N%d\n", id() );
        _needs_store_check = false;
        removeFailureIfPossible();
    } else if ( r == _arg ) {
        AbstractArrayAtPutNode::assert_preg_type( r, klasses, n );
    }
}


void TArithRRNode::collectTypeTests( GrowableArray <PseudoRegister *> & regs, GrowableArray <GrowableArray < KlassOop> *
> & klasses ) const {
// tests receiver and/or arg for smi_t-ness
if (
canFail()
) {
GrowableArray <KlassOop> * t = make_smi_type();
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


void TArithRRNode::assert_preg_type( PseudoRegister * r, GrowableArray <KlassOop> * klasses, LoopHeaderNode * n ) {
    if ( is_smi_type( klasses ) and r == _src ) {
        if ( CompilerDebug and not _arg1IsInt )
            cout( PrintLoopOpts )->print( "*removing arith arg1 tag check from N%d\n", id() );
        _arg1IsInt = true;
    }
    if ( is_smi_type( klasses ) and r == _oper ) {
        if ( CompilerDebug and not _arg2IsInt )
            cout( PrintLoopOpts )->print( "*removing arith arg2 tag check from N%d\n", id() );
        _arg2IsInt = true;
    }
    removeFailureIfPossible();
}


void TypeTestNode::collectTypeTests( GrowableArray <PseudoRegister *> & regs, GrowableArray <GrowableArray < KlassOop> *
> & klasses ) const {
regs.
append( _src );
klasses.
append( _classes );
}


void TypeTestNode::assert_preg_type( PseudoRegister * r, GrowableArray <KlassOop> * k, LoopHeaderNode * n ) {
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

const int PrintStringLen = 40;    // width of output before printing address

void BasicNode::print_short() {
    char buf[1024];
    lprintf( print_string( buf, PrintHexAddresses ) );
}


static int id_of( Node * node ) {
    return node == nullptr ? -1 : node->id();
}


const char * PrologueNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "Prologue" );
    if ( printAddr )
        my_sprintf( buf, "((PrologueNode*)%#lx", this );
    return b;
}


const char * InterruptCheckNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "InterruptCheckNode" );
    if ( printAddr )
        my_sprintf( buf, "((InterruptCheckNode*)%#lx)", this );
    return b;
}


const char * LoadOffsetNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "LoadOffset %s := %s[%#lx]", _dest->safeName(), _src->safeName(), _offset );
    if ( printAddr )
        my_sprintf( buf, "((LoadOffsetNode*)%#lx)", this );
    return b;
}


const char * LoadIntNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "LoadInt %s := %#lx", _dest->safeName(), _value );
    if ( printAddr )
        my_sprintf( buf, "((LoadIntNode*)%#lx)", this );
    return b;
}


const char * LoadUplevelNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "LoadUpLevel %s := %s^%d[%d]", _dest->safeName(), _context0->safeName(), _nofLevels, _offset );
    if ( printAddr )
        my_sprintf( buf, "((LoadUplevelNode*)%#lx)", this );
    return b;
}


const char * StoreOffsetNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "StoreOffset %s[%#lx] := %s", _base->safeName(), _offset, _src->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((StoreOffsetNode*)%#lx)", this );
    return b;
}


const char * StoreUplevelNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "StoreUpLevel %s^%d[%d] := %s", _context0->safeName(), _nofLevels, _offset, _src->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((StoreUplevelNode*)%#lx)", this );
    return b;
}


const char * AssignNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s", _dest->safeName(), _src->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((AssignNode*)%#lx)", this );
    return b;
}


const char * SendNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "Send %s NonLocalReturn %ld ", _key->print_string(), id_of( nlrTestPoint() ) );
    if ( printAddr )
        my_sprintf( buf, "((SendNode*)%#lx)", this );
    return b;
}


const char * PrimitiveNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "PrimCall _%s NonLocalReturn %ld", _pdesc->name(), id_of( nlrTestPoint() ) );
    if ( printAddr )
        my_sprintf( buf, "((PrimitiveNode*)%#lx)", this );
    return b;
}


const char * DLLNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "DLLCall <%s, %s> NonLocalReturn %ld", _dll_name->as_string(), _function_name->as_string(), id_of( nlrTestPoint() ) );
    if ( printAddr )
        my_sprintf( buf, "((DLLNode*)%#lx)", this );
    return b;
}


const char * BlockCreateNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "BlockCreate %s", _dest->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((BlockCreateNode*)%#lx)", this );
    return b;
}


const char * BlockMaterializeNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "BlockMaterialize %s", _dest->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((BlockMaterializeNode*)%#lx)", this );
    return b;
}


const char * InlinedReturnNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "InlinedReturn %s := %s", _dest->safeName(), _src->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((InlinedReturnNode*)%#lx)", this );
    return b;
}


const char * NonLocalReturnSetupNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "NonLocalReturneturn %s := %s", _dest->safeName(), _src->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((NonLocalReturnSetupNode*)%#lx)", this );
    return b;
}


const char * NonLocalReturnContinuationNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "NonLocalReturn Continuation" );
    if ( printAddr )
        my_sprintf( buf, "((NonLocalReturnContinuationNode*)%#lx)", this );
    return b;
}


const char * ReturnNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "MethodReturn  %s", _src->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((ReturnNode*)%#lx)", this );
    return b;
}


const char * NonLocalReturnTestNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "NonLocalReturnTest  N%ld N%ld", id_of( next1() ), id_of( next() ) );
    if ( printAddr )
        my_sprintf( buf, "((NonLocalReturnTestNode*)%#lx)", this );
    return b;
}


const char * ArithNode::opName() const {
    return ArithOpName[ _op ];
}


const char * ArithRRNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s %s", _dest->safeName(), _src->safeName(), opName(), _oper->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((ArithRRNode*)%#lx)", this );
    return b;
}


const char * FloatArithRRNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s %s", _dest->safeName(), _src->safeName(), opName(), _oper->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((FloatArithRRNode*)%#lx)", this );
    return b;
}


const char * FloatUnaryArithNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s", _dest->safeName(), opName(), _src->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((FloatUnaryArithNode*)%#lx)", this );
    return b;
}


const char * TArithRRNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s %s   N%d, N%d", _dest->safeName(), _src->safeName(), ArithOpName[ _op ], _oper->safeName(), id_of( next1() ), id_of( next() ) );
    if ( printAddr )
        my_sprintf( buf, "((TArithRRNode*)%#lx)", this );
    return b;
}


const char * ArithRCNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s := %s %s %#lx", _dest->safeName(), _src->safeName(), opName(), _oper );
    if ( printAddr )
        my_sprintf( buf, "((ArithRCNode*)%#lx)", this );
    return b;
}


const char * BranchNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "%s  N%ld N%ld", BranchOpName[ _op ], id_of( next1() ), id_of( next() ) );
    if ( printAddr )
        my_sprintf( buf, "((BranchNode*)%#lx)", this );
    return b;
}


const char * TypeTestNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf( buf, "TypeTest %s, ", _src->safeName() );
    for ( int i = 1; i <= _classes->length(); i++ ) {
        KlassOop m = _classes->at( i - 1 );
        my_sprintf( buf, m->print_value_string() );
        my_sprintf( buf, ": N%ld; ", ( i < nSuccessors() and next( i ) not_eq nullptr ) ? next( i )->id() : -1 );
    }
    my_sprintf_len( buf, b + PrintStringLen - buf, "N%ld%s", id_of( next() ), _hasUnknown ? "" : "*" );
    if ( printAddr )
        my_sprintf( buf, "((TypeTestNode*)%#lx)", this );
    return b;
}


const char * ArrayAtNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "ArrayAt %s := %s[%s]", _dest->safeName(), _src->safeName(), _arg->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((ArrayAtNode*)%#lx)", this );
    return b;
}


const char * ArrayAtPutNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "ArrayAtPut %s[%s] := %s", _src->safeName(), _arg->safeName(), elem->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((ArrayAtPutNode*)%#lx)", this );
    return b;
}


const char * FixedCodeNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "DeadEnd" );
    if ( printAddr )
        my_sprintf( buf, "((FixedCodeNode*)%#lx)", this );
    return b;
}


static int prevsLen;
static const char * mergePrintBuf;


static void printPrevNodes( Node * n ) {
    my_sprintf( mergePrintBuf, "N%ld%s", id_of( n ), --prevsLen > 0 ? ", " : "" );
}


const char * MergeNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf( buf, "Merge " );
    prevsLen      = _prevs->length();
    mergePrintBuf = buf;
    _prevs->apply( printPrevNodes );
    buf = mergePrintBuf;
    my_sprintf_len( buf, b + PrintStringLen - buf, " " );
    if ( printAddr )
        my_sprintf( buf, "((MergeNode*)%#lx)", this );
    return b;
}


const char * LoopHeaderNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf( buf, "LoopHeader " );
    if ( _activated ) {
        if ( _integerLoop ) {
            my_sprintf( buf, "int " );
            my_sprintf( buf, "%s=[%s..%s] ", _loopVar->safeName(), _lowerBound->safeName(), _upperBound ? _upperBound->safeName() : _upperLoad->base()->safeName() );
        }
        if ( _registerCandidates not_eq nullptr ) {
            my_sprintf( buf, "reg vars = " );
            for ( int i = 0; i < _registerCandidates->length(); i++ )
                my_sprintf( buf, "%s ", _registerCandidates->at( i )->preg()->name() );
        }
        if ( _tests not_eq nullptr ) {
            for ( int i = 0; i < _tests->length(); i++ ) {
                HoistedTypeTest * t = _tests->at( i );
                if ( t->_testedPR->_location not_eq unAllocated ) {
                    StringOutputStream s( 50 );
                    t->print_test_on( &s );
                    my_sprintf( buf, "%s ", s.as_string() );
                }
            }
        }
        my_sprintf_len( buf, PrintStringLen - ( buf - b ), " " );
    } else {
        my_sprintf_len( buf, PrintStringLen - 11, "(inactive)" );
    }
    if ( printAddr )
        my_sprintf( buf, "((LoopHeaderNode*)%#lx)", this );
    return b;
}


const char * ContextCreateNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "Create Context %s", _dest->safeName() );
    if ( printAddr )
        my_sprintf( buf, "((ContextCreateNode*)%#lx)", this );
    return b;
}


const char * ContextInitNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf( buf, "Initialize context " );
    if ( _src == nullptr ) {
        my_sprintf( buf, "(optimized away) " );
    } else {
        my_sprintf( buf, "%s { ", _src->safeName() );
        for ( int i = 0; i < contents()->length(); i++ ) {
            my_sprintf( buf, " %s := ", contents()->at( i )->preg()->safeName() );
            Expression * e = _initializers->at( i );
            my_sprintf( buf, " %s; ", e->preg()->safeName() );
        }
    }
    my_sprintf_len( buf, b + PrintStringLen - buf, "}" );
    if ( printAddr )
        my_sprintf( buf, "((ContextInitNode*)%#lx)", this );
    return b;
}


const char * ContextZapNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "Zap Context %s", isActive() ? context()->safeName() : "- inactive" );
    if ( printAddr )
        my_sprintf( buf, "((ContextZapNode*)%#lx)", this );
    return b;
}


const char * InlinedPrimitiveNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf( buf, "%s := ", _dest->safeName() );
    const char * op_name;
    switch ( _operation ) {
        case InlinedPrimitiveNode::Operation::obj_klass:
            op_name = "obj_klass";
            break;
        case InlinedPrimitiveNode::Operation::obj_hash:
            op_name = "obj_hash";
            break;
        case InlinedPrimitiveNode::Operation::proxy_byte_at:
            op_name = "proxy_byte_at";
            break;
        case InlinedPrimitiveNode::Operation::proxy_byte_at_put:
            op_name = "proxy_byte_at_put";
            break;
        default:
            op_name = "*** unknown primitive ***";
            break;
    }
    my_sprintf( buf, "%s(", op_name );
    my_sprintf( buf, " %s", _src->safeName() );
    my_sprintf( buf, " %s", _arg1->safeName() );
    my_sprintf( buf, " %s", _arg2->safeName() );
    my_sprintf_len( buf, b + PrintStringLen - buf, ")" );
    if ( printAddr )
        my_sprintf( buf, "((InlinedPrimitiveNode*)%#lx)", this );
    return b;
}


const char * UncommonNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "UncommonBranch" );
    if ( printAddr )
        my_sprintf( buf, "((UncommonNode*)%#lx)", this );
    return b;
}


const char * UncommonSendNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "UncommonSend(%d arg%s)", _argCount, _argCount not_eq 1 ? "s" : "" );
    if ( printAddr )
        my_sprintf( buf, "((UncommonSendNode*)%#lx)", this );
    return b;
}


const char * NopNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "Nop" );
    if ( printAddr )
        my_sprintf( buf, "((NopNode*)%#lx)", this );
    return b;
}


const char * CommentNode::print_string( const char * buf, bool_t printAddr ) const {
    const char * b = buf;
    my_sprintf_len( buf, PrintStringLen, "'%s' ", comment );
    if ( printAddr )
        my_sprintf( buf, "((CommentNode*)%#lx)", this );
    return b;
}


void BasicNode::printID() const {
    lprintf( "%4ld:%1s %-4s", id(), _deleted ? "D" : " ", " " );
    //c hasSplitSig() ? splitSig()->prefix(buf) : " ");
}


void Node::verify() const {
    if ( _deleted )
        return;
    if ( not firstPrev() and not isPrologueNode() )
        error( "Node %#lx: no predecessor", this );
    if ( firstPrev() and not firstPrev()->isSuccessor( this ) )
        error( "prev->next not_eq this for Node %#lx", this );
    if ( _basicBlock and not _basicBlock->contains( this ) )
        error( "BasicBlock doesn't contain Node %#lx", this );
    if ( next() and not next()->isPredecessor( this ) )
        error( "next->prev not_eq this for Node %#lx", this );
    if ( bbIterator->_blocksBuilt and _basicBlock == nullptr )
        error( "Node %#lx: doesn't belong to any BasicBlock", this );
    if ( next() == nullptr and not isExitNode() and not isCommentNode() )   // for the "rest of method omitted (dead)" comment
        error( "Node %#lx has no successor", this );
    if ( next() not_eq nullptr and isExitNode() ) {
        Node * n = next();
        for ( ; n and ( n->isCommentNode() or n->isDeadEndNode() ); n = n->next() );
        if ( n )
            error( "exit node %#lx has a successor (%#lx)", this, next() );
    }
}


void NonTrivialNode::verify() const {
    if ( _deleted )
        return;
    if ( hasSrc() )
        src()->verify();
    if ( hasDest() ) {
        dest()->verify();
        if ( dest()->isConstPReg() ) {
            error( "Node %#lx: dest %#lx is ConstPR", this, dest() );
        }
    }
    if ( isAssignmentLike() and ( not hasSrc() or not hasDest() ) )
        error( "Node %#lx: isAssignmentLike() implies hasSrc/Dest", this );
    Node::verify();
}


void LoadOffsetNode::verify() const {
    if ( _deleted )
        return;
    NonTrivialNode::verify();
    base()->verify();
    if ( _offset < 0 )
        error( "Node %#lx: offset must be >= 0", this );
}


void LoadUplevelNode::verify() const {
    if ( _deleted )
        return;
    if ( _context0 == nullptr )
        error( "Node %#lx: context0 is nullptr", this );
    if ( _nofLevels < 0 )
        error( "Node %#lx: nofLevels must be >= 0", this );
    if ( _offset < 0 )
        error( "Node %#lx: offset must be >= 0", this );
    NonTrivialNode::verify();
    _context0->verify();
}


void StoreOffsetNode::verify() const {
    if ( _deleted )
        return;
    NonTrivialNode::verify();
    base()->verify();
    if ( _offset < 0 )
        error( "Node %#lx: offset must be >= 0", this );
}


void StoreUplevelNode::verify() const {
    if ( _deleted )
        return;
    if ( _context0 == nullptr )
        error( "Node %#lx: context0 is nullptr", this );
    if ( _nofLevels < 0 )
        error( "Node %#lx: nofLevels must be > 0", this );
    if ( _offset < 0 )
        error( "Node %#lx: offset must be >= 0", this );
    NonTrivialNode::verify();
    _context0->verify();
}


void MergeNode::verify() const {
    if ( _deleted )
        return;
    if ( _isLoopStart and _isLoopEnd )
        error( "MergeNode %#x: cannot be both start and end of loop" );
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
        error( "ReturnNode %#lx has a successor", this );
}


void NonLocalReturnSetupNode::verify() const {
    if ( _deleted )
        return;
    AbstractReturnNode::verify();
    if ( next() )
        error( "NonLocalReturnSetupNode %#lx has a successor", this );
}


void NonLocalReturnContinuationNode::verify() const {
    if ( _deleted )
        return;
    AbstractReturnNode::verify();
    if ( next() )
        error( "NonLocalReturnContinuationNode %#lx has a successor", this );
}


void NonLocalReturnTestNode::verify() const {
    if ( _deleted )
        return;
    AbstractBranchNode::verify( false );
    if ( next() == nullptr )
        error( "NonLocalReturnTestNode %#lx has no continue-NonLocalReturn node", this );
    if ( next1() == nullptr )
        error( "NonLocalReturnTestNode %#lx has no end-of-NonLocalReturn node", this );
}


void InlinedReturnNode::verify() const {
    if ( _deleted )
        return;
    AbstractReturnNode::verify();
    if ( not next() ) {
        error( "InlinedReturnNode %#lx has no successor", this );
    } else {
        Node * nextAfterMerge = next()->next();
        if ( nextAfterMerge->scope() == scope() )
            error( "InlinedReturnNode %#lx: successor is in same scope", this );
    }
}


void ContextCreateNode::verify() const {
    PrimitiveNode::verify();
}


void ContextInitNode::verify() const {
    if ( _deleted )
        return;
    int n = nofTemps();
    if ( ( n not_eq contents()->length() ) or ( n not_eq _initializers->length() ) or ( ( _contentDefs not_eq nullptr ) and ( n not_eq _contentDefs->length() ) ) or ( ( _initializerUses not_eq nullptr ) and ( n not_eq _initializerUses->length() ) ) ) {
        error( "ContextInitNode %#lx: bad nofTemps %d", this, n );
    }
    int i = nofTemps();
    while ( i-- > 0 ) {
        Expression * e = _initializers->at( i );
        if ( e not_eq nullptr )
            e->verify();
        contents()->at( i )->verify();
        PseudoRegister * r = contents()->at( i )->preg();
        if ( _src == nullptr and r->_location.isContextLocation() ) {
            ( ( ContextInitNode * )
            this )->print();
            scope()->print();
            error( "ContextInitNode %#lx: context eliminated but temp %d is context location", this, i );
        }
        // isInContext is accessing _isInContext which is never set (and thus always 0 initially)
        // Should check if we're missing something here or if we can remove the code completely.
        // - gri 9/10/96
        /*
    if (r->isSAPReg() and not r->isBlockPReg() and not ((SinglyAssignedPseudoRegister*)r)->isInContext()) {
    // I'm not sure what the isInContext() flag is supposed to do....but the error is triggered
    // in the test suite (when running the standard tests).  Could be because of copy propagation.
    // If the assertion does make sense, please also put it in ContextInitNode::initialize().
    // (But please turn the condition above into a method, don't duplicate it.)
    // Urs 9/8/96
    // error("ContextInitNode %#lx: temp %d is non-context SinglyAssignedPseudoRegister %s", this, i, r->safeName());
    }
    */
    }
}


void ContextZapNode::verify() const {
    if ( _deleted )
        return;
    if ( _src not_eq scope()->context() ) {
        error( "ContextZapNode %#lx: wrong context %#lx", this, _src );
    }
    NonTrivialNode::verify();
}


void CallNode::verify() const {
    if ( _deleted )
        return;
    if ( ( exprStack not_eq nullptr ) and ( args not_eq nullptr ) ) {
        if ( exprStack->length() + 1 < args->length() ) {
            error( "CallNode %#lx: exprStack is too short", this );
        }
    }
}


void ArithRRNode::verify() const {
    if ( _deleted )
        return;
    ArithNode::verify();
    _oper->verify();
}


void TArithRRNode::verify() const {
    if ( _deleted )
        return;
    AbstractBranchNode::verify( true );
    if ( ( _op < tAddArithOp ) or ( tCmpArithOp < _op ) ) {
        error( "TArithRRNode %#lx: wrong opcode %ld", this, _op );
    }
}


void FloatArithRRNode::verify() const {
    if ( _deleted )
        return;
    ArithRRNode::verify();
    // fix this -- check opcode
}


void FloatUnaryArithNode::verify() const {
    if ( _deleted )
        return;
    ArithNode::verify();
    // fix this -- check opcode
}


void AbstractBranchNode::verify( bool_t verifySuccessors ) const {
    if ( _deleted )
        return;
    NonTrivialNode::verify();
    if ( verifySuccessors and not canFail() and failureBranch() not_eq nullptr ) {
        error( "Node %#x: cannot fail, but failure branch is still there", this );
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
    if ( ( Node * ) this not_eq bb()->_last )
    error( "UncommonNode %#lx: not last node in BasicBlock", this );
    NonTrivialNode::verify();
}


void TypeTestNode::verify() const {
    if ( _deleted )
        return;
    if ( ( Node * ) this not_eq bb()->_last )
    error( "TypeTestNode %#lx: not last node in BasicBlock", this );
    NonTrivialNode::verify();
}


// for debugging
void printNodes( Node * n ) {
    for ( ; n; n = n->next() ) {
        n->printID();
        n->print_short();
        lprintf( "\n" );
    }
}
