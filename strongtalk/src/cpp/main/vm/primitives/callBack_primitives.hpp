
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
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
        static Oop __CALLING_CONVENTION initialize( Oop receiver, Oop selector );

        //%prim
        // <NoReceiver> primitiveCallBackRegisterPascalCall: index   <SmallInteger>
        //                                numberOfArguments: nofArgs <SmallInteger>
        //                                           result: proxy   <Proxy>
        //                                           ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::registerPascalCall' }
        //%
        static Oop __CALLING_CONVENTION registerPascalCall( Oop index, Oop nofArgs, Oop result );

        //%prim
        // <NoReceiver> primitiveCallBackRegisterCCall: index   <SmallInteger>
        //                                      result: proxy   <Proxy>
        //                                      ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::registerCCall' }
        //%
        static Oop __CALLING_CONVENTION registerCCall( Oop index, Oop result );

        //%prim
        // <Object> primitiveCallBackUnregister: proxy <Proxy>
        //                               ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::unregister' }
        //%
        static Oop __CALLING_CONVENTION unregister( Oop proxy );

        //%prim
        // <Object> primitiveCallBackInvokePascal2: proxy <Proxy>
        //                                  ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::invokePascal' }
        //%
        static Oop __CALLING_CONVENTION invokePascal( Oop proxy );

        //%prim
        // <Object> primitiveCallBackInvokeC2: proxy <Proxy>
        //                             ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { name = 'callBackPrimitives::invokeC' }
        //%
        static Oop __CALLING_CONVENTION invokeC( Oop proxy );

};

