
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"

// Primitives for call back

class callBackPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <NoReceiver> primitiveCallBackReceiver: receiver <Object>
        //                               selector: selector <Symbol>
        //                                 ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::initialize' }
        //%
        static PRIM_DECL_2( initialize, Oop receiver, Oop selector );

        //%prim
        // <NoReceiver> primitiveCallBackRegisterPascalCall: index   <SmallInteger>
        //                                numberOfArguments: nofArgs <SmallInteger>
        //                                           result: proxy   <Proxy>
        //                                           ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::registerPascalCall' }
        //%
        static PRIM_DECL_3( registerPascalCall, Oop index, Oop nofArgs, Oop result );

        //%prim
        // <NoReceiver> primitiveCallBackRegisterCCall: index   <SmallInteger>
        //                                      result: proxy   <Proxy>
        //                                      ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::registerCCall' }
        //%
        static PRIM_DECL_2( registerCCall, Oop index, Oop result );

        //%prim
        // <Object> primitiveCallBackUnregister: proxy <Proxy>
        //                               ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::unregister' }
        //%
        static PRIM_DECL_1( unregister, Oop proxy );

        //%prim
        // <Object> primitiveCallBackInvokePascal2: proxy <Proxy>
        //                                  ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::invokePascal' }
        //%
        static PRIM_DECL_1( invokePascal, Oop proxy );

        //%prim
        // <Object> primitiveCallBackInvokeC2: proxy <Proxy>
        //                             ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::invokeC' }
        //%
        static PRIM_DECL_1( invokeC, Oop proxy );

};
