//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/runtime/flags.hpp"

#include <gtest/gtest.h>


class OldGenerationTest : public ::testing::Test {

protected:
    void SetUp() override {
    }


    void TearDown() override {
    }


    void expandAndCheckCapacity( std::int32_t expansionSize ) {
        char msg[100];
        std::int32_t  oldSize = Universe::old_gen.capacity();

        Universe::old_gen.expand( expansionSize );

        std::int32_t expectedIncrement = ReservedSpace::align_size( expansionSize, ObjectHeapExpandSize * 1024 );
        std::int32_t actual            = Universe::old_gen.capacity();
        std::int32_t expectedSize      = oldSize + expectedIncrement;
        sprintf( msg, "Generation has wrong capacity. Expected: %d, but was: %d", expectedSize, actual );
        EXPECT_EQ( expectedSize, actual ) << msg;
    }

};


TEST_F( OldGenerationTest, expansionShouldExpandOldGenerationCapacity
) {
expandAndCheckCapacity( 1000 * 1024 );
}


TEST_F( OldGenerationTest, expansionShouldExpandOldGenerationCapacityByMinimumAmount
) {
expandAndCheckCapacity( 10 );
}


TEST_F( OldGenerationTest, expansionShouldAlignExpansionWithExpansionSize
) {
expandAndCheckCapacity( ObjectHeapExpandSize
* 1024 * 3 / 2 );
}


TEST_F( OldGenerationTest, allocateWithoutExpansionWhenEmptyShouldFail
) {
std::int32_t free = Universe::old_gen.free();
Oop *result = Universe::old_gen.allocate( free + 1, false );
EXPECT_EQ( nullptr, result ) << "Result should be nullptr";
}


TEST_F( OldGenerationTest, shrinkShouldReduceOldSpaceCapacity
) {
std::int32_t freeSpace = Universe::old_gen.free();
Universe::old_gen.
expand( ObjectHeapExpandSize
* 1024 );
Universe::old_gen.
shrink( ObjectHeapExpandSize
* 1024 );
ASSERT_EQ( freeSpace, Universe::old_gen
.
free()
);
}


TEST_F( OldGenerationTest, shrinkShouldReturnZeroWhenInsufficientFreeSpace
) {
std::int32_t freeSpace = Universe::old_gen.free();
ASSERT_EQ( 0, Universe::old_gen.
shrink( freeSpace
+ 1 ) );
ASSERT_EQ( freeSpace, Universe::old_gen
.
free()
);
}
