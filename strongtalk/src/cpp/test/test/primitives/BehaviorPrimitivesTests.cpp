
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/primitive/BehaviorPrimitives.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "test/memory/EdenMark.hpp"


#include <gtest/gtest.h>


class BehaviorPrimitivesTests : public ::testing::Test {

public:
    BehaviorPrimitivesTests() :
        ::testing::Test(),
        edenMark{},
        objectClass{} {}


protected:
    void SetUp() override {
        edenMark.setToEnd();
        objectClass = KlassOop( Universe::find_global( "Object" ) );
    }


    void TearDown() override {

    }


    EdenMark edenMark;
    KlassOop objectClass;

};


TEST_F( BehaviorPrimitivesTests, allocateForMemOopShouldReportFailureWhenNoSpace ) {
    EXPECT_TRUE( Universe::new_gen.eden()->free() < ( 2 * OOP_SIZE ) ) << "Too much free Space";
    EXPECT_EQ( markSymbol( vmSymbols::failed_allocation() ), BehaviorPrimitives::allocate3( Universe::falseObject(), objectClass ) ) << "Allocation should fail";
}


TEST_F( BehaviorPrimitivesTests, allocateForMemOopShouldAllocateTenureWhenRequired ) {
    ASSERT_TRUE( BehaviorPrimitives::allocate3( Universe::trueObject(), objectClass )->is_old() );
}


TEST_F( BehaviorPrimitivesTests, allocateForMemOopShouldCheckTenuredIsBoolean ) {
    ASSERT_TRUE( markSymbol( vmSymbols::second_argument_has_wrong_type() ) == BehaviorPrimitives::allocate3( Universe::nilObject(), objectClass ) );
}


TEST_F( BehaviorPrimitivesTests, allocateForMemOopShouldScavengeAndAllocateWhenAllowed ) {

    HandleMark mark;
    Handle     objectClassHandle( objectClass );

    EXPECT_TRUE( Universe::new_gen.eden()->free() < ( 2 * OOP_SIZE ) ) << "Too much free Space";

    Oop object = BehaviorPrimitives::allocate( objectClass );

    EXPECT_TRUE( !object->isMarkOop() ) << "result should not be marked";
    EXPECT_TRUE( object->isMemOop() ) << "result should be mem";
    EXPECT_EQ( MemOop( object ) -> klass(), objectClassHandle.as_memOop() ) << "wrong class";
}
