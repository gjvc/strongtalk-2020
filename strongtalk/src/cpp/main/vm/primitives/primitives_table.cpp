//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

//#pragma once

#include "vm/primitives/primitives.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/compiler/Expression.hpp"
#include "vm/runtime/Process.hpp"

#include "vm/primitives/behavior_primitives.hpp"
#include "vm/primitives/block_primitives.hpp"
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/primitives/callBack_primitives.hpp"
#include "vm/primitives/dByteArray_primitives.hpp"
#include "vm/primitives/debug_primitives.hpp"
#include "vm/primitives/double_primitives.hpp"
#include "vm/primitives/dValueArray_primitives.hpp"
#include "vm/primitives/method_primitives.hpp"
#include "vm/primitives/mixin_primitives.hpp"
#include "vm/primitives/objArray_primitives.hpp"
#include "vm/primitives/oop_primitives.hpp"
#include "vm/primitives/process_primitives.hpp"
#include "vm/primitives/proxy_primitives.hpp"
#include "vm/primitives/smi_primitives.hpp"
#include "vm/primitives/system_primitives.hpp"
#include "vm/primitives/vframe_primitives.hpp"


// -----------------------------------------------------------------------------

static const char *signature_0[] = { "Proxy", "Proxy", "Proxy" };
static const char *errors_0[]    = { nullptr };
static PrimitiveDescriptor primitive_0 = {
        "primitiveAPICallResult:ifFail:", primitiveFunctionType( &proxyOopPrimitives::callOut0 ), 1507330, signature_0, errors_0
};

static const char *signature_1[] = { "Proxy", "Proxy", "Proxy|SmallInteger", "Proxy" };
static const char *errors_1[]    = { nullptr };
static PrimitiveDescriptor primitive_1 = {
        "primitiveAPICallValue:result:ifFail:", primitiveFunctionType( &proxyOopPrimitives::callOut1 ), 1507331, signature_1, errors_1
};

static const char *signature_2[] = { "Proxy", "Proxy", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy" };
static const char *errors_2[]    = { nullptr };
static PrimitiveDescriptor primitive_2 = {
        "primitiveAPICallValue:value:result:ifFail:", primitiveFunctionType( &proxyOopPrimitives::callOut2 ), 1507332, signature_2, errors_2
};

static const char *signature_3[] = { "Proxy", "Proxy", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy" };
static const char *errors_3[]    = { nullptr };
static PrimitiveDescriptor primitive_3 = {
        "primitiveAPICallValue:value:value:result:ifFail:", primitiveFunctionType( &proxyOopPrimitives::callOut3 ), 1507333, signature_3, errors_3
};

static const char *signature_4[] = { "Proxy", "Proxy", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy" };
static const char *errors_4[]    = { nullptr };
static PrimitiveDescriptor primitive_4 = {
        "primitiveAPICallValue:value:value:value:result:ifFail:", primitiveFunctionType( &proxyOopPrimitives::callOut4 ), 1507334, signature_4, errors_4
};

static const char *signature_5[] = { "Proxy", "Proxy", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy|SmallInteger", "Proxy" };
static const char *errors_5[]    = { nullptr };
static PrimitiveDescriptor primitive_5 = {
        "primitiveAPICallValue:value:value:value:value:result:ifFail:", primitiveFunctionType( &proxyOopPrimitives::callOut5 ), 1507335, signature_5, errors_5
};

static const char *signature_6[] = { "IndexedInstanceVariables", "Activation" };
static const char *errors_6[]    = { nullptr };
static PrimitiveDescriptor primitive_6 = {
        "primitiveActivationArgumentsIfFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::arguments ), 1835009, signature_6, errors_6
};

static const char *signature_7[] = { "SmallInteger", "Activation" };
static const char *errors_7[]    = { nullptr };
static PrimitiveDescriptor primitive_7 = {
        "primitiveActivationByteCodeIndexIfFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::byte_code_index ), 1835009, signature_7, errors_7
};

static const char *signature_8[] = { "IndexedInstanceVariables", "Activation" };
static const char *errors_8[]    = { nullptr };
static PrimitiveDescriptor primitive_8 = {
        "primitiveActivationExpressionStackIfFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::expression_stack ), 1835009, signature_8, errors_8
};

static const char *signature_9[] = { "SmallInteger", "Activation" };
static const char *errors_9[]    = { nullptr };
static PrimitiveDescriptor primitive_9 = {
        "primitiveActivationIndex", primitiveFunctionType( &VirtualFrameOopPrimitives::index ), 1572865, signature_9, errors_9
};

static const char *signature_10[] = { "Boolean", "Activation" };
static const char *errors_10[]    = { nullptr };
static PrimitiveDescriptor primitive_10 = {
        "primitiveActivationIsSmalltalkActivationIfFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::is_smalltalk_activation ), 1835009, signature_10, errors_10
};

static const char *signature_11[] = { "SmallInteger", "Activation" };
static const char *errors_11[]    = { nullptr };
static PrimitiveDescriptor primitive_11 = {
        "primitiveActivationMethodIfFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::method ), 1835009, signature_11, errors_11
};

static const char *signature_12[] = { "IndexedByteInstanceVariables", "Activation" };
static const char *errors_12[]    = { nullptr };
static PrimitiveDescriptor primitive_12 = {
        "primitiveActivationPrettyPrintIfFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::pretty_print ), 1835009, signature_12, errors_12
};

static const char *signature_13[] = { "SmallInteger", "Activation" };
static const char *errors_13[]    = { nullptr };
static PrimitiveDescriptor primitive_13 = {
        "primitiveActivationProcess", primitiveFunctionType( &VirtualFrameOopPrimitives::process ), 1572865, signature_13, errors_13
};

static const char *signature_14[] = { "SmallInteger", "Activation" };
static const char *errors_14[]    = { nullptr };
static PrimitiveDescriptor primitive_14 = {
        "primitiveActivationReceiverIfFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::receiver ), 1835009, signature_14, errors_14
};

static const char *signature_15[] = { "Object", "Activation" };
static const char *errors_15[]    = { "ProcessCannotContinue", "Dead", nullptr };
static PrimitiveDescriptor primitive_15 = {
        "primitiveActivationSingleStep:ifFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::single_step ), 327681, signature_15, errors_15
};

static const char *signature_16[] = { "Object", "Activation" };
static const char *errors_16[]    = { "ProcessCannotContinue", "Dead", nullptr };
static PrimitiveDescriptor primitive_16 = {
        "primitiveActivationStepNext:ifFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::step_next ), 327681, signature_16, errors_16
};

static const char *signature_17[] = { "Object", "Activation" };
static const char *errors_17[]    = { "ProcessCannotContinue", "Dead", nullptr };
static PrimitiveDescriptor primitive_17 = {
        "primitiveActivationStepReturn:ifFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::step_return ), 327681, signature_17, errors_17
};

static const char *signature_18[] = { "IndexedInstanceVariables", "Activation" };
static const char *errors_18[]    = { nullptr };
static PrimitiveDescriptor primitive_18 = {
        "primitiveActivationTemporariesIfFail:", primitiveFunctionType( &VirtualFrameOopPrimitives::temporaries ), 1835009, signature_18, errors_18
};

static const char *signature_19[] = { "SmallInteger", "Activation" };
static const char *errors_19[]    = { nullptr };
static PrimitiveDescriptor primitive_19 = {
        "primitiveActivationTimeStamp", primitiveFunctionType( &VirtualFrameOopPrimitives::time_stamp ), 1572865, signature_19, errors_19
};

static const char *signature_20[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_20[]    = { "Overflow", nullptr };
static PrimitiveDescriptor primitive_20 = {
        "primitiveAdd:ifFail:", primitiveFunctionType( &smiOopPrimitives_add ), 6029826, signature_20, errors_20
};

static const char *signature_21[] = { "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "Integer" };
static const char *errors_21[]    = { nullptr };
static PrimitiveDescriptor primitive_21 = {
        "primitiveAlienAddress:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienSetAddress ), 1312258, signature_21, errors_21
};

static const char *signature_22[] = { "Integer", "IndexedByteInstanceVariables" };
static const char *errors_22[]    = { nullptr };
static PrimitiveDescriptor primitive_22 = {
        "primitiveAlienAddressIfFail:", primitiveFunctionType( &byteArrayPrimitives::alienGetAddress ), 1312257, signature_22, errors_22
};

static const char *signature_23[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_23[]    = { nullptr };
static PrimitiveDescriptor primitive_23 = {
        "primitiveAlienCallResult:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResult0 ), 1312258, signature_23, errors_23
};

static const char *signature_24[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables|SmallInteger" };
static const char *errors_24[]    = { nullptr };
static PrimitiveDescriptor primitive_24 = {
        "primitiveAlienCallResult:with:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResult1 ), 1312259, signature_24, errors_24
};

static const char *signature_25[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger" };
static const char *errors_25[]    = { nullptr };
static PrimitiveDescriptor primitive_25 = {
        "primitiveAlienCallResult:with:with:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResult2 ), 1312260, signature_25, errors_25
};

static const char *signature_26[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger" };
static const char *errors_26[]    = { nullptr };
static PrimitiveDescriptor primitive_26 = {
        "primitiveAlienCallResult:with:with:with:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResult3 ), 1312261, signature_26, errors_26
};

static const char *signature_27[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger" };
static const char *errors_27[]    = { nullptr };
static PrimitiveDescriptor primitive_27 = {
        "primitiveAlienCallResult:with:with:with:with:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResult4 ), 1312262, signature_27, errors_27
};

static const char *signature_28[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger" };
static const char *errors_28[]    = { nullptr };
static PrimitiveDescriptor primitive_28 = {
        "primitiveAlienCallResult:with:with:with:with:with:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResult5 ), 1312263, signature_28, errors_28
};

static const char *signature_29[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger" };
static const char *errors_29[]    = { nullptr };
static PrimitiveDescriptor primitive_29 = {
        "primitiveAlienCallResult:with:with:with:with:with:with:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResult6 ), 1312264, signature_29, errors_29
};

static const char *signature_30[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables|SmallInteger" };
static const char *errors_30[]    = { nullptr };
static PrimitiveDescriptor primitive_30 = {
        "primitiveAlienCallResult:with:with:with:with:with:with:with:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResult7 ), 1312265, signature_30, errors_30
};

static const char *signature_31[] = { "Integer", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "IndexedInstanceVariables" };
static const char *errors_31[]    = { nullptr };
static PrimitiveDescriptor primitive_31 = {
        "primitiveAlienCallResult:withArguments:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienCallResultWithArguments ), 1312259, signature_31, errors_31
};

static const char *signature_32[] = { "Float", "SmallInteger" };
static const char *errors_32[]    = { nullptr };
static PrimitiveDescriptor primitive_32 = {
        "primitiveAlienCalloc:ifFail:", primitiveFunctionType( &SystemPrimitives::alienCalloc ), 327681, signature_32, errors_32
};

static const char *signature_33[] = { "Double", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_33[]    = { nullptr };
static PrimitiveDescriptor primitive_33 = {
        "primitiveAlienDoubleAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienDoubleAt ), 1312258, signature_33, errors_33
};

static const char *signature_34[] = { "Double", "IndexedByteInstanceVariables", "SmallInteger", "Double" };
static const char *errors_34[]    = { nullptr };
static PrimitiveDescriptor primitive_34 = {
        "primitiveAlienDoubleAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienDoubleAtPut ), 1312259, signature_34, errors_34
};

static const char *signature_35[] = { "Double", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_35[]    = { nullptr };
static PrimitiveDescriptor primitive_35 = {
        "primitiveAlienFloatAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienFloatAt ), 1312258, signature_35, errors_35
};

static const char *signature_36[] = { "Double", "IndexedByteInstanceVariables", "SmallInteger", "Double" };
static const char *errors_36[]    = { nullptr };
static PrimitiveDescriptor primitive_36 = {
        "primitiveAlienFloatAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienFloatAtPut ), 1312259, signature_36, errors_36
};

static const char *signature_37[] = { "Float", "SmallInteger|LargeInteger" };
static const char *errors_37[]    = { nullptr };
static PrimitiveDescriptor primitive_37 = {
        "primitiveAlienFree:ifFail:", primitiveFunctionType( &SystemPrimitives::alienFree ), 327681, signature_37, errors_37
};

static const char *signature_38[] = { "Float", "SmallInteger" };
static const char *errors_38[]    = { nullptr };
static PrimitiveDescriptor primitive_38 = {
        "primitiveAlienMalloc:ifFail:", primitiveFunctionType( &SystemPrimitives::alienMalloc ), 327681, signature_38, errors_38
};

static const char *signature_39[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_39[]    = { nullptr };
static PrimitiveDescriptor primitive_39 = {
        "primitiveAlienSignedByteAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienSignedByteAt ), 1312258, signature_39, errors_39
};

static const char *signature_40[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger", "SmallInteger" };
static const char *errors_40[]    = { nullptr };
static PrimitiveDescriptor primitive_40 = {
        "primitiveAlienSignedByteAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienSignedByteAtPut ), 1312259, signature_40, errors_40
};

static const char *signature_41[] = { "SmallInteger|LargeInteger", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_41[]    = { nullptr };
static PrimitiveDescriptor primitive_41 = {
        "primitiveAlienSignedLongAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienSignedLongAt ), 1312258, signature_41, errors_41
};

static const char *signature_42[] = { "SmallInteger|LargeInteger", "IndexedByteInstanceVariables", "SmallInteger", "SmallInteger|LargeInteger" };
static const char *errors_42[]    = { nullptr };
static PrimitiveDescriptor primitive_42 = {
        "primitiveAlienSignedLongAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienSignedLongAtPut ), 1312259, signature_42, errors_42
};

static const char *signature_43[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_43[]    = { nullptr };
static PrimitiveDescriptor primitive_43 = {
        "primitiveAlienSignedShortAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienSignedShortAt ), 1312258, signature_43, errors_43
};

static const char *signature_44[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger", "SmallInteger" };
static const char *errors_44[]    = { nullptr };
static PrimitiveDescriptor primitive_44 = {
        "primitiveAlienSignedShortAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienSignedShortAtPut ), 1312259, signature_44, errors_44
};

static const char *signature_45[] = { "IndexedByteInstanceVariables", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_45[]    = { nullptr };
static PrimitiveDescriptor primitive_45 = {
        "primitiveAlienSize:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienSetSize ), 1312258, signature_45, errors_45
};

static const char *signature_46[] = { "SmallInteger", "IndexedByteInstanceVariables" };
static const char *errors_46[]    = { nullptr };
static PrimitiveDescriptor primitive_46 = {
        "primitiveAlienSizeIfFail:", primitiveFunctionType( &byteArrayPrimitives::alienGetSize ), 1312257, signature_46, errors_46
};

static const char *signature_47[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_47[]    = { nullptr };
static PrimitiveDescriptor primitive_47 = {
        "primitiveAlienUnsignedByteAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienUnsignedByteAt ), 1312258, signature_47, errors_47
};

static const char *signature_48[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger", "SmallInteger" };
static const char *errors_48[]    = { nullptr };
static PrimitiveDescriptor primitive_48 = {
        "primitiveAlienUnsignedByteAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienUnsignedByteAtPut ), 1312259, signature_48, errors_48
};

static const char *signature_49[] = { "SmallInteger|LargeInteger", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_49[]    = { nullptr };
static PrimitiveDescriptor primitive_49 = {
        "primitiveAlienUnsignedLongAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienUnsignedLongAt ), 1312258, signature_49, errors_49
};

static const char *signature_50[] = { "SmallInteger|LargeInteger", "IndexedByteInstanceVariables", "SmallInteger", "SmallInteger|LargeInteger" };
static const char *errors_50[]    = { nullptr };
static PrimitiveDescriptor primitive_50 = {
        "primitiveAlienUnsignedLongAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienUnsignedLongAtPut ), 1312259, signature_50, errors_50
};

static const char *signature_51[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_51[]    = { nullptr };
static PrimitiveDescriptor primitive_51 = {
        "primitiveAlienUnsignedShortAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienUnsignedShortAt ), 1312258, signature_51, errors_51
};

static const char *signature_52[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger", "SmallInteger" };
static const char *errors_52[]    = { nullptr };
static PrimitiveDescriptor primitive_52 = {
        "primitiveAlienUnsignedShortAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::alienUnsignedShortAtPut ), 1312259, signature_52, errors_52
};

static const char *signature_53[] = { "IndexedInstanceVariables", "SmallInteger" };
static const char *errors_53[]    = { nullptr };
static PrimitiveDescriptor primitive_53 = {
        "primitiveAllObjectsLimit:ifFail:", primitiveFunctionType( &SystemPrimitives::all_objects ), 327681, signature_53, errors_53
};

static const char *signature_54[] = { "Object", "IndexedInstanceVariables" };
static const char *errors_54[]    = { nullptr };
static PrimitiveDescriptor primitive_54 = {
        "primitiveApplyChange:ifFail:", primitiveFunctionType( &SystemPrimitives::applyChange ), 327681, signature_54, errors_54
};

static const char *signature_55[] = { "Float", "SmallInteger" };
static const char *errors_55[]    = { nullptr };
static PrimitiveDescriptor primitive_55 = {
        "primitiveAsFloat", primitiveFunctionType( &double_from_smi ), 1573377, signature_55, errors_55
};

static const char *signature_56[] = { "SmallInteger", "Object" };
static const char *errors_56[]    = { nullptr };
static PrimitiveDescriptor primitive_56 = {
        "primitiveAsObjectID", primitiveFunctionType( &oopPrimitives::asObjectID ), 1114113, signature_56, errors_56
};

static const char *signature_57[] = { "Object", "SmallInteger" };
static const char *errors_57[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_57 = {
        "primitiveAsObjectIfFail:", primitiveFunctionType( &smiOopPrimitives::asObject ), 1310721, signature_57, errors_57
};

static const char *signature_58[] = { "Object", "Object", "Object" };
static const char *errors_58[]    = { "RecieverHasWrongType", nullptr };
static PrimitiveDescriptor primitive_58 = {
        "primitiveBecome:ifFail:", primitiveFunctionType( &oopPrimitives::become ), 1376258, signature_58, errors_58
};

static const char *signature_59[] = { "GlobalAssociation", "Behavior", "SmallInteger" };
static const char *errors_59[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_59 = {
        "primitiveBehavior:classVariableAt:ifFail:", primitiveFunctionType( &behaviorPrimitives::classVariableAt ), 327682, signature_59, errors_59
};

static const char *signature_60[] = { "IndexedInstanceVariables", "Behavior" };
static const char *errors_60[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_60 = {
        "primitiveBehavior:classVariablesIfFail:", primitiveFunctionType( &behaviorPrimitives::classVariables ), 327681, signature_60, errors_60
};

static const char *signature_61[] = { "Boolean", "Behavior" };
static const char *errors_61[]    = { nullptr };
static PrimitiveDescriptor primitive_61 = {
        "primitiveBehaviorCanBeSubclassed:ifFail:", primitiveFunctionType( &behaviorPrimitives::can_be_subclassed ), 327681, signature_61, errors_61
};

static const char *signature_62[] = { "Boolean", "Behavior" };
static const char *errors_62[]    = { nullptr };
static PrimitiveDescriptor primitive_62 = {
        "primitiveBehaviorCanHaveInstanceVariables:ifFail:", primitiveFunctionType( &behaviorPrimitives::can_have_instance_variables ), 327681, signature_62, errors_62
};

static const char *signature_63[] = { "Symbol", "Behavior" };
static const char *errors_63[]    = { nullptr };
static PrimitiveDescriptor primitive_63 = {
        "primitiveBehaviorFormat:ifFail:", primitiveFunctionType( &behaviorPrimitives::format ), 327681, signature_63, errors_63
};

static const char *signature_64[] = { "SmallInteger", "Behavior" };
static const char *errors_64[]    = { nullptr };
static PrimitiveDescriptor primitive_64 = {
        "primitiveBehaviorHeaderSizeOf:ifFail:", primitiveFunctionType( &behaviorPrimitives::headerSize ), 327681, signature_64, errors_64
};

static const char *signature_65[] = { "Boolean", "Behavior", "Object" };
static const char *errors_65[]    = { nullptr };
static PrimitiveDescriptor primitive_65 = {
        "primitiveBehaviorIsClassOf:", primitiveFunctionType( &behaviorPrimitives::is_class_of ), 5308418, signature_65, errors_65
};

static const char *signature_66[] = { "Boolean", "Behavior" };
static const char *errors_66[]    = { nullptr };
static PrimitiveDescriptor primitive_66 = {
        "primitiveBehaviorIsSpecializedClass:ifFail:", primitiveFunctionType( &behaviorPrimitives::is_specialized_class ), 327681, signature_66, errors_66
};

static const char *signature_67[] = { "Mixin", "Behavior" };
static const char *errors_67[]    = { nullptr };
static PrimitiveDescriptor primitive_67 = {
        "primitiveBehaviorMixinOf:ifFail:", primitiveFunctionType( &behaviorPrimitives::mixinOf ), 327681, signature_67, errors_67
};

static const char *signature_68[] = { "SmallInteger", "Behavior" };
static const char *errors_68[]    = { nullptr };
static PrimitiveDescriptor primitive_68 = {
        "primitiveBehaviorNonIndexableSizeOf:ifFail:", primitiveFunctionType( &behaviorPrimitives::nonIndexableSize ), 327681, signature_68, errors_68
};

static const char *signature_69[] = { "Symbol", "Behavior" };
static const char *errors_69[]    = { nullptr };
static PrimitiveDescriptor primitive_69 = {
        "primitiveBehaviorVMType:ifFail:", primitiveFunctionType( &behaviorPrimitives::vm_type ), 327681, signature_69, errors_69
};

static const char *signature_70[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_70[]    = { nullptr };
static PrimitiveDescriptor primitive_70 = {
        "primitiveBitAnd:ifFail:", primitiveFunctionType( &smiOopPrimitives::bitAnd ), 6029826, signature_70, errors_70
};

static const char *signature_71[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_71[]    = { nullptr };
static PrimitiveDescriptor primitive_71 = {
        "primitiveBitOr:ifFail:", primitiveFunctionType( &smiOopPrimitives::bitOr ), 6029826, signature_71, errors_71
};

static const char *signature_72[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_72[]    = { nullptr };
static PrimitiveDescriptor primitive_72 = {
        "primitiveBitShift:ifFail:", primitiveFunctionType( &smiOopPrimitives::bitShift ), 6029826, signature_72, errors_72
};

static const char *signature_73[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_73[]    = { nullptr };
static PrimitiveDescriptor primitive_73 = {
        "primitiveBitXor:ifFail:", primitiveFunctionType( &smiOopPrimitives::bitXor ), 6029826, signature_73, errors_73
};

static const char *signature_74[] = { "Boolean", "Block" };
static const char *errors_74[]    = { nullptr };
static PrimitiveDescriptor primitive_74 = {
        "primitiveBlockIsOptimized", primitiveFunctionType( &block_is_optimized ), 1114113, signature_74, errors_74
};

static const char *signature_75[] = { "Method", "Block" };
static const char *errors_75[]    = { nullptr };
static PrimitiveDescriptor primitive_75 = {
        "primitiveBlockMethod", primitiveFunctionType( &block_method ), 1114113, signature_75, errors_75
};

static const char *signature_76[] = { "Boolean", "Symbol" };
static const char *errors_76[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_76 = {
        "primitiveBooleanFlagAt:ifFail:", primitiveFunctionType( &debugPrimitives::boolAt ), 327681, signature_76, errors_76
};

static const char *signature_77[] = { "Boolean", "Symbol", "Boolean" };
static const char *errors_77[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_77 = {
        "primitiveBooleanFlagAt:put:ifFail:", primitiveFunctionType( &debugPrimitives::boolAtPut ), 327682, signature_77, errors_77
};

static const char *signature_78[] = { "Object" };
static const char *errors_78[]    = { nullptr };
static PrimitiveDescriptor primitive_78 = {
        "primitiveBreakpoint", primitiveFunctionType( &SystemPrimitives::breakpoint ), 65536, signature_78, errors_78
};

static const char *signature_79[] = { "Object", "Object", "Proxy" };
static const char *errors_79[]    = { nullptr };
static PrimitiveDescriptor primitive_79 = {
        "primitiveCallBackInvokeC2:ifFail:", primitiveFunctionType( &callBackPrimitives::invokeC ), 1376258, signature_79, errors_79
};

static const char *signature_80[] = { "Object", "Object", "Proxy" };
static const char *errors_80[]    = { nullptr };
static PrimitiveDescriptor primitive_80 = {
        "primitiveCallBackInvokePascal2:ifFail:", primitiveFunctionType( &callBackPrimitives::invokePascal ), 1376258, signature_80, errors_80
};

static const char *signature_81[] = { "Object", "Object", "Symbol" };
static const char *errors_81[]    = { nullptr };
static PrimitiveDescriptor primitive_81 = {
        "primitiveCallBackReceiver:selector:ifFail:", primitiveFunctionType( &callBackPrimitives::initialize ), 327682, signature_81, errors_81
};

static const char *signature_82[] = { "Object", "SmallInteger", "Proxy" };
static const char *errors_82[]    = { nullptr };
static PrimitiveDescriptor primitive_82 = {
        "primitiveCallBackRegisterCCall:result:ifFail:", primitiveFunctionType( &callBackPrimitives::registerCCall ), 327682, signature_82, errors_82
};

static const char *signature_83[] = { "Object", "SmallInteger", "SmallInteger", "Proxy" };
static const char *errors_83[]    = { nullptr };
static PrimitiveDescriptor primitive_83 = {
        "primitiveCallBackRegisterPascalCall:numberOfArguments:result:ifFail:", primitiveFunctionType( &callBackPrimitives::registerPascalCall ), 327683, signature_83, errors_83
};

static const char *signature_84[] = { "Object", "Object", "Proxy" };
static const char *errors_84[]    = { nullptr };
static PrimitiveDescriptor primitive_84 = {
        "primitiveCallBackUnregister:ifFail:", primitiveFunctionType( &callBackPrimitives::unregister ), 1376258, signature_84, errors_84
};

static const char *signature_85[] = { "Boolean" };
static const char *errors_85[]    = { nullptr };
static PrimitiveDescriptor primitive_85 = {
        "primitiveCanScavenge", primitiveFunctionType( &SystemPrimitives::canScavenge ), 65536, signature_85, errors_85
};

static const char *signature_86[] = { "Proxy", "SmallInteger" };
static const char *errors_86[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_86 = {
        "primitiveCharacterFor:ifFail:", primitiveFunctionType( &SystemPrimitives::characterFor ), 327681, signature_86, errors_86
};

static const char *signature_87[] = { "Self class", "Object" };
static const char *errors_87[]    = { nullptr };
static PrimitiveDescriptor primitive_87 = {
        "primitiveClass", primitiveFunctionType( &oopPrimitives::klass ), 5767169, signature_87, errors_87
};

static const char *signature_88[] = { "Behavior", "Object" };
static const char *errors_88[]    = { nullptr };
static PrimitiveDescriptor primitive_88 = {
        "primitiveClassOf:", primitiveFunctionType( &oopPrimitives::klass_of ), 4718593, signature_88, errors_88
};

static const char *signature_89[] = { "Object" };
static const char *errors_89[]    = { nullptr };
static PrimitiveDescriptor primitive_89 = {
        "primitiveClearInlineCaches", primitiveFunctionType( &debugPrimitives::clearInlineCaches ), 65536, signature_89, errors_89
};

static const char *signature_90[] = { "Object" };
static const char *errors_90[]    = { nullptr };
static PrimitiveDescriptor primitive_90 = {
        "primitiveClearInvocationCounters", primitiveFunctionType( &debugPrimitives::clearInvocationCounters ), 65536, signature_90, errors_90
};

static const char *signature_91[] = { "Object" };
static const char *errors_91[]    = { nullptr };
static PrimitiveDescriptor primitive_91 = {
        "primitiveClearLookupCache", primitiveFunctionType( &debugPrimitives::clearLookupCache ), 65536, signature_91, errors_91
};

static const char *signature_92[] = { "Object" };
static const char *errors_92[]    = { nullptr };
static PrimitiveDescriptor primitive_92 = {
        "primitiveClearLookupCacheStatistics", primitiveFunctionType( &debugPrimitives::clearLookupCacheStatistics ), 65536, signature_92, errors_92
};

static const char *signature_93[] = { "Object" };
static const char *errors_93[]    = { nullptr };
static PrimitiveDescriptor primitive_93 = {
        "primitiveClearNativeMethodCounters", primitiveFunctionType( &debugPrimitives::clearNativeMethodCounters ), 65536, signature_93, errors_93
};

static const char *signature_94[] = { "Object" };
static const char *errors_94[]    = { nullptr };
static PrimitiveDescriptor primitive_94 = {
        "primitiveClearPrimitiveCounters", primitiveFunctionType( &debugPrimitives::clearPrimitiveCounters ), 65536, signature_94, errors_94
};

static const char *signature_95[] = { "Array[String]" };
static const char *errors_95[]    = { nullptr };
static PrimitiveDescriptor primitive_95 = {
        "primitiveCommandLineArgs", primitiveFunctionType( &SystemPrimitives::command_line_args ), 65536, signature_95, errors_95
};

static const char *signature_96[] = { "Block" };
static const char *errors_96[]    = { nullptr };
static PrimitiveDescriptor primitive_96 = {
        "primitiveCompiledBlockAllocate0", primitiveFunctionType( &allocateBlock0 ), 6359040, signature_96, errors_96
};

static const char *signature_97[] = { "Block" };
static const char *errors_97[]    = { nullptr };
static PrimitiveDescriptor primitive_97 = {
        "primitiveCompiledBlockAllocate1", primitiveFunctionType( &allocateBlock1 ), 6359040, signature_97, errors_97
};

static const char *signature_98[] = { "Block" };
static const char *errors_98[]    = { nullptr };
static PrimitiveDescriptor primitive_98 = {
        "primitiveCompiledBlockAllocate2", primitiveFunctionType( &allocateBlock2 ), 6359040, signature_98, errors_98
};

static const char *signature_99[] = { "Block" };
static const char *errors_99[]    = { nullptr };
static PrimitiveDescriptor primitive_99 = {
        "primitiveCompiledBlockAllocate3", primitiveFunctionType( &allocateBlock3 ), 6359040, signature_99, errors_99
};

static const char *signature_100[] = { "Block" };
static const char *errors_100[]    = { nullptr };
static PrimitiveDescriptor primitive_100 = {
        "primitiveCompiledBlockAllocate4", primitiveFunctionType( &allocateBlock4 ), 6359040, signature_100, errors_100
};

static const char *signature_101[] = { "Block" };
static const char *errors_101[]    = { nullptr };
static PrimitiveDescriptor primitive_101 = {
        "primitiveCompiledBlockAllocate5", primitiveFunctionType( &allocateBlock5 ), 6359040, signature_101, errors_101
};

static const char *signature_102[] = { "Block" };
static const char *errors_102[]    = { nullptr };
static PrimitiveDescriptor primitive_102 = {
        "primitiveCompiledBlockAllocate6", primitiveFunctionType( &allocateBlock6 ), 6359040, signature_102, errors_102
};

static const char *signature_103[] = { "Block" };
static const char *errors_103[]    = { nullptr };
static PrimitiveDescriptor primitive_103 = {
        "primitiveCompiledBlockAllocate7", primitiveFunctionType( &allocateBlock7 ), 6359040, signature_103, errors_103
};

static const char *signature_104[] = { "Block" };
static const char *errors_104[]    = { nullptr };
static PrimitiveDescriptor primitive_104 = {
        "primitiveCompiledBlockAllocate8", primitiveFunctionType( &allocateBlock8 ), 6359040, signature_104, errors_104
};

static const char *signature_105[] = { "Block" };
static const char *errors_105[]    = { nullptr };
static PrimitiveDescriptor primitive_105 = {
        "primitiveCompiledBlockAllocate9", primitiveFunctionType( &allocateBlock9 ), 6359040, signature_105, errors_105
};

static const char *signature_106[] = { "Block", "SmallInteger" };
static const char *errors_106[]    = { nullptr };
static PrimitiveDescriptor primitive_106 = {
        "primitiveCompiledBlockAllocate:", primitiveFunctionType( &allocateBlock ), 2164737, signature_106, errors_106
};

static const char *signature_107[] = { "Object" };
static const char *errors_107[]    = { nullptr };
static PrimitiveDescriptor primitive_107 = {
        "primitiveCompiledContextAllocate0", primitiveFunctionType( &allocateContext0 ), 6359040, signature_107, errors_107
};

static const char *signature_108[] = { "Object" };
static const char *errors_108[]    = { nullptr };
static PrimitiveDescriptor primitive_108 = {
        "primitiveCompiledContextAllocate1", primitiveFunctionType( &allocateContext1 ), 6359040, signature_108, errors_108
};

static const char *signature_109[] = { "Object" };
static const char *errors_109[]    = { nullptr };
static PrimitiveDescriptor primitive_109 = {
        "primitiveCompiledContextAllocate2", primitiveFunctionType( &allocateContext2 ), 6359040, signature_109, errors_109
};

static const char *signature_110[] = { "Object", "SmallInteger" };
static const char *errors_110[]    = { nullptr };
static PrimitiveDescriptor primitive_110 = {
        "primitiveCompiledContextAllocate:", primitiveFunctionType( &allocateContext ), 6359041, signature_110, errors_110
};

static const char *signature_111[] = { "Method", "Object", "SmallInteger", "SmallInteger", "Array", "ByteArray", "Array" };
static const char *errors_111[]    = { nullptr };
static PrimitiveDescriptor primitive_111 = {
        "primitiveConstructMethod:flags:nofArgs:debugInfo:bytes:oops:ifFail:", primitiveFunctionType( &methodOopPrimitives::constructMethod ), 327686, signature_111, errors_111
};

static const char *signature_112[] = { "Object", "Object" };
static const char *errors_112[]    = { "NotOops", nullptr };
static PrimitiveDescriptor primitive_112 = {
        "primitiveCopyTenuredIfFail:", primitiveFunctionType( &oopPrimitives::copy_tenured ), 1376257, signature_112, errors_112
};

static const char *signature_113[] = { "GlobalAssociation", "Mixin", "Symbol", "Boolean", "Behavior", "Symbol" };
static const char *errors_113[]    = { "WrongFormat", nullptr };
static PrimitiveDescriptor primitive_113 = {
        "primitiveCreateInvocationOf:named:isPrimaryInvocation:superclass:format:ifFail:", primitiveFunctionType( &SystemPrimitives::createNamedInvocation ), 327685, signature_113, errors_113
};

static const char *signature_114[] = { "GlobalAssociation", "Mixin", "Behavior", "Symbol" };
static const char *errors_114[]    = { "WrongFormat", nullptr };
static PrimitiveDescriptor primitive_114 = {
        "primitiveCreateInvocationOf:superclass:format:ifFail:", primitiveFunctionType( &SystemPrimitives::createInvocation ), 327683, signature_114, errors_114
};

static const char *signature_115[] = { "SmallInteger" };
static const char *errors_115[]    = { nullptr };
static PrimitiveDescriptor primitive_115 = {
        "primitiveCurrentThreadId", primitiveFunctionType( &SystemPrimitives::current_thread_id ), 65536, signature_115, errors_115
};

static const char *signature_116[] = { "Proxy", "Symbol", "Proxy" };
static const char *errors_116[]    = { nullptr };
static PrimitiveDescriptor primitive_116 = {
        "primitiveDLLLoad:result:ifFail:", primitiveFunctionType( &SystemPrimitives::dll_load ), 327682, signature_116, errors_116
};

static const char *signature_117[] = { "Proxy", "Symbol", "Proxy", "Proxy" };
static const char *errors_117[]    = { nullptr };
static PrimitiveDescriptor primitive_117 = {
        "primitiveDLLLookup:in:result:ifFail:", primitiveFunctionType( &SystemPrimitives::dll_lookup ), 327683, signature_117, errors_117
};

static const char *signature_118[] = { "Object", "Object", "Symbol" };
static const char *errors_118[]    = { nullptr };
static PrimitiveDescriptor primitive_118 = {
        "primitiveDLLSetupLookup:selector:ifFail:", primitiveFunctionType( &SystemPrimitives::dll_setup ), 327682, signature_118, errors_118
};

static const char *signature_119[] = { "Object", "Proxy" };
static const char *errors_119[]    = { nullptr };
static PrimitiveDescriptor primitive_119 = {
        "primitiveDLLUnload:ifFail:", primitiveFunctionType( &SystemPrimitives::dll_unload ), 327681, signature_119, errors_119
};

static const char *signature_120[] = { "Object" };
static const char *errors_120[]    = { nullptr };
static PrimitiveDescriptor primitive_120 = {
        "primitiveDecodeAllMethods", primitiveFunctionType( &debugPrimitives::decodeAllMethods ), 65536, signature_120, errors_120
};

static const char *signature_121[] = { "Object", "Object", "Symbol" };
static const char *errors_121[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_121 = {
        "primitiveDecodeMethod:ifFail:", primitiveFunctionType( &debugPrimitives::decodeMethod ), 1376258, signature_121, errors_121
};

static const char *signature_122[] = { "Proxy", "Proxy" };
static const char *errors_122[]    = { nullptr };
static PrimitiveDescriptor primitive_122 = {
        "primitiveDefWindowProc:ifFail:", primitiveFunctionType( &SystemPrimitives::defWindowProc ), 327681, signature_122, errors_122
};

static const char *signature_123[] = { "Object" };
static const char *errors_123[]    = { nullptr };
static PrimitiveDescriptor primitive_123 = {
        "primitiveDeoptimizeStacks", primitiveFunctionType( &debugPrimitives::deoptimizeStacks ), 196608, signature_123, errors_123
};

static const char *signature_124[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_124[]    = { "Overflow", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_124 = {
        "primitiveDiv:ifFail:", primitiveFunctionType( &smiOopPrimitives_div ), 6029826, signature_124, errors_124
};

static const char *signature_125[] = { "Float" };
static const char *errors_125[]    = { nullptr };
static PrimitiveDescriptor primitive_125 = {
        "primitiveElapsedTime", primitiveFunctionType( &SystemPrimitives::elapsedTime ), 65536, signature_125, errors_125
};

static const char *signature_126[] = { "Boolean", "Object", "Object" };
static const char *errors_126[]    = { nullptr };
static PrimitiveDescriptor primitive_126 = {
        "primitiveEqual:", primitiveFunctionType( &oopPrimitives::equal ), 1572866, signature_126, errors_126
};

static const char *signature_127[] = { "Object", "SmallInteger" };
static const char *errors_127[]    = { nullptr };
static PrimitiveDescriptor primitive_127 = {
        "primitiveExpandMemory:", primitiveFunctionType( &SystemPrimitives::expandMemory ), 65537, signature_127, errors_127
};

static const char *signature_128[] = { "SmallInteger" };
static const char *errors_128[]    = { nullptr };
static PrimitiveDescriptor primitive_128 = {
        "primitiveExpansions", primitiveFunctionType( &SystemPrimitives::expansions ), 65536, signature_128, errors_128
};

static const char *signature_129[] = { "Process|nil" };
static const char *errors_129[]    = { nullptr };
static PrimitiveDescriptor primitive_129 = {
        "primitiveFlatProfilerDisengage", primitiveFunctionType( &SystemPrimitives::flat_profiler_disengage ), 65536, signature_129, errors_129
};

static const char *signature_130[] = { "Process", "Process" };
static const char *errors_130[]    = { nullptr };
static PrimitiveDescriptor primitive_130 = {
        "primitiveFlatProfilerEngage:ifFail:", primitiveFunctionType( &SystemPrimitives::flat_profiler_engage ), 327681, signature_130, errors_130
};

static const char *signature_131[] = { "Object" };
static const char *errors_131[]    = { nullptr };
static PrimitiveDescriptor primitive_131 = {
        "primitiveFlatProfilerPrint", primitiveFunctionType( &SystemPrimitives::flat_profiler_print ), 65536, signature_131, errors_131
};

static const char *signature_132[] = { "Process|nil" };
static const char *errors_132[]    = { nullptr };
static PrimitiveDescriptor primitive_132 = {
        "primitiveFlatProfilerProcess", primitiveFunctionType( &SystemPrimitives::flat_profiler_process ), 65536, signature_132, errors_132
};

static const char *signature_133[] = { "Object" };
static const char *errors_133[]    = { nullptr };
static PrimitiveDescriptor primitive_133 = {
        "primitiveFlatProfilerReset", primitiveFunctionType( &SystemPrimitives::flat_profiler_reset ), 65536, signature_133, errors_133
};

static const char *signature_134[] = { "Float", "Float", "Float" };
static const char *errors_134[]    = { nullptr };
static PrimitiveDescriptor primitive_134 = {
        "primitiveFloatAdd:ifFail:", primitiveFunctionType( &double_add ), 6030338, signature_134, errors_134
};

static const char *signature_135[] = { "Float", "Float" };
static const char *errors_135[]    = { nullptr };
static PrimitiveDescriptor primitive_135 = {
        "primitiveFloatArcCosineIfFail:", primitiveFunctionType( &doubleOopPrimitives::arcCosine ), 1836033, signature_135, errors_135
};

static const char *signature_136[] = { "Float", "Float" };
static const char *errors_136[]    = { nullptr };
static PrimitiveDescriptor primitive_136 = {
        "primitiveFloatArcSineIfFail:", primitiveFunctionType( &doubleOopPrimitives::arcSine ), 1836033, signature_136, errors_136
};

static const char *signature_137[] = { "Float", "Float" };
static const char *errors_137[]    = { nullptr };
static PrimitiveDescriptor primitive_137 = {
        "primitiveFloatArcTangentIfFail:", primitiveFunctionType( &doubleOopPrimitives::arcTangent ), 1836033, signature_137, errors_137
};

static const char *signature_138[] = { "SmallInteger", "Float" };
static const char *errors_138[]    = { "SmallIntegerConversionFailed", nullptr };
static PrimitiveDescriptor primitive_138 = {
        "primitiveFloatAsSmallIntegerIfFail:", primitiveFunctionType( &doubleOopPrimitives::asSmallInteger ), 1836033, signature_138, errors_138
};

static const char *signature_139[] = { "Float", "Float" };
static const char *errors_139[]    = { nullptr };
static PrimitiveDescriptor primitive_139 = {
        "primitiveFloatCeiling", primitiveFunctionType( &doubleOopPrimitives::ceiling ), 1573889, signature_139, errors_139
};

static const char *signature_140[] = { "Float", "Float" };
static const char *errors_140[]    = { nullptr };
static PrimitiveDescriptor primitive_140 = {
        "primitiveFloatCosine", primitiveFunctionType( &doubleOopPrimitives::cosine ), 1573889, signature_140, errors_140
};

static const char *signature_141[] = { "Float", "Float", "Float" };
static const char *errors_141[]    = { nullptr };
static PrimitiveDescriptor primitive_141 = {
        "primitiveFloatDivide:ifFail:", primitiveFunctionType( &double_divide ), 6030338, signature_141, errors_141
};

static const char *signature_142[] = { "Boolean", "Float", "Float" };
static const char *errors_142[]    = { nullptr };
static PrimitiveDescriptor primitive_142 = {
        "primitiveFloatEqual:ifFail:", primitiveFunctionType( &doubleOopPrimitives::equal ), 6030082, signature_142, errors_142
};

static const char *signature_143[] = { "Float", "Float" };
static const char *errors_143[]    = { nullptr };
static PrimitiveDescriptor primitive_143 = {
        "primitiveFloatExp", primitiveFunctionType( &doubleOopPrimitives::exp ), 1573889, signature_143, errors_143
};

static const char *signature_144[] = { "SmallInteger", "Float" };
static const char *errors_144[]    = { nullptr };
static PrimitiveDescriptor primitive_144 = {
        "primitiveFloatExponent", primitiveFunctionType( &doubleOopPrimitives::exponent ), 1573889, signature_144, errors_144
};

static const char *signature_145[] = { "Float", "Float" };
static const char *errors_145[]    = { nullptr };
static PrimitiveDescriptor primitive_145 = {
        "primitiveFloatFloor", primitiveFunctionType( &doubleOopPrimitives::floor ), 1573889, signature_145, errors_145
};

static const char *signature_146[] = { "Boolean", "Float", "Float" };
static const char *errors_146[]    = { nullptr };
static PrimitiveDescriptor primitive_146 = {
        "primitiveFloatGreaterThan:ifFail:", primitiveFunctionType( &doubleOopPrimitives::greaterThan ), 6030082, signature_146, errors_146
};

static const char *signature_147[] = { "Boolean", "Float", "Float" };
static const char *errors_147[]    = { nullptr };
static PrimitiveDescriptor primitive_147 = {
        "primitiveFloatGreaterThanOrEqual:ifFail:", primitiveFunctionType( &doubleOopPrimitives::greaterThanOrEqual ), 6030082, signature_147, errors_147
};

static const char *signature_148[] = { "Float", "Float" };
static const char *errors_148[]    = { nullptr };
static PrimitiveDescriptor primitive_148 = {
        "primitiveFloatHyperbolicCosineIfFail:", primitiveFunctionType( &doubleOopPrimitives::hyperbolicCosine ), 1836033, signature_148, errors_148
};

static const char *signature_149[] = { "Float", "Float" };
static const char *errors_149[]    = { nullptr };
static PrimitiveDescriptor primitive_149 = {
        "primitiveFloatHyperbolicSineIfFail:", primitiveFunctionType( &doubleOopPrimitives::hyperbolicSine ), 1836033, signature_149, errors_149
};

static const char *signature_150[] = { "Float", "Float" };
static const char *errors_150[]    = { nullptr };
static PrimitiveDescriptor primitive_150 = {
        "primitiveFloatHyperbolicTangentIfFail:", primitiveFunctionType( &doubleOopPrimitives::hyperbolicTangent ), 1836033, signature_150, errors_150
};

static const char *signature_151[] = { "Boolean", "Float" };
static const char *errors_151[]    = { nullptr };
static PrimitiveDescriptor primitive_151 = {
        "primitiveFloatIsFinite", primitiveFunctionType( &doubleOopPrimitives::isFinite ), 1573889, signature_151, errors_151
};

static const char *signature_152[] = { "Boolean", "Float" };
static const char *errors_152[]    = { nullptr };
static PrimitiveDescriptor primitive_152 = {
        "primitiveFloatIsNan", primitiveFunctionType( &doubleOopPrimitives::isNan ), 1573889, signature_152, errors_152
};

static const char *signature_153[] = { "Boolean", "Float", "Float" };
static const char *errors_153[]    = { nullptr };
static PrimitiveDescriptor primitive_153 = {
        "primitiveFloatLessThan:ifFail:", primitiveFunctionType( &doubleOopPrimitives::lessThan ), 6030082, signature_153, errors_153
};

static const char *signature_154[] = { "Boolean", "Float", "Float" };
static const char *errors_154[]    = { nullptr };
static PrimitiveDescriptor primitive_154 = {
        "primitiveFloatLessThanOrEqual:ifFail:", primitiveFunctionType( &doubleOopPrimitives::lessThanOrEqual ), 6030082, signature_154, errors_154
};

static const char *signature_155[] = { "Float", "Float" };
static const char *errors_155[]    = { "ReceiverNotStrictlyPositive", nullptr };
static PrimitiveDescriptor primitive_155 = {
        "primitiveFloatLnIfFail:", primitiveFunctionType( &doubleOopPrimitives::ln ), 1836033, signature_155, errors_155
};

static const char *signature_156[] = { "Float", "Float" };
static const char *errors_156[]    = { "ReceiverNotStrictlyPositive", nullptr };
static PrimitiveDescriptor primitive_156 = {
        "primitiveFloatLog10IfFail:", primitiveFunctionType( &doubleOopPrimitives::log10 ), 1836033, signature_156, errors_156
};

static const char *signature_157[] = { "Float", "Float" };
static const char *errors_157[]    = { nullptr };
static PrimitiveDescriptor primitive_157 = {
        "primitiveFloatMantissa", primitiveFunctionType( &doubleOopPrimitives::mantissa ), 1573889, signature_157, errors_157
};

static const char *signature_158[] = { "Float" };
static const char *errors_158[]    = { nullptr };
static PrimitiveDescriptor primitive_158 = {
        "primitiveFloatMaxValue", primitiveFunctionType( &doubleOopPrimitives::min_positive_value ), 524288, signature_158, errors_158
};

static const char *signature_159[] = { "Float" };
static const char *errors_159[]    = { nullptr };
static PrimitiveDescriptor primitive_159 = {
        "primitiveFloatMinPositiveValue", primitiveFunctionType( &doubleOopPrimitives::min_positive_value ), 524288, signature_159, errors_159
};

static const char *signature_160[] = { "Float", "Float", "Float" };
static const char *errors_160[]    = { nullptr };
static PrimitiveDescriptor primitive_160 = {
        "primitiveFloatMod:ifFail:", primitiveFunctionType( &doubleOopPrimitives::mod ), 1836034, signature_160, errors_160
};

static const char *signature_161[] = { "Float", "Float", "Float" };
static const char *errors_161[]    = { nullptr };
static PrimitiveDescriptor primitive_161 = {
        "primitiveFloatMultiply:ifFail:", primitiveFunctionType( &double_multiply ), 6030338, signature_161, errors_161
};

static const char *signature_162[] = { "Boolean", "Float", "Float" };
static const char *errors_162[]    = { nullptr };
static PrimitiveDescriptor primitive_162 = {
        "primitiveFloatNotEqual:ifFail:", primitiveFunctionType( &doubleOopPrimitives::notEqual ), 6030082, signature_162, errors_162
};

static const char *signature_163[] = { "Self", "Float", "IndexedByteInstanceVariables" };
static const char *errors_163[]    = { nullptr };
static PrimitiveDescriptor primitive_163 = {
        "primitiveFloatPrintFormat:ifFail:", primitiveFunctionType( &doubleOopPrimitives::printFormat ), 1310722, signature_163, errors_163
};

static const char *signature_164[] = { "IndexedByteInstanceVariables", "Float" };
static const char *errors_164[]    = { nullptr };
static PrimitiveDescriptor primitive_164 = {
        "primitiveFloatPrintString", primitiveFunctionType( &doubleOopPrimitives::printString ), 1048577, signature_164, errors_164
};

static const char *signature_165[] = { "SmallInteger", "Float" };
static const char *errors_165[]    = { "SmallIntegerConversionFailed", nullptr };
static PrimitiveDescriptor primitive_165 = {
        "primitiveFloatRoundedAsSmallIntegerIfFail:", primitiveFunctionType( &doubleOopPrimitives::roundedAsSmallInteger ), 1836033, signature_165, errors_165
};

static const char *signature_166[] = { "Float", "Float" };
static const char *errors_166[]    = { nullptr };
static PrimitiveDescriptor primitive_166 = {
        "primitiveFloatSine", primitiveFunctionType( &doubleOopPrimitives::sine ), 1573889, signature_166, errors_166
};

static const char *signature_167[] = { "SmallInteger", "Float" };
static const char *errors_167[]    = { "ConversionFailed", nullptr };
static PrimitiveDescriptor primitive_167 = {
        "primitiveFloatSmallIntegerFloorIfFail:", primitiveFunctionType( &doubleOopPrimitives::smi_floor ), 6030337, signature_167, errors_167
};

static const char *signature_168[] = { "Float", "Float" };
static const char *errors_168[]    = { "ReceiverNegative", nullptr };
static PrimitiveDescriptor primitive_168 = {
        "primitiveFloatSqrtIfFail:", primitiveFunctionType( &doubleOopPrimitives::sqrt ), 1836033, signature_168, errors_168
};

static const char *signature_169[] = { "Float", "Float" };
static const char *errors_169[]    = { nullptr };
static PrimitiveDescriptor primitive_169 = {
        "primitiveFloatSquared", primitiveFunctionType( &doubleOopPrimitives::squared ), 1573889, signature_169, errors_169
};

static const char *signature_170[] = { "ByteArray", "Float" };
static const char *errors_170[]    = { nullptr };
static PrimitiveDescriptor primitive_170 = {
        "primitiveFloatStoreString", primitiveFunctionType( &doubleOopPrimitives::store_string ), 1048577, signature_170, errors_170
};

static const char *signature_171[] = { "Float", "Float", "Float" };
static const char *errors_171[]    = { nullptr };
static PrimitiveDescriptor primitive_171 = {
        "primitiveFloatSubtract:ifFail:", primitiveFunctionType( &double_subtract ), 6030338, signature_171, errors_171
};

static const char *signature_172[] = { "Float", "Float" };
static const char *errors_172[]    = { nullptr };
static PrimitiveDescriptor primitive_172 = {
        "primitiveFloatTangentIfFail:", primitiveFunctionType( &doubleOopPrimitives::tangent ), 1836033, signature_172, errors_172
};

static const char *signature_173[] = { "Float", "Float", "SmallInteger" };
static const char *errors_173[]    = { "RangeError", nullptr };
static PrimitiveDescriptor primitive_173 = {
        "primitiveFloatTimesTwoPower:ifFail:", primitiveFunctionType( &doubleOopPrimitives::timesTwoPower ), 1836034, signature_173, errors_173
};

static const char *signature_174[] = { "Float", "Float" };
static const char *errors_174[]    = { nullptr };
static PrimitiveDescriptor primitive_174 = {
        "primitiveFloatTruncated", primitiveFunctionType( &doubleOopPrimitives::truncated ), 1573889, signature_174, errors_174
};

static const char *signature_175[] = { "Object" };
static const char *errors_175[]    = { nullptr };
static PrimitiveDescriptor primitive_175 = {
        "primitiveFlushCodeCache", primitiveFunctionType( &SystemPrimitives::flush_code_cache ), 65536, signature_175, errors_175
};

static const char *signature_176[] = { "Object" };
static const char *errors_176[]    = { nullptr };
static PrimitiveDescriptor primitive_176 = {
        "primitiveFlushDeadCode", primitiveFunctionType( &SystemPrimitives::flush_dead_code ), 65536, signature_176, errors_176
};

static const char *signature_177[] = { "SmallInteger" };
static const char *errors_177[]    = { nullptr };
static PrimitiveDescriptor primitive_177 = {
        "primitiveFreeSpace", primitiveFunctionType( &SystemPrimitives::freeSpace ), 65536, signature_177, errors_177
};

static const char *signature_178[] = { "Self", "Object" };
static const char *errors_178[]    = { nullptr };
static PrimitiveDescriptor primitive_178 = {
        "primitiveGarbageCollect", primitiveFunctionType( &SystemPrimitives::garbageGollect ), 1114113, signature_178, errors_178
};

static const char *signature_179[] = { "Object", "Object", "Symbol" };
static const char *errors_179[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_179 = {
        "primitiveGenerateIR:ifFail:", primitiveFunctionType( &debugPrimitives::generateIR ), 1376258, signature_179, errors_179
};

static const char *signature_180[] = { "Integer" };
static const char *errors_180[]    = { nullptr };
static PrimitiveDescriptor primitive_180 = {
        "primitiveGetLastError", primitiveFunctionType( &SystemPrimitives::getLastError ), 65536, signature_180, errors_180
};

static const char *signature_181[] = { "Boolean", "GlobalAssociation" };
static const char *errors_181[]    = { nullptr };
static PrimitiveDescriptor primitive_181 = {
        "primitiveGlobalAssociationIsConstant", primitiveFunctionType( &SystemPrimitives::globalAssociationIsConstant ), 1114113, signature_181, errors_181
};

static const char *signature_182[] = { "Symbol", "GlobalAssociation" };
static const char *errors_182[]    = { nullptr };
static PrimitiveDescriptor primitive_182 = {
        "primitiveGlobalAssociationKey", primitiveFunctionType( &SystemPrimitives::globalAssociationKey ), 1114113, signature_182, errors_182
};

static const char *signature_183[] = { "Boolean", "GlobalAssociation", "Boolean" };
static const char *errors_183[]    = { nullptr };
static PrimitiveDescriptor primitive_183 = {
        "primitiveGlobalAssociationSetConstant:", primitiveFunctionType( &SystemPrimitives::globalAssociationSetConstant ), 1114114, signature_183, errors_183
};

static const char *signature_184[] = { "Object", "GlobalAssociation", "Symbol" };
static const char *errors_184[]    = { nullptr };
static PrimitiveDescriptor primitive_184 = {
        "primitiveGlobalAssociationSetKey:", primitiveFunctionType( &SystemPrimitives::globalAssociationSetKey ), 1114114, signature_184, errors_184
};

static const char *signature_185[] = { "Object", "GlobalAssociation", "Object" };
static const char *errors_185[]    = { nullptr };
static PrimitiveDescriptor primitive_185 = {
        "primitiveGlobalAssociationSetValue:", primitiveFunctionType( &SystemPrimitives::globalAssociationSetValue ), 1114114, signature_185, errors_185
};

static const char *signature_186[] = { "Object", "GlobalAssociation" };
static const char *errors_186[]    = { nullptr };
static PrimitiveDescriptor primitive_186 = {
        "primitiveGlobalAssociationValue", primitiveFunctionType( &SystemPrimitives::globalAssociationValue ), 1114113, signature_186, errors_186
};

static const char *signature_187[] = { "Boolean", "SmallInteger", "SmallInteger" };
static const char *errors_187[]    = { nullptr };
static PrimitiveDescriptor primitive_187 = {
        "primitiveGreaterThan:ifFail:", primitiveFunctionType( &smiOopPrimitives::greaterThan ), 6029570, signature_187, errors_187
};

static const char *signature_188[] = { "Boolean", "SmallInteger", "SmallInteger" };
static const char *errors_188[]    = { nullptr };
static PrimitiveDescriptor primitive_188 = {
        "primitiveGreaterThanOrEqual:ifFail:", primitiveFunctionType( &smiOopPrimitives::greaterThanOrEqual ), 6029570, signature_188, errors_188
};

static const char *signature_189[] = { "Boolean", "Object" };
static const char *errors_189[]    = { nullptr };
static PrimitiveDescriptor primitive_189 = {
        "primitiveHadNearDeathExperience:", primitiveFunctionType( &SystemPrimitives::hadNearDeathExperience ), 65537, signature_189, errors_189
};

static const char *signature_190[] = { "Object" };
static const char *errors_190[]    = { nullptr };
static PrimitiveDescriptor primitive_190 = {
        "primitiveHalt", primitiveFunctionType( &SystemPrimitives::halt ), 65536, signature_190, errors_190
};

static const char *signature_191[] = { "SmallInteger", "Object" };
static const char *errors_191[]    = { nullptr };
static PrimitiveDescriptor primitive_191 = {
        "primitiveHash", primitiveFunctionType( &oopPrimitives::hash ), 1114113, signature_191, errors_191
};

static const char *signature_192[] = { "SmallInteger", "Object" };
static const char *errors_192[]    = { nullptr };
static PrimitiveDescriptor primitive_192 = {
        "primitiveHashOf:", primitiveFunctionType( &oopPrimitives::hash_of ), 65537, signature_192, errors_192
};

static const char *signature_193[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_193[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_193 = {
        "primitiveIndexedByteAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::at ), 1312258, signature_193, errors_193
};

static const char *signature_194[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger", "SmallInteger" };
static const char *errors_194[]    = { "OutOfBounds", "ValueOutOfBounds", nullptr };
static PrimitiveDescriptor primitive_194 = {
        "primitiveIndexedByteAt:put:ifFail:", primitiveFunctionType( &byteArrayPrimitives::atPut ), 1312259, signature_194, errors_194
};

static const char *signature_195[] = { "Self", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_195[]    = { nullptr };
static PrimitiveDescriptor primitive_195 = {
        "primitiveIndexedByteAtAllPut:ifFail:", primitiveFunctionType( &byteArrayPrimitives::at_all_put ), 1376258, signature_195, errors_195
};

static const char *signature_196[] = { "SmallInteger", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_196[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_196 = {
        "primitiveIndexedByteCharacterAt:ifFail:", primitiveFunctionType( &byteArrayPrimitives::characterAt ), 1312258, signature_196, errors_196
};

static const char *signature_197[] = { "SmallInteger", "IndexedByteInstanceVariables", "String" };
static const char *errors_197[]    = { nullptr };
static PrimitiveDescriptor primitive_197 = {
        "primitiveIndexedByteCompare:ifFail:", primitiveFunctionType( &byteArrayPrimitives::compare ), 1376258, signature_197, errors_197
};

static const char *signature_198[] = { "SmallInteger", "IndexedByteInstanceVariables" };
static const char *errors_198[]    = { nullptr };
static PrimitiveDescriptor primitive_198 = {
        "primitiveIndexedByteHash", primitiveFunctionType( &byteArrayPrimitives::hash ), 1574401, signature_198, errors_198
};

static const char *signature_199[] = { "CompressedSymbol", "IndexedByteInstanceVariables" };
static const char *errors_199[]    = { "ValueOutOfBounds", nullptr };
static PrimitiveDescriptor primitive_199 = {
        "primitiveIndexedByteInternIfFail:", primitiveFunctionType( &byteArrayPrimitives::intern ), 1376257, signature_199, errors_199
};

static const char *signature_200[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_200[]    = { "ArgumentIsInvalid", nullptr };
static PrimitiveDescriptor primitive_200 = {
        "primitiveIndexedByteLargeIntegerAdd:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerAdd ), 1312258, signature_200, errors_200
};

static const char *signature_201[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_201[]    = { "ArgumentIsInvalid", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_201 = {
        "primitiveIndexedByteLargeIntegerAnd:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerAnd ), 1312258, signature_201, errors_201
};

static const char *signature_202[] = { "Float", "IndexedByteInstanceVariables" };
static const char *errors_202[]    = { nullptr };
static PrimitiveDescriptor primitive_202 = {
        "primitiveIndexedByteLargeIntegerAsFloatIfFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerToFloat ), 1312257, signature_202, errors_202
};

static const char *signature_203[] = { "SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_203[]    = { nullptr };
static PrimitiveDescriptor primitive_203 = {
        "primitiveIndexedByteLargeIntegerCompare:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerCompare ), 1312258, signature_203, errors_203
};

static const char *signature_204[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_204[]    = { "ArgumentIsInvalid", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_204 = {
        "primitiveIndexedByteLargeIntegerDiv:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerDiv ), 1312258, signature_204, errors_204
};

static const char *signature_205[] = { "IndexedByteInstanceVariables", "IndexedByteInstanceVariables class", "Float" };
static const char *errors_205[]    = { nullptr };
static PrimitiveDescriptor primitive_205 = {
        "primitiveIndexedByteLargeIntegerFromFloat:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerFromDouble ), 1310722, signature_205, errors_205
};

static const char *signature_206[] = { "IndexedByteInstanceVariables", "IndexedByteInstanceVariables class", "SmallInteger" };
static const char *errors_206[]    = { nullptr };
static PrimitiveDescriptor primitive_206 = {
        "primitiveIndexedByteLargeIntegerFromSmallInteger:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerFromSmallInteger ), 1310722, signature_206, errors_206
};

static const char *signature_207[] = { "IndexedByteInstanceVariables", "IndexedByteInstanceVariables class", "String", "Integer" };
static const char *errors_207[]    = { "ConversionFailed", nullptr };
static PrimitiveDescriptor primitive_207 = {
        "primitiveIndexedByteLargeIntegerFromString:base:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerFromString ), 1312259, signature_207, errors_207
};

static const char *signature_208[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_208[]    = { "ArgumentIsInvalid", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_208 = {
        "primitiveIndexedByteLargeIntegerMod:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerMod ), 1312258, signature_208, errors_208
};

static const char *signature_209[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_209[]    = { "ArgumentIsInvalid", nullptr };
static PrimitiveDescriptor primitive_209 = {
        "primitiveIndexedByteLargeIntegerMultiply:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerMultiply ), 1312258, signature_209, errors_209
};

static const char *signature_210[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_210[]    = { "ArgumentIsInvalid", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_210 = {
        "primitiveIndexedByteLargeIntegerOr:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerOr ), 1312258, signature_210, errors_210
};

static const char *signature_211[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_211[]    = { "ArgumentIsInvalid", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_211 = {
        "primitiveIndexedByteLargeIntegerQuo:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerQuo ), 1312258, signature_211, errors_211
};

static const char *signature_212[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_212[]    = { "ArgumentIsInvalid", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_212 = {
        "primitiveIndexedByteLargeIntegerRem:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerRem ), 1312258, signature_212, errors_212
};

static const char *signature_213[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "SmallInt" };
static const char *errors_213[]    = { "ArgumentIsInvalid", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_213 = {
        "primitiveIndexedByteLargeIntegerShift:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerShift ), 1312258, signature_213, errors_213
};

static const char *signature_214[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_214[]    = { "ArgumentIsInvalid", nullptr };
static PrimitiveDescriptor primitive_214 = {
        "primitiveIndexedByteLargeIntegerSubtract:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerSubtract ), 1312258, signature_214, errors_214
};

static const char *signature_215[] = { "String", "IndexedByteInstanceVariables", "SmallInteger" };
static const char *errors_215[]    = { nullptr };
static PrimitiveDescriptor primitive_215 = {
        "primitiveIndexedByteLargeIntegerToStringBase:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerToString ), 1312258, signature_215, errors_215
};

static const char *signature_216[] = { "IndexedByteInstanceVariables|SmallInteger", "IndexedByteInstanceVariables", "IndexedByteInstanceVariables" };
static const char *errors_216[]    = { "ArgumentIsInvalid", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_216 = {
        "primitiveIndexedByteLargeIntegerXor:ifFail:", primitiveFunctionType( &byteArrayPrimitives::largeIntegerXor ), 1312258, signature_216, errors_216
};

static const char *signature_217[] = { "Object", "IndexedByteInstanceVariables class", "SmallInteger" };
static const char *errors_217[]    = { "NegativeSize", nullptr };
static PrimitiveDescriptor primitive_217 = {
        "primitiveIndexedByteNew:ifFail:", primitiveFunctionType( &byteArrayPrimitives::allocateSize ), 1376258, signature_217, errors_217
};

static const char *signature_218[] = { "Object", "IndexedByteInstanceVariables class", "SmallInteger", "Boolean" };
static const char *errors_218[]    = { "NegativeSize", nullptr };
static PrimitiveDescriptor primitive_218 = {
        "primitiveIndexedByteNew:size:tenured:ifFail:", primitiveFunctionType( &byteArrayPrimitives::allocateSize2 ), 327683, signature_218, errors_218
};

static const char *signature_219[] = { "SmallInteger", "IndexedByteInstanceVariables" };
static const char *errors_219[]    = { nullptr };
static PrimitiveDescriptor primitive_219 = {
        "primitiveIndexedByteSize", primitiveFunctionType( &byteArrayPrimitives::size ), 1574401, signature_219, errors_219
};

static const char *signature_220[] = { "SmallInteger", "IndexedDoubleByteInstanceVariables", "SmallInteger" };
static const char *errors_220[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_220 = {
        "primitiveIndexedDoubleByteAt:ifFail:", primitiveFunctionType( &doubleByteArrayPrimitives::at ), 1312514, signature_220, errors_220
};

static const char *signature_221[] = { "SmallInteger", "IndexedDoubleByteInstanceVariables", "SmallInteger", "SmallInteger" };
static const char *errors_221[]    = { "OutOfBounds", "ValueOutOfBounds", nullptr };
static PrimitiveDescriptor primitive_221 = {
        "primitiveIndexedDoubleByteAt:put:ifFail:", primitiveFunctionType( &doubleByteArrayPrimitives::atPut ), 1312515, signature_221, errors_221
};

static const char *signature_222[] = { "SmallInteger", "IndexedDoubleByteInstanceVariables", "SmallInteger" };
static const char *errors_222[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_222 = {
        "primitiveIndexedDoubleByteCharacterAt:ifFail:", primitiveFunctionType( &doubleByteArrayPrimitives::characterAt ), 1312514, signature_222, errors_222
};

static const char *signature_223[] = { "SmallInteger", "IndexedDoubleByteInstanceVariables", "String" };
static const char *errors_223[]    = { nullptr };
static PrimitiveDescriptor primitive_223 = {
        "primitiveIndexedDoubleByteCompare:ifFail:", primitiveFunctionType( &doubleByteArrayPrimitives::compare ), 1310722, signature_223, errors_223
};

static const char *signature_224[] = { "SmallInteger", "IndexedDoubleByteInstanceVariables" };
static const char *errors_224[]    = { nullptr };
static PrimitiveDescriptor primitive_224 = {
        "primitiveIndexedDoubleByteHash", primitiveFunctionType( &doubleByteArrayPrimitives::hash ), 1114113, signature_224, errors_224
};

static const char *signature_225[] = { "CompressedSymbol", "IndexedDoubleByteInstanceVariables" };
static const char *errors_225[]    = { "ValueOutOfBounds", nullptr };
static PrimitiveDescriptor primitive_225 = {
        "primitiveIndexedDoubleByteInternIfFail:", primitiveFunctionType( &doubleByteArrayPrimitives::intern ), 1376257, signature_225, errors_225
};

static const char *signature_226[] = { "Object", "IndexedDoubleByteInstanceVariables class", "SmallInteger" };
static const char *errors_226[]    = { "NegativeSize", nullptr };
static PrimitiveDescriptor primitive_226 = {
        "primitiveIndexedDoubleByteNew:ifFail:", primitiveFunctionType( &doubleByteArrayPrimitives::allocateSize ), 1376258, signature_226, errors_226
};

static const char *signature_227[] = { "Object", "IndexedDoubleByteInstanceVariables class", "SmallInteger", "Boolean" };
static const char *errors_227[]    = { "NegativeSize", nullptr };
static PrimitiveDescriptor primitive_227 = {
        "primitiveIndexedDoubleByteNew:size:tenured:ifFail:", primitiveFunctionType( &doubleByteArrayPrimitives::allocateSize2 ), 327683, signature_227, errors_227
};

static const char *signature_228[] = { "SmallInteger", "IndexedDoubleByteInstanceVariables" };
static const char *errors_228[]    = { nullptr };
static PrimitiveDescriptor primitive_228 = {
        "primitiveIndexedDoubleByteSize", primitiveFunctionType( &doubleByteArrayPrimitives::size ), 1574657, signature_228, errors_228
};

static const char *signature_229[] = { "Float", "IndexedFloatValueInstanceVariables", "SmallInteger" };
static const char *errors_229[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_229 = {
        "primitiveIndexedFloatValueAt:ifFail:", primitiveFunctionType( &doubleValueArrayPrimitives::at ), 1310722, signature_229, errors_229
};

static const char *signature_230[] = { "Float", "IndexedFloatValueInstanceVariables", "SmallInteger", "Float" };
static const char *errors_230[]    = { "OutOfBounds", "ValueOutOfBounds", nullptr };
static PrimitiveDescriptor primitive_230 = {
        "primitiveIndexedFloatValueAt:put:ifFail:", primitiveFunctionType( &doubleValueArrayPrimitives::atPut ), 1310723, signature_230, errors_230
};

static const char *signature_231[] = { "Object", "IndexedFloatValueInstanceVariables class", "SmallInteger" };
static const char *errors_231[]    = { "NegativeSize", nullptr };
static PrimitiveDescriptor primitive_231 = {
        "primitiveIndexedFloatValueNew:ifFail:", primitiveFunctionType( &doubleValueArrayPrimitives::allocateSize ), 1376258, signature_231, errors_231
};

static const char *signature_232[] = { "SmallInteger", "IndexedFloatValueInstanceVariables" };
static const char *errors_232[]    = { nullptr };
static PrimitiveDescriptor primitive_232 = {
        "primitiveIndexedFloatValueSize", primitiveFunctionType( &doubleValueArrayPrimitives::size ), 1572865, signature_232, errors_232
};

static const char *signature_233[] = { "SmallInteger", "IndexedInstanceVariables", "SmallInteger" };
static const char *errors_233[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_233 = {
        "primitiveIndexedObjectAt:ifFail:", primitiveFunctionType( &objArrayPrimitives::at ), 1312002, signature_233, errors_233
};

static const char *signature_234[] = { "Object", "IndexedInstanceVariables", "SmallInteger", "Object" };
static const char *errors_234[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_234 = {
        "primitiveIndexedObjectAt:put:ifFail:", primitiveFunctionType( &objArrayPrimitives::atPut ), 1312003, signature_234, errors_234
};

static const char *signature_235[] = { "Self", "IndexedInstanceVariables", "Object" };
static const char *errors_235[]    = { nullptr };
static PrimitiveDescriptor primitive_235 = {
        "primitiveIndexedObjectAtAllPut:", primitiveFunctionType( &objArrayPrimitives::at_all_put ), 1049858, signature_235, errors_235
};

static const char *signature_236[] = { "Self", "IndexedInstanceVariables", "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_236[]    = { "OutOfBounds", "NegativeSize", nullptr };
static PrimitiveDescriptor primitive_236 = {
        "primitiveIndexedObjectCopyFrom:startingAt:size:ifFail:", primitiveFunctionType( &objArrayPrimitives::copy_size ), 1377540, signature_236, errors_236
};

static const char *signature_237[] = { "Object", "IndexedInstanceVariables class", "SmallInteger" };
static const char *errors_237[]    = { "NegativeSize", nullptr };
static PrimitiveDescriptor primitive_237 = {
        "primitiveIndexedObjectNew:ifFail:", primitiveFunctionType( &objArrayPrimitives::allocateSize ), 1376258, signature_237, errors_237
};

static const char *signature_238[] = { "Object", "IndexedInstanceVariables class", "SmallInteger", "Boolean" };
static const char *errors_238[]    = { "NegativeSize", nullptr };
static PrimitiveDescriptor primitive_238 = {
        "primitiveIndexedObjectNew:size:tenured:ifFail:", primitiveFunctionType( &objArrayPrimitives::allocateSize2 ), 327683, signature_238, errors_238
};

static const char *signature_239[] = { "Self", "IndexedInstanceVariables", "SmallInteger", "SmallInteger", "IndexedInstanceVariables", "SmallInteger" };
static const char *errors_239[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_239 = {
        "primitiveIndexedObjectReplaceFrom:to:with:startingAt:ifFail:", primitiveFunctionType( &objArrayPrimitives::replace_from_to ), 1377541, signature_239, errors_239
};

static const char *signature_240[] = { "Self", "IndexedInstanceVariables" };
static const char *errors_240[]    = { nullptr };
static PrimitiveDescriptor primitive_240 = {
        "primitiveIndexedObjectSize", primitiveFunctionType( &objArrayPrimitives::size ), 1574145, signature_240, errors_240
};

static const char *signature_241[] = { "Instance", "Behavior", "SmallInt" };
static const char *errors_241[]    = { nullptr };
static PrimitiveDescriptor primitive_241 = {
        "primitiveInlineAllocations:count:", primitiveFunctionType( &primitiveInlineAllocations ), 4259842, signature_241, errors_241
};

static const char *signature_242[] = { "Boolean", "Behavior", "Symbol" };
static const char *errors_242[]    = { nullptr };
static PrimitiveDescriptor primitive_242 = {
        "primitiveInliningDatabaseAddLookupEntryClass:selector:ifFail:", primitiveFunctionType( &SystemPrimitives::inlining_database_add_entry ), 327682, signature_242, errors_242
};

static const char *signature_243[] = { "Boolean" };
static const char *errors_243[]    = { nullptr };
static PrimitiveDescriptor primitive_243 = {
        "primitiveInliningDatabaseCompile", primitiveFunctionType( &SystemPrimitives::inlining_database_compile_next ), 65536, signature_243, errors_243
};

static const char *signature_244[] = { "Object", "String" };
static const char *errors_244[]    = { nullptr };
static PrimitiveDescriptor primitive_244 = {
        "primitiveInliningDatabaseCompile:ifFail:", primitiveFunctionType( &SystemPrimitives::inlining_database_compile ), 327681, signature_244, errors_244
};

static const char *signature_245[] = { "IndexedByteInstanceVariables", "String" };
static const char *errors_245[]    = { nullptr };
static PrimitiveDescriptor primitive_245 = {
        "primitiveInliningDatabaseCompileDemangled:ifFail:", primitiveFunctionType( &SystemPrimitives::inlining_database_demangle ), 327681, signature_245, errors_245
};

static const char *signature_246[] = { "Symbol" };
static const char *errors_246[]    = { nullptr };
static PrimitiveDescriptor primitive_246 = {
        "primitiveInliningDatabaseDirectory", primitiveFunctionType( &SystemPrimitives::inlining_database_directory ), 65536, signature_246, errors_246
};

static const char *signature_247[] = { "SmallInteger" };
static const char *errors_247[]    = { nullptr };
static PrimitiveDescriptor primitive_247 = {
        "primitiveInliningDatabaseFileOutAllIfFail:", primitiveFunctionType( &SystemPrimitives::inlining_database_file_out_all ), 327680, signature_247, errors_247
};

static const char *signature_248[] = { "SmallInteger", "Behavior" };
static const char *errors_248[]    = { nullptr };
static PrimitiveDescriptor primitive_248 = {
        "primitiveInliningDatabaseFileOutClass:ifFail:", primitiveFunctionType( &SystemPrimitives::inlining_database_file_out_class ), 327681, signature_248, errors_248
};

static const char *signature_249[] = { "IndexedByteInstanceVariables", "String" };
static const char *errors_249[]    = { nullptr };
static PrimitiveDescriptor primitive_249 = {
        "primitiveInliningDatabaseMangle:ifFail:", primitiveFunctionType( &SystemPrimitives::inlining_database_mangle ), 327681, signature_249, errors_249
};

static const char *signature_250[] = { "Symbol", "Symbol" };
static const char *errors_250[]    = { nullptr };
static PrimitiveDescriptor primitive_250 = {
        "primitiveInliningDatabaseSetDirectory:ifFail:", primitiveFunctionType( &SystemPrimitives::inlining_database_set_directory ), 327681, signature_250, errors_250
};

static const char *signature_251[] = { "Object", "Object", "SmallInteger" };
static const char *errors_251[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_251 = {
        "primitiveInstVarAt:ifFail:", primitiveFunctionType( &oopPrimitives::instVarAt ), 1376258, signature_251, errors_251
};

static const char *signature_252[] = { "Symbol", "Reciever", "Object", "SmallInteger" };
static const char *errors_252[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_252 = {
        "primitiveInstVarNameFor:at:ifFail:", primitiveFunctionType( &oopPrimitives::instance_variable_name_at ), 1376259, signature_252, errors_252
};

static const char *signature_253[] = { "Object", "Object", "SmallInteger", "Object" };
static const char *errors_253[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_253 = {
        "primitiveInstVarOf:at:put:ifFail:", primitiveFunctionType( &oopPrimitives::instVarAtPut ), 327683, signature_253, errors_253
};

static const char *signature_254[] = { "IndexedInstanceVariables", "Class", "SmallInteger" };
static const char *errors_254[]    = { nullptr };
static PrimitiveDescriptor primitive_254 = {
        "primitiveInstancesOf:limit:ifFail:", primitiveFunctionType( &SystemPrimitives::instances_of ), 327682, signature_254, errors_254
};

static const char *signature_255[] = { "SmallInteger" };
static const char *errors_255[]    = { nullptr };
static PrimitiveDescriptor primitive_255 = {
        "primitiveInterpreterInvocationCounterLimit", primitiveFunctionType( &debugPrimitives::interpreterInvocationCounterLimit ), 65536, signature_255, errors_255
};

static const char *signature_256[] = { "SmallInteger", "IndexedByteInstanceVariables" };
static const char *errors_256[]    = { nullptr };
static PrimitiveDescriptor primitive_256 = {
        "primitiveLargeIntegerHash", primitiveFunctionType( &byteArrayPrimitives::largeIntegerHash ), 1574401, signature_256, errors_256
};

static const char *signature_257[] = { "Boolean", "SmallInteger", "SmallInteger" };
static const char *errors_257[]    = { nullptr };
static PrimitiveDescriptor primitive_257 = {
        "primitiveLessThan:ifFail:", primitiveFunctionType( &smiOopPrimitives::lessThan ), 6029570, signature_257, errors_257
};

static const char *signature_258[] = { "Boolean", "SmallInteger", "SmallInteger" };
static const char *errors_258[]    = { nullptr };
static PrimitiveDescriptor primitive_258 = {
        "primitiveLessThanOrEqual:ifFail:", primitiveFunctionType( &smiOopPrimitives::lessThanOrEqual ), 6029570, signature_258, errors_258
};

static const char *signature_259[] = { "SmallInteger", "Float", "Float", "SmallInteger" };
static const char *errors_259[]    = { nullptr };
static PrimitiveDescriptor primitive_259 = {
        "primitiveMandelbrotAtRe:im:iterate:ifFail:", primitiveFunctionType( &doubleOopPrimitives::mandelbrot ), 4980739, signature_259, errors_259
};

static const char *signature_260[] = { "Block", "Method", "Object" };
static const char *errors_260[]    = { nullptr };
static PrimitiveDescriptor primitive_260 = {
        "primitiveMethodAllocateBlock:ifFail:", primitiveFunctionType( &methodOopPrimitives::allocate_block_self ), 1376258, signature_260, errors_260
};

static const char *signature_261[] = { "Block", "Method" };
static const char *errors_261[]    = { nullptr };
static PrimitiveDescriptor primitive_261 = {
        "primitiveMethodAllocateBlockIfFail:", primitiveFunctionType( &methodOopPrimitives::allocate_block ), 1376257, signature_261, errors_261
};

static const char *signature_262[] = { "Object", "Method" };
static const char *errors_262[]    = { nullptr };
static PrimitiveDescriptor primitive_262 = {
        "primitiveMethodBody", primitiveFunctionType( &methodOopPrimitives::fileout_body ), 1114113, signature_262, errors_262
};

static const char *signature_263[] = { "Object", "Method" };
static const char *errors_263[]    = { nullptr };
static PrimitiveDescriptor primitive_263 = {
        "primitiveMethodDebugInfo", primitiveFunctionType( &methodOopPrimitives::debug_info ), 1114113, signature_263, errors_263
};

static const char *signature_264[] = { "Method", "Behavior", "CompressedSymbol" };
static const char *errors_264[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_264 = {
        "primitiveMethodFor:ifFail:", primitiveFunctionType( &behaviorPrimitives::methodFor ), 1376258, signature_264, errors_264
};

static const char *signature_265[] = { "Symbol", "Method" };
static const char *errors_265[]    = { nullptr };
static PrimitiveDescriptor primitive_265 = {
        "primitiveMethodInliningInfo", primitiveFunctionType( &methodOopPrimitives::inlining_info ), 1114113, signature_265, errors_265
};

static const char *signature_266[] = { "SmallInteger", "Method" };
static const char *errors_266[]    = { nullptr };
static PrimitiveDescriptor primitive_266 = {
        "primitiveMethodNumberOfArguments", primitiveFunctionType( &methodOopPrimitives::numberOfArguments ), 1114113, signature_266, errors_266
};

static const char *signature_267[] = { "Symbol", "Method", "Method" };
static const char *errors_267[]    = { nullptr };
static PrimitiveDescriptor primitive_267 = {
        "primitiveMethodOuter:ifFail:", primitiveFunctionType( &methodOopPrimitives::setOuter ), 1376258, signature_267, errors_267
};

static const char *signature_268[] = { "Method", "Method" };
static const char *errors_268[]    = { "ReceiverNotBlockMethod", nullptr };
static PrimitiveDescriptor primitive_268 = {
        "primitiveMethodOuterIfFail:", primitiveFunctionType( &methodOopPrimitives::outer ), 1376257, signature_268, errors_268
};

static const char *signature_269[] = { "Method", "Method", "Object" };
static const char *errors_269[]    = { nullptr };
static PrimitiveDescriptor primitive_269 = {
        "primitiveMethodPrettyPrintKlass:ifFail:", primitiveFunctionType( &methodOopPrimitives::prettyPrint ), 1376258, signature_269, errors_269
};

static const char *signature_270[] = { "ByteIndexedInstanceVariables", "Method", "Object" };
static const char *errors_270[]    = { nullptr };
static PrimitiveDescriptor primitive_270 = {
        "primitiveMethodPrettyPrintSourceKlass:ifFail:", primitiveFunctionType( &methodOopPrimitives::prettyPrintSource ), 1376258, signature_270, errors_270
};

static const char *signature_271[] = { "Symbol", "Method" };
static const char *errors_271[]    = { nullptr };
static PrimitiveDescriptor primitive_271 = {
        "primitiveMethodPrintCodes", primitiveFunctionType( &methodOopPrimitives::printCodes ), 1114113, signature_271, errors_271
};

static const char *signature_272[] = { "IndexedInstanceVariables", "Method" };
static const char *errors_272[]    = { nullptr };
static PrimitiveDescriptor primitive_272 = {
        "primitiveMethodReferencedClassVarNames", primitiveFunctionType( &methodOopPrimitives::referenced_class_variable_names ), 1114113, signature_272, errors_272
};

static const char *signature_273[] = { "IndexedInstanceVariables", "Method" };
static const char *errors_273[]    = { nullptr };
static PrimitiveDescriptor primitive_273 = {
        "primitiveMethodReferencedGlobalNames", primitiveFunctionType( &methodOopPrimitives::referenced_global_names ), 1114113, signature_273, errors_273
};

static const char *signature_274[] = { "IndexedInstanceVariables", "Method", "Mixin" };
static const char *errors_274[]    = { nullptr };
static PrimitiveDescriptor primitive_274 = {
        "primitiveMethodReferencedInstVarNamesMixin:ifFail:", primitiveFunctionType( &methodOopPrimitives::referenced_instance_variable_names ), 1376258, signature_274, errors_274
};

static const char *signature_275[] = { "Symbol", "Method" };
static const char *errors_275[]    = { nullptr };
static PrimitiveDescriptor primitive_275 = {
        "primitiveMethodSelector", primitiveFunctionType( &methodOopPrimitives::selector ), 1114113, signature_275, errors_275
};

static const char *signature_276[] = { "Symbol", "Method", "Symbol" };
static const char *errors_276[]    = { nullptr };
static PrimitiveDescriptor primitive_276 = {
        "primitiveMethodSelector:ifFail:", primitiveFunctionType( &methodOopPrimitives::setSelector ), 1376258, signature_276, errors_276
};

static const char *signature_277[] = { "IndexedInstanceVariables", "Method" };
static const char *errors_277[]    = { nullptr };
static PrimitiveDescriptor primitive_277 = {
        "primitiveMethodSenders", primitiveFunctionType( &methodOopPrimitives::senders ), 1114113, signature_277, errors_277
};

static const char *signature_278[] = { "Symbol", "Method", "Symbol" };
static const char *errors_278[]    = { "ArgumentIsInvalid", nullptr };
static PrimitiveDescriptor primitive_278 = {
        "primitiveMethodSetInliningInfo:ifFail:", primitiveFunctionType( &methodOopPrimitives::set_inlining_info ), 1376258, signature_278, errors_278
};

static const char *signature_279[] = { "Object", "Method" };
static const char *errors_279[]    = { nullptr };
static PrimitiveDescriptor primitive_279 = {
        "primitiveMethodSizeAndFlags", primitiveFunctionType( &methodOopPrimitives::size_and_flags ), 1114113, signature_279, errors_279
};

static const char *signature_280[] = { "Mixin", "Mixin", "Symbol" };
static const char *errors_280[]    = { "IsInstalled", "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_280 = {
        "primitiveMixin:addClassVariable:ifFail:", primitiveFunctionType( &mixinOopPrimitives::add_class_variable ), 327682, signature_280, errors_280
};

static const char *signature_281[] = { "Symbol", "Mixin", "Symbol" };
static const char *errors_281[]    = { "IsInstalled", "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_281 = {
        "primitiveMixin:addInstanceVariable:ifFail:", primitiveFunctionType( &mixinOopPrimitives::add_instance_variable ), 327682, signature_281, errors_281
};

static const char *signature_282[] = { "Method", "Mixin", "Method" };
static const char *errors_282[]    = { "IsInstalled", nullptr };
static PrimitiveDescriptor primitive_282 = {
        "primitiveMixin:addMethod:ifFail:", primitiveFunctionType( &mixinOopPrimitives::add_method ), 327682, signature_282, errors_282
};

static const char *signature_283[] = { "Symbol", "Mixin", "SmallInteger" };
static const char *errors_283[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_283 = {
        "primitiveMixin:classVariableAt:ifFail:", primitiveFunctionType( &mixinOopPrimitives::class_variable_at ), 327682, signature_283, errors_283
};

static const char *signature_284[] = { "Symbol", "Mixin" };
static const char *errors_284[]    = { nullptr };
static PrimitiveDescriptor primitive_284 = {
        "primitiveMixin:classVariablesIfFail:", primitiveFunctionType( &mixinOopPrimitives::class_variables ), 327681, signature_284, errors_284
};

static const char *signature_285[] = { "Symbol", "Mixin", "SmallInteger" };
static const char *errors_285[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_285 = {
        "primitiveMixin:instanceVariableAt:ifFail:", primitiveFunctionType( &mixinOopPrimitives::instance_variable_at ), 327682, signature_285, errors_285
};

static const char *signature_286[] = { "Symbol", "Mixin" };
static const char *errors_286[]    = { nullptr };
static PrimitiveDescriptor primitive_286 = {
        "primitiveMixin:instanceVariablesIfFail:", primitiveFunctionType( &mixinOopPrimitives::instance_variables ), 327681, signature_286, errors_286
};

static const char *signature_287[] = { "Method", "Mixin", "SmallInteger" };
static const char *errors_287[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_287 = {
        "primitiveMixin:methodAt:ifFail:", primitiveFunctionType( &mixinOopPrimitives::method_at ), 327682, signature_287, errors_287
};

static const char *signature_288[] = { "Symbol", "Mixin" };
static const char *errors_288[]    = { nullptr };
static PrimitiveDescriptor primitive_288 = {
        "primitiveMixin:methodsIfFail:", primitiveFunctionType( &mixinOopPrimitives::methods ), 327681, signature_288, errors_288
};

static const char *signature_289[] = { "Symbol", "Mixin", "SmallInteger" };
static const char *errors_289[]    = { "IsInstalled", "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_289 = {
        "primitiveMixin:removeClassVariableAt:ifFail:", primitiveFunctionType( &mixinOopPrimitives::remove_class_variable_at ), 327682, signature_289, errors_289
};

static const char *signature_290[] = { "Symbol", "Mixin", "SmallInteger" };
static const char *errors_290[]    = { "IsInstalled", "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_290 = {
        "primitiveMixin:removeInstanceVariableAt:ifFail:", primitiveFunctionType( &mixinOopPrimitives::remove_instance_variable_at ), 327682, signature_290, errors_290
};

static const char *signature_291[] = { "Method", "Mixin", "SmallInteger" };
static const char *errors_291[]    = { "IsInstalled", "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_291 = {
        "primitiveMixin:removeMethodAt:ifFail:", primitiveFunctionType( &mixinOopPrimitives::remove_method_at ), 327682, signature_291, errors_291
};

static const char *signature_292[] = { "Mixin", "Mixin" };
static const char *errors_292[]    = { nullptr };
static PrimitiveDescriptor primitive_292 = {
        "primitiveMixinClassMixinOf:ifFail:", primitiveFunctionType( &mixinOopPrimitives::class_mixin ), 327681, signature_292, errors_292
};

static const char *signature_293[] = { "Boolean", "Mixin" };
static const char *errors_293[]    = { nullptr };
static PrimitiveDescriptor primitive_293 = {
        "primitiveMixinIsInstalled:ifFail:", primitiveFunctionType( &mixinOopPrimitives::is_installed ), 327681, signature_293, errors_293
};

static const char *signature_294[] = { "SmallInteger", "Mixin" };
static const char *errors_294[]    = { nullptr };
static PrimitiveDescriptor primitive_294 = {
        "primitiveMixinNumberOfClassVariablesOf:ifFail:", primitiveFunctionType( &mixinOopPrimitives::number_of_class_variables ), 327681, signature_294, errors_294
};

static const char *signature_295[] = { "SmallInteger", "Mixin" };
static const char *errors_295[]    = { nullptr };
static PrimitiveDescriptor primitive_295 = {
        "primitiveMixinNumberOfInstanceVariablesOf:ifFail:", primitiveFunctionType( &mixinOopPrimitives::number_of_instance_variables ), 327681, signature_295, errors_295
};

static const char *signature_296[] = { "SmallInteger", "Mixin" };
static const char *errors_296[]    = { nullptr };
static PrimitiveDescriptor primitive_296 = {
        "primitiveMixinNumberOfMethodsOf:ifFail:", primitiveFunctionType( &mixinOopPrimitives::number_of_methods ), 327681, signature_296, errors_296
};

static const char *signature_297[] = { "Class", "Mixin" };
static const char *errors_297[]    = { nullptr };
static PrimitiveDescriptor primitive_297 = {
        "primitiveMixinPrimaryInvocationOf:ifFail:", primitiveFunctionType( &mixinOopPrimitives::primary_invocation ), 327681, signature_297, errors_297
};

static const char *signature_298[] = { "Mixin", "Mixin", "Mixin" };
static const char *errors_298[]    = { "IsInstalled", nullptr };
static PrimitiveDescriptor primitive_298 = {
        "primitiveMixinSetClassMixinOf:to:ifFail:", primitiveFunctionType( &mixinOopPrimitives::set_class_mixin ), 327682, signature_298, errors_298
};

static const char *signature_299[] = { "Boolean", "Mixin" };
static const char *errors_299[]    = { nullptr };
static PrimitiveDescriptor primitive_299 = {
        "primitiveMixinSetInstalled:ifFail:", primitiveFunctionType( &mixinOopPrimitives::set_installed ), 327681, signature_299, errors_299
};

static const char *signature_300[] = { "Class", "Mixin", "Class" };
static const char *errors_300[]    = { "IsInstalled", nullptr };
static PrimitiveDescriptor primitive_300 = {
        "primitiveMixinSetPrimaryInvocationOf:to:ifFail:", primitiveFunctionType( &mixinOopPrimitives::set_primary_invocation ), 327682, signature_300, errors_300
};

static const char *signature_301[] = { "Boolean", "Mixin" };
static const char *errors_301[]    = { nullptr };
static PrimitiveDescriptor primitive_301 = {
        "primitiveMixinSetUnInstalled:ifFail:", primitiveFunctionType( &mixinOopPrimitives::set_uninstalled ), 327681, signature_301, errors_301
};

static const char *signature_302[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_302[]    = { "Overflow", "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_302 = {
        "primitiveMod:ifFail:", primitiveFunctionType( &smiOopPrimitives_mod ), 6029826, signature_302, errors_302
};

static const char *signature_303[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_303[]    = { "Overflow", nullptr };
static PrimitiveDescriptor primitive_303 = {
        "primitiveMultiply:ifFail:", primitiveFunctionType( &smiOopPrimitives_multiply ), 6029826, signature_303, errors_303
};

static const char *signature_304[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_304[]    = { nullptr };
static PrimitiveDescriptor primitive_304 = {
        "primitiveNew0:ifFail:", primitiveFunctionType( &primitiveNew0 ), 7667714, signature_304, errors_304
};

static const char *signature_305[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_305[]    = { nullptr };
static PrimitiveDescriptor primitive_305 = {
        "primitiveNew1:ifFail:", primitiveFunctionType( &primitiveNew1 ), 7667714, signature_305, errors_305
};

static const char *signature_306[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_306[]    = { nullptr };
static PrimitiveDescriptor primitive_306 = {
        "primitiveNew2:ifFail:", primitiveFunctionType( &primitiveNew2 ), 7667714, signature_306, errors_306
};

static const char *signature_307[] = { "Instance", "Behavior" };
static const char *errors_307[]    = { "ReceiverIsIndexable", nullptr };
static PrimitiveDescriptor primitive_307 = {
        "primitiveNew2IfFail:", primitiveFunctionType( &behaviorPrimitives::allocate2 ), 1376257, signature_307, errors_307
};

static const char *signature_308[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_308[]    = { nullptr };
static PrimitiveDescriptor primitive_308 = {
        "primitiveNew3:ifFail:", primitiveFunctionType( &primitiveNew3 ), 7667714, signature_308, errors_308
};

static const char *signature_309[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_309[]    = { nullptr };
static PrimitiveDescriptor primitive_309 = {
        "primitiveNew4:ifFail:", primitiveFunctionType( &primitiveNew4 ), 7667714, signature_309, errors_309
};

static const char *signature_310[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_310[]    = { nullptr };
static PrimitiveDescriptor primitive_310 = {
        "primitiveNew5:ifFail:", primitiveFunctionType( &primitiveNew5 ), 7667714, signature_310, errors_310
};

static const char *signature_311[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_311[]    = { nullptr };
static PrimitiveDescriptor primitive_311 = {
        "primitiveNew6:ifFail:", primitiveFunctionType( &primitiveNew6 ), 7667714, signature_311, errors_311
};

static const char *signature_312[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_312[]    = { nullptr };
static PrimitiveDescriptor primitive_312 = {
        "primitiveNew7:ifFail:", primitiveFunctionType( &primitiveNew7 ), 7667714, signature_312, errors_312
};

static const char *signature_313[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_313[]    = { nullptr };
static PrimitiveDescriptor primitive_313 = {
        "primitiveNew8:ifFail:", primitiveFunctionType( &primitiveNew8 ), 7667714, signature_313, errors_313
};

static const char *signature_314[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_314[]    = { nullptr };
static PrimitiveDescriptor primitive_314 = {
        "primitiveNew9:ifFail:", primitiveFunctionType( &primitiveNew9 ), 7667714, signature_314, errors_314
};

static const char *signature_315[] = { "Instance", "Behavior", "Boolean" };
static const char *errors_315[]    = { "ReceiverIsIndexable", nullptr };
static PrimitiveDescriptor primitive_315 = {
        "primitiveNew:tenured:ifFail:", primitiveFunctionType( &behaviorPrimitives::allocate3 ), 327682, signature_315, errors_315
};

static const char *signature_316[] = { "Instance", "Behavior" };
static const char *errors_316[]    = { "ReceiverIsIndexable", nullptr };
static PrimitiveDescriptor primitive_316 = {
        "primitiveNewIfFail:", primitiveFunctionType( &behaviorPrimitives::allocate ), 1376257, signature_316, errors_316
};

static const char *signature_317[] = { "Boolean", "Object", "Object" };
static const char *errors_317[]    = { nullptr };
static PrimitiveDescriptor primitive_317 = {
        "primitiveNotEqual:", primitiveFunctionType( &oopPrimitives::not_equal ), 1572866, signature_317, errors_317
};

static const char *signature_318[] = { "Object" };
static const char *errors_318[]    = { "EmptyQueue", nullptr };
static PrimitiveDescriptor primitive_318 = {
        "primitiveNotificationQueueGetIfFail:", primitiveFunctionType( &SystemPrimitives::notificationQueueGet ), 327680, signature_318, errors_318
};

static const char *signature_319[] = { "Object", "Object" };
static const char *errors_319[]    = { nullptr };
static PrimitiveDescriptor primitive_319 = {
        "primitiveNotificationQueuePut:", primitiveFunctionType( &SystemPrimitives::notificationQueuePut ), 65537, signature_319, errors_319
};

static const char *signature_320[] = { "SmallInteger" };
static const char *errors_320[]    = { nullptr };
static PrimitiveDescriptor primitive_320 = {
        "primitiveNumberOfLookupCacheMisses", primitiveFunctionType( &debugPrimitives::numberOfLookupCacheMisses ), 65536, signature_320, errors_320
};

static const char *signature_321[] = { "SmallInteger" };
static const char *errors_321[]    = { nullptr };
static PrimitiveDescriptor primitive_321 = {
        "primitiveNumberOfMethodInvocations", primitiveFunctionType( &debugPrimitives::numberOfMethodInvocations ), 65536, signature_321, errors_321
};

static const char *signature_322[] = { "SmallInteger" };
static const char *errors_322[]    = { nullptr };
static PrimitiveDescriptor primitive_322 = {
        "primitiveNumberOfNativeMethodInvocations", primitiveFunctionType( &debugPrimitives::numberOfNativeMethodInvocations ), 65536, signature_322, errors_322
};

static const char *signature_323[] = { "SmallInteger" };
static const char *errors_323[]    = { nullptr };
static PrimitiveDescriptor primitive_323 = {
        "primitiveNumberOfPrimaryLookupCacheHits", primitiveFunctionType( &debugPrimitives::numberOfPrimaryLookupCacheHits ), 65536, signature_323, errors_323
};

static const char *signature_324[] = { "SmallInteger" };
static const char *errors_324[]    = { nullptr };
static PrimitiveDescriptor primitive_324 = {
        "primitiveNumberOfSecondaryLookupCacheHits", primitiveFunctionType( &debugPrimitives::numberOfSecondaryLookupCacheHits ), 65536, signature_324, errors_324
};

static const char *signature_325[] = { "SmallInteger" };
static const char *errors_325[]    = { nullptr };
static PrimitiveDescriptor primitive_325 = {
        "primitiveNurseryFreeSpace", primitiveFunctionType( &SystemPrimitives::nurseryFreeSpace ), 65536, signature_325, errors_325
};

static const char *signature_326[] = { "Float" };
static const char *errors_326[]    = { nullptr };
static PrimitiveDescriptor primitive_326 = {
        "primitiveObjectMemorySize", primitiveFunctionType( &SystemPrimitives::object_memory_size ), 65536, signature_326, errors_326
};

static const char *signature_327[] = { "SmallInteger", "Object" };
static const char *errors_327[]    = { nullptr };
static PrimitiveDescriptor primitive_327 = {
        "primitiveOopSize", primitiveFunctionType( &oopPrimitives::oop_size ), 1572865, signature_327, errors_327
};

static const char *signature_328[] = { "Object", "Object", "Symbol" };
static const char *errors_328[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_328 = {
        "primitiveOptimizeMethod:ifFail:", primitiveFunctionType( &debugPrimitives::optimizeMethod ), 1376258, signature_328, errors_328
};

static const char *signature_329[] = { "Object", "Object", "CompressedSymbol", "Array" };
static const char *errors_329[]    = { "SelectorHasWrongNumberOfArguments", nullptr };
static PrimitiveDescriptor primitive_329 = {
        "primitivePerform:arguments:ifFail:", primitiveFunctionType( &oopPrimitives::performArguments ), 1507331, signature_329, errors_329
};

static const char *signature_330[] = { "Object", "Object", "CompressedSymbol" };
static const char *errors_330[]    = { "SelectorHasWrongNumberOfArguments", nullptr };
static PrimitiveDescriptor primitive_330 = {
        "primitivePerform:ifFail:", primitiveFunctionType( &oopPrimitives::perform ), 1376258, signature_330, errors_330
};

static const char *signature_331[] = { "Object", "Object", "CompressedSymbol", "Object" };
static const char *errors_331[]    = { "SelectorHasWrongNumberOfArguments", nullptr };
static PrimitiveDescriptor primitive_331 = {
        "primitivePerform:with:ifFail:", primitiveFunctionType( &oopPrimitives::performWith ), 1507331, signature_331, errors_331
};

static const char *signature_332[] = { "Object", "Object", "CompressedSymbol", "Object", "Object" };
static const char *errors_332[]    = { "SelectorHasWrongNumberOfArguments", nullptr };
static PrimitiveDescriptor primitive_332 = {
        "primitivePerform:with:with:ifFail:", primitiveFunctionType( &oopPrimitives::performWithWith ), 1507332, signature_332, errors_332
};

static const char *signature_333[] = { "Object", "Object", "CompressedSymbol", "Object", "Object", "Object" };
static const char *errors_333[]    = { "SelectorHasWrongNumberOfArguments", nullptr };
static PrimitiveDescriptor primitive_333 = {
        "primitivePerform:with:with:with:ifFail:", primitiveFunctionType( &oopPrimitives::performWithWithWith ), 1507333, signature_333, errors_333
};

static const char *signature_334[] = { "Self", "Object" };
static const char *errors_334[]    = { nullptr };
static PrimitiveDescriptor primitive_334 = {
        "primitivePrint", primitiveFunctionType( &oopPrimitives::print ), 1114113, signature_334, errors_334
};

static const char *signature_335[] = { "SmallInteger", "SmallInteger" };
static const char *errors_335[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_335 = {
        "primitivePrintCharacterIfFail:", primitiveFunctionType( &smiOopPrimitives::printCharacter ), 1310721, signature_335, errors_335
};

static const char *signature_336[] = { "Object", "SmallInteger" };
static const char *errors_336[]    = { nullptr };
static PrimitiveDescriptor primitive_336 = {
        "primitivePrintInvocationCounterHistogram:ifFail:", primitiveFunctionType( &debugPrimitives::printInvocationCounterHistogram ), 327681, signature_336, errors_336
};

static const char *signature_337[] = { "Object" };
static const char *errors_337[]    = { nullptr };
static PrimitiveDescriptor primitive_337 = {
        "primitivePrintLayout", primitiveFunctionType( &debugPrimitives::printMemoryLayout ), 65536, signature_337, errors_337
};

static const char *signature_338[] = { "Object" };
static const char *errors_338[]    = { nullptr };
static PrimitiveDescriptor primitive_338 = {
        "primitivePrintLookupCacheStatistics", primitiveFunctionType( &debugPrimitives::printLookupCacheStatistics ), 65536, signature_338, errors_338
};

static const char *signature_339[] = { "Object" };
static const char *errors_339[]    = { nullptr };
static PrimitiveDescriptor primitive_339 = {
        "primitivePrintMemory", primitiveFunctionType( &SystemPrimitives::print_memory ), 65536, signature_339, errors_339
};

static const char *signature_340[] = { "Behavior", "Behavior", "ByteArray" };
static const char *errors_340[]    = { nullptr };
static PrimitiveDescriptor primitive_340 = {
        "primitivePrintMethod:ifFail:", primitiveFunctionType( &behaviorPrimitives::printMethod ), 1376258, signature_340, errors_340
};

static const char *signature_341[] = { "Object", "Object", "Symbol" };
static const char *errors_341[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_341 = {
        "primitivePrintMethodCodes:ifFail:", primitiveFunctionType( &debugPrimitives::printMethodCodes ), 1376258, signature_341, errors_341
};

static const char *signature_342[] = { "Object", "SmallInteger" };
static const char *errors_342[]    = { nullptr };
static PrimitiveDescriptor primitive_342 = {
        "primitivePrintNativeMethodCounterHistogram:ifFail:", primitiveFunctionType( &debugPrimitives::printNativeMethodCounterHistogram ), 327681, signature_342, errors_342
};

static const char *signature_343[] = { "Object" };
static const char *errors_343[]    = { nullptr };
static PrimitiveDescriptor primitive_343 = {
        "primitivePrintObjectHistogram", primitiveFunctionType( &debugPrimitives::printObjectHistogram ), 65536, signature_343, errors_343
};

static const char *signature_344[] = { "Object" };
static const char *errors_344[]    = { nullptr };
static PrimitiveDescriptor primitive_344 = {
        "primitivePrintPrimitiveCounters", primitiveFunctionType( &debugPrimitives::printPrimitiveCounters ), 65536, signature_344, errors_344
};

static const char *signature_345[] = { "Object" };
static const char *errors_345[]    = { nullptr };
static PrimitiveDescriptor primitive_345 = {
        "primitivePrintPrimitiveTable", primitiveFunctionType( &SystemPrimitives::printPrimitiveTable ), 65536, signature_345, errors_345
};

static const char *signature_346[] = { "Self", "Object" };
static const char *errors_346[]    = { nullptr };
static PrimitiveDescriptor primitive_346 = {
        "primitivePrintValue", primitiveFunctionType( &oopPrimitives::printValue ), 1114113, signature_346, errors_346
};

static const char *signature_347[] = { "Object" };
static const char *errors_347[]    = { nullptr };
static PrimitiveDescriptor primitive_347 = {
        "primitivePrintZone", primitiveFunctionType( &SystemPrimitives::print_zone ), 65536, signature_347, errors_347
};

static const char *signature_348[] = { "Object" };
static const char *errors_348[]    = { nullptr };
static PrimitiveDescriptor primitive_348 = {
        "primitiveProcessActiveProcess", primitiveFunctionType( &processOopPrimitives::activeProcess ), 65536, signature_348, errors_348
};

static const char *signature_349[] = { "Process", "Process class", "BlockWithoutArguments" };
static const char *errors_349[]    = { "ProcessAllocationFailed", nullptr };
static PrimitiveDescriptor primitive_349 = {
        "primitiveProcessCreate:ifFail:", primitiveFunctionType( &processOopPrimitives::create ), 1376258, signature_349, errors_349
};

static const char *signature_350[] = { "Process" };
static const char *errors_350[]    = { nullptr };
static PrimitiveDescriptor primitive_350 = {
        "primitiveProcessEnterCritical", primitiveFunctionType( &processOopPrimitives::enter_critical ), 65536, signature_350, errors_350
};

static const char *signature_351[] = { "Process" };
static const char *errors_351[]    = { nullptr };
static PrimitiveDescriptor primitive_351 = {
        "primitiveProcessLeaveCritical", primitiveFunctionType( &processOopPrimitives::leave_critical ), 65536, signature_351, errors_351
};

static const char *signature_352[] = { "Boolean", "Process", "SmallInteger" };
static const char *errors_352[]    = { nullptr };
static PrimitiveDescriptor primitive_352 = {
        "primitiveProcessSchedulerWait:ifFail:", primitiveFunctionType( &processOopPrimitives::scheduler_wait ), 1376258, signature_352, errors_352
};

static const char *signature_353[] = { "Symbol", "Process", "Symbol", "Activation", "Object" };
static const char *errors_353[]    = { "InScheduler", "Dead", nullptr };
static PrimitiveDescriptor primitive_353 = {
        "primitiveProcessSetMode:activation:returnValue:ifFail:", primitiveFunctionType( &processOopPrimitives::set_mode ), 1376260, signature_353, errors_353
};

static const char *signature_354[] = { "IndexedInstanceVariables", "Process", "SmallInteger" };
static const char *errors_354[]    = { nullptr };
static PrimitiveDescriptor primitive_354 = {
        "primitiveProcessStackLimit:ifFail:", primitiveFunctionType( &processOopPrimitives::stack ), 1376258, signature_354, errors_354
};

static const char *signature_355[] = { "Object", "Process" };
static const char *errors_355[]    = { "NotInScheduler", "ProcessCannotContinue", "Dead", nullptr };
static PrimitiveDescriptor primitive_355 = {
        "primitiveProcessStartEvaluator:ifFail:", primitiveFunctionType( &processOopPrimitives::start_evaluator ), 327681, signature_355, errors_355
};

static const char *signature_356[] = { "Symbol", "Process" };
static const char *errors_356[]    = { nullptr };
static PrimitiveDescriptor primitive_356 = {
        "primitiveProcessStatus", primitiveFunctionType( &processOopPrimitives::status ), 1114113, signature_356, errors_356
};

static const char *signature_357[] = { "Process" };
static const char *errors_357[]    = { nullptr };
static PrimitiveDescriptor primitive_357 = {
        "primitiveProcessStop", primitiveFunctionType( &processOopPrimitives::stop ), 65536, signature_357, errors_357
};

static const char *signature_358[] = { "Float", "Process" };
static const char *errors_358[]    = { nullptr };
static PrimitiveDescriptor primitive_358 = {
        "primitiveProcessSystemTime", primitiveFunctionType( &processOopPrimitives::user_time ), 1114113, signature_358, errors_358
};

static const char *signature_359[] = { "Self", "Process" };
static const char *errors_359[]    = { "Dead", nullptr };
static PrimitiveDescriptor primitive_359 = {
        "primitiveProcessTerminateIfFail:", primitiveFunctionType( &processOopPrimitives::terminate ), 1507329, signature_359, errors_359
};

static const char *signature_360[] = { "Self", "Process", "SmallInteger" };
static const char *errors_360[]    = { nullptr };
static PrimitiveDescriptor primitive_360 = {
        "primitiveProcessTraceStack:ifFail:", primitiveFunctionType( &processOopPrimitives::trace_stack ), 1376258, signature_360, errors_360
};

static const char *signature_361[] = { "Object", "Process" };
static const char *errors_361[]    = { "NotInScheduler", "ProcessCannotContinue", "Dead", nullptr };
static PrimitiveDescriptor primitive_361 = {
        "primitiveProcessTransferTo:ifFail:", primitiveFunctionType( &processOopPrimitives::transferTo ), 327681, signature_361, errors_361
};

static const char *signature_362[] = { "Float", "Process" };
static const char *errors_362[]    = { nullptr };
static PrimitiveDescriptor primitive_362 = {
        "primitiveProcessUserTime", primitiveFunctionType( &processOopPrimitives::user_time ), 1114113, signature_362, errors_362
};

static const char *signature_363[] = { "Process" };
static const char *errors_363[]    = { nullptr };
static PrimitiveDescriptor primitive_363 = {
        "primitiveProcessYield", primitiveFunctionType( &processOopPrimitives::yield ), 65536, signature_363, errors_363
};

static const char *signature_364[] = { "Process" };
static const char *errors_364[]    = { nullptr };
static PrimitiveDescriptor primitive_364 = {
        "primitiveProcessYieldInCritical", primitiveFunctionType( &processOopPrimitives::yield_in_critical ), 65536, signature_364, errors_364
};

static const char *signature_365[] = { "SmallInteger", "Proxy", "SmallInteger" };
static const char *errors_365[]    = { nullptr };
static PrimitiveDescriptor primitive_365 = {
        "primitiveProxyByteAt:ifFail:", primitiveFunctionType( &proxyOopPrimitives::byteAt ), 5570562, signature_365, errors_365
};

static const char *signature_366[] = { "SmallInteger", "Proxy", "SmallInteger", "SmallInteger" };
static const char *errors_366[]    = { nullptr };
static PrimitiveDescriptor primitive_366 = {
        "primitiveProxyByteAt:put:ifFail:", primitiveFunctionType( &proxyOopPrimitives::byteAtPut ), 5570563, signature_366, errors_366
};

static const char *signature_367[] = { "Self", "Proxy", "SmallInteger" };
static const char *errors_367[]    = { nullptr };
static PrimitiveDescriptor primitive_367 = {
        "primitiveProxyCalloc:ifFail:", primitiveFunctionType( &proxyOopPrimitives::calloc ), 1376258, signature_367, errors_367
};

static const char *signature_368[] = { "SmallInteger", "Proxy", "SmallInteger" };
static const char *errors_368[]    = { nullptr };
static PrimitiveDescriptor primitive_368 = {
        "primitiveProxyDoubleByteAt:ifFail:", primitiveFunctionType( &proxyOopPrimitives::doubleByteAt ), 1376258, signature_368, errors_368
};

static const char *signature_369[] = { "SmallInteger", "Proxy", "SmallInteger", "SmallInteger" };
static const char *errors_369[]    = { nullptr };
static PrimitiveDescriptor primitive_369 = {
        "primitiveProxyDoubleByteAt:put:ifFail:", primitiveFunctionType( &proxyOopPrimitives::doubleByteAtPut ), 1376259, signature_369, errors_369
};

static const char *signature_370[] = { "Float", "Proxy", "SmallInteger" };
static const char *errors_370[]    = { nullptr };
static PrimitiveDescriptor primitive_370 = {
        "primitiveProxyDoublePrecisionFloatAt:ifFail:", primitiveFunctionType( &proxyOopPrimitives::doublePrecisionFloatAt ), 1376258, signature_370, errors_370
};

static const char *signature_371[] = { "Self", "Proxy", "SmallInteger", "Float" };
static const char *errors_371[]    = { "ConversionFailed", nullptr };
static PrimitiveDescriptor primitive_371 = {
        "primitiveProxyDoublePrecisionFloatAt:put:ifFail:", primitiveFunctionType( &proxyOopPrimitives::doublePrecisionFloatAtPut ), 1376259, signature_371, errors_371
};

static const char *signature_372[] = { "Self", "Proxy" };
static const char *errors_372[]    = { nullptr };
static PrimitiveDescriptor primitive_372 = {
        "primitiveProxyFree", primitiveFunctionType( &proxyOopPrimitives::free ), 1114113, signature_372, errors_372
};

static const char *signature_373[] = { "SmallInteger", "Proxy" };
static const char *errors_373[]    = { nullptr };
static PrimitiveDescriptor primitive_373 = {
        "primitiveProxyGetHigh", primitiveFunctionType( &proxyOopPrimitives::getHigh ), 1114113, signature_373, errors_373
};

static const char *signature_374[] = { "SmallInteger", "Proxy" };
static const char *errors_374[]    = { "ConversionFailed", nullptr };
static PrimitiveDescriptor primitive_374 = {
        "primitiveProxyGetIfFail:", primitiveFunctionType( &proxyOopPrimitives::getSmi ), 1376257, signature_374, errors_374
};

static const char *signature_375[] = { "SmallInteger", "Proxy" };
static const char *errors_375[]    = { nullptr };
static PrimitiveDescriptor primitive_375 = {
        "primitiveProxyGetLow", primitiveFunctionType( &proxyOopPrimitives::getLow ), 1114113, signature_375, errors_375
};

static const char *signature_376[] = { "Boolean", "Proxy" };
static const char *errors_376[]    = { nullptr };
static PrimitiveDescriptor primitive_376 = {
        "primitiveProxyIsAllOnes", primitiveFunctionType( &proxyOopPrimitives::isAllOnes ), 1114113, signature_376, errors_376
};

static const char *signature_377[] = { "Boolean", "Proxy" };
static const char *errors_377[]    = { nullptr };
static PrimitiveDescriptor primitive_377 = {
        "primitiveProxyIsNull", primitiveFunctionType( &proxyOopPrimitives::isNull ), 1114113, signature_377, errors_377
};

static const char *signature_378[] = { "Self", "Proxy", "SmallInteger" };
static const char *errors_378[]    = { nullptr };
static PrimitiveDescriptor primitive_378 = {
        "primitiveProxyMalloc:ifFail:", primitiveFunctionType( &proxyOopPrimitives::malloc ), 1376258, signature_378, errors_378
};

static const char *signature_379[] = { "Proxy", "Proxy", "SmallInteger", "Proxy" };
static const char *errors_379[]    = { nullptr };
static PrimitiveDescriptor primitive_379 = {
        "primitiveProxyProxyAt:put:ifFail:", primitiveFunctionType( &proxyOopPrimitives::proxyAtPut ), 1376259, signature_379, errors_379
};

static const char *signature_380[] = { "Proxy", "Proxy", "SmallInteger", "Proxy" };
static const char *errors_380[]    = { nullptr };
static PrimitiveDescriptor primitive_380 = {
        "primitiveProxyProxyAt:result:ifFail:", primitiveFunctionType( &proxyOopPrimitives::proxyAt ), 1376259, signature_380, errors_380
};

static const char *signature_381[] = { "Self", "Proxy", "SmallInteger|Proxy" };
static const char *errors_381[]    = { nullptr };
static PrimitiveDescriptor primitive_381 = {
        "primitiveProxySet:ifFail:", primitiveFunctionType( &proxyOopPrimitives::set ), 1376258, signature_381, errors_381
};

static const char *signature_382[] = { "Self", "Proxy", "SmallInteger", "SmallInteger" };
static const char *errors_382[]    = { nullptr };
static PrimitiveDescriptor primitive_382 = {
        "primitiveProxySetHigh:low:ifFail:", primitiveFunctionType( &proxyOopPrimitives::setHighLow ), 1376259, signature_382, errors_382
};

static const char *signature_383[] = { "Float", "Proxy", "SmallInteger" };
static const char *errors_383[]    = { nullptr };
static PrimitiveDescriptor primitive_383 = {
        "primitiveProxySinglePrecisionFloatAt:ifFail:", primitiveFunctionType( &proxyOopPrimitives::singlePrecisionFloatAt ), 1376258, signature_383, errors_383
};

static const char *signature_384[] = { "Self", "Proxy", "SmallInteger", "Float" };
static const char *errors_384[]    = { "ConversionFailed", nullptr };
static PrimitiveDescriptor primitive_384 = {
        "primitiveProxySinglePrecisionFloatAt:put:ifFail:", primitiveFunctionType( &proxyOopPrimitives::singlePrecisionFloatAtPut ), 1376259, signature_384, errors_384
};

static const char *signature_385[] = { "SmallInteger", "Proxy", "SmallInteger" };
static const char *errors_385[]    = { "ConversionFailed", nullptr };
static PrimitiveDescriptor primitive_385 = {
        "primitiveProxySmiAt:ifFail:", primitiveFunctionType( &proxyOopPrimitives::smiAt ), 1376258, signature_385, errors_385
};

static const char *signature_386[] = { "SmallInteger", "Proxy", "SmallInteger", "SmallInteger" };
static const char *errors_386[]    = { nullptr };
static PrimitiveDescriptor primitive_386 = {
        "primitiveProxySmiAt:put:ifFail:", primitiveFunctionType( &proxyOopPrimitives::smiAtPut ), 1376259, signature_386, errors_386
};

static const char *signature_387[] = { "Proxy", "Proxy", "SmallInteger", "Proxy" };
static const char *errors_387[]    = { nullptr };
static PrimitiveDescriptor primitive_387 = {
        "primitiveProxySubProxyAt:result:ifFail:", primitiveFunctionType( &proxyOopPrimitives::subProxyAt ), 1376259, signature_387, errors_387
};

static const char *signature_388[] = { "BottomType" };
static const char *errors_388[]    = { nullptr };
static PrimitiveDescriptor primitive_388 = {
        "primitiveQuit", primitiveFunctionType( &SystemPrimitives::quit ), 65536, signature_388, errors_388
};

static const char *signature_389[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_389[]    = { "NotImplementedYet", nullptr };
static PrimitiveDescriptor primitive_389 = {
        "primitiveQuo:ifFail:", primitiveFunctionType( &smiOopPrimitives_quo ), 6029826, signature_389, errors_389
};

static const char *signature_390[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_390[]    = { nullptr };
static PrimitiveDescriptor primitive_390 = {
        "primitiveRawBitShift:ifFail:", primitiveFunctionType( &smiOopPrimitives::rawBitShift ), 6029826, signature_390, errors_390
};

static const char *signature_391[] = { "Object", "Process" };
static const char *errors_391[]    = { nullptr };
static PrimitiveDescriptor primitive_391 = {
        "primitiveRecordMainProcessIfFail:", primitiveFunctionType( &processOopPrimitives::setMainProcess ), 1376257, signature_391, errors_391
};

static const char *signature_392[] = { "IndexedInstanceVariables", "Object", "SmallInteger" };
static const char *errors_392[]    = { nullptr };
static PrimitiveDescriptor primitive_392 = {
        "primitiveReferencesTo:limit:ifFail:", primitiveFunctionType( &SystemPrimitives::references_to ), 327682, signature_392, errors_392
};

static const char *signature_393[] = { "IndexedInstanceVariables", "Class", "SmallInteger" };
static const char *errors_393[]    = { nullptr };
static PrimitiveDescriptor primitive_393 = {
        "primitiveReferencesToInstancesOf:limit:ifFail:", primitiveFunctionType( &SystemPrimitives::references_to_instances_of ), 327682, signature_393, errors_393
};

static const char *signature_394[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_394[]    = { "DivisionByZero", nullptr };
static PrimitiveDescriptor primitive_394 = {
        "primitiveRemainder:ifFail:", primitiveFunctionType( &smiOopPrimitives_remainder ), 6029826, signature_394, errors_394
};

static const char *signature_395[] = { "BottomType", "BlockWithoutArguments" };
static const char *errors_395[]    = { nullptr };
static PrimitiveDescriptor primitive_395 = {
        "primitiveRepeat", primitiveFunctionType( &blockRepeat ), 1245185, signature_395, errors_395
};

static const char *signature_396[] = { "Self", "Object" };
static const char *errors_396[]    = { nullptr };
static PrimitiveDescriptor primitive_396 = {
        "primitiveScavenge", primitiveFunctionType( &SystemPrimitives::scavenge ), 1114113, signature_396, errors_396
};

static const char *signature_397[] = { "Object", "SmallInteger" };
static const char *errors_397[]    = { nullptr };
static PrimitiveDescriptor primitive_397 = {
        "primitiveSetInterpreterInvocationCounterLimitTo:ifFail:", primitiveFunctionType( &debugPrimitives::setInterpreterInvocationCounterLimit ), 327681, signature_397, errors_397
};

static const char *signature_398[] = { "Behavior", "Behavior", "Behavior" };
static const char *errors_398[]    = { "NotAClass", nullptr };
static PrimitiveDescriptor primitive_398 = {
        "primitiveSetSuperclassOf:toClass:ifFail:", primitiveFunctionType( &behaviorPrimitives::setSuperclass ), 327682, signature_398, errors_398
};

static const char *signature_399[] = { "Object", "Object" };
static const char *errors_399[]    = { "ReceiverHasWrongType", nullptr };
static PrimitiveDescriptor primitive_399 = {
        "primitiveShallowCopyIfFail:", primitiveFunctionType( &oopPrimitives::shallowCopy ), 1376257, signature_399, errors_399
};

static const char *signature_400[] = { "Object", "SmallInteger" };
static const char *errors_400[]    = { nullptr };
static PrimitiveDescriptor primitive_400 = {
        "primitiveShrinkMemory:", primitiveFunctionType( &SystemPrimitives::shrinkMemory ), 65537, signature_400, errors_400
};

static const char *signature_401[] = { "SmallInteger" };
static const char *errors_401[]    = { nullptr };
static PrimitiveDescriptor primitive_401 = {
        "primitiveSizeOfOop", primitiveFunctionType( &SystemPrimitives::oopSize ), 65536, signature_401, errors_401
};

static const char *signature_402[] = { "IndexedInstanceVariables" };
static const char *errors_402[]    = { nullptr };
static PrimitiveDescriptor primitive_402 = {
        "primitiveSlidingSystemAverageIfFail:", primitiveFunctionType( &SystemPrimitives::sliding_system_average ), 327680, signature_402, errors_402
};

static const char *signature_403[] = { "Boolean", "SmallInteger", "SmallInteger" };
static const char *errors_403[]    = { nullptr };
static PrimitiveDescriptor primitive_403 = {
        "primitiveSmallIntegerEqual:ifFail:", primitiveFunctionType( &smiOopPrimitives::equal ), 6029570, signature_403, errors_403
};

static const char *signature_404[] = { "SmallInteger", "Symbol" };
static const char *errors_404[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_404 = {
        "primitiveSmallIntegerFlagAt:ifFail:", primitiveFunctionType( &debugPrimitives::smiAt ), 327681, signature_404, errors_404
};

static const char *signature_405[] = { "Boolean", "Symbol", "Boolean" };
static const char *errors_405[]    = { "NotFound", nullptr };
static PrimitiveDescriptor primitive_405 = {
        "primitiveSmallIntegerFlagAt:put:ifFail:", primitiveFunctionType( &debugPrimitives::smiAtPut ), 327682, signature_405, errors_405
};

static const char *signature_406[] = { "Boolean", "SmallInteger", "SmallInteger" };
static const char *errors_406[]    = { nullptr };
static PrimitiveDescriptor primitive_406 = {
        "primitiveSmallIntegerNotEqual:ifFail:", primitiveFunctionType( &smiOopPrimitives::notEqual ), 6029570, signature_406, errors_406
};

static const char *signature_407[] = { "IndexedInstanceVariables" };
static const char *errors_407[]    = { nullptr };
static PrimitiveDescriptor primitive_407 = {
        "primitiveSmalltalkArray", primitiveFunctionType( &SystemPrimitives::smalltalk_array ), 65536, signature_407, errors_407
};

static const char *signature_408[] = { "GlobalAssociation", "Symbol", "Object" };
static const char *errors_408[]    = { nullptr };
static PrimitiveDescriptor primitive_408 = {
        "primitiveSmalltalkAt:Put:ifFail:", primitiveFunctionType( &SystemPrimitives::smalltalk_at_put ), 327682, signature_408, errors_408
};

static const char *signature_409[] = { "GlobalAssociation", "SmallInteger" };
static const char *errors_409[]    = { "OutOfBounds", nullptr };
static PrimitiveDescriptor primitive_409 = {
        "primitiveSmalltalkAt:ifFail:", primitiveFunctionType( &SystemPrimitives::smalltalk_at ), 327681, signature_409, errors_409
};

static const char *signature_410[] = { "GlobalAssociation", "SmallInteger" };
static const char *errors_410[]    = { nullptr };
static PrimitiveDescriptor primitive_410 = {
        "primitiveSmalltalkRemoveAt:ifFail:", primitiveFunctionType( &SystemPrimitives::smalltalk_remove_at ), 327681, signature_410, errors_410
};

static const char *signature_411[] = { "SmallInteger" };
static const char *errors_411[]    = { nullptr };
static PrimitiveDescriptor primitive_411 = {
        "primitiveSmalltalkSize", primitiveFunctionType( &SystemPrimitives::smalltalk_size ), 65536, signature_411, errors_411
};

static const char *signature_412[] = { "SmallInteger", "SmallInteger", "SmallInteger" };
static const char *errors_412[]    = { "Overflow", nullptr };
static PrimitiveDescriptor primitive_412 = {
        "primitiveSubtract:ifFail:", primitiveFunctionType( &smiOopPrimitives_subtract ), 6029826, signature_412, errors_412
};

static const char *signature_413[] = { "Behavior|Nil", "Behavior" };
static const char *errors_413[]    = { nullptr };
static PrimitiveDescriptor primitive_413 = {
        "primitiveSuperclass", primitiveFunctionType( &behaviorPrimitives::superclass ), 1114113, signature_413, errors_413
};

static const char *signature_414[] = { "Behavior|Nil", "Behavior" };
static const char *errors_414[]    = { nullptr };
static PrimitiveDescriptor primitive_414 = {
        "primitiveSuperclassOf:ifFail:", primitiveFunctionType( &behaviorPrimitives::superclass_of ), 327681, signature_414, errors_414
};

static const char *signature_415[] = { "SmallInteger", "IndexedByteInstanceVariables" };
static const char *errors_415[]    = { nullptr };
static PrimitiveDescriptor primitive_415 = {
        "primitiveSymbolNumberOfArguments", primitiveFunctionType( &byteArrayPrimitives::numberOfArguments ), 1574401, signature_415, errors_415
};

static const char *signature_416[] = { "Float" };
static const char *errors_416[]    = { nullptr };
static PrimitiveDescriptor primitive_416 = {
        "primitiveSystemTime", primitiveFunctionType( &SystemPrimitives::systemTime ), 65536, signature_416, errors_416
};

static const char *signature_417[] = { "Object" };
static const char *errors_417[]    = { nullptr };
static PrimitiveDescriptor primitive_417 = {
        "primitiveTimerPrintBuffer", primitiveFunctionType( &debugPrimitives::timerPrintBuffer ), 65536, signature_417, errors_417
};

static const char *signature_418[] = { "Object" };
static const char *errors_418[]    = { nullptr };
static PrimitiveDescriptor primitive_418 = {
        "primitiveTimerStart", primitiveFunctionType( &debugPrimitives::timerStart ), 65536, signature_418, errors_418
};

static const char *signature_419[] = { "Object" };
static const char *errors_419[]    = { nullptr };
static PrimitiveDescriptor primitive_419 = {
        "primitiveTimerStop", primitiveFunctionType( &debugPrimitives::timerStop ), 65536, signature_419, errors_419
};

static const char *signature_420[] = { "Object" };
static const char *errors_420[]    = { nullptr };
static PrimitiveDescriptor primitive_420 = {
        "primitiveTraceStack", primitiveFunctionType( &SystemPrimitives::traceStack ), 65536, signature_420, errors_420
};

static const char *signature_421[] = { "Object", "BlockWithoutArguments", "BlockWithoutArguments" };
static const char *errors_421[]    = { nullptr };
static PrimitiveDescriptor primitive_421 = {
        "primitiveUnwindProtect:ifFail:", primitiveFunctionType( &unwindprotect ), 1507330, signature_421, errors_421
};

static const char *signature_422[] = { "Float" };
static const char *errors_422[]    = { nullptr };
static PrimitiveDescriptor primitive_422 = {
        "primitiveUserTime", primitiveFunctionType( &SystemPrimitives::userTime ), 65536, signature_422, errors_422
};

static const char *signature_423[] = { "Object" };
static const char *errors_423[]    = { nullptr };
static PrimitiveDescriptor primitive_423 = {
        "primitiveVMBreakpoint", primitiveFunctionType( &SystemPrimitives::vmbreakpoint ), 65536, signature_423, errors_423
};

static const char *signature_424[] = { "Object", "BlockWithoutArguments" };
static const char *errors_424[]    = { nullptr };
static PrimitiveDescriptor primitive_424 = {
        "primitiveValue", primitiveFunctionType( &primitiveValue0 ), 5441537, signature_424, errors_424
};

static const char *signature_425[] = { "Object", "BlockWithOneArgument", "Object" };
static const char *errors_425[]    = { nullptr };
static PrimitiveDescriptor primitive_425 = {
        "primitiveValue:", primitiveFunctionType( &primitiveValue1 ), 5441538, signature_425, errors_425
};

static const char *signature_426[] = { "Object", "BlockWithTwoArguments", "Object", "Object" };
static const char *errors_426[]    = { nullptr };
static PrimitiveDescriptor primitive_426 = {
        "primitiveValue:value:", primitiveFunctionType( &primitiveValue2 ), 5441539, signature_426, errors_426
};

static const char *signature_427[] = { "Object", "BlockWithThreeArguments", "Object", "Object", "Object" };
static const char *errors_427[]    = { nullptr };
static PrimitiveDescriptor primitive_427 = {
        "primitiveValue:value:value:", primitiveFunctionType( &primitiveValue3 ), 5441540, signature_427, errors_427
};

static const char *signature_428[] = { "Object", "BlockWithFourArguments", "Object", "Object", "Object", "Object" };
static const char *errors_428[]    = { nullptr };
static PrimitiveDescriptor primitive_428 = {
        "primitiveValue:value:value:value:", primitiveFunctionType( &primitiveValue4 ), 5441541, signature_428, errors_428
};

static const char *signature_429[] = { "Object", "BlockWithFiveArguments", "Object", "Object", "Object", "Object", "Object" };
static const char *errors_429[]    = { nullptr };
static PrimitiveDescriptor primitive_429 = {
        "primitiveValue:value:value:value:value:", primitiveFunctionType( &primitiveValue5 ), 5441542, signature_429, errors_429
};

static const char *signature_430[] = { "Object", "BlockWithSixArguments", "Object", "Object", "Object", "Object", "Object", "Object" };
static const char *errors_430[]    = { nullptr };
static PrimitiveDescriptor primitive_430 = {
        "primitiveValue:value:value:value:value:value:", primitiveFunctionType( &primitiveValue6 ), 5441543, signature_430, errors_430
};

static const char *signature_431[] = { "Object", "BlockWithSevenArguments", "Object", "Object", "Object", "Object", "Object", "Object", "Object" };
static const char *errors_431[]    = { nullptr };
static PrimitiveDescriptor primitive_431 = {
        "primitiveValue:value:value:value:value:value:value:", primitiveFunctionType( &primitiveValue7 ), 5441544, signature_431, errors_431
};

static const char *signature_432[] = { "Object", "BlockWithEightArguments", "Object", "Object", "Object", "Object", "Object", "Object", "Object", "Object" };
static const char *errors_432[]    = { nullptr };
static PrimitiveDescriptor primitive_432 = {
        "primitiveValue:value:value:value:value:value:value:value:", primitiveFunctionType( &primitiveValue8 ), 5441545, signature_432, errors_432
};

static const char *signature_433[] = { "Object", "BlockWithNineArguments", "Object", "Object", "Object", "Object", "Object", "Object", "Object", "Object", "Object" };
static const char *errors_433[]    = { nullptr };
static PrimitiveDescriptor primitive_433 = {
        "primitiveValue:value:value:value:value:value:value:value:value:", primitiveFunctionType( &primitiveValue9 ), 5441546, signature_433, errors_433
};

static const char *signature_434[] = { "Object" };
static const char *errors_434[]    = { nullptr };
static PrimitiveDescriptor primitive_434 = {
        "primitiveVerify", primitiveFunctionType( &debugPrimitives::verify ), 65536, signature_434, errors_434
};

static const char *signature_435[] = { "Proxy", "Proxy" };
static const char *errors_435[]    = { nullptr };
static PrimitiveDescriptor primitive_435 = {
        "primitiveWindowsHInstance:ifFail:", primitiveFunctionType( &SystemPrimitives::windowsHInstance ), 327681, signature_435, errors_435
};

static const char *signature_436[] = { "Proxy", "Proxy" };
static const char *errors_436[]    = { nullptr };
static PrimitiveDescriptor primitive_436 = {
        "primitiveWindowsHPrevInstance:ifFail:", primitiveFunctionType( &SystemPrimitives::windowsHPrevInstance ), 327681, signature_436, errors_436
};

static const char *signature_437[] = { "Object" };
static const char *errors_437[]    = { nullptr };
static PrimitiveDescriptor primitive_437 = {
        "primitiveWindowsNCmdShow", primitiveFunctionType( &SystemPrimitives::windowsNCmdShow ), 65536, signature_437, errors_437
};

static const char *signature_438[] = { "Object", "String" };
static const char *errors_438[]    = { nullptr };
static PrimitiveDescriptor primitive_438 = {
        "primitiveWriteSnapshot:", primitiveFunctionType( &SystemPrimitives::writeSnapshot ), 65537, signature_438, errors_438
};

static std::int32_t size_of_primitive_table = 439;
static PrimitiveDescriptor *primitive_table[] = {
        &primitive_0, \
        &primitive_1, \
        &primitive_2, \
        &primitive_3, \
        &primitive_4, \
        &primitive_5, \
        &primitive_6, \
        &primitive_7, \
        &primitive_8, \
        &primitive_9, \
        &primitive_10, \
        &primitive_11, \
        &primitive_12, \
        &primitive_13, \
        &primitive_14, \
        &primitive_15, \
        &primitive_16, \
        &primitive_17, \
        &primitive_18, \
        &primitive_19, \
        &primitive_20, \
        &primitive_21, \
        &primitive_22, \
        &primitive_23, \
        &primitive_24, \
        &primitive_25, \
        &primitive_26, \
        &primitive_27, \
        &primitive_28, \
        &primitive_29, \
        &primitive_30, \
        &primitive_31, \
        &primitive_32, \
        &primitive_33, \
        &primitive_34, \
        &primitive_35, \
        &primitive_36, \
        &primitive_37, \
        &primitive_38, \
        &primitive_39, \
        &primitive_40, \
        &primitive_41, \
        &primitive_42, \
        &primitive_43, \
        &primitive_44, \
        &primitive_45, \
        &primitive_46, \
        &primitive_47, \
        &primitive_48, \
        &primitive_49, \
        &primitive_50, \
        &primitive_51, \
        &primitive_52, \
        &primitive_53, \
        &primitive_54, \
        &primitive_55, \
        &primitive_56, \
        &primitive_57, \
        &primitive_58, \
        &primitive_59, \
        &primitive_60, \
        &primitive_61, \
        &primitive_62, \
        &primitive_63, \
        &primitive_64, \
        &primitive_65, \
        &primitive_66, \
        &primitive_67, \
        &primitive_68, \
        &primitive_69, \
        &primitive_70, \
        &primitive_71, \
        &primitive_72, \
        &primitive_73, \
        &primitive_74, \
        &primitive_75, \
        &primitive_76, \
        &primitive_77, \
        &primitive_78, \
        &primitive_79, \
        &primitive_80, \
        &primitive_81, \
        &primitive_82, \
        &primitive_83, \
        &primitive_84, \
        &primitive_85, \
        &primitive_86, \
        &primitive_87, \
        &primitive_88, \
        &primitive_89, \
        &primitive_90, \
        &primitive_91, \
        &primitive_92, \
        &primitive_93, \
        &primitive_94, \
        &primitive_95, \
        &primitive_96, \
        &primitive_97, \
        &primitive_98, \
        &primitive_99, \
        &primitive_100, \
        &primitive_101, \
        &primitive_102, \
        &primitive_103, \
        &primitive_104, \
        &primitive_105, \
        &primitive_106, \
        &primitive_107, \
        &primitive_108, \
        &primitive_109, \
        &primitive_110, \
        &primitive_111, \
        &primitive_112, \
        &primitive_113, \
        &primitive_114, \
        &primitive_115, \
        &primitive_116, \
        &primitive_117, \
        &primitive_118, \
        &primitive_119, \
        &primitive_120, \
        &primitive_121, \
        &primitive_122, \
        &primitive_123, \
        &primitive_124, \
        &primitive_125, \
        &primitive_126, \
        &primitive_127, \
        &primitive_128, \
        &primitive_129, \
        &primitive_130, \
        &primitive_131, \
        &primitive_132, \
        &primitive_133, \
        &primitive_134, \
        &primitive_135, \
        &primitive_136, \
        &primitive_137, \
        &primitive_138, \
        &primitive_139, \
        &primitive_140, \
        &primitive_141, \
        &primitive_142, \
        &primitive_143, \
        &primitive_144, \
        &primitive_145, \
        &primitive_146, \
        &primitive_147, \
        &primitive_148, \
        &primitive_149, \
        &primitive_150, \
        &primitive_151, \
        &primitive_152, \
        &primitive_153, \
        &primitive_154, \
        &primitive_155, \
        &primitive_156, \
        &primitive_157, \
        &primitive_158, \
        &primitive_159, \
        &primitive_160, \
        &primitive_161, \
        &primitive_162, \
        &primitive_163, \
        &primitive_164, \
        &primitive_165, \
        &primitive_166, \
        &primitive_167, \
        &primitive_168, \
        &primitive_169, \
        &primitive_170, \
        &primitive_171, \
        &primitive_172, \
        &primitive_173, \
        &primitive_174, \
        &primitive_175, \
        &primitive_176, \
        &primitive_177, \
        &primitive_178, \
        &primitive_179, \
        &primitive_180, \
        &primitive_181, \
        &primitive_182, \
        &primitive_183, \
        &primitive_184, \
        &primitive_185, \
        &primitive_186, \
        &primitive_187, \
        &primitive_188, \
        &primitive_189, \
        &primitive_190, \
        &primitive_191, \
        &primitive_192, \
        &primitive_193, \
        &primitive_194, \
        &primitive_195, \
        &primitive_196, \
        &primitive_197, \
        &primitive_198, \
        &primitive_199, \
        &primitive_200, \
        &primitive_201, \
        &primitive_202, \
        &primitive_203, \
        &primitive_204, \
        &primitive_205, \
        &primitive_206, \
        &primitive_207, \
        &primitive_208, \
        &primitive_209, \
        &primitive_210, \
        &primitive_211, \
        &primitive_212, \
        &primitive_213, \
        &primitive_214, \
        &primitive_215, \
        &primitive_216, \
        &primitive_217, \
        &primitive_218, \
        &primitive_219, \
        &primitive_220, \
        &primitive_221, \
        &primitive_222, \
        &primitive_223, \
        &primitive_224, \
        &primitive_225, \
        &primitive_226, \
        &primitive_227, \
        &primitive_228, \
        &primitive_229, \
        &primitive_230, \
        &primitive_231, \
        &primitive_232, \
        &primitive_233, \
        &primitive_234, \
        &primitive_235, \
        &primitive_236, \
        &primitive_237, \
        &primitive_238, \
        &primitive_239, \
        &primitive_240, \
        &primitive_241, \
        &primitive_242, \
        &primitive_243, \
        &primitive_244, \
        &primitive_245, \
        &primitive_246, \
        &primitive_247, \
        &primitive_248, \
        &primitive_249, \
        &primitive_250, \
        &primitive_251, \
        &primitive_252, \
        &primitive_253, \
        &primitive_254, \
        &primitive_255, \
        &primitive_256, \
        &primitive_257, \
        &primitive_258, \
        &primitive_259, \
        &primitive_260, \
        &primitive_261, \
        &primitive_262, \
        &primitive_263, \
        &primitive_264, \
        &primitive_265, \
        &primitive_266, \
        &primitive_267, \
        &primitive_268, \
        &primitive_269, \
        &primitive_270, \
        &primitive_271, \
        &primitive_272, \
        &primitive_273, \
        &primitive_274, \
        &primitive_275, \
        &primitive_276, \
        &primitive_277, \
        &primitive_278, \
        &primitive_279, \
        &primitive_280, \
        &primitive_281, \
        &primitive_282, \
        &primitive_283, \
        &primitive_284, \
        &primitive_285, \
        &primitive_286, \
        &primitive_287, \
        &primitive_288, \
        &primitive_289, \
        &primitive_290, \
        &primitive_291, \
        &primitive_292, \
        &primitive_293, \
        &primitive_294, \
        &primitive_295, \
        &primitive_296, \
        &primitive_297, \
        &primitive_298, \
        &primitive_299, \
        &primitive_300, \
        &primitive_301, \
        &primitive_302, \
        &primitive_303, \
        &primitive_304, \
        &primitive_305, \
        &primitive_306, \
        &primitive_307, \
        &primitive_308, \
        &primitive_309, \
        &primitive_310, \
        &primitive_311, \
        &primitive_312, \
        &primitive_313, \
        &primitive_314, \
        &primitive_315, \
        &primitive_316, \
        &primitive_317, \
        &primitive_318, \
        &primitive_319, \
        &primitive_320, \
        &primitive_321, \
        &primitive_322, \
        &primitive_323, \
        &primitive_324, \
        &primitive_325, \
        &primitive_326, \
        &primitive_327, \
        &primitive_328, \
        &primitive_329, \
        &primitive_330, \
        &primitive_331, \
        &primitive_332, \
        &primitive_333, \
        &primitive_334, \
        &primitive_335, \
        &primitive_336, \
        &primitive_337, \
        &primitive_338, \
        &primitive_339, \
        &primitive_340, \
        &primitive_341, \
        &primitive_342, \
        &primitive_343, \
        &primitive_344, \
        &primitive_345, \
        &primitive_346, \
        &primitive_347, \
        &primitive_348, \
        &primitive_349, \
        &primitive_350, \
        &primitive_351, \
        &primitive_352, \
        &primitive_353, \
        &primitive_354, \
        &primitive_355, \
        &primitive_356, \
        &primitive_357, \
        &primitive_358, \
        &primitive_359, \
        &primitive_360, \
        &primitive_361, \
        &primitive_362, \
        &primitive_363, \
        &primitive_364, \
        &primitive_365, \
        &primitive_366, \
        &primitive_367, \
        &primitive_368, \
        &primitive_369, \
        &primitive_370, \
        &primitive_371, \
        &primitive_372, \
        &primitive_373, \
        &primitive_374, \
        &primitive_375, \
        &primitive_376, \
        &primitive_377, \
        &primitive_378, \
        &primitive_379, \
        &primitive_380, \
        &primitive_381, \
        &primitive_382, \
        &primitive_383, \
        &primitive_384, \
        &primitive_385, \
        &primitive_386, \
        &primitive_387, \
        &primitive_388, \
        &primitive_389, \
        &primitive_390, \
        &primitive_391, \
        &primitive_392, \
        &primitive_393, \
        &primitive_394, \
        &primitive_395, \
        &primitive_396, \
        &primitive_397, \
        &primitive_398, \
        &primitive_399, \
        &primitive_400, \
        &primitive_401, \
        &primitive_402, \
        &primitive_403, \
        &primitive_404, \
        &primitive_405, \
        &primitive_406, \
        &primitive_407, \
        &primitive_408, \
        &primitive_409, \
        &primitive_410, \
        &primitive_411, \
        &primitive_412, \
        &primitive_413, \
        &primitive_414, \
        &primitive_415, \
        &primitive_416, \
        &primitive_417, \
        &primitive_418, \
        &primitive_419, \
        &primitive_420, \
        &primitive_421, \
        &primitive_422, \
        &primitive_423, \
        &primitive_424, \
        &primitive_425, \
        &primitive_426, \
        &primitive_427, \
        &primitive_428, \
        &primitive_429, \
        &primitive_430, \
        &primitive_431, \
        &primitive_432, \
        &primitive_433, \
        &primitive_434, \
        &primitive_435, \
        &primitive_436, \
        &primitive_437, \
        &primitive_438
};
