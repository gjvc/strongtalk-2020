//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"

// Primitives for methods

class MethodOopPrimitives : AllStatic {
private:
    static void inc_calls() {
        number_of_calls++;
    }


public:
    static std::int32_t number_of_calls;

    //%prim
    // <Method> primitiveMethodSelector ^<Symbol> =
    //   Internal { name = 'MethodOopPrimitives::selector' }
    //%
    static PRIM_DECL_1( selector, Oop receiver );

    //%prim
    // <Method> primitiveMethodSelector: name      <Symbol>
    //                           ifFail: failBlock <PrimFailBlock> ^<Symbol> =
    //   Internal { name = 'MethodOopPrimitives::setSelector' }
    //%
    static PRIM_DECL_2( setSelector, Oop receiver, Oop name );

    //%prim
    // <Method> primitiveMethodNumberOfArguments ^<SmallInteger> =
    //   Internal { name = 'MethodOopPrimitives::numberOfArguments' }
    //%
    static PRIM_DECL_1( numberOfArguments, Oop receiver );

    //%prim
    // <Method> primitiveMethodOuterIfFail: failBlock <PrimFailBlock> ^<Method> =
    //   Internal { error = #(ReceiverNotBlockMethod)
    //              name  = 'MethodOopPrimitives::outer' }
    //%
    static PRIM_DECL_1( outer, Oop receiver );

    //%prim
    // <Method> primitiveMethodOuter: method    <Method>
    //                        ifFail: failBlock <PrimFailBlock> ^<Symbol> =
    //   Internal { name = 'MethodOopPrimitives::setOuter' }
    //%
    static PRIM_DECL_2( setOuter, Oop receiver, Oop method );

    //%prim
    // <Method> primitiveMethodReferencedInstVarNamesMixin: mixin <Mixin>
    //                                              ifFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
    //   Internal { name = 'MethodOopPrimitives::referenced_instance_variable_names' }
    //%
    static PRIM_DECL_2( referenced_instance_variable_names, Oop receiver, Oop mixin );

    //%prim
    // <Method> primitiveMethodReferencedClassVarNames ^<IndexedInstanceVariables> =
    //   Internal { name = 'MethodOopPrimitives::referenced_class_variable_names' }
    //%
    static PRIM_DECL_1( referenced_class_variable_names, Oop receiver );

    //%prim
    // <Method> primitiveMethodReferencedGlobalNames ^<IndexedInstanceVariables> =
    //   Internal { name = 'MethodOopPrimitives::referenced_global_names' }
    //%
    static PRIM_DECL_1( referenced_global_names, Oop receiver );

    //%prim
    // <Method> primitiveMethodSenders ^<IndexedInstanceVariables> =
    //   Internal { name = 'MethodOopPrimitives::senders' }
    //%
    static PRIM_DECL_1( senders, Oop receiver );

    //%prim
    // <Method> primitiveMethodPrettyPrintKlass: klass     <Object>
    //                                   ifFail: failBlock <PrimFailBlock> ^<Method> =
    //   Internal { name = 'MethodOopPrimitives::prettyPrint' }
    //%
    static PRIM_DECL_2( prettyPrint, Oop receiver, Oop klass );

    //%prim
    // <Method> primitiveMethodPrettyPrintSourceKlass: klass     <Object>
    //                                         ifFail: failBlock <PrimFailBlock> ^<ByteIndexedInstanceVariables> =
    //   Internal { name = 'MethodOopPrimitives::prettyPrintSource' }
    //%
    static PRIM_DECL_2( prettyPrintSource, Oop receiver, Oop klass );

    //%prim
    // <Method> primitiveMethodPrintCodes ^<Symbol> =
    //   Internal { name = 'MethodOopPrimitives::printCodes' }
    //%
    static PRIM_DECL_1( printCodes, Oop receiver );

    //%prim
    // <Method> primitiveMethodDebugInfo ^<Object> =
    //   Internal { name = 'MethodOopPrimitives::debug_info' }
    //%
    static PRIM_DECL_1( debug_info, Oop receiver );

    //%prim
    // <Method> primitiveMethodSizeAndFlags ^<Object> =
    //   Internal { name = 'MethodOopPrimitives::size_and_flags' }
    //%
    static PRIM_DECL_1( size_and_flags, Oop receiver );

    //%prim
    // <Method> primitiveMethodBody ^<Object> =
    //   Internal { name = 'MethodOopPrimitives::fileout_body' }
    //%
    static PRIM_DECL_1( fileout_body, Oop receiver );

    //%prim
    // <NoReceiver> primitiveConstructMethod: selector_or_method <Object>
    //                                 flags: flags              <SmallInteger>
    //                               nofArgs: nofArgs            <SmallInteger>
    //                             debugInfo: debugInfo          <Array>
    //                                 bytes: bytes              <ByteArray>
    //                                  oops: oops               <Array>
    //                                ifFail: failBlock          <PrimFailBlock> ^<Method> =
    //   Internal { name = 'MethodOopPrimitives::constructMethod' }
    //%
    static PRIM_DECL_6( constructMethod, Oop selector_or_method, Oop flags, Oop nofArgs, Oop debugInfo, Oop bytes, Oop oops );

    //%prim
    // <Method> primitiveMethodAllocateBlockIfFail: failBlock <PrimFailBlock> ^<Block> =
    //   Internal {
    //     name  = 'MethodOopPrimitives::allocate_block' }
    //%
    static PRIM_DECL_1( allocate_block, Oop receiver );

    //%prim
    // <Method> primitiveMethodAllocateBlock: receiver <Object> ifFail: failBlock <PrimFailBlock> ^<Block> =
    //   Internal {
    //     name  = 'MethodOopPrimitives::allocate_block_self' }
    //%
    static PRIM_DECL_2( allocate_block_self, Oop receiver, Oop self );

    //%prim
    // <Method> primitiveMethodSetInliningInfo: info      <Symbol>
    //                                  ifFail: failBlock <PrimFailBlock> ^<Symbol> =
    //   Internal { doc  = 'Sets the inlining info for the method (#Normal, #Never, or #Always)'
    //              error = #(ArgumentIsInvalid)
    //              name = 'MethodOopPrimitives::set_inlining_info' }
    //%
    static PRIM_DECL_2( set_inlining_info, Oop receiver, Oop info );


    //%prim
    // <Method> primitiveMethodInliningInfo ^<Symbol> =
    //   Internal { doc  = 'Returns #Normal, #Never, or #Always'
    //              name = 'MethodOopPrimitives::inlining_info' }
    //%
    static PRIM_DECL_1( inlining_info, Oop receiver );
};
