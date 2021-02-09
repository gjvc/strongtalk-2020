//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"


class ProxyOopPrimitives : AllStatic {
private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static std::int32_t number_of_calls;

    // Conversion

    //%prim
    // <Proxy> primitiveProxyGetIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { error = #(ConversionFailed)
    //              name  = 'ProxyOopPrimitives::getSmi' }
    //%
    static PRIM_DECL_1( getSmi, Oop receiver );

    //%prim
    // <Proxy> primitiveProxySet: value     <SmallInteger|Proxy>
    //                    ifFail: failBlock <PrimFailBlock> ^<Self> =
    //   Internal { name = 'ProxyOopPrimitives::set' }
    //%
    static PRIM_DECL_2( set, Oop receiver, Oop value );

    //%prim
    // <Proxy> primitiveProxySetHigh: high      <SmallInteger>
    //                           low: low       <SmallInteger>
    //                        ifFail: failBlock <PrimFailBlock> ^<Self> =
    //   Internal { name = 'ProxyOopPrimitives::setHighLow' }
    //%
    static PRIM_DECL_3( setHighLow, Oop receiver, Oop high, Oop low );

    //%prim
    // <Proxy> primitiveProxyGetHigh ^<SmallInteger> =
    //   Internal { name = 'ProxyOopPrimitives::getHigh' }
    //%
    static PRIM_DECL_1( getHigh, Oop receiver );

    //%prim
    // <Proxy> primitiveProxyGetLow ^<SmallInteger> =
    //   Internal { name = 'ProxyOopPrimitives::getLow' }
    //%
    static PRIM_DECL_1( getLow, Oop receiver );

    // Testing

    //%prim
    // <Proxy> primitiveProxyIsNull ^<Boolean> =
    //   Internal { name = 'ProxyOopPrimitives::isNull' }
    //%
    static PRIM_DECL_1( isNull, Oop receiver );

    //%prim
    // <Proxy> primitiveProxyIsAllOnes ^<Boolean> =
    //   Internal { name = 'ProxyOopPrimitives::isAllOnes' }
    //%
    static PRIM_DECL_1( isAllOnes, Oop receiver );

    // Memory management

    //%prim
    // <Proxy> primitiveProxyMalloc: size      <SmallInteger>
    //                       ifFail: failBlock <PrimFailBlock> ^<Self> =
    //   Internal { name = 'ProxyOopPrimitives::malloc' }
    //%
    static PRIM_DECL_2( malloc, Oop receiver, Oop size );

    //%prim
    // <Proxy> primitiveProxyCalloc: size      <SmallInteger>
    //                       ifFail: failBlock <PrimFailBlock> ^<Self> =
    //   Internal { name = 'ProxyOopPrimitives::calloc' }
    //%
    static PRIM_DECL_2( calloc, Oop receiver, Oop size );

    //%prim
    // <Proxy> primitiveProxyFree ^<Self> =
    //   Internal { name = 'ProxyOopPrimitives::free' }
    //%
    static PRIM_DECL_1( free, Oop receiver );

    // The remaining primitives are used for
    // dereferencing the proxy value.

    // Byte

    //%prim
    // <Proxy> primitiveProxyByteAt: offset    <SmallInteger>
    //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { flags = #(LastDeltaFrameNotNeeded)
    //              name = 'ProxyOopPrimitives::byteAt' }
    //%
    static PRIM_DECL_2( byteAt, Oop receiver, Oop offset );

    //%prim
    // <Proxy> primitiveProxyByteAt: offset    <SmallInteger>
    //                          put: value     <SmallInteger>
    //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { flags = #(LastDeltaFrameNotNeeded)
    //              name  = 'ProxyOopPrimitives::byteAtPut' }
    //%
    static PRIM_DECL_3( byteAtPut, Oop receiver, Oop offset, Oop value );

    // Double byte

    //%prim
    // <Proxy> primitiveProxyDoubleByteAt: offset   <SmallInteger>
    //                            ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { name = 'ProxyOopPrimitives::doubleByteAt' }
    //%
    static PRIM_DECL_2( doubleByteAt, Oop receiver, Oop offset );

    //%prim
    // <Proxy> primitiveProxyDoubleByteAt: offset    <SmallInteger>
    //                                put: value     <SmallInteger>
    //                             ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { name = 'ProxyOopPrimitives::doubleByteAtPut' }
    //%
    static PRIM_DECL_3( doubleByteAtPut, Oop receiver, Oop offset, Oop value );

    // Smi

    //%prim
    // <Proxy> primitiveProxySmiAt: offset    <SmallInteger>
    //                      ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { error = #(ConversionFailed)
    //              name  = 'ProxyOopPrimitives::smiAt' }
    //%
    static PRIM_DECL_2( smiAt, Oop receiver, Oop offset );

    //%prim
    // <Proxy> primitiveProxySmiAt: offset    <SmallInteger>
    //                         put: value     <SmallInteger>
    //                      ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
    //   Internal { name = 'ProxyOopPrimitives::smiAtPut' }
    //%
    static PRIM_DECL_3( smiAtPut, Oop receiver, Oop offset, Oop value );

    // Proxy

    //%prim
    // <Proxy> primitiveProxySubProxyAt: offset    <SmallInteger>
    //                           result: result    <Proxy>
    //                           ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { name = 'ProxyOopPrimitives::subProxyAt' }
    //%
    static PRIM_DECL_3( subProxyAt, Oop receiver, Oop offset, Oop result );

    //%prim
    // <Proxy> primitiveProxyProxyAt: offset    <SmallInteger>
    //                        result: result    <Proxy>
    //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { name = 'ProxyOopPrimitives::proxyAt' }
    //%
    static PRIM_DECL_3( proxyAt, Oop receiver, Oop offset, Oop result );

    //%prim
    // <Proxy> primitiveProxyProxyAt: offset    <SmallInteger>
    //                           put: value     <Proxy>
    //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { name = 'ProxyOopPrimitives::proxyAtPut' }
    //%
    static PRIM_DECL_3( proxyAtPut, Oop receiver, Oop offset, Oop value );

    // Single precision floats

    //%prim
    // <Proxy> primitiveProxySinglePrecisionFloatAt: offset    <SmallInteger>
    //                                       ifFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { name  = 'ProxyOopPrimitives::singlePrecisionFloatAt' }
    //%
    static PRIM_DECL_2( singlePrecisionFloatAt, Oop receiver, Oop offset );

    //%prim
    // <Proxy> primitiveProxySinglePrecisionFloatAt: offset    <SmallInteger>
    //                                          put: value     <Float>
    //                                       ifFail: failBlock <PrimFailBlock> ^<Self> =
    //   Internal { error = #(ConversionFailed)
    //              name = 'ProxyOopPrimitives::singlePrecisionFloatAtPut' }
    //%
    static PRIM_DECL_3( singlePrecisionFloatAtPut, Oop receiver, Oop offset, Oop value );

    // Double precision floats

    //%prim
    // <Proxy> primitiveProxyDoublePrecisionFloatAt: offset    <SmallInteger>
    //                                       ifFail: failBlock <PrimFailBlock> ^<Float> =
    //   Internal { name  = 'ProxyOopPrimitives::doublePrecisionFloatAt' }
    //%
    static PRIM_DECL_2( doublePrecisionFloatAt, Oop receiver, Oop offset );

    //%prim
    // <Proxy> primitiveProxyDoublePrecisionFloatAt: offset    <SmallInteger>
    //                                          put: value     <Float>
    //                                       ifFail: failBlock <PrimFailBlock> ^<Self> =
    //   Internal { error = #(ConversionFailed)
    //              name = 'ProxyOopPrimitives::doublePrecisionFloatAtPut' }
    //%
    static PRIM_DECL_3( doublePrecisionFloatAtPut, Oop receiver, Oop offset, Oop value );

    // API Calls through proxies

    //%prim
    // <Proxy> primitiveAPICallResult: proxy <Proxy>
    //                         ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { flags = #NonLocalReturn
    //              name  = 'ProxyOopPrimitives::callOut0' }
    //%
    static PRIM_DECL_2( callOut0, Oop receiver, Oop result );

    //%prim
    // <Proxy> primitiveAPICallValue: arg1  <Proxy|SmallInteger>
    //                        result: proxy <Proxy>
    //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { flags = #NonLocalReturn
    //              name  = 'ProxyOopPrimitives::callOut1' }
    //%
    static PRIM_DECL_3( callOut1, Oop receiver, Oop arg1, Oop result );

    //%prim
    // <Proxy> primitiveAPICallValue: arg1      <Proxy|SmallInteger>
    //                         value: arg2      <Proxy|SmallInteger>
    //                        result: proxy     <Proxy>
    //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { flags = #NonLocalReturn
    //              name  = 'ProxyOopPrimitives::callOut2' }
    //%
    static PRIM_DECL_4( callOut2, Oop receiver, Oop arg1, Oop arg2, Oop result );

    //%prim
    // <Proxy> primitiveAPICallValue: arg1      <Proxy|SmallInteger>
    //                         value: arg2      <Proxy|SmallInteger>
    //                         value: arg3      <Proxy|SmallInteger>
    //                        result: proxy     <Proxy>
    //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { flags = #NonLocalReturn
    //              name  = 'ProxyOopPrimitives::callOut3' }
    //%
    static PRIM_DECL_5( callOut3, Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop result );

    //%prim
    // <Proxy> primitiveAPICallValue: arg1      <Proxy|SmallInteger>
    //                         value: arg2      <Proxy|SmallInteger>
    //                         value: arg3      <Proxy|SmallInteger>
    //                         value: arg4      <Proxy|SmallInteger>
    //                        result: proxy     <Proxy>
    //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { flags = #NonLocalReturn
    //              name  = 'ProxyOopPrimitives::callOut4' }
    //%
    static PRIM_DECL_6( callOut4, Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop result );

    //%prim
    // <Proxy> primitiveAPICallValue: arg1      <Proxy|SmallInteger>
    //                         value: arg2      <Proxy|SmallInteger>
    //                         value: arg3      <Proxy|SmallInteger>
    //                         value: arg4      <Proxy|SmallInteger>
    //                         value: arg5      <Proxy|SmallInteger>
    //                        result: proxy     <Proxy>
    //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
    //   Internal { flags = #NonLocalReturn
    //              name  = 'ProxyOopPrimitives::callOut5' }
    //%
    static PRIM_DECL_7( callOut5, Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop result );


};
