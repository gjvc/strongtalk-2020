//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"


#include <gtest/gtest.h>

extern "C" Oop *eden_top;
extern "C" Oop *eden_end;


class ObjArrayKlassTests : public ::testing::Test {

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


TEST_F( ObjArrayKlassTests, shouldBeObjArray ) {
    eden_top = eden_end;
    ASSERT_TRUE( theClass->klass_part()->oop_is_objArray() );
}


TEST_F( ObjArrayKlassTests, allocateShouldFailWhenAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_EQ( (std::int32_t) nullptr, (std::int32_t) ( theClass->klass_part()->allocateObjectSize( 100, false ) ) );
}


TEST_F( ObjArrayKlassTests, allocateShouldAllocateTenuredWhenRequired ) {
    ASSERT_TRUE( Universe::old_gen.contains( theClass->klass_part()->allocateObjectSize( 100, false, true ) ) );
}


TEST_F( ObjArrayKlassTests, allocateShouldNotFailWhenNotAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_TRUE( Universe::new_gen.eden()->free() < 4 * OOP_SIZE );
    ASSERT_TRUE( Universe::new_gen.contains( theClass->klass_part()->allocateObjectSize( 100, true ) ) );
}


TEST_F( ObjArrayKlassTests, allocateShouldExpandOldSpaceDuringTenuredAllocWhenAllowed ) {
    OldSpaceMark mark   = Universe::old_gen.memo();
//    OldSpace     *space = mark.theSpace;
    std::int32_t free   = Universe::old_gen.free() / OOP_SIZE;
    Oop          *temp  = Universe::allocate_tenured( free - 1, false );
    ASSERT_TRUE( temp != nullptr );
    ASSERT_TRUE( Universe::old_gen.contains( theClass->klass_part()->allocateObjectSize( 100, true, true ) ) );
}


TEST_F( ObjArrayKlassTests, allocateShouldFailDuringTenuredAllocWhenOldSpaceExpansionNotAllowed ) {
    OldSpaceMark mark   = Universe::old_gen.memo();
//    OldSpace     *space = mark.theSpace;
    std::int32_t free   = Universe::old_gen.free() / OOP_SIZE;
    Oop          *temp  = Universe::allocate_tenured( free - 1, false );
    ASSERT_TRUE( temp != nullptr );
    ASSERT_TRUE( nullptr == theClass->klass_part()->allocateObjectSize( 100, false, true ) );
}
