//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/assembler/Displacement.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/primitives/OopPrimitives.hpp"
#include "vm/utilities/ObjectIDTable.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/OopFactory.hpp"

#include <gtest/gtest.h>


class OopPrimitivesBecomeTest : public ::testing::Test {

public:
    OopPrimitivesBecomeTest() : ::testing::Test(), targetContainer{},replacementContainer{},tenuredTargetContainer{}, tenuredReplacementContainer{},target{},replacement{} {}

protected:
    void SetUp() override {
        KlassOop objectClass = KlassOop( Universe::find_global( "Object" ) );

        target      = MemOop( objectClass->primitive_allocate() );
        replacement = MemOop( objectClass->primitive_allocate() );

        targetContainer      = OopFactory::new_objectArray( 1 );
        filler               = OopFactory::new_objectArray( 128 ); // make sure containers are on different cards
        replacementContainer = OopFactory::new_objectArray( 1 );

        targetContainer->obj_at_put( 1, target );
        replacementContainer->obj_at_put( 1, replacement );

        tenuredTargetContainer      = OopFactory::new_association( vmSymbols::value(), target, false );
        tenuredReplacementContainer = OopFactory::new_association( vmSymbols::value(), replacement, false );

        st_assert( Universe::old_gen.contains( tenuredTargetContainer ), "target container should be tenured" );
        st_assert( Universe::old_gen.contains( tenuredReplacementContainer ), "replacement container should be tenured" );

        saveNil = Universe::nilObject();
    }


    void TearDown() override {
        targetContainer        = replacementContainer        = nullptr;
        tenuredTargetContainer = tenuredReplacementContainer = nullptr;
        target                 = replacement                 = nullptr;
        if ( Universe::nilObject() != saveNil )
            OopPrimitives::become( saveNil, Universe::nilObject() );
        MarkSweep::collect();
    }


    ObjectArrayOop targetContainer, replacementContainer, filler;
    AssociationOop tenuredTargetContainer, tenuredReplacementContainer;
    MemOop         target, replacement;
    Oop            saveNil;


};

TEST_F( OopPrimitivesBecomeTest, becomeShouldSwapTargetAndReplacement ) {
    OopPrimitives::become( replacement, target );
    EXPECT_EQ( replacement, targetContainer->obj_at( 1 ) ) << "target of become: has not been replaced by replacement";
    EXPECT_EQ( target, replacementContainer->obj_at( 1 ) ) << "replacement has not been replaced by target of become:";
}


TEST_F( OopPrimitivesBecomeTest, becomeShouldReturnTarget ) { EXPECT_EQ( target, OopPrimitives::become( replacement, target ) ) << "should return target"; }


TEST_F( OopPrimitivesBecomeTest, becomeShouldMarkStoredCards ) {
    Universe::remembered_set->clear();
    OopPrimitives::become( replacement, target );
    EXPECT_TRUE( Universe::remembered_set->is_dirty( targetContainer ) ) << "target container should be diry";
    EXPECT_TRUE( Universe::remembered_set->is_dirty( replacementContainer ) ) << "replacement container should be diry";
}


TEST_F( OopPrimitivesBecomeTest, becomeShouldSwapTargetAndReplacementReferencesInTenuredObjects ) {
    OopPrimitives::become( replacement, target );
    EXPECT_EQ( replacement, tenuredTargetContainer->value() ) << "target of become: has not been replaced by replacement";
    EXPECT_EQ( target, tenuredReplacementContainer->value() ) << "replacement has not been replaced by target of become:";
}


TEST_F( OopPrimitivesBecomeTest, becomeShouldUpdateRoots ) {
    OopPrimitives::become( Universe::nilObject(), target );
    EXPECT_EQ( target, Universe::nilObject() ) << "nilObject should now be target";
}


TEST_F( OopPrimitivesBecomeTest, becomeShouldReturnErrorWhenReceiverIsSmallInteger ) {
    EXPECT_EQ( markSymbol( vmSymbols::first_argument_has_wrong_type() ), OopPrimitives::become( replacement, smiOop_one ) ) << "receiver cannot be small integer";
}


TEST_F( OopPrimitivesBecomeTest, becomeShouldReturnErrorWhenReplacementIsSmallInteger ) {
    EXPECT_EQ( markSymbol( vmSymbols::second_argument_has_wrong_type() ), OopPrimitives::become( smiOop_one, replacement ) ) << "replacement cannot be small integer";
}
