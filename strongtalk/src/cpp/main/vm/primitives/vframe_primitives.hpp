//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
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
        static Oop __CALLING_CONVENTION process( Oop receiver );

        //%prim
        // <Activation> primitiveActivationIndex ^<SmallInteger> =
        //   Internal { flags = #(Pure)
        //              name  = 'VirtualFrameOopPrimitives::index' }
        //%
        static Oop __CALLING_CONVENTION index( Oop receiver );

        //%prim
        // <Activation> primitiveActivationTimeStamp ^<SmallInteger> =
        //   Internal { flags = #(Pure)
        //              name  = 'VirtualFrameOopPrimitives::time_stamp' }
        //%
        static Oop __CALLING_CONVENTION time_stamp( Oop receiver );

        //%prim
        // <Activation> primitiveActivationIsSmalltalkActivationIfFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { flags = #(Pure)
        //              errors = #(ActivationIsInvalid)
        //              name  = 'VirtualFrameOopPrimitives::is_smalltalk_activation' }
        //%
        static Oop __CALLING_CONVENTION is_smalltalk_activation( Oop receiver );

        //%prim
        // <Activation> primitiveActivationByteCodeIndexIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::byte_code_index' }
        //%
        static Oop __CALLING_CONVENTION byte_code_index( Oop receiver );

        //%prim
        // <Activation> primitiveActivationExpressionStackIfFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::expression_stack' }
        //%
        static Oop __CALLING_CONVENTION expression_stack( Oop receiver );

        //%prim
        // <Activation> primitiveActivationMethodIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::method' }
        //%
        static Oop __CALLING_CONVENTION method( Oop receiver );

        //%prim
        // <Activation> primitiveActivationReceiverIfFail: failBlock <PrimFailBlock> ^<SmallInteger> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::receiver' }
        //%
        static Oop __CALLING_CONVENTION receiver( Oop recv );

        //%prim
        // <Activation> primitiveActivationTemporariesIfFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::temporaries' }
        //%
        static Oop __CALLING_CONVENTION temporaries( Oop receiver );

        //%prim
        // <Activation> primitiveActivationArgumentsIfFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::arguments' }
        //%
        static Oop __CALLING_CONVENTION arguments( Oop receiver );

        //%prim
        // <Activation> primitiveActivationPrettyPrintIfFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { flags  = #(Pure)
        //              errors = #(ActivationIsInvalid ExternalActivation)
        //              name   = 'VirtualFrameOopPrimitives::pretty_print' }
        //%
        static Oop __CALLING_CONVENTION pretty_print( Oop receiver );

        //%prim
        // <NoReceiver> primitiveActivationSingleStep: activation <Activation>
        //                                 ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Sets up single stepping for the activation''s process.'
        //              doc   = 'Returns the activation.'
        //              error = #(ProcessCannotContinue Dead)
        //              name  = 'VirtualFrameOopPrimitives::single_step' }
        //%
        static Oop __CALLING_CONVENTION single_step( Oop activation );

        //%prim
        // <NoReceiver> primitiveActivationStepNext: activation <Activation>
        //                                 ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Sets up stepping up to the next bytecode of the activation''s method.'
        //              doc   = 'Returns the activation.'
        //              error = #(ProcessCannotContinue Dead)
        //              name  = 'VirtualFrameOopPrimitives::step_next' }
        //%
        static Oop __CALLING_CONVENTION step_next( Oop activation );

        //%prim
        // <NoReceiver> primitiveActivationStepReturn: activation <Activation>
        //                                 ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Sets up stepping up to the return from the activation''s method.'
        //              doc   = 'Returns the activation.'
        //              error = #(ProcessCannotContinue Dead)
        //              name  = 'VirtualFrameOopPrimitives::step_return' }
        //%
        static Oop __CALLING_CONVENTION step_return( Oop activation );

};
