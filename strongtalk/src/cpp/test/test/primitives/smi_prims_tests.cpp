//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/OopFactory.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/primitive/Primitives.hpp"
#include "vm/utility/Integer.hpp"
#include "vm/utility/IntegerOps.hpp"

#include <gtest/gtest.h>


typedef Oop (__CALLING_CONVENTION *smifntype)( SmallIntegerOop, SmallIntegerOop );

extern "C" std::int32_t expansion_count;


class SmiPrimitivesTests : public ::testing::Test {

public:
    SmiPrimitivesTests() :
        ::testing::Test(),
        smiQuo{},
        quoSymbol{} {}


protected:
    void SetUp() override {

        quoSymbol = OopFactory::new_symbol( "primitiveQuo:ifFail:" );
        PrimitiveDescriptor *prim = Primitives::lookup( quoSymbol );
        smiQuo = smifntype( prim->fn() );
    }


    void TearDown() override {

    }


    smifntype smiQuo;
    SymbolOop quoSymbol;

};


TEST_F( SmiPrimitivesTests, quoShouldReturnDivideReceiverByArgument ) {
    ASSERT_EQ( 5, SmallIntegerOop( smiQuo( smiOopFromValue( 2 ), smiOopFromValue( 10 ) ) )->value() );
}


TEST_F( SmiPrimitivesTests, quoShouldReturnReceiverHasWrongTypeWhenNotSMI ) {
    Oop result = smiQuo( smiOopFromValue( 2 ), SmallIntegerOop( quoSymbol ) );
    ASSERT_EQ( (std::int32_t) markSymbol( vmSymbols::receiver_has_wrong_type() ), (std::int32_t) result );
}
