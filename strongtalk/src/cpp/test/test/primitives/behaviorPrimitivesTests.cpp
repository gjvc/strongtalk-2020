
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/primitives/behavior_primitives.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "test/memory/edenMark.hpp"
#include "vm/system/sizes.hpp"


#include <gtest/gtest.h>


class BehaviorPrimitives : public ::testing::Test {

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


TEST_F( BehaviorPrimitives, allocateForMemOopShouldReportFailureWhenNoSpace ) {
    EXPECT_TRUE( Universe::new_gen.eden()->free() < ( 2 * oopSize ) ) << "Too much free Space";
    EXPECT_EQ( markSymbol( vmSymbols::failed_allocation() ), behaviorPrimitives::allocate3( Universe::falseObj(), objectClass ) ) << "Allocation should fail";
}


TEST_F( BehaviorPrimitives, allocateForMemOopShouldAllocateTenureWhenRequired ) {
    ASSERT_TRUE( behaviorPrimitives::allocate3( Universe::trueObj(), objectClass )->is_old() );
}


TEST_F( BehaviorPrimitives, allocateForMemOopShouldCheckTenuredIsBoolean ) {
    ASSERT_TRUE( markSymbol( vmSymbols::second_argument_has_wrong_type() ) == behaviorPrimitives::allocate3( Universe::nilObj(), objectClass ) );
}


TEST_F( BehaviorPrimitives, allocateForMemOopShouldScavengeAndAllocateWhenAllowed ) {
    HandleMark       mark;
    Handle           objectClassHandle( objectClass );
    EXPECT_TRUE( Universe::new_gen.eden()->free() < ( 2 * oopSize ) ) << "Too much free Space";
    Oop              object = behaviorPrimitives::allocate( objectClass );
    EXPECT_TRUE( !object->is_mark() ) << "result should not be marked";
    EXPECT_TRUE( object->is_mem() ) << "result should be mem";
    EXPECT_EQ( MemOop( object )->klass(), objectClassHandle.as_memOop() ) << "wrong class";
}
