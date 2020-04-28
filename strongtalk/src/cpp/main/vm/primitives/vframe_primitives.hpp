//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for VirtualFrame

class VirtualFrameOopPrimitives : AllStatic {

    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <Activation> primitiveActivationProcess ^<SmallInteger> =
        //   Internal { flags = #(Pure)
        //              name  = 'VirtualFrameOopPrimitives::process' }
        //%
        static PRIM_DECL_1( process, Oop receiver );

        //%prim
        // <Activation> primitiveActivationIndex ^<SmallInteger> =
        //   Internal { flags = #(Pure)
        //              name  = 'VirtualFrameOopPrimitives::index' }
        //%
        static PRIM_DECL_1( index, Oop receiver );

        //%prim
        // <Activation> primitiveActivationTimeStamp ^<SmallInteger> =
        //   Internal { flags = #(Pure)
        //              name  = 'VirtualFrameOopPrimitives::time_stamp' }
        //%
        static PRIM_DECL_1( time_stamp, Oop receiver );

        //%prim
        // <Activation> primitiveActivationIsSmalltalkActivationIfFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { flags = #(Pure)
        //              errors = #(ActivationIsInvalid)
        //              name  = 'VirtualFrameOopPrimitives::is_smalltalk_activation' }
        //%
        static PRIM_DECL_1( is_smalltalk_activation, Oop receiver );

        //%prim
        // <Activation> primitiveActivationByteCodeIndexIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::byte_code_index' }
        //%
        static PRIM_DECL_1( byte_code_index, Oop receiver );

        //%prim
        // <Activation> primitiveActivationExpressionStackIfFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::expression_stack' }
        //%
        static PRIM_DECL_1( expression_stack, Oop receiver );

        //%prim
        // <Activation> primitiveActivationMethodIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::method' }
        //%
        static PRIM_DECL_1( method, Oop receiver );

        //%prim
        // <Activation> primitiveActivationReceiverIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::receiver' }
        //%
        static PRIM_DECL_1( receiver, Oop recv );

        //%prim
        // <Activation> primitiveActivationTemporariesIfFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::temporaries' }
        //%
        static PRIM_DECL_1( temporaries, Oop receiver );

        //%prim
        // <Activation> primitiveActivationArgumentsIfFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::arguments' }
        //%
        static PRIM_DECL_1( arguments, Oop receiver );

        //%prim
        // <Activation> primitiveActivationPrettyPrintIfFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::pretty_print' }
        //%
        static PRIM_DECL_1( pretty_print, Oop receiver );

        //%prim
        // <NoReceiver> primitiveActivationSingleStep: activation <Activation>
        //                                 ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Sets up single stepping for the activation''s process.'
        //              doc   = 'Returns the activation.'
        //              error = #(ProcessCannotContinue Dead)
        //              name  = 'VirtualFrameOopPrimitives::single_step' }
        //%
        static PRIM_DECL_1( single_step, Oop activation );

        //%prim
        // <NoReceiver> primitiveActivationStepNext: activation <Activation>
        //                                 ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Sets up stepping up to the next bytecode of the activation''s method.'
        //              doc   = 'Returns the activation.'
        //              error = #(ProcessCannotContinue Dead)
        //              name  = 'VirtualFrameOopPrimitives::step_next' }
        //%
        static PRIM_DECL_1( step_next, Oop activation );

        //%prim
        // <NoReceiver> primitiveActivationStepReturn: activation <Activation>
        //                                 ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Sets up stepping up to the return from the activation''s method.'
        //              doc   = 'Returns the activation.'
        //              error = #(ProcessCannotContinue Dead)
        //              name  = 'VirtualFrameOopPrimitives::step_return' }
        //%
        static PRIM_DECL_1( step_return, Oop activation );

};
