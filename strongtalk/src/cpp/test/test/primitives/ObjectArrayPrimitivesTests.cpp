//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/assembler/Displacement.hpp"
#include "vm/memory/Array.hpp"
#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/compiler/BasicBlockIterator.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/primitives/ObjectArrayPrimitives.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/OopFactory.hpp"


#include <gtest/gtest.h>

extern "C" std::int32_t expansion_count;


class ObjectArrayPrimitivesTests : public ::testing::Test {

public:
    ObjectArrayPrimitivesTests()
        : ::testing::Test(), arrayClass{} {}

protected:
    void SetUp() override {
        arrayClass = KlassOop( Universe::find_global( "Array" ) );
    }


    void TearDown() override {
        MarkSweep::collect();
    }

    KlassOop arrayClass;

};

TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldAllocateArrayOfCorrectSize ) {

    HandleMark handles;
    Handle     arrayClassHandle( arrayClass );
    Oop        result = ObjectArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( 10 ), arrayClass );

    ASSERT_TRUE( result->isObjectArray() );
    ASSERT_EQ( 10, ObjectArrayOop( result ) -> length() );
    ASSERT_EQ( (const char *) arrayClassHandle.as_klass(), (const char *) result->klass() );

    for ( std::int32_t index = 10; index > 0; index-- )ASSERT_TRUE ( Universe::nilObject() == ( ObjectArrayOop( result )->obj_at( index ) ) );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldAllocateTenuredArrayOfCorrectSize ) {

    HandleMark handles;
    Handle     arrayClassHandle( arrayClass );
    Oop        result = ObjectArrayPrimitives::allocateSize2( trueObject, smiOopFromValue( 10 ), arrayClass );

    ASSERT_TRUE( result->isObjectArray() );
    ASSERT_EQ( 10, ObjectArrayOop( result ) -> length() );
    ASSERT_TRUE( result->is_old() );
    ASSERT_EQ( (const char *) arrayClassHandle.as_klass(), (const char *) result->klass() );

    for ( std::int32_t index = 10; index > 0; index-- )ASSERT_TRUE ( Universe::nilObject() == ( ObjectArrayOop( result )->obj_at( index ) ) );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWithNonObjectArray ) {
    Oop result = ObjectArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( 10 ), Universe::find_global( "Object" ) );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (const char *) markSymbol( vmSymbols::invalid_klass() ), (const char *) result );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWithNonInteger ) {
    Oop result = ObjectArrayPrimitives::allocateSize2( falseObject, arrayClass, arrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (const char *) markSymbol( vmSymbols::first_argument_has_wrong_type() ), (const char *) result );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWithNegativeSize ) {
    Oop result = ObjectArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( -1 ), arrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (const char *) markSymbol( vmSymbols::negative_size() ), (const char *) result );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWhenTenuredNotBoolean ) {
    Oop result = ObjectArrayPrimitives::allocateSize2( Universe::nilObject(), smiOopFromValue( 10 ), arrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    ASSERT_EQ( (const char *) markSymbol( vmSymbols::second_argument_has_wrong_type() ), (const char *) result );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWhenInsufficientSpace ) {
    std::int32_t size   = Universe::new_gen.eden()->free() / OOP_SIZE;
    Oop          result = ObjectArrayPrimitives::allocateSize2( falseObject, smiOopFromValue( size + 1 ), arrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    EXPECT_EQ( markSymbol( vmSymbols::failed_allocation() ), result ) << unmarkSymbol( result )->as_string();
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWhenTooBigForOldGen ) {
    std::int32_t size   = Universe::old_gen.free() / OOP_SIZE;
    Oop          result = ObjectArrayPrimitives::allocateSize2( trueObject, smiOopFromValue( size + 1 ), arrayClass );
    ASSERT_TRUE( result->isMarkOop() );
    EXPECT_EQ( markSymbol( vmSymbols::failed_allocation() ), result ) << unmarkSymbol( result )->as_string();
}
