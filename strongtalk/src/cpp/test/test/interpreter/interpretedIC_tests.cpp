//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/memory/Scavenge.hpp"

#include <gtest/gtest.h>


class InterpretedICTest : public ::testing::Test {

    protected:

        void SetUp() override {
            KlassOop objectClass = KlassOop( Universe::find_global( "DoesNotUnderstandFixture" ) );
            fixture = objectClass->klass_part()->allocateObject();

        }


        void TearDown() override {
            fixture = nullptr;
            MarkSweep::collect();

        }


        Oop fixture;

};


TEST_F( InterpretedICTest, noArgSendWithUnknownSelectorShouldInvokeDoesNotUnderstand ) {

    BlockScavenge bs;
    SymbolOop     selector         = oopFactory::new_symbol( "dnuTrigger1", 11 );
    SymbolOop     returnedSelector = oopFactory::new_symbol( "quack", 5 );
    Oop      result        = Delta::call( fixture, selector );
    KlassOop expectedKlass = KlassOop( Universe::find_global( "Message" ) );

    EXPECT_TRUE( result->is_mem() ) << "result should be object";
    EXPECT_EQ( expectedKlass, result->klass() ) << "wrong class returned";
    EXPECT_EQ( fixture, MemOop         ( result ) ->raw_at(2) ) << "message should contain receiver";
    EXPECT_EQ( returnedSelector, MemOop( result ) ->raw_at(3) ) << "message should contain correct selector";
}
