//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/memory/Handle.hpp"


#include <gtest/gtest.h>

extern "C" std::int32_t expansion_count;


class OldSpaceTests : public ::testing::Test {

protected:
    void SetUp() override {
    }


    void TearDown() override {
        MarkSweep::collect();
    }

};


TEST_F( OldSpaceTests, expandShouldIncreaseExpansionCount
) {
std::int32_t expansions = expansion_count;
OldSpace *theSpace = Universe::old_gen.top_mark()._space;
theSpace->
expand( ObjectHeapExpandSize
* 1024 );
ASSERT_EQ( expansions
+ 1, expansion_count );
}


TEST_F( OldSpaceTests, oldSpaceMarkShouldRestoreTop
) {
OldSpace *theSpace = Universe::old_gen.top_mark()._space;
std::int32_t freeSpace = theSpace->free();
Oop *oldTop = theSpace->top();
{
OldSpaceMark mark( theSpace );
ASSERT_TRUE( theSpace
->allocate( 100, false ) != nullptr );
ASSERT_EQ( freeSpace
- ( 100 * oopSize ), theSpace->
free()
);
}
ASSERT_EQ( freeSpace, theSpace
->
free()
);
ASSERT_EQ( ( const char * ) oldTop, ( const char * ) theSpace->
top()
);
}


TEST_F( OldSpaceTests, expandAndAllocateShouldLeaveFreeSpaceUnchanged
) {
HandleMark mark;
Handle     byteArrayClass( Universe::find_global( "ByteArray" ) );

OldSpace *theSpace = Universe::old_gen.top_mark()._space;
std::int32_t freeSpace = theSpace->free();
{
OldSpaceMark mark( theSpace );
Oop *result = theSpace->expand_and_allocate( ObjectHeapExpandSize * 1024 );
ASSERT_EQ( freeSpace, theSpace
->
free()
);
}
}
