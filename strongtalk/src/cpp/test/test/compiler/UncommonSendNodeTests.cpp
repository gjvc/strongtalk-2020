//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/assembler/Displacement.hpp"
#include "vm/lookup/LookupResult.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/compiler/NodeFactory.hpp"

#include "test/utilities/TestNotifier.hpp"
#include <gtest/gtest.h>


class UncommonSendNodeTests : public ::testing::Test {

protected:

    void SetUp() override {
        mark                = new HeapResourceMark();
        saved               = Notifier::current;
        notifier            = new TestNotifier;
        Notifier::current   = notifier;

        LookupKey    key( KlassOop( Universe::find_global( "Object" ) ), oopFactory::new_symbol( "=" ) );
        LookupResult result = LookupCache::lookup( &key );

        theCompiler = new Compiler( &key, result.method() );
        topScope    = theCompiler->topScope;
        theCompiler->enterScope( topScope );
        topScope->createTemporaries( 1 );

        NodeFactory::_cumulativeCost = 0;
        exprStack                    = new GrowableArray<PseudoRegister *>( 10 );
    }


    void TearDown() override {
        theCompiler       = nullptr;
        node              = nullptr;
        Notifier::current = saved;
        notifier          = nullptr;
        delete mark;
        mark = nullptr;
    }


    HeapResourceMark                *mark;
    UncommonSendNode                *node;
    GrowableArray<PseudoRegister *> *exprStack;
    Notifier                        *saved;
    TestNotifier                    *notifier;
    InlinedScope                    *topScope;
    BasicBlock                      *bb;


    void checkSingleUse( std::int32_t index, Node *expected ) {
        DefinitionUsageInfo *info = bb->duInfo.info->at( index );
        ASSERT_EQ( 1, info->_usages.length() );
        Usage               *use  = info->_usages.at( 0 );
        ASSERT_TRUE( expected == use->_node );

    }
};

TEST_F( UncommonSendNodeTests, construction ) {
    node = NodeFactory::UncommonSendNode( exprStack, 0, 0 );
    ASSERT_EQ( 4, NodeFactory::_cumulativeCost );
    ASSERT_TRUE( node->isUncommonSendNode() );
}


TEST_F( UncommonSendNodeTests, cloneShouldCopyContents ) {
    node                         = NodeFactory::UncommonSendNode( exprStack, 1, 2 );
    Node             *clone      = node->clone( nullptr, nullptr );
    ASSERT_TRUE( clone->isUncommonSendNode() );
    UncommonSendNode *clonedNode = ( (UncommonSendNode *) clone );
    ASSERT_EQ( 1, clonedNode->byteCodeIndex() );
    ASSERT_EQ( 2, clonedNode->args() );
    ASSERT_TRUE( clonedNode->expressionStack() != node->expressionStack() );
}


TEST_F( UncommonSendNodeTests, verifyShouldCallErrorWhenNotLastInBB ) {
    node          = NodeFactory::UncommonSendNode( exprStack, 1, 0 );
    Node *nop     = NodeFactory::createAndRegisterNode<NopNode>();
    Node *comment = NodeFactory::createAndRegisterNode<CommentNode>( "test" );
    nop->append( node );
    node->append( comment );
    BasicBlock *bb = new BasicBlock( nop, comment, 0 );
    node->setBasicBlock( bb );
    node->verify();
    ASSERT_EQ( 1, notifier->errorCount() );
}


TEST_F( UncommonSendNodeTests, verifyShouldCallErrorWhenMoreArgsThanExpressions ) {
    node      = NodeFactory::UncommonSendNode( exprStack, 1, 1 );
    Node *nop = NodeFactory::createAndRegisterNode<NopNode>();
    nop->append( node );
    char buffer[2048];

    sprintf( buffer, "Too few expressions on stack for ((UncommonSendNode*)0x%08x): required %d, but got %d", std::int32_t( node ), 1, 0 );
    BasicBlock *bb = new BasicBlock( nop, node, 0 );
    node->setBasicBlock( bb );
    node->verify();
    ASSERT_EQ( 1, notifier->errorCount() );
    ASSERT_EQ( 0, strcmp( buffer, notifier->errorAt( 0 ) ) );
}


TEST_F( UncommonSendNodeTests, toStringShouldReturnFormattedString ) {
    node = NodeFactory::UncommonSendNode( exprStack, 1, 1 );
    char buffer[2048];
    node->toString( buffer, false );
    EXPECT_EQ( 0, strncmp( "UncommonSend(1 arg)", buffer, 19 ) ) << buffer;
}


TEST_F( UncommonSendNodeTests, toStringShouldReturnFormattedStringWithAddress ) {
    node = NodeFactory::UncommonSendNode( exprStack, 1, 1 );
    char expected[100];
    sprintf( expected, "UncommonSend(1 arg)                     (UncommonSendNode*)0x%08x)", std::int32_t( node ) );
    char buffer[100];
    node->toString( buffer );
    EXPECT_EQ( 0, strcmp( expected, buffer ) ) << buffer;
}


TEST_F( UncommonSendNodeTests, makeUsesShouldAddUseForOneArgument ) {
    PseudoRegister *expr = new PseudoRegister( topScope );
    exprStack->append( expr );
    Node *nop = NodeFactory::createAndRegisterNode<NopNode>();
    node = NodeFactory::UncommonSendNode( exprStack, 1, 1 );
    nop->append( node );
    bbIterator->pseudoRegisterTable = new GrowableArray<PseudoRegister *>;
    bb = new BasicBlock( nop, node, 2 );
    node->setBasicBlock( bb );
    ASSERT_TRUE( bb->duInfo.info == nullptr );
    bb->makeUses();
    ASSERT_EQ( 1, bb->duInfo.info->length() );
    checkSingleUse( 0, node );
}


TEST_F( UncommonSendNodeTests, makeUsesShouldAddUseForTwoArguments ) {
    PseudoRegister *arg1 = new PseudoRegister( topScope );
    PseudoRegister *arg2 = new PseudoRegister( topScope );
    exprStack->append( arg1 );
    exprStack->append( arg2 );
    Node *nop = NodeFactory::createAndRegisterNode<NopNode>();
    node = NodeFactory::UncommonSendNode( exprStack, 1, 2 );
    nop->append( node );
    bbIterator->pseudoRegisterTable = new GrowableArray<PseudoRegister *>;
    bb = new BasicBlock( nop, node, 2 );
    node->setBasicBlock( bb );
    ASSERT_TRUE                    ( bb->duInfo.info == nullptr );
    bb->makeUses();
    ASSERT_EQ( 2, bb->duInfo.info->length() );
    checkSingleUse( 0, node );
    checkSingleUse( 1, node );
}
