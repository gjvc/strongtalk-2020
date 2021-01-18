//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/system/sizes.hpp"


#include <gtest/gtest.h>


class UniverseTests : public ::testing::Test {

protected:
    void SetUp() override {
        eden_old_top = eden_top;
    }


    void TearDown() override {
        // Can't collect because the allocated Space is not an object.
        eden_top = eden_old_top;
    }


    Oop *eden_old_top;

};


TEST_F( UniverseTests, allocateShouldAllocateInNewSpaceWhenSpaceAvailable
) {
ASSERT_TRUE( Universe::new_gen
.eden()->free() > 10 );
Oop *chunk = Universe::allocate( 10, nullptr, false );
ASSERT_TRUE( chunk
!= nullptr );
ASSERT_TRUE( Universe::new_gen
.eden()->
contains   ( chunk )
);
}


TEST_F( UniverseTests, allocateShouldFailWhenNoSpaceAndScavengeDisallowed
) {
int freeSpace = Universe::new_gen.eden()->free();
Oop *chunk = Universe::allocate( freeSpace / oopSize + 1, nullptr, false );
ASSERT_EQ( nullptr, ( const char * ) chunk );
}


TEST_F( UniverseTests, allocateTenuredShouldFailWhenNoSpaceAndExpansionDisallowed
) {
int freeSpace = Universe::old_gen.free();
Oop *chunk = Universe::allocate_tenured( freeSpace / oopSize + 1, false );
ASSERT_EQ( nullptr, ( const char * ) chunk );
}
