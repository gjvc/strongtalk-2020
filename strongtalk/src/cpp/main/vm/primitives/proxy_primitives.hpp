//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for proxy

class proxyOopPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        // Conversion

        //%prim
        // <Proxy> primitiveProxyGetIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(ConversionFailed)
        //              name  = 'proxyOopPrimitives::getSmi' }
        //%
        static Oop __CALLING_CONVENTION getSmi( Oop receiver );

        //%prim
        // <Proxy> primitiveProxySet: value     <SmallInteger|Proxy>
        //                    ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { name = 'proxyOopPrimitives::set' }
        //%
        static Oop __CALLING_CONVENTION set( Oop receiver, Oop value );

        //%prim
        // <Proxy> primitiveProxySetHigh: high      <SmallInteger>
        //                           low: low       <SmallInteger>
        //                        ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { name = 'proxyOopPrimitives::setHighLow' }
        //%
        static Oop __CALLING_CONVENTION setHighLow( Oop receiver, Oop high, Oop low );

        //%prim
        // <Proxy> primitiveProxyGetHigh ^<SmallInteger> =
        //   Internal { name = 'proxyOopPrimitives::getHigh' }
        //%
        static Oop __CALLING_CONVENTION getHigh( Oop receiver );

        //%prim
        // <Proxy> primitiveProxyGetLow ^<SmallInteger> =
        //   Internal { name = 'proxyOopPrimitives::getLow' }
        //%
        static Oop __CALLING_CONVENTION getLow( Oop receiver );

        // Testing

        //%prim
        // <Proxy> primitiveProxyIsNull ^<Boolean> =
        //   Internal { name = 'proxyOopPrimitives::isNull' }
        //%
        static Oop __CALLING_CONVENTION isNull( Oop receiver );

        //%prim
        // <Proxy> primitiveProxyIsAllOnes ^<Boolean> =
        //   Internal { name = 'proxyOopPrimitives::isAllOnes' }
        //%
        static Oop __CALLING_CONVENTION isAllOnes( Oop receiver );

        // Memory management

        //%prim
        // <Proxy> primitiveProxyMalloc: size      <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { name = 'proxyOopPrimitives::malloc' }
        //%
        static Oop __CALLING_CONVENTION malloc( Oop receiver, Oop size );

        //%prim
        // <Proxy> primitiveProxyCalloc: size      <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { name = 'proxyOopPrimitives::calloc' }
        //%
        static Oop __CALLING_CONVENTION calloc( Oop receiver, Oop size );

        //%prim
        // <Proxy> primitiveProxyFree ^<Self> =
        //   Internal { name = 'proxyOopPrimitives::free' }
        //%
        static Oop __CALLING_CONVENTION free( Oop receiver );

        // The remaining primitives are used for
        // dereferencing the proxy value.

        // Byte

        //%prim
        // <Proxy> primitiveProxyByteAt: offset    <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(LastDeltaFrameNotNeeded)
        //              name = 'proxyOopPrimitives::byteAt' }
        //%
        static Oop __CALLING_CONVENTION byteAt( Oop receiver, Oop offset );

        //%prim
        // <Proxy> primitiveProxyByteAt: offset    <SmallInteger>
        //                          put: value     <SmallInteger>
        //                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags = #(LastDeltaFrameNotNeeded)
        //              name  = 'proxyOopPrimitives::byteAtPut' }
        //%
        static Oop __CALLING_CONVENTION byteAtPut( Oop receiver, Oop offset, Oop value );

        // Double byte

        //%prim
        // <Proxy> primitiveProxyDoubleByteAt: offset   <SmallInteger>
        //                            ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { name = 'proxyOopPrimitives::doubleByteAt' }
        //%
        static Oop __CALLING_CONVENTION doubleByteAt( Oop receiver, Oop offset );

        //%prim
        // <Proxy> primitiveProxyDoubleByteAt: offset    <SmallInteger>
        //                                put: value     <SmallInteger>
        //                             ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { name = 'proxyOopPrimitives::doubleByteAtPut' }
        //%
        static Oop __CALLING_CONVENTION doubleByteAtPut( Oop receiver, Oop offset, Oop value );

        // Smi

        //%prim
        // <Proxy> primitiveProxySmiAt: offset    <SmallInteger>
        //                      ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { error = #(ConversionFailed)
        //              name  = 'proxyOopPrimitives::smiAt' }
        //%
        static Oop __CALLING_CONVENTION smiAt( Oop receiver, Oop offset );

        //%prim
        // <Proxy> primitiveProxySmiAt: offset    <SmallInteger>
        //                         put: value     <SmallInteger>
        //                      ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { name = 'proxyOopPrimitives::smiAtPut' }
        //%
        static Oop __CALLING_CONVENTION smiAtPut( Oop receiver, Oop offset, Oop value );

        // Proxy

        //%prim
        // <Proxy> primitiveProxySubProxyAt: offset    <SmallInteger>
        //                           result: result    <Proxy>
        //                           ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { name = 'proxyOopPrimitives::subProxyAt' }
        //%
        static Oop __CALLING_CONVENTION subProxyAt( Oop receiver, Oop offset, Oop result );

        //%prim
        // <Proxy> primitiveProxyProxyAt: offset    <SmallInteger>
        //                        result: result    <Proxy>
        //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { name = 'proxyOopPrimitives::proxyAt' }
        //%
        static Oop __CALLING_CONVENTION proxyAt( Oop receiver, Oop offset, Oop result );

        //%prim
        // <Proxy> primitiveProxyProxyAt: offset    <SmallInteger>
        //                           put: value     <Proxy>
        //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { name = 'proxyOopPrimitives::proxyAtPut' }
        //%
        static Oop __CALLING_CONVENTION proxyAtPut( Oop receiver, Oop offset, Oop value );

        // Single precision floats

        //%prim
        // <Proxy> primitiveProxySinglePrecisionFloatAt: offset    <SmallInteger>
        //                                       ifFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { name  = 'proxyOopPrimitives::singlePrecisionFloatAt' }
        //%
        static Oop __CALLING_CONVENTION singlePrecisionFloatAt( Oop receiver, Oop offset );

        //%prim
        // <Proxy> primitiveProxySinglePrecisionFloatAt: offset    <SmallInteger>
        //                                          put: value     <Float>
        //                                       ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { error = #(ConversionFailed)
        //              name = 'proxyOopPrimitives::singlePrecisionFloatAtPut' }
        //%
        static Oop __CALLING_CONVENTION singlePrecisionFloatAtPut( Oop receiver, Oop offset, Oop value );

        // Double precision floats

        //%prim
        // <Proxy> primitiveProxyDoublePrecisionFloatAt: offset    <SmallInteger>
        //                                       ifFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { name  = 'proxyOopPrimitives::doublePrecisionFloatAt' }
        //%
        static Oop __CALLING_CONVENTION doublePrecisionFloatAt( Oop receiver, Oop offset );

        //%prim
        // <Proxy> primitiveProxyDoublePrecisionFloatAt: offset    <SmallInteger>
        //                                          put: value     <Float>
        //                                       ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { error = #(ConversionFailed)
        //              name = 'proxyOopPrimitives::doublePrecisionFloatAtPut' }
        //%
        static Oop __CALLING_CONVENTION doublePrecisionFloatAtPut( Oop receiver, Oop offset, Oop value );

        // API Calls through proxies

        //%prim
        // <Proxy> primitiveAPICallResult: proxy <Proxy>
        //                         ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { flags = #NonLocalReturn
        //              name  = 'proxyOopPrimitives::callOut0' }
        //%
        static Oop __CALLING_CONVENTION callOut0( Oop receiver, Oop result );

        //%prim
        // <Proxy> primitiveAPICallValue: arg1  <Proxy|SmallInteger>
        //                        result: proxy <Proxy>
        //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { flags = #NonLocalReturn
        //              name  = 'proxyOopPrimitives::callOut1' }
        //%
        static Oop __CALLING_CONVENTION callOut1( Oop receiver, Oop arg1, Oop result );

        //%prim
        // <Proxy> primitiveAPICallValue: arg1      <Proxy|SmallInteger>
        //                         value: arg2      <Proxy|SmallInteger>
        //                        result: proxy     <Proxy>
        //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { flags = #NonLocalReturn
        //              name  = 'proxyOopPrimitives::callOut2' }
        //%
        static Oop __CALLING_CONVENTION callOut2( Oop receiver, Oop arg1, Oop arg2, Oop result );

        //%prim
        // <Proxy> primitiveAPICallValue: arg1      <Proxy|SmallInteger>
        //                         value: arg2      <Proxy|SmallInteger>
        //                         value: arg3      <Proxy|SmallInteger>
        //                        result: proxy     <Proxy>
        //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { flags = #NonLocalReturn
        //              name  = 'proxyOopPrimitives::callOut3' }
        //%
        static Oop __CALLING_CONVENTION callOut3( Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop result );

        //%prim
        // <Proxy> primitiveAPICallValue: arg1      <Proxy|SmallInteger>
        //                         value: arg2      <Proxy|SmallInteger>
        //                         value: arg3      <Proxy|SmallInteger>
        //                         value: arg4      <Proxy|SmallInteger>
        //                        result: proxy     <Proxy>
        //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { flags = #NonLocalReturn
        //              name  = 'proxyOopPrimitives::callOut4' }
        //%
        static Oop __CALLING_CONVENTION callOut4( Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop result );

        //%prim
        // <Proxy> primitiveAPICallValue: arg1      <Proxy|SmallInteger>
        //                         value: arg2      <Proxy|SmallInteger>
        //                         value: arg3      <Proxy|SmallInteger>
        //                         value: arg4      <Proxy|SmallInteger>
        //                         value: arg5      <Proxy|SmallInteger>
        //                        result: proxy     <Proxy>
        //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { flags = #NonLocalReturn
        //              name  = 'proxyOopPrimitives::callOut5' }
        //%
        static Oop __CALLING_CONVENTION callOut5( Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop result );


};

