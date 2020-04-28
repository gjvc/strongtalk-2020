//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/runtime/ResourceObject.hpp"

class Node;

class PrologueNode;

class LoadIntNode;

class LoadOffsetNode;

class LoadUplevelNode;

class AssignNode;

class StoreOffsetNode;

class StoreUplevelNode;

class ArithRRNode;

class ArithRCNode;

class FloatArithRRNode;

class FloatUnaryArithNode;

class TArithRRNode;

class ContextCreateNode;

class ContextInitNode;

class ContextZapNode;

class BlockCreateNode;

class BlockMaterializeNode;

class SendNode;

class PrimitiveNode;

class DLLNode;

class LoopHeaderNode;

class ReturnNode;

class NonLocalReturnSetupNode;

class InlinedReturnNode;

class NonLocalReturnContinuationNode;

class BranchNode;

class TypeTestNode;

class NonLocalReturnTestNode;

class MergeNode;

class ArrayAtNode;

class ArrayAtPutNode;

class InlinedPrimitiveNode;

class UncommonNode;

class FixedCodeNode;

class NopNode;

class CommentNode;


class NodeVisitor : public ResourceObject {

    public:
        // Basic blocks
        virtual void beginOfBasicBlock( Node * node ) = 0;    // called for the first node in a BasicBlock, before the node's method is called
        virtual void endOfBasicBlock( Node * node ) = 0;        // called for the last node in a BasicBlock, after the node's method was called

    public:
        // For all nodes
        virtual void beginOfNode( Node * node ) = 0;        // called for each node, before the node's method is called
        virtual void endOfNode( Node * node ) = 0;        // called for each node, after the node's method was called

    public:
        // Individual nodes
        virtual void aPrologueNode( PrologueNode * node ) = 0;

        virtual void aLoadIntNode( LoadIntNode * node ) = 0;

        virtual void aLoadOffsetNode( LoadOffsetNode * node ) = 0;

        virtual void aLoadUplevelNode( LoadUplevelNode * node ) = 0;

        virtual void anAssignNode( AssignNode * node ) = 0;

        virtual void aStoreOffsetNode( StoreOffsetNode * node ) = 0;

        virtual void aStoreUplevelNode( StoreUplevelNode * node ) = 0;

        virtual void anArithRRNode( ArithRRNode * node ) = 0;

        virtual void aFloatArithRRNode( FloatArithRRNode * node ) = 0;

        virtual void aFloatUnaryArithNode( FloatUnaryArithNode * node ) = 0;

        virtual void anArithRCNode( ArithRCNode * node ) = 0;

        virtual void aTArithRRNode( TArithRRNode * node ) = 0;

        virtual void aContextCreateNode( ContextCreateNode * node ) = 0;

        virtual void aContextInitNode( ContextInitNode * node ) = 0;

        virtual void aContextZapNode( ContextZapNode * node ) = 0;

        virtual void aBlockCreateNode( BlockCreateNode * node ) = 0;

        virtual void aBlockMaterializeNode( BlockMaterializeNode * node ) = 0;

        virtual void aSendNode( SendNode * node ) = 0;

        virtual void aPrimitiveNode( PrimitiveNode * node ) = 0;

        virtual void aDLLNode( DLLNode * node ) = 0;

        virtual void aLoopHeaderNode( LoopHeaderNode * node ) = 0;

        virtual void aReturnNode( ReturnNode * node ) = 0;

        virtual void aNonLocalReturnSetupNode( NonLocalReturnSetupNode * node ) = 0;

        virtual void anInlinedReturnNode( InlinedReturnNode * node ) = 0;

        virtual void aNonLocalReturnContinuationNode( NonLocalReturnContinuationNode * node ) = 0;

        virtual void aBranchNode( BranchNode * node ) = 0;

        virtual void aTypeTestNode( TypeTestNode * node ) = 0;

        virtual void aNonLocalReturnTestNode( NonLocalReturnTestNode * node ) = 0;

        virtual void aMergeNode( MergeNode * node ) = 0;

        virtual void anArrayAtNode( ArrayAtNode * node ) = 0;

        virtual void anArrayAtPutNode( ArrayAtPutNode * node ) = 0;

        virtual void anInlinedPrimitiveNode( InlinedPrimitiveNode * node ) = 0;

        virtual void anUncommonNode( UncommonNode * node ) = 0;

        virtual void aFixedCodeNode( FixedCodeNode * node ) = 0;

        virtual void aNopNode( NopNode * node ) = 0;

        virtual void aCommentNode( CommentNode * node ) = 0;
};

