//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
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
#include "vm/primitives/objArray_primitives.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/system/sizes.hpp"


#include <gtest/gtest.h>

extern "C" int expansion_count;


class ObjectArrayPrimitivesTests : public ::testing::Test {

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

    Oop                          result = objArrayPrimitives::allocateSize2( falseObj, smiOopFromValue( 10 ), arrayClass );
    ASSERT_TRUE( result->is_objArray() );
    ASSERT_EQ( 10, ObjectArrayOop( result )->length() );
    ASSERT_EQ( ( const char * ) arrayClassHandle.as_klass(), ( const char * ) result->klass() );
    for ( int                    index  = 10; index > 0; index-- )
        ASSERT_TRUE ( Universe::nilObj() == ( ObjectArrayOop( result )->obj_at( index ) ) );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldAllocateTenuredArrayOfCorrectSize ) {

    HandleMark handles;
    Handle     arrayClassHandle( arrayClass );

    Oop                          result = objArrayPrimitives::allocateSize2( trueObj, smiOopFromValue( 10 ), arrayClass );
    ASSERT_TRUE( result->is_objArray() );
    ASSERT_EQ( 10, ObjectArrayOop( result )->length() );
    ASSERT_TRUE( result->is_old() );
    ASSERT_EQ( ( const char * ) arrayClassHandle.as_klass(), ( const char * ) result->klass() );
    for ( int                    index  = 10; index > 0; index-- )
        ASSERT_TRUE ( Universe::nilObj() == ( ObjectArrayOop( result )->obj_at( index ) ) );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWithNonObjectArray ) {
    Oop result = objArrayPrimitives::allocateSize2( falseObj, smiOopFromValue( 10 ), Universe::find_global( "Object" ) );
    ASSERT_TRUE( result->is_mark() );
    ASSERT_EQ( ( const char * ) markSymbol( vmSymbols::invalid_klass() ), ( const char * ) result );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWithNonInteger ) {
    Oop result = objArrayPrimitives::allocateSize2( falseObj, arrayClass, arrayClass );
    ASSERT_TRUE( result->is_mark() );
    ASSERT_EQ( ( const char * ) markSymbol( vmSymbols::first_argument_has_wrong_type() ), ( const char * ) result );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWithNegativeSize ) {
    Oop result = objArrayPrimitives::allocateSize2( falseObj, smiOopFromValue( -1 ), arrayClass );
    ASSERT_TRUE( result->is_mark() );
    ASSERT_EQ( ( const char * ) markSymbol( vmSymbols::negative_size() ), ( const char * ) result );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWhenTenuredNotBoolean ) {
    Oop result = objArrayPrimitives::allocateSize2( Universe::nilObj(), smiOopFromValue( 10 ), arrayClass );
    ASSERT_TRUE( result->is_mark() );
    ASSERT_EQ( ( const char * ) markSymbol( vmSymbols::second_argument_has_wrong_type() ), ( const char * ) result );
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWhenInsufficientSpace ) {
    int size   = Universe::new_gen.eden()->free() / oopSize;
    Oop result = objArrayPrimitives::allocateSize2( falseObj, smiOopFromValue( size + 1 ), arrayClass );
    ASSERT_TRUE( result->is_mark() );
    EXPECT_EQ( markSymbol( vmSymbols::failed_allocation() ), result ) << unmarkSymbol( result )->as_string();
}


TEST_F( ObjectArrayPrimitivesTests, allocateSize2ShouldFailWhenTooBigForOldGen ) {
    int size   = Universe::old_gen.free() / oopSize;
    Oop result = objArrayPrimitives::allocateSize2( trueObj, smiOopFromValue( size + 1 ), arrayClass );
    ASSERT_TRUE( result->is_mark() );
    EXPECT_EQ( markSymbol( vmSymbols::failed_allocation() ), result ) << unmarkSymbol( result )->as_string();
}
