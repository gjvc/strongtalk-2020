//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/interpreter/HeapCodeBuffer.hpp"
#include "vm/memory/MarkSweep.hpp"


#include <gtest/gtest.h>

extern "C" Oop *eden_top;
extern "C" Oop *eden_end;


class ByteArrayKlassTests : public ::testing::Test {

public:
    ByteArrayKlassTests() :
        ::testing::Test(),
        theClass{},
        oldEdenTop{ nullptr } {}


protected:

    void SetUp() override {
        theClass   = KlassOop( Universe::find_global( "ByteArray" ) );
        oldEdenTop = eden_top;

    }


    void TearDown() override {
        eden_top = oldEdenTop;
        MarkSweep::collect();
    }


    KlassOop theClass;
    Oop      *oldEdenTop;

};


TEST_F( ByteArrayKlassTests, shouldBeDoubleByteArray ) {
    eden_top = eden_end;
    ASSERT_TRUE( theClass->klass_part()->oopIsByteArray() );
}


TEST_F( ByteArrayKlassTests, allocateShouldFailWhenAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_EQ( (std::int32_t) nullptr, (std::int32_t) ( theClass->klass_part()->allocateObjectSize( 100, false ) ) );
}


TEST_F( ByteArrayKlassTests, allocateShouldAllocateTenuredWhenRequired ) {
    ASSERT_TRUE( Universe::old_gen.contains( theClass->klass_part()->allocateObjectSize( 100, false, true ) ) );
}


TEST_F( ByteArrayKlassTests, allocateShouldNotFailWhenNotAllowedAndNoSpace ) {
    eden_top = eden_end;
    ASSERT_TRUE( Universe::new_gen.eden()->free() < 4 * OOP_SIZE );
    ASSERT_TRUE( Universe::new_gen.contains( theClass->klass_part()->allocateObjectSize( 100, true ) ) );
}
