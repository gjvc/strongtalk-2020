//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/compiler/BasicBlock.hpp"
#include "vm/assembler/MacroAssembler.hpp"


// The OldCodeGenerator is the new interface to the gen() routines in x86_node.cpp.
// It allows to call the old code generation routines via the new apply method.

class OldCodeGenerator : public NodeVisitor {
public:
    // Basic blocks
    void beginOfBasicBlock( Node *node );

    void endOfBasicBlock( Node *node );

public:
    // For all nodes
    void beginOfNode( Node *node );

    void endOfNode( Node *node );

public:
    // Individual nodes
    void aPrologueNode( PrologueNode *node );

    void aLoadIntNode( LoadIntNode *node );

    void aLoadOffsetNode( LoadOffsetNode *node );

    void aLoadUplevelNode( LoadUplevelNode *node );

    void anAssignNode( AssignNode *node );

    void aStoreOffsetNode( StoreOffsetNode *node );

    void aStoreUplevelNode( StoreUplevelNode *node );

    void anArithRRNode( ArithRRNode *node );

    void aFloatArithRRNode( FloatArithRRNode *node );

    void aFloatUnaryArithNode( FloatUnaryArithNode *node );

    void anArithRCNode( ArithRCNode *node );

    void aTArithRRNode( TArithRRNode *node );

    void aContextCreateNode( ContextCreateNode *node );

    void aContextInitNode( ContextInitNode *node );

    void aContextZapNode( ContextZapNode *node );

    void aBlockCreateNode( BlockCreateNode *node );

    void aBlockMaterializeNode( BlockMaterializeNode *node );

    void aSendNode( SendNode *node );

    void aPrimitiveNode( PrimitiveNode *node );

    void aDLLNode( DLLNode *node );

    void aLoopHeaderNode( LoopHeaderNode *node );

    void aReturnNode( ReturnNode *node );

    void aNonLocalReturnSetupNode( NonLocalReturnSetupNode *node );

    void anInlinedReturnNode( InlinedReturnNode *node );

    void aNonLocalReturnContinuationNode( NonLocalReturnContinuationNode *node );

    void aBranchNode( BranchNode *node );

    void aTypeTestNode( TypeTestNode *node );

    void aNonLocalReturnTestNode( NonLocalReturnTestNode *node );

    void aMergeNode( MergeNode *node );

    void anArrayAtNode( ArrayAtNode *node );

    void anArrayAtPutNode( ArrayAtPutNode *node );

    void anInlinedPrimitiveNode( InlinedPrimitiveNode *node );

    void anUncommonNode( UncommonNode *node );

    void aFixedCodeNode( FixedCodeNode *node );

    void aNopNode( NopNode *node );

    void aCommentNode( CommentNode *node );
};
