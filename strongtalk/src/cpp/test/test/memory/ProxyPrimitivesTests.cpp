//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Handle.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/primitives/proxy_primitives.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/memory/vmSymbols.hpp"

#include <gtest/gtest.h>


extern "C" {
extern int expansion_count;
}


class ProxyPrimitivesTests : public ::testing::Test {

    protected:

        HeapResourceMark * rm;
        ProxyOop  proxy, subProxy, validProxy;
        DoubleOop doubleValue;
        SMIOop    smi0, smi1;
        int       address;


        void SetUp() override {
            rm = new HeapResourceMark();
            PersistentHandle proxyClass( Universe::find_global( "ExternalProxy" ) );
            PersistentHandle proxyHandle( proxyClass.as_klassOop()->klass_part()->allocateObject() );
            PersistentHandle subProxyHandle( proxyClass.as_klassOop()->klass_part()->allocateObject() );
            PersistentHandle validProxyHandle( proxyClass.as_klassOop()->klass_part()->allocateObject() );

            doubleValue = oopFactory::new_double( 1.2345 );

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


        void checkMarkedSymbol( const char * message, Oop result, SymbolOop expected ) {
            char text[200];
            EXPECT_TRUE( result->is_mark() ) << "Should be marked";
            sprintf( text, "Should be: %s, was: %s", message, unmarkSymbol( result )->as_string() );
            EXPECT_TRUE( unmarkSymbol( result ) == expected ) << text;
        }


};


TEST_F( ProxyPrimitivesTests, smiAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::smiAtPut( smi1, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, smiAtShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::smiAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, byteAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::byteAtPut( smi1, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, byteAtShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::byteAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, doubleByteAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::doubleByteAtPut( smi1, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, doubleByteAtShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::doubleByteAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, subProxyAtShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::subProxyAt( subProxy, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
    EXPECT_TRUE( subProxy->is_null() ) << "subproxy should have null pointer";
}


TEST_F( ProxyPrimitivesTests, proxyAtShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::proxyAt( subProxy, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
    EXPECT_TRUE( subProxy->is_null() ) << "subproxy should have null pointer";
}


TEST_F( ProxyPrimitivesTests, proxyAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::proxyAtPut( subProxy, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
    EXPECT_TRUE( subProxy->is_null() ) << "subproxy should have null pointer";
}


TEST_F( ProxyPrimitivesTests, proxyAtPutShouldFailWhenValuePointerIsNULL ) {
    int addr;
    proxy->set_pointer( &addr );
    addr = 1;
    Oop result = proxyOopPrimitives::proxyAtPut( subProxy, smi0, proxy );
    EXPECT_EQ( 0, addr ) << "Should overwrite value";
}


TEST_F( ProxyPrimitivesTests, singlePrecisionFloatAtShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::singlePrecisionFloatAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, singlePrecisionFloatAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::singlePrecisionFloatAtPut( doubleValue, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, doublePrecisionFloatAtShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::doublePrecisionFloatAt( smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, doublePrecisionFloatAtPutShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::doublePrecisionFloatAtPut( doubleValue, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout0ShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::callOut0( subProxy, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout1ShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::callOut1( subProxy, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout2ShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::callOut2( subProxy, smi0, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout3ShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::callOut3( subProxy, smi0, smi0, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout4ShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::callOut4( subProxy, smi0, smi0, smi0, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout5ShouldFailWhenPointerIsNULL ) {
    Oop result = proxyOopPrimitives::callOut5( subProxy, smi0, smi0, smi0, smi0, smi0, proxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::illegal_state() );
}


TEST_F( ProxyPrimitivesTests, callout5ShouldFailWhenReceiverNotProxy ) {
    Oop result = proxyOopPrimitives::callOut5( subProxy, smi0, smi0, smi0, smi0, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout4ShouldFailWhenReceiverNotProxy ) {
    Oop result = proxyOopPrimitives::callOut4( subProxy, smi0, smi0, smi0, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout3ShouldFailWhenReceiverNotProxy ) {
    Oop result = proxyOopPrimitives::callOut3( subProxy, smi0, smi0, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout2ShouldFailWhenReceiverNotProxy ) {
    Oop result = proxyOopPrimitives::callOut2( subProxy, smi0, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout1ShouldFailWhenReceiverNotProxy ) {
    Oop result = proxyOopPrimitives::callOut1( subProxy, smi0, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout0ShouldFailWhenReceiverNotProxy ) {
    Oop result = proxyOopPrimitives::callOut0( subProxy, smi0 );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::receiver_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout3ShouldFailWhenResultNotProxy ) {
    Oop result = proxyOopPrimitives::callOut3( smi0, smi0, smi0, smi0, validProxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::fourth_argument_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout4ShouldFailWhenResultNotProxy ) {
    Oop result = proxyOopPrimitives::callOut4( smi0, smi0, smi0, smi0, smi0, validProxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::fifth_argument_has_wrong_type() );
}


TEST_F( ProxyPrimitivesTests, callout5ShouldFailWhenResultNotProxy ) {
    Oop result = proxyOopPrimitives::callOut5( smi0, smi0, smi0, smi0, smi0, smi0, validProxy );
    checkMarkedSymbol( "receiver invalid", result, vmSymbols::sixth_argument_has_wrong_type() );
}
