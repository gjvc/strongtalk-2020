
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/primitive/primitive_declarations.hpp"
#include "vm/primitive/primitive_tracing.hpp"

// Primitives for call back

class CallBackPrimitives : AllStatic {
private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static std::int32_t number_of_calls;

    //%prim
    // <NoReceiver> primitiveCallBackReceiver: receiver <Object>
    //                               selector: selector <Symbol>
    //                                 ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'CallBackPrimitives::initialize' }
    //%
    static PRIM_DECL_2( initialize, Oop receiver, Oop selector );

    //%prim
    // <NoReceiver> primitiveCallBackRegisterPascalCall: index   <SmallInteger>
    //                                numberOfArguments: nofArgs <SmallInteger>
    //                                           result: proxy   <Proxy>
    //                                           ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'CallBackPrimitives::registerPascalCall' }
    //%
    static PRIM_DECL_3( registerPascalCall, Oop index, Oop nofArgs, Oop result );

    //%prim
    // <NoReceiver> primitiveCallBackRegisterCCall: index   <SmallInteger>
    //                                      result: proxy   <Proxy>
    //                                      ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'CallBackPrimitives::registerCCall' }
    //%
    static PRIM_DECL_2( registerCCall, Oop index, Oop result );

    //%prim
    // <Object> primitiveCallBackUnregister: proxy <Proxy>
    //                               ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'CallBackPrimitives::unregister' }
    //%
    static PRIM_DECL_1( unregister, Oop proxy );

    //%prim
    // <Object> primitiveCallBackInvokePascal2: proxy <Proxy>
    //                                  ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'CallBackPrimitives::invokePascal' }
    //%
    static PRIM_DECL_1( invokePascal, Oop proxy );

    //%prim
    // <Object> primitiveCallBackInvokeC2: proxy <Proxy>
    //                             ifFail: failBlock <PrimFailBlock> ^<Object> =
    //   Internal { name = 'CallBackPrimitives::invokeC' }
    //%
    static PRIM_DECL_1( invokeC, Oop proxy );

};
