//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"


#include <gtest/gtest.h>

extern "C" Oop *eden_top;
extern "C" Oop *eden_end;


class ObjectArrayKlassTests : public ::testing::Test {

public:
    ObjectArrayKlassTests() :
        ::testing::Test(),
        theClass{},
        oldEdenTop{ nullptr } {}


protected:
    void SetUp() override {
        theClass   = KlassOop( Universe::find_global( "Array" ) );
        oldEdenTop = eden_top;
    }


    void TearDown() override {
        eden_top = oldEdenTop;
        MarkSweep::collect();
    }


    KlassOop theClass;
    Oop      *oldEdenTop;

};


TEST_F( ObjectArrayKlassTests, shouldBeObjectArray ) {
    eden_top = eden_end;
    ASSERT_TRUE( theClass->klass_part()->oopIsObjectArray() );
}


TEST_F( ObjectArrayKlassTests, allocateShouldFailWhenAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_EQ( (std::int32_t) nullptr, (std::int32_t) ( theClass->klass_part()->allocateObjectSize( 100, false ) ) );
}


TEST_F( ObjectArrayKlassTests, allocateShouldAllocateTenuredWhenRequired ) {
    ASSERT_TRUE( Universe::old_gen.contains( theClass->klass_part()->allocateObjectSize( 100, false, true ) ) );
}


TEST_F( ObjectArrayKlassTests, allocateShouldNotFailWhenNotAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_TRUE( Universe::new_gen.eden()->free() < 4 * OOP_SIZE );
    ASSERT_TRUE( Universe::new_gen.contains( theClass->klass_part()->allocateObjectSize( 100, true ) ) );
}


TEST_F( ObjectArrayKlassTests, allocateShouldExpandOldSpaceDuringTenuredAllocWhenAllowed ) {
    OldSpaceMark mark  = Universe::old_gen.memo();
//    OldSpace     *space = mark.theSpace;
    std::int32_t free  = Universe::old_gen.free() / OOP_SIZE;
    Oop          *temp = Universe::allocate_tenured( free - 1, false );
    ASSERT_TRUE( temp != nullptr );
    ASSERT_TRUE( Universe::old_gen.contains( theClass->klass_part()->allocateObjectSize( 100, true, true ) ) );
}


TEST_F( ObjectArrayKlassTests, allocateShouldFailDuringTenuredAllocWhenOldSpaceExpansionNotAllowed ) {
    OldSpaceMark mark  = Universe::old_gen.memo();
//    OldSpace     *space = mark.theSpace;
    std::int32_t free  = Universe::old_gen.free() / OOP_SIZE;
    Oop          *temp = Universe::allocate_tenured( free - 1, false );
    ASSERT_TRUE( temp != nullptr );
    ASSERT_TRUE( nullptr == theClass->klass_part()->allocateObjectSize( 100, false, true ) );
}
