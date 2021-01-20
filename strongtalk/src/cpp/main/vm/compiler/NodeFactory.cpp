
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/NodeFactory.hpp"
#include "vm/primitives/PrimitiveDescriptor.hpp"
#include "vm/assembler/x86_mapping.hpp"


std::size_t NodeFactory::_cumulativeCost;


NonLocalReturnContinuationNode *NodeFactory::NonLocalReturnContinuationNode( int byteCodeIndex ) {
    InlinedScope                         *scope = theCompiler->currentScope();
    PseudoRegister                       *reg   = new PseudoRegister( scope, NonLocalReturnResultLoc, false, false );
    class NonLocalReturnContinuationNode *res   = new class NonLocalReturnContinuationNode( byteCodeIndex, reg, reg );
    registerNode( res );
    return res;
}


NonLocalReturnTestNode *NodeFactory::NonLocalReturnTestNode( int byteCodeIndex ) {
    class NonLocalReturnTestNode *res = new class NonLocalReturnTestNode( byteCodeIndex );
    registerNode( res );
    theCompiler->nlrTestPoints->append( res );
    return res;
}


SendNode *NodeFactory::SendNode( LookupKey *key, class MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack, bool_t superSend, SendInfo *info ) {
    class SendNode *res = new class SendNode( key, nlrTestPoint, args, expr_stack, superSend, info );
    st_assert( expr_stack, "must have expression stack" );
    res->scope()->addSend( expr_stack, true );  // arguments to call are debug-visible
    registerNode( res );
    return res;
}


PrimitiveNode *NodeFactory::PrimitiveNode( PrimitiveDescriptor *pdesc, class MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack ) {
    class PrimitiveNode *res = new class PrimitiveNode( pdesc, nlrTestPoint, args, expr_stack );
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


DLLNode *NodeFactory::DLLNode( SymbolOop dll_name, SymbolOop function_name, dll_func_ptr_t function, bool_t async, class MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack ) {
    class DLLNode *res = new class DLLNode( dll_name, function_name, function, async, nlrTestPoint, args, expr_stack );
    res->scope()->addSend( expr_stack, true );  // arguments to DLL call are debug-visible
    registerNode( res );
    return res;
}


TypeTestNode *NodeFactory::TypeTestNode( PseudoRegister *recv, GrowableArray<KlassOop> *classes, bool_t hasUnknown ) {
    class TypeTestNode *res = new class TypeTestNode( recv, classes, hasUnknown );
    registerNode( res );
    res->scope()->addTypeTest( res );
    return res;
}


InlinedPrimitiveNode *NodeFactory::InlinedPrimitiveNode( InlinedPrimitiveNode::Operation op, PseudoRegister *result, PseudoRegister *error, PseudoRegister *recv, PseudoRegister *arg1, bool_t arg1_is_smi, PseudoRegister *arg2, bool_t arg2_is_smi ) {
    class InlinedPrimitiveNode *res = new class InlinedPrimitiveNode( op, result, error, recv, arg1, arg1_is_smi, arg2, arg2_is_smi );
    registerNode( res );
    return res;
}


UncommonNode *NodeFactory::UncommonNode( GrowableArray<PseudoRegister *> *exprStack, int byteCodeIndex ) {
    class UncommonNode *res = new class UncommonNode( exprStack, byteCodeIndex );
    registerNode( res );
    st_assert( exprStack, "must have expr. stack" );
    res->scope()->addSend( exprStack, false );  // current expr stack is debug-visible
    return res;
}


UncommonSendNode *NodeFactory::UncommonSendNode( GrowableArray<PseudoRegister *> *exprStack, int byteCodeIndex, int args ) {
    class UncommonSendNode *res = new class UncommonSendNode( exprStack, byteCodeIndex, args );
    registerNode( res );
    st_assert( exprStack, "must have expr. stack" );
    res->scope()->addSend( exprStack, false );  // current expr stack is debug-visible
    return res;
}
