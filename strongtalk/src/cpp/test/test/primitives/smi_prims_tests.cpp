//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/oopFactory.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/utilities/IntegerOps.hpp"

#include <gtest/gtest.h>

typedef Oop (__CALLING_CONVENTION *smifntype)( SMIOop, SMIOop );

extern "C" std::int32_t expansion_count;


class SmiPrimitivessTests : public ::testing::Test {

protected:
    void SetUp() override {

        quoSymbol = oopFactory::new_symbol( "primitiveQuo:ifFail:" );
        PrimitiveDescriptor *prim = Primitives::lookup( quoSymbol );
        smiQuo = smifntype( prim->fn() );
    }


    void TearDown() override {

    }


    smifntype smiQuo;
    SymbolOop quoSymbol;

};


TEST_F( SmiPrimitivessTests, quoShouldReturnDivideReceiverByArgument
) {
ASSERT_EQ( 5,
SMIOop( smiQuo( smiOopFromValue( 2 ), smiOopFromValue( 10 ) )
)->
value()
);
}


TEST_F( SmiPrimitivessTests, quoShouldReturnReceiverHasWrongTypeWhenNotSMI
) {
Oop result = smiQuo( smiOopFromValue( 2 ), SMIOop( quoSymbol ) );
ASSERT_EQ( ( std::int32_t )
markSymbol( vmSymbols::receiver_has_wrong_type() ),
( std::int32_t ) result );
}
