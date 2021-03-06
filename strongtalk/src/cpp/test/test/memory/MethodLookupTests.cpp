//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/memory/Scavenge.hpp"

#include <gtest/gtest.h>


class MethodLookupTests : public ::testing::Test {

public:
    MethodLookupTests() :
        ::testing::Test(),
        fixture{ nullptr } {}


protected:
    void SetUp() override {
        fixture = new PersistentHandle( Universe::find_global( "DoesNotUnderstandFixture" ) );
    }


    void TearDown() override {
        delete fixture;
    }


    PersistentHandle *fixture;

};


TEST_F( MethodLookupTests, lookupShouldAddDNUInvokerWhenNoMatch ) {
    BlockScavenge bs;
    SymbolOop     selector = OopFactory::new_symbol( "unknownSelector" );
    MethodOop     method   = fixture->as_klassOop()->klass_part()->lookup( selector );
    EXPECT_TRUE( method != nullptr ) << "Should find method";
    EXPECT_TRUE( selector == method->selector() ) << "Wrong selector";
    method->pretty_print();
}


TEST_F( MethodLookupTests, lookupShouldAddDNUInvokerForOneArgSelector ) {
    BlockScavenge bs;
    SymbolOop     selector = OopFactory::new_symbol( "unknownSelector:" );
    MethodOop     method   = fixture->as_klassOop()->klass_part()->lookup( selector );
    EXPECT_TRUE( method != nullptr ) << "Should find method";
    EXPECT_TRUE( selector == method->selector() ) << "Wrong selector";
    method->pretty_print();
}
