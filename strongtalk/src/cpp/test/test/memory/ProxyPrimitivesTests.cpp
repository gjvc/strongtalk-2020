//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/oop/ProxyOopDescriptor.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/primitive/ProxyOopPrimitives.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/runtime/VMSymbol.hpp"

#include <gtest/gtest.h>


extern "C" {
extern std::int32_t expansion_count;
}


class ProxyPrimitivesTests : public ::testing::Test {

public:
    ProxyPrimitivesTests() :
        ::testing::Test(),
        rm{ nullptr },
        proxy{},
        subProxy{},
        validProxy{},
        doubleValue{},
        smi0{},
        smi1{},
        address{} {}


protected:

    HeapResourceMark *rm;
    ProxyOop         proxy, subProxy, validProxy;
    DoubleOop        doubleValue;
    SmallIntegerOop  smi0, smi1;
    std::int32_t     address;


    void SetUp() override {
        rm = new HeapResourceMark();
        PersistentHandle proxyClass( Universe::find_global( "ExternalProxy" ) );
        PersistentHandle proxyHandle( proxyClass.as_klassOop()->klass_part()->allocateObject() );
        PersistentHandle subProxyHandle( proxyClass.as_klassOop()->klass_part()->allocateObject() );
        PersistentHandle validProxyHandle( proxyClass.as_klassOop()->klass_part()->allocateObject() );

        doubleValue = OopFactory::new_double( 1.2345 );

        smi0    = smiOopFromValue( 0 );
        smi1    = smiOopFromValue( 1 );
        address = 0;

        proxy      = ProxyOop( proxyHandle.as_oop() );
        subProxy   = ProxyOop( subProxyHandle.as_oop() );
        validProxy = ProxyOop( validProxyHandle.as_oop() );
        validProxy->set_pointer( &address );
    }


    void TearDown() override {
        delete rm;
        rm = nullptr;
    }


    void checkMarkedSymbol( const char *message, Oop result, SymbolOop expected ) {
        char text[200];
        EXPECT_TRUE( result->isMarkOop() ) << "Should be marked";
        sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
        EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
    }


};


TEST_F( ProxyPrimitivesTests, smiAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::smiAtPut( smi1, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, smiAtShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::smiAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, byteAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::byteAtPut( smi1, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, byteAtShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::byteAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, doubleByteAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::doubleByteAtPut( smi1, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, doubleByteAtShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::doubleByteAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, subProxyAtShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::subProxyAt( subProxy, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
    EXPECT_TRUE( subProxy->is_null() ) << "subproxy should have null pointer";
}


TEST_F( ProxyPrimitivesTests, proxyAtShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::proxyAt( subProxy, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
    EXPECT_TRUE( subProxy->is_null() ) << "subproxy should have null pointer";
}


TEST_F( ProxyPrimitivesTests, proxyAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::proxyAtPut( subProxy, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
    EXPECT_TRUE( subProxy->is_null() ) << "subproxy should have null pointer";
}


TEST_F( ProxyPrimitivesTests, proxyAtPutShouldFailWhenValuePointerIsNULL ) {
    std::int32_t addr;
    proxy->set_pointer( &addr );
    addr = 1;
//    Oop result = ProxyOopPrimitives::proxyAtPut( subProxy, smi0, proxy );
    EXPECT_EQ( 0, addr ) << "Should overwrite value";
}


TEST_F( ProxyPrimitivesTests, singlePrecisionFloatAtShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::singlePrecisionFloatAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, singlePrecisionFloatAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::singlePrecisionFloatAtPut( doubleValue, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, doublePrecisionFloatAtShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::doublePrecisionFloatAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, doublePrecisionFloatAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::doublePrecisionFloatAtPut( doubleValue, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout0ShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::callOut0( subProxy, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout1ShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::callOut1( subProxy, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout2ShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::callOut2( subProxy, smi0, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout3ShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::callOut3( subProxy, smi0, smi0, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout4ShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::callOut4( subProxy, smi0, smi0, smi0, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout5ShouldFailWhenPointerIsNULL ) {
    Oop result = ProxyOopPrimitives::callOut5( subProxy, smi0, smi0, smi0, smi0, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout5ShouldFailWhenReceiverNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut5( subProxy, smi0, smi0, smi0, smi0, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout4ShouldFailWhenReceiverNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut4( subProxy, smi0, smi0, smi0, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout3ShouldFailWhenReceiverNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut3( subProxy, smi0, smi0, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout2ShouldFailWhenReceiverNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut2( subProxy, smi0, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout1ShouldFailWhenReceiverNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut1( subProxy, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout0ShouldFailWhenReceiverNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut0( subProxy, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout3ShouldFailWhenResultNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut3( smi0, smi0, smi0, smi0, validProxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::fourth_argument_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout4ShouldFailWhenResultNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut4( smi0, smi0, smi0, smi0, smi0, validProxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::fifth_argument_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout5ShouldFailWhenResultNotProxy ) {
    Oop result = ProxyOopPrimitives::callOut5( smi0, smi0, smi0, smi0, smi0, smi0, validProxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::sixth_argument_has_wrong_type() );
}
