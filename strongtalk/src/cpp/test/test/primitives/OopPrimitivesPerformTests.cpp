
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Universe.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/primitives/oop_primitives.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"

#include <gtest/gtest.h>


class OopPrimitivesPerformTest : public ::testing::Test {

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


TEST_F( OopPrimitivesPerformTest, noArgPerformWithUnknownShouldInvokeDoesNotUnderstand ) {

    SymbolOop selector = oopFactory::new_symbol( "unknown" );
//    Oop       result        = oopPrimitives::perform( selector, fixture );
//    klassOop  expectedKlass = klassOop( Universe::find_global( "Message" ) );

//    EXPECT_TRUE( result->is_mem() ) << "result should be object";
//    EXPECT_EQ( expectedKlass, result->klass() ) << "wrong class returned";
//    EXPECT_EQ( fixture, MemOop ( result )->raw_at( 2 ) ) << "message should contain receiver";
//    EXPECT_EQ( selector, MemOop( result )->raw_at( 3 ) ) << "message should contain selector";
}


TEST_F( OopPrimitivesPerformTest, oneArgPerformWithUnknownShouldInvokeDoesNotUnderstand ) {

    SymbolOop selector      = oopFactory::new_symbol( "unknown:", 8 );
    SymbolOop arg1          = oopFactory::new_symbol( "arg1", 4 );
    Oop       result        = oopPrimitives::performWith( arg1, selector, fixture );
    KlassOop  expectedKlass = KlassOop( Universe::find_global( "Message" ) );

    EXPECT_TRUE( result->is_mem() ) << "result should be object";
    EXPECT_EQ( expectedKlass, result->klass() ) << "wrong class returned";

    EXPECT_EQ( fixture, MemOop ( result ) ->raw_at( 2 ) ) << "message should contain receiver";
    EXPECT_EQ( selector, MemOop( result ) ->raw_at( 3 ) ) << "message should contain selector";

    Oop                            args = MemOop( result )->raw_at( 4 );
    EXPECT_TRUE( args->is_objArray() ) << "args should be object array";
    EXPECT_EQ( arg1, ObjectArrayOop( args ) ->obj_at( 1 ) ) << "message should contain argument 1";
}


TEST_F( OopPrimitivesPerformTest, twoArgPerformWithUnknownShouldInvokeDoesNotUnderstand ) {
    SymbolOop selector      = oopFactory::new_symbol( "unknown:with:", 13 );
    SymbolOop arg1          = oopFactory::new_symbol( "arg1", 4 );
    SymbolOop arg2          = oopFactory::new_symbol( "arg2", 4 );
    Oop       result        = oopPrimitives::performWithWith( arg2, arg1, selector, fixture );
    KlassOop  expectedKlass = KlassOop( Universe::find_global( "Message" ) );

    EXPECT_TRUE( result->is_mem() ) << "result should be object";
    EXPECT_EQ( expectedKlass, result->klass() ) << "wrong class returned";
    EXPECT_EQ( fixture, MemOop     ( result ) ->raw_at( 2 ) ) << "message should contain receiver";
    EXPECT_EQ( selector, MemOop    ( result ) ->raw_at( 3 ) ) << "message should contain selector";

    Oop                            args = MemOop( result )->raw_at( 4 );
    EXPECT_TRUE( args->is_objArray() ) << "args should be object array";
    EXPECT_EQ( 2, ObjectArrayOop   ( args ) -> length() ) << "wrong number of arguments";
    EXPECT_EQ( arg1, ObjectArrayOop( args ) ->obj_at( 1 ) ) << "message should contain argument 1";
    EXPECT_EQ( arg2, ObjectArrayOop( args ) ->obj_at( 2 ) ) << "message should contain argument 2";
}


TEST_F( OopPrimitivesPerformTest, threeArgPerformWithUnknownShouldInvokeDoesNotUnderstand ) {
    SymbolOop                      selector      = oopFactory::new_symbol( "unknown:with:with:", 18 );
    SymbolOop                      arg1          = oopFactory::new_symbol( "arg1", 4 );
    SymbolOop                      arg2          = oopFactory::new_symbol( "arg2", 4 );
    SymbolOop                      arg3          = oopFactory::new_symbol( "arg3", 4 );
    Oop                            result        = oopPrimitives::performWithWithWith( arg3, arg2, arg1, selector, fixture );
    KlassOop                       expectedKlass = KlassOop( Universe::find_global( "Message" ) );
    EXPECT_TRUE( result->is_mem() ) << "result should be object";
    EXPECT_EQ( expectedKlass, result->klass() ) << "wrong class returned";
    EXPECT_EQ( fixture, MemOop     ( result ) ->raw_at( 2 ) ) << "message should contain receiver";
    EXPECT_EQ( selector, MemOop    ( result ) ->raw_at( 3 ) ) << "message should contain selector";
    Oop                            args          = MemOop( result )->raw_at( 4 );
    EXPECT_TRUE( args->is_objArray() ) << "args should be object array";
    EXPECT_EQ( 3, ObjectArrayOop   ( args ) -> length() ) << "wrong number of arguments";
    EXPECT_EQ( arg1, ObjectArrayOop( args ) ->obj_at( 1 ) ) << "message should contain argument 1";
    EXPECT_EQ( arg2, ObjectArrayOop( args ) ->obj_at( 2 ) ) << "message should contain argument 2";
    EXPECT_EQ( arg3, ObjectArrayOop( args ) ->obj_at( 3 ) ) << "message should contain argument 3";
}


TEST_F( OopPrimitivesPerformTest, varArgPerformWithUnknownShouldInvokeDoesNotUnderstand ) {
    SymbolOop      selector  = oopFactory::new_symbol( "unknown:with:with:with:", 23 );
    SymbolOop      arg1      = oopFactory::new_symbol( "arg1", 4 );
    SymbolOop      arg2      = oopFactory::new_symbol( "arg2", 4 );
    SymbolOop      arg3      = oopFactory::new_symbol( "arg3", 4 );
    SymbolOop      arg4      = oopFactory::new_symbol( "arg4", 4 );
    ObjectArrayOop inputArgs = oopFactory::new_objArray( 4 );
    inputArgs->obj_at_put( 1, arg1 );
    inputArgs->obj_at_put( 2, arg2 );
    inputArgs->obj_at_put( 3, arg3 );
    inputArgs->obj_at_put( 4, arg4 );
    Oop      result        = oopPrimitives::performArguments( inputArgs, selector, fixture );
    KlassOop expectedKlass = KlassOop( Universe::find_global( "Message" ) );

    EXPECT_TRUE( result->is_mem() ) << "result should be object";
    EXPECT_EQ( expectedKlass, result->klass() ) << "wrong class returned";
    EXPECT_EQ( fixture, MemOop     ( result ) ->raw_at( 2 ) ) << "message should contain receiver";
    EXPECT_EQ( selector, MemOop    ( result ) ->raw_at( 3 ) ) << "message should contain selector";

    Oop args = MemOop( result )->raw_at( 4 );

    EXPECT_TRUE( args->is_objArray() ) << "args should be object array";
    EXPECT_EQ( 4, ObjectArrayOop   ( args ) -> length() ) << "wrong number of arguments";
    EXPECT_EQ( arg1, ObjectArrayOop( args ) ->obj_at( 1 ) ) << "message should contain argument 1";
    EXPECT_EQ( arg2, ObjectArrayOop( args ) ->obj_at( 2 ) ) << "message should contain argument 2";
    EXPECT_EQ( arg3, ObjectArrayOop( args ) ->obj_at( 3 ) ) << "message should contain argument 3";
    EXPECT_EQ( arg4, ObjectArrayOop( args ) ->obj_at( 4 ) ) << "message should contain argument 4";
}
