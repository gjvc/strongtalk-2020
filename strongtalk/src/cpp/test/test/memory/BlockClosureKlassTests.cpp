//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/system/sizes.hpp"


#include <gtest/gtest.h>

extern "C" Oop * eden_top;
extern "C" Oop * eden_end;


class BlockClosureKlassTests : public ::testing::Test {

    protected:

        KlassOop theClass;
        Oop      * oldEdenTop;


        void SetUp() override {
            theClass   = KlassOop( Universe::find_global( "BlockWithoutArguments" ) );
            oldEdenTop = eden_top;
        }


        void TearDown() override {
            eden_top = oldEdenTop;
            MarkSweep::collect();
        }


};


TEST_F( BlockClosureKlassTests, allocateShouldFailWhenAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_EQ( ( int ) nullptr, ( int ) ( theClass->klass_part()->allocateObject( false ) ) );
}


TEST_F( BlockClosureKlassTests, allocateShouldAllocateTenuredWhenRequired ) {
    ASSERT_TRUE( Universe::old_gen.contains( theClass->klass_part()->allocateObject( false, true ) ) );
}


TEST_F( BlockClosureKlassTests, allocateShouldNotFailWhenNotAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_TRUE( Universe::new_gen.eden()->free() < 4 * oopSize );
    ASSERT_TRUE( Universe::new_gen.contains( theClass->klass_part()->allocateObject( true ) ) );
}
