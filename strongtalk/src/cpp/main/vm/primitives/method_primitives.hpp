//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Primitives for methods

class methodOopPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <Method> primitiveMethodSelector ^<Symbol> =
        //   Internal { name = 'methodOopPrimitives::selector' }
        //%
        static Oop __CALLING_CONVENTION selector( Oop receiver );

        //%prim
        // <Method> primitiveMethodSelector: name      <Symbol>
        //                           ifFail: failBlock <PrimFailBlock> ^<Symbol> =
        //   Internal { name = 'methodOopPrimitives::setSelector' }
        //%
        static Oop __CALLING_CONVENTION setSelector( Oop receiver, Oop name );

        //%prim
        // <Method> primitiveMethodNumberOfArguments ^<SmallInteger> =
        //   Internal { name = 'methodOopPrimitives::numberOfArguments' }
        //%
        static Oop __CALLING_CONVENTION numberOfArguments( Oop receiver );

        //%prim
        // <Method> primitiveMethodOuterIfFail: failBlock <PrimFailBlock> ^<Method> =
        //   Internal { error = #(ReceiverNotBlockMethod)
        //              name  = 'methodOopPrimitives::outer' }
        //%
        static Oop __CALLING_CONVENTION outer( Oop receiver );

        //%prim
        // <Method> primitiveMethodOuter: method    <Method>
        //                        ifFail: failBlock <PrimFailBlock> ^<Symbol> =
        //   Internal { name = 'methodOopPrimitives::setOuter' }
        //%
        static Oop __CALLING_CONVENTION setOuter( Oop receiver, Oop method );

        //%prim
        // <Method> primitiveMethodReferencedInstVarNamesMixin: mixin <Mixin>
        //                                              ifFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { name = 'methodOopPrimitives::referenced_instance_variable_names' }
        //%
        static Oop __CALLING_CONVENTION referenced_instance_variable_names( Oop receiver, Oop mixin );

        //%prim
        // <Method> primitiveMethodReferencedClassVarNames ^<IndexedInstanceVariables> =
        //   Internal { name = 'methodOopPrimitives::referenced_class_variable_names' }
        //%
        static Oop __CALLING_CONVENTION referenced_class_variable_names( Oop receiver );

        //%prim
        // <Method> primitiveMethodReferencedGlobalNames ^<IndexedInstanceVariables> =
        //   Internal { name = 'methodOopPrimitives::referenced_global_names' }
        //%
        static Oop __CALLING_CONVENTION referenced_global_names( Oop receiver );

        //%prim
        // <Method> primitiveMethodSenders ^<IndexedInstanceVariables> =
        //   Internal { name = 'methodOopPrimitives::senders' }
        //%
        static Oop __CALLING_CONVENTION senders( Oop receiver );

        //%prim
        // <Method> primitiveMethodPrettyPrintKlass: klass     <Object>
        //                                   ifFail: failBlock <PrimFailBlock> ^<Method> =
        //   Internal { name = 'methodOopPrimitives::prettyPrint' }
        //%
        static Oop __CALLING_CONVENTION prettyPrint( Oop receiver, Oop klass );

        //%prim
        // <Method> primitiveMethodPrettyPrintSourceKlass: klass     <Object>
        //                                         ifFail: failBlock <PrimFailBlock> ^<ByteIndexedInstanceVariables> =
        //   Internal { name = 'methodOopPrimitives::prettyPrintSource' }
        //%
        static Oop __CALLING_CONVENTION prettyPrintSource( Oop receiver, Oop klass );

        //%prim
        // <Method> primitiveMethodPrintCodes ^<Symbol> =
        //   Internal { name = 'methodOopPrimitives::printCodes' }
        //%
        static Oop __CALLING_CONVENTION printCodes( Oop receiver );

        //%prim
        // <Method> primitiveMethodDebugInfo ^<Object> =
        //   Internal { name = 'methodOopPrimitives::debug_info' }
        //%
        static Oop __CALLING_CONVENTION debug_info( Oop receiver );

        //%prim
        // <Method> primitiveMethodSizeAndFlags ^<Object> =
        //   Internal { name = 'methodOopPrimitives::size_and_flags' }
        //%
        static Oop __CALLING_CONVENTION size_and_flags( Oop receiver );

        //%prim
        // <Method> primitiveMethodBody ^<Object> =
        //   Internal { name = 'methodOopPrimitives::fileout_body' }
        //%
        static Oop __CALLING_CONVENTION fileout_body( Oop receiver );

        //%prim
        // <NoReceiver> primitiveConstructMethod: selector_or_method <Object>
        //                                 flags: flags              <SmallInteger>
        //                               nofArgs: nofArgs            <SmallInteger>
        //                             debugInfo: debugInfo          <Array>
        //                                 bytes: bytes              <ByteArray>
        //                                  oops: oops               <Array>
        //                                ifFail: failBlock          <PrimFailBlock> ^<Method> =
        //   Internal { name = 'methodOopPrimitives::constructMethod' }
        //%
        static Oop __CALLING_CONVENTION constructMethod( Oop selector_or_method, Oop flags, Oop nofArgs, Oop debugInfo, Oop bytes, Oop oops );

        //%prim
        // <Method> primitiveMethodAllocateBlockIfFail: failBlock <PrimFailBlock> ^<Block> =
        //   Internal {
        //     name  = 'methodOopPrimitives::allocate_block' }
        //%
        static Oop __CALLING_CONVENTION allocate_block( Oop receiver );

        //%prim
        // <Method> primitiveMethodAllocateBlock: receiver <Object> ifFail: failBlock <PrimFailBlock> ^<Block> =
        //   Internal {
        //     name  = 'methodOopPrimitives::allocate_block_self' }
        //%
        static Oop __CALLING_CONVENTION allocate_block_self( Oop receiver, Oop self );

        //%prim
        // <Method> primitiveMethodSetInliningInfo: info      <Symbol>
        //                                  ifFail: failBlock <PrimFailBlock> ^<Symbol> =
        //   Internal { doc  = 'Sets the inlining info for the method (#Normal, #Never, or #Always)'
        //              error = #(ArgumentIsInvalid)
        //              name = 'methodOopPrimitives::set_inlining_info' }
        //%
        static Oop __CALLING_CONVENTION set_inlining_info( Oop receiver, Oop info );


        //%prim
        // <Method> primitiveMethodInliningInfo ^<Symbol> =
        //   Internal { doc  = 'Returns #Normal, #Never, or #Always'
        //              name = 'methodOopPrimitives::inlining_info' }
        //%
        static Oop __CALLING_CONVENTION inlining_info( Oop receiver );
};

