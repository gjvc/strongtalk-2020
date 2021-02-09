//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/DoubleKlass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"


#include <gtest/gtest.h>

extern "C" Oop *eden_top;
extern "C" Oop *eden_end;


class DoubleKlassTests : public ::testing::Test {

public:
    DoubleKlassTests() :
        ::testing::Test(),
        theClass{},
        oldEdenTop{ nullptr } {}


protected:
    void SetUp() override {
        theClass   = KlassOop( Universe::find_global( "Float" ) );
        oldEdenTop = eden_top;
    }


    void TearDown() override {
        eden_top = oldEdenTop;
        MarkSweep::collect();
    }


    KlassOop theClass;
    Oop      *oldEdenTop;


};


TEST_F( DoubleKlassTests, floatShouldBeDoubleC ) {
    ASSERT_TRUE( theClass->klass_part()->oop_is_double() );
}


TEST_F( DoubleKlassTests, allocateShouldFailWhenAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_TRUE( Universe::new_gen.eden()->free() < 4 * OOP_SIZE );
    ASSERT_EQ( (std::int32_t) nullptr, (std::int32_t) ( theClass->klass_part()->allocateObject( false ) ) );
}


TEST_F( DoubleKlassTests, allocateShouldAllocateTenuredWhenRequired ) {
    ASSERT_TRUE( Universe::old_gen.contains( theClass->klass_part()->allocateObject( false, true ) ) );
}


TEST_F( DoubleKlassTests, allocateShouldNotFailWhenNotAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_TRUE( Universe::new_gen.eden()->free() < 4 * OOP_SIZE );
    ASSERT_TRUE( Universe::new_gen.contains( theClass->klass_part()->allocateObject( true ) ) );
}
