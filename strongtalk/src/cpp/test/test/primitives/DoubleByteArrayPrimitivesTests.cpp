
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/assembler/Displacement.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/primitive/DoubleByteArray_primitives.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/OopFactory.hpp"

#include <gtest/gtest.h>

extern "C" std::int32_t expansion_count;


class DoubleByteArrayPrimitivesTests : public ::testing::Test {

public:
    DoubleByteArrayPrimitivesTests() :
    ::testing::Test(),
    dByteArrayClass{}
    {}

protected:
    void SetUp() override {
        dByteArrayClass = KlassOop( Universe::find_global( "String" ) );
    }


    void TearDown() override {
        MarkSweep::collect();
    }


    KlassOop dByteArrayClass;


};

TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldAllocateDByteArrayOfCorrectSize ) {

    HandleMark handles;
    Handle     dByteArrayClassHandle( dByteArrayClass );
    Oop        result = DoubleByteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( 10 ), dByteArrayClass );

    ASSERT_TRUE( result->isDoubleByteArray() );
    ASSERT_TRUE( !Universe::old_gen.contains( result ) );
    ASSERT_TRUE( Universe::new_gen.contains( result ) );

    ASSERT_EQ( 10, DoubleByteArrayOop(result) -> length() );
    ASSERT_EQ( (const char *) dByteArrayClassHandle.as_klass(), (const char *) result->klass() );

    for ( std::int32_t index = 10; index > 0; index-- ) {
        ASSERT_EQ( std::uint16_t( 0 ), DoubleByteArrayOop( result ) -> doubleByte_at(index) );
    }

}


TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldAllocateTenuredWhenRequested ) {

    HandleMark handles;
    Handle     classHandle( dByteArrayClass );
    Oop        result = DoubleByteArrayPrimitives::allocateSize2( trueObject, smiOopFromValue( 10 ), dByteArrayClass );

    ASSERT_TRUE( result->isDoubleByteArray() );
    ASSERT_TRUE( Universe::old_gen.contains( result ) );
    ASSERT_EQ( 10, DoubleByteArrayOop(result) -> length() );
    ASSERT_EQ( (const char *) classHandle.as_klass(), (const char *) result->klass() );
}


TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldFailWithNonDoubleByteArray ) {
    Oop result = DoubleByteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( 10 ), Universe::find_global( "Object" ) );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (std::int32_t) markSymbol( vmSymbols::invalid_klass() ), (std::int32_t) result );
}


TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldFailWithNonKlass ) {
    Oop result = DoubleByteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( 10 ), Universe::trueObject() );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (std::int32_t) markSymbol( vmSymbols::invalid_klass() ), (std::int32_t) result );
}


TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldFailWithNonInteger ) {
    Oop result = DoubleByteArrayPrimitives::allocateSize2( falseObject, dByteArrayClass, dByteArrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (const char *) markSymbol( vmSymbols::first_argument_has_wrong_type() ), (const char *) result );
}


TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldFailWithNegativeSize ) {
    Oop result = DoubleByteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( -1 ), dByteArrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (const char *) markSymbol( vmSymbols::negative_size() ), (const char *) result );
}


TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldFailWhenTenuredNotBoolean ) {
    Oop result = DoubleByteArrayPrimitives::allocateSize2( Universe::nilObject(), smiOopFromValue( 10 ), dByteArrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (const char *) markSymbol( vmSymbols::second_argument_has_wrong_type() ), (const char *) result );
}


TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldFailWhenInsufficientSpace ) {
    std::int32_t size   = Universe::new_gen.eden()->free();
    Oop          result = DoubleByteArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( size + 1 ), dByteArrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    EXPECT_EQ( markSymbol( vmSymbols::failed_allocation() ), result ) << unmarkSymbol( result )->as_string();
}


TEST_F( DoubleByteArrayPrimitivesTests, allocateSize2ShouldFailWhenTooBigForOldGen ) {
    std::int32_t size   = Universe::old_gen.free();
    Oop          result = DoubleByteArrayPrimitives::allocateSize2( trueObject, smiOopFromValue( size + 1 ), dByteArrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    EXPECT_EQ( markSymbol( vmSymbols::failed_allocation() ), result ) << unmarkSymbol( result )->as_string();
}
