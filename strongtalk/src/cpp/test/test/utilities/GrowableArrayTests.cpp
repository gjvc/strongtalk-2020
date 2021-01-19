//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/GrowableArray.hpp"

#include <gtest/gtest.h>


class GenericGrowableArrayTests : public ::testing::Test {

protected:
    void SetUp() override {
    }


    void TearDown() override {
    }


    GenericGrowableArray gga;


};


class GrowableArrayTests : public ::testing::Test {

protected:
    void SetUp() override {
    }


    void TearDown() override {
    }


};


TEST( GrowableArrayTests, push_pop
) {
GrowableArray<std::int32_t> ga;
ga.push( 32 );
auto result = ga.pop();
EXPECT_EQ( result,
32 );
}


TEST( GrowableArrayTests, append_pop
) {
GrowableArray<std::int32_t> ga;
ga.append( 32 );
auto result = ga.pop();
EXPECT_EQ( result,
32 );
}


TEST( GrowableArrayTests, test_two
) {
}
