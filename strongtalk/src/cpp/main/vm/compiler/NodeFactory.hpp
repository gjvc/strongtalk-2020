
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/compiler/BitVector.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/compiler/OpCode.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/compiler/Node.hpp"



// NodeFactory is used to create new nodes.

class NodeFactory : AllStatic {

public:
    static std::int32_t _cumulativeCost; // cumulative cost of all nodes generated so far

    static void registerNode( Node *n ) {
        _cumulativeCost += n->cost();
    }


    static class PrologueNode *PrologueNode( LookupKey *key, std::int32_t nofArgs, std::int32_t nofTemps );

    static class LoadOffsetNode *LoadOffsetNode( PseudoRegister *dst, PseudoRegister *base, std::int32_t offs, bool_t isArray );

    static class LoadUplevelNode *LoadUplevelNode( PseudoRegister *dst, PseudoRegister *context0, std::int32_t nofLevels, std::int32_t offset, SymbolOop name );

    static class LoadIntNode *LoadIntNode( PseudoRegister *dst, std::int32_t value );

    static class StoreOffsetNode *StoreOffsetNode( PseudoRegister *src, PseudoRegister *base, std::int32_t offs, bool_t needStoreCheck );

    static class StoreUplevelNode *StoreUplevelNode( PseudoRegister *src, PseudoRegister *context0, std::int32_t nofLevels, std::int32_t offset, SymbolOop name, bool_t needStoreCheck );

    static class AssignNode *AssignNode( PseudoRegister *src, PseudoRegister *dst );

    static class ReturnNode *ReturnNode( PseudoRegister *res, std::int32_t byteCodeIndex );

    static class InlinedReturnNode *InlinedReturnNode( std::int32_t byteCodeIndex, PseudoRegister *src, PseudoRegister *dst );

    static class NonLocalReturnSetupNode *NonLocalReturnSetupNode( PseudoRegister *result, std::int32_t byteCodeIndex );

    static class NonLocalReturnContinuationNode *NonLocalReturnContinuationNode( std::int32_t byteCodeIndex );

    static class NonLocalReturnTestNode *NonLocalReturnTestNode( std::int32_t byteCodeIndex );

    static ArithRRNode *ArithRRNode( ArithOpCode op, PseudoRegister *src, PseudoRegister *o2, PseudoRegister *dst );

    static ArithRCNode *ArithRCNode( ArithOpCode op, PseudoRegister *src, std::int32_t o2, PseudoRegister *dst );

    static TArithRRNode *TArithRRNode( ArithOpCode op, PseudoRegister *src, PseudoRegister *o2, PseudoRegister *dst, bool_t a1, bool_t a2 );

    static FloatArithRRNode *FloatArithRRNode( ArithOpCode op, PseudoRegister *src, PseudoRegister *o2, PseudoRegister *dst );

    static FloatUnaryArithNode *FloatUnaryArithNode( ArithOpCode op, PseudoRegister *src, PseudoRegister *dst );

    static class MergeNode *MergeNode( Node *prev1, Node *prev2 );

    static class MergeNode *MergeNode( std::int32_t byteCodeIndex );

    static class SendNode *SendNode( LookupKey *key, class MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack, bool_t superSend, SendInfo *info );

    static class PrimitiveNode *PrimitiveNode( PrimitiveDescriptor *pdesc, class MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack );

    static class DLLNode *DLLNode( SymbolOop dll_name, SymbolOop function_name, dll_func_ptr_t function, bool_t async, class MergeNode *nlrTestPoint, GrowableArray<PseudoRegister *> *args, GrowableArray<PseudoRegister *> *expr_stack );

    static class InterruptCheckNode *InterruptCheckNode( GrowableArray<PseudoRegister *> *expr_stack );

    static class LoopHeaderNode *LoopHeaderNode();

    static class BlockCreateNode *BlockCreateNode( BlockPseudoRegister *b, GrowableArray<PseudoRegister *> *expr_stack );

    static class BlockMaterializeNode *BlockMaterializeNode( BlockPseudoRegister *b, GrowableArray<PseudoRegister *> *expr_stack );

    static class ContextCreateNode *ContextCreateNode( PseudoRegister *parent, PseudoRegister *context, std::int32_t nofTemps, GrowableArray<PseudoRegister *> *expr_stack );

    static class ContextCreateNode *ContextCreateNode( PseudoRegister *b, const class ContextCreateNode *n, GrowableArray<PseudoRegister *> *expr_stack );

    static class ContextInitNode *ContextInitNode( class ContextCreateNode *creator );

    static class ContextInitNode *ContextInitNode( PseudoRegister *b, const class ContextInitNode *n );

    static class ContextZapNode *ContextZapNode( PseudoRegister *context );

    static class BranchNode *BranchNode( BranchOpCode op, bool_t taken_is_uncommon = false );

    static class TypeTestNode *TypeTestNode( PseudoRegister *recv, GrowableArray<KlassOop> *classes, bool_t hasUnknown );

    static class ArrayAtNode *ArrayAtNode( ArrayAtNode::AccessType access_type, PseudoRegister *array, PseudoRegister *index, bool_t smiIndex, PseudoRegister *result, PseudoRegister *error, std::int32_t data_offset, std::int32_t length_offset );

    static class ArrayAtPutNode *ArrayAtPutNode( ArrayAtPutNode::AccessType access_type, PseudoRegister *array, PseudoRegister *index, bool_t smi_index, PseudoRegister *element, bool_t smi_element, PseudoRegister *result, PseudoRegister *error, std::int32_t data_offset, std::int32_t length_offset, bool_t needs_store_check );

    static class InlinedPrimitiveNode *InlinedPrimitiveNode( InlinedPrimitiveNode::Operation op, PseudoRegister *result, PseudoRegister *error = nullptr, PseudoRegister *recv = nullptr, PseudoRegister *arg1 = nullptr, bool_t arg1_is_smi = false, PseudoRegister *arg2 = nullptr, bool_t arg2_is_smi = false );

    static class UncommonNode *UncommonNode( GrowableArray<PseudoRegister *> *exprStack, std::int32_t byteCodeIndex );

    static class UncommonSendNode *UncommonSendNode( GrowableArray<PseudoRegister *> *exprStack, std::int32_t byteCodeIndex, std::int32_t args );

    static class FixedCodeNode *FixedCodeNode( FixedCodeNode::FixedCodeKind k );

    static class NopNode *NopNode();

    static class CommentNode *CommentNode( const char *comment );




    template <typename T, typename...Args>
    static T *createAndRegisterNode(Args... args) {
        T *res = new T( args... );
        registerNode( res );
        return res;
    }

};
