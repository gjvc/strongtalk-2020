//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/system/asserts.hpp"


//
//    ByteCodes comprise the definition of all bytecodes and provides utility functions working on bytecodes.
//
//    Naming conventions for send bytecodes::
//
//        a) <send type>_send_<argument specification>
//        b) <send_type>_send_<argument specification>_pop
//        c) <name of predicted smi_t selector>
//
//
//    <send_type> is one of:
//
//
//        interpreted	monomorphic send to interpreted method
//        compiled	    monomorphic send to compiled method
//        primitive	    monomorphic send to primitive method
//        access	    monomorphic send to access method
//        polymorphic	polymorphic send to interpreted or compiled method
//        megamorphic	megamorphic send to interpreted or compiled method
//
//
//    <argument specification> is one of:
//
//
//
//        0	        receiver & 0 arguments on stack, normal send
//        1	        receiver & 1 argument  on stack, normal send
//        2	        receiver & 2 arguments on stack, normal send
//        n	        receiver & n arguments on stack, normal send
//        self	    no receiver, arguments on stack, normal send
//        super	    no receiver, arguments on stack, super  send
//
//
//  EXAMPLES
//
//
//      interpreted_send_self_pop
//          interpreted	monomorphic send to interpreted method
//          self	    no receiver, arguments on stack, normal send
//
//


class ByteCodes : AllStatic {

public:
    // Returns the bytecode set version number.
    // Must match the version number generated by the Digitalk system.
    static std::size_t version() {
        return 2;
    }


    enum class Format { // Format of instruction: * means align to Oop
        B,          // {byte}
        BB,         // {byte, byte}
        BBB,        // {byte, byte, byte}
        BBBB,       // {byte, byte, byte, byte}
        BBO,        // {byte, byte, *, Oop}
        BBL,        // {byte, byte, *, std::int32_t}
        BO,         // {byte, *, Oop}
        BOO,        // {byte, *, Oop, Oop}
        BLO,        // {byte, *, std::int32_t, Oop}
        BOL,        // {byte, *, Oop, std::int32_t}
        BLL,        // {byte, *, std::int32_t, std::int32_t}
        BL,         // {byte, *, std::int32_t}
        BLB,        // {byte, *, byte} slr - surely this should be {byte, *, std::int32_t, byte}?
        BBOO,       // {byte, byte, *, Oop, Oop}
        BBLO,       // {byte, byte, *, std::int32_t, Oop}
        BOOLB,      // {byte, *, Oop, Oop, std::int32_t, byte}
        BBS,        // {byte, byte = number of bytes to follow, {byte}*}
        UNDEFINED,  // for undefined codes

        NUMBER_OF_FORMATS
    };

    enum class CodeType {       // Instruction classification
        local_access,       // loads & stores of temps and args
        instVar_access,     // loads & stores of instVars
        context_access,     // loads & stores of context temps
        classVar_access,    // loads & stores to class variables
        global_access,      // loads & stores to global variables
        new_closure,        // closure creation
        new_context,        // context creation
        control_structure,  // control structures (incl. local return)
        message_send,       // all sends
        nonlocal_return,    // non-local returns
        primitive_call,     // primitive calls
        dll_call,           // dll calls
        float_operation,    // float operations
        miscellaneous,      // for all other instructions

        NUMBER_OF_CODE_TYPES
    };

    enum class ArgumentSpec {     // Argument specification for sends
        recv_0_args,        // recv & 0 arguments on stack
        recv_1_args,        // recv & 1 arguments on stack
        recv_2_args,        // recv & 2 arguments on stack
        recv_n_args,        // recv & n arguments on stack
        args_only,          // only arguments on stack
        no_args,            // for non-send instructions

        NUMBER_OF_ARGUMENT_SPECS
    };


    // Send classification
    enum class SendType {
        interpreted_send,   // interpreted monomorphic send
        compiled_send,      // compiled monomorphic send
        polymorphic_send,   // interpreted polymorphic send
        megamorphic_send,   // interpreted megamorphic send
        predicted_send,     // send to interpreted predicted method (method containing a predicted primitive call with own bytecode)
        accessor_send,      // send to interpreted accessor method (method containing a predicted instVar access)
        primitive_send,     // send to interpreted primitive method (method containing a predicted primitive call)
        no_send,            // for non-send instructions

        NUMBER_OF_SEND_TYPES
    };


    // Loop classification
    enum class LoopType {
        loop_start,         // instruction starting a loop
        loop_end,           // instruction ending a loop
        no_loop,            // for non-loop instructions

        NUMBER_OF_LOOP_TYPES
    };

    enum class Code {

        // row 0x00
        push_temp_0      = 0x00, //
        push_temp_1      = 0x01, //
        push_temp_2      = 0x02, //
        push_temp_3      = 0x03, //
        push_temp_4      = 0x04, //
        push_temp_5      = 0x05, //
        unimplemented_06 = 0x06, //
        push_temp_n      = 0x07, //
        push_arg_1       = 0x08, // n-1
        push_arg_2       = 0x09, // n-2
        push_arg_3       = 0x0a, // n-3
        push_arg_n       = 0x0b, // n-1-b
        allocate_temp_1  = 0x0c, //
        allocate_temp_2  = 0x0d, //
        allocate_temp_3  = 0x0e, //
        allocate_temp_n  = 0x0f, // 0 means 256

        // row 0x01
        store_temp_0_pop = 0x10, //
        store_temp_1_pop = 0x11, //
        store_temp_2_pop = 0x12, //
        store_temp_3_pop = 0x13, //
        store_temp_4_pop = 0x14, //
        store_temp_5_pop = 0x15, //
        store_temp_n     = 0x16, // 255-b
        store_temp_n_pop = 0x17, // 255-b
        push_neg_n       = 0x18, // -b
        push_succ_n      = 0x19, // b+1
        push_literal     = 0x1a, //
        push_tos         = 0x1b, //
        push_self        = 0x1c, //
        push_nil         = 0x1d, //
        push_true        = 0x1e, //
        push_false       = 0x1f, //

        // row 0x02
        unimplemented_20    = 0x20, //
        unimplemented_21    = 0x21, //
        unimplemented_22    = 0x22, //
        unimplemented_23    = 0x23, //
        unimplemented_24    = 0x24, //
        unimplemented_25    = 0x25, //
        unimplemented_26    = 0x26, //
        unimplemented_27    = 0x27, //
        return_instVar_name = 0x28, //
        push_classVar       = 0x29, //
        store_classVar_pop  = 0x2a, //
        store_classVar      = 0x2b, //
        return_instVar      = 0x2c, //
        push_instVar        = 0x2d, //
        store_instVar_pop   = 0x2e, //
        store_instVar       = 0x2f, //

        // row 0x03
        float_allocate         = 0x30, //
        float_floatify_pop     = 0x31, //
        float_move             = 0x32, //
        float_set              = 0x33, //
        float_nullary_op       = 0x34, //
        float_unary_op         = 0x35, //
        float_binary_op        = 0x36, //
        float_unary_op_to_oop  = 0x37, //
        float_binary_op_to_oop = 0x38, //
        unimplemented_39       = 0x39, //
        unimplemented_3a       = 0x3a, //
        unimplemented_3b       = 0x3b, //
        unimplemented_3c       = 0x3c, //
        push_instVar_name      = 0x3d, //
        store_instVar_pop_name = 0x3e, //
        store_instVar_name     = 0x3f, //

        // row 0x04
        push_temp_0_context_0        = 0x40, //
        push_temp_1_context_0        = 0x41, //
        push_temp_2_context_0        = 0x42, //
        push_temp_n_context_0        = 0x43, //
        store_temp_0_context_0_pop   = 0x44, //
        store_temp_1_context_0_pop   = 0x45, //
        store_temp_2_context_0_pop   = 0x46, //
        store_temp_n_context_0_pop   = 0x47, //
        push_new_closure_context_0   = 0x48, //
        push_new_closure_context_1   = 0x49, //
        push_new_closure_context_2   = 0x4a, //
        push_new_closure_context_n   = 0x4b, //
        install_new_context_method_0 = 0x4c, //
        install_new_context_method_1 = 0x4d, //
        install_new_context_method_2 = 0x4e, //
        install_new_context_method_n = 0x4f, //

        // row 0x05
        push_temp_0_context_1       = 0x50, //
        push_temp_1_context_1       = 0x51, //
        push_temp_2_context_1       = 0x52, //
        push_temp_n_context_1       = 0x53, //
        store_temp_0_context_1_pop  = 0x54, //
        store_temp_1_context_1_pop  = 0x55, //
        store_temp_2_context_1_pop  = 0x56, //
        store_temp_n_context_1_pop  = 0x57, //
        push_new_closure_tos_0      = 0x58, //
        push_new_closure_tos_1      = 0x59, //
        push_new_closure_tos_2      = 0x5a, //
        push_new_closure_tos_n      = 0x5b, //
        only_pop                    = 0x5c, //
        install_new_context_block_1 = 0x5d, //
        install_new_context_block_2 = 0x5e, //
        install_new_context_block_n = 0x5f, //

        // row 0x06
        push_temp_0_context_n      = 0x60, //
        push_temp_1_context_n      = 0x61, //
        push_temp_2_context_n      = 0x62, //
        push_temp_n_context_n      = 0x63, //
        store_temp_0_context_n_pop = 0x64, //
        store_temp_1_context_n_pop = 0x65, //
        store_temp_2_context_n_pop = 0x66, //
        store_temp_n_context_n_pop = 0x67, //
        set_self_via_context       = 0x68, //
        copy_1_into_context        = 0x69, //
        copy_2_into_context        = 0x6a, //
        copy_n_into_context        = 0x6b, //
        copy_self_into_context     = 0x6c, //
        copy_self_1_into_context   = 0x6d, //
        copy_self_2_into_context   = 0x6e, //
        copy_self_n_into_context   = 0x6f, //

        // row 0x07
        ifTrue_byte     = 0x70, //
        ifFalse_byte    = 0x71, //
        and_byte        = 0x72, //
        or_byte         = 0x73, //
        whileTrue_byte  = 0x74, //
        whileFalse_byte = 0x75, //
        jump_else_byte  = 0x76, //
        jump_loop_byte  = 0x77, //
        ifTrue_word     = 0x78, //
        ifFalse_word    = 0x79, //
        and_word        = 0x7a, //
        or_word         = 0x7b, //
        whileTrue_word  = 0x7c, //
        whileFalse_word = 0x7d, //
        jump_else_word  = 0x7e, //
        jump_loop_word  = 0x7f, //

        // row 0x08
        interpreted_send_0         = 0x80, //
        interpreted_send_1         = 0x81, //
        interpreted_send_2         = 0x82, //
        interpreted_send_n         = 0x83, //
        interpreted_send_0_pop     = 0x84, //
        interpreted_send_1_pop     = 0x85, //
        interpreted_send_2_pop     = 0x86, //
        interpreted_send_n_pop     = 0x87, //
        interpreted_send_self      = 0x88, //
        interpreted_send_self_pop  = 0x89, //
        interpreted_send_super     = 0x8a, //
        interpreted_send_super_pop = 0x8b, //
        return_tos_pop_0           = 0x8c, //
        return_tos_pop_1           = 0x8d, //
        return_tos_pop_2           = 0x8e, //
        return_tos_pop_n           = 0x8f, //

        // row 0x09
        polymorphic_send_0         = 0x90, //
        polymorphic_send_1         = 0x91, //
        polymorphic_send_2         = 0x92, //
        polymorphic_send_n         = 0x93, //
        polymorphic_send_0_pop     = 0x94, //
        polymorphic_send_1_pop     = 0x95, //
        polymorphic_send_2_pop     = 0x96, //
        polymorphic_send_n_pop     = 0x97, //
        polymorphic_send_self      = 0x98, //
        polymorphic_send_self_pop  = 0x99, //
        polymorphic_send_super     = 0x9a, //
        polymorphic_send_super_pop = 0x9b, //
        return_self_pop_0          = 0x9c, //
        return_self_pop_1          = 0x9d, //
        return_self_pop_2          = 0x9e, //
        return_self_pop_n          = 0x9f, //

        // row 0x0a
        compiled_send_0             = 0xa0, //
        compiled_send_1             = 0xa1, //
        compiled_send_2             = 0xa2, //
        compiled_send_n             = 0xa3, //
        compiled_send_0_pop         = 0xa4, //
        compiled_send_1_pop         = 0xa5, //
        compiled_send_2_pop         = 0xa6, //
        compiled_send_n_pop         = 0xa7, //
        compiled_send_self          = 0xa8, //
        compiled_send_self_pop      = 0xa9, //
        compiled_send_super         = 0xaa, //
        compiled_send_super_pop     = 0xab, //
        return_tos_zap_pop_n        = 0xac, //
        return_self_zap_pop_n       = 0xad, //
        non_local_return_tos_pop_n  = 0xae, //
        non_local_return_self_pop_n = 0xaf, //

        // row 0x0b
        prim_call                      = 0xb0, //
        predict_primitive_call         = 0xb1, //
        primitive_call_failure         = 0xb2, //
        predict_primitive_call_failure = 0xb3, //
        dll_call_sync                  = 0xb4, //
        primitive_call_self            = 0xb5, //
        primitive_call_self_failure    = 0xb6, //
        unimplemented_b7               = 0xb7, //
        access_send_self               = 0xb8, //
        primitive_send_0               = 0xb9, //
        primitive_send_super           = 0xba, //
        primitive_send_super_pop       = 0xbb, //
        unimplemented_bc               = 0xbc, //
        primitive_send_1               = 0xbd, //
        primitive_send_2               = 0xbe, //
        primitive_send_n               = 0xbf, //

        // row 0x0c
        primitive_call_lookup                 = 0xc0, //
        predict_primitive_call_lookup         = 0xc1, //
        primitive_call_failure_lookup         = 0xc2, //
        predict_primitive_call_failure_lookup = 0xc3, //
        dll_call_async                        = 0xc4, //
        primitive_call_self_lookup            = 0xc5, //
        primitive_call_self_failure_lookup    = 0xc6, //
        unimplemented_c7                      = 0xc7, //
        access_send_0                         = 0xc8, //
        primitive_send_0_pop                  = 0xc9, //
        primitive_send_self                   = 0xca, //
        primitive_send_self_pop               = 0xcb, //
        unimplemented_cc                      = 0xcc, //
        primitive_send_1_pop                  = 0xcd, //
        primitive_send_2_pop                  = 0xce, //
        primitive_send_n_pop                  = 0xcf, //

        // row 0x0d
        megamorphic_send_0            = 0xd0, //
        megamorphic_send_1            = 0xd1, //
        megamorphic_send_2            = 0xd2, //
        megamorphic_send_n            = 0xd3, //
        megamorphic_send_0_pop        = 0xd4, //
        megamorphic_send_1_pop        = 0xd5, //
        megamorphic_send_2_pop        = 0xd6, //
        megamorphic_send_n_pop        = 0xd7, //
        megamorphic_send_self         = 0xd8, //
        megamorphic_send_self_pop     = 0xd9, //
        megamorphic_send_super        = 0xda, //
        megamorphic_send_super_pop    = 0xdb, //
        unimplemented_dc              = 0xdc, //
        special_primitive_send_1_hint = 0xdd, //
        unimplemented_de              = 0xde, //
        unimplemented_df              = 0xdf, //

        // row 0x0e
        smi_add           = 0xe0, //
        smi_sub           = 0xe1, //
        smi_mult          = 0xe2, //
        smi_div           = 0xe3, //
        smi_mod           = 0xe4, //
        smi_create_point  = 0xe5, //
        smi_equal         = 0xe6, //
        smi_not_equal     = 0xe7, //
        smi_less          = 0xe8, //
        smi_less_equal    = 0xe9, //
        smi_greater       = 0xea, //
        smi_greater_equal = 0xeb, //
        objArray_at       = 0xec, //
        objArray_at_put   = 0xed, //
        double_equal      = 0xee, //
        double_tilde      = 0xef, //

        // row 0x0f
        push_global             = 0xf0, //
        store_global_pop        = 0xf1, //
        store_global            = 0xf2, //
        push_classVar_name      = 0xf3, //
        store_classVar_pop_name = 0xf4, //
        store_classVar_name     = 0xf5, //
        smi_and                 = 0xf6, //
        smi_or                  = 0xf7, //
        smi_xor                 = 0xf8, //
        smi_shift               = 0xf9, //
        unimplemented_fa        = 0xfa, //
        unimplemented_fb        = 0xfb, //
        unimplemented_fc        = 0xfc, //
        unimplemented_fd        = 0xfd, //
        unimplemented_fe        = 0xfe, //
        halt                    = 0xff, //

        //
        NUMBER_OF_CODES = 0x100 //

    };


private:
    static const char   *_entry_point[static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)];
    static const char   *_name[static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)];
    static Format       _format[static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)];
    static CodeType     _code_type[static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)];
    static ArgumentSpec _argument_spec[static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)];
    static SendType     _send_type[static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)];
    static bool_t       _single_step[static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)];
    static bool_t       _pop_tos[static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)];

    static void def( Code code );

    static void def( Code code, const char *name, Format format, CodeType code_type, bool_t single_step, bool_t pop_tos = false );

    static void def( Code code, const char *name, Format format, ArgumentSpec argument_spec, SendType send_type, bool_t pop_tos = false );

    static void def( Code code, const char *name, Format format, CodeType code_type, bool_t single_step, ArgumentSpec argument_spec, SendType send_type, bool_t pop_tos );

public:
    // Define entry points
    static void set_entry_point( Code code, const char *entry_point );


    // Testers
    static bool_t is_defined( Code code ) {
        return 0 <= static_cast<std::size_t>(code)
               and static_cast<std::size_t>(code) < static_cast<std::size_t>(ByteCodes::Code::NUMBER_OF_CODES)
               and _format[ static_cast<std::size_t>(code) ] not_eq ByteCodes::Format::UNDEFINED;
    }


    static bool_t is_self_send( Code code );

    static bool_t is_super_send( Code code );

    static bool_t has_access_send_code( Code code );

    static bool_t has_predicted_send_code( Code code );


    static bool_t is_send_code( Code code ) {
        return send_type( code ) not_eq ByteCodes::SendType::no_send;
    }


    // Bytecode specification
    static const char *entry_point( const Code code ) {
        return _entry_point[ static_cast<std::size_t>(code) ];
    }


    static const char *name( const Code code ) {
        return _name[ static_cast<std::size_t>(code) ];
    }


    static Format format( const Code code ) {
        return _format[ static_cast<std::size_t>(code) ];
    }


    static CodeType code_type( const Code code ) {
        return _code_type[ static_cast<std::size_t>(code) ];
    }


    static ArgumentSpec argument_spec( const Code code ) {
        return _argument_spec[ static_cast<std::size_t>(code) ];
    }


    static SendType send_type( const Code code ) {
        return _send_type[ static_cast<std::size_t>(code) ];
    }


    static bool_t single_step( const Code code ) {
        return _single_step[ static_cast<std::size_t>(code) ];
    }


    static bool_t pop_tos( const Code code ) {
        return _pop_tos[ static_cast<std::size_t>(code) ];
    }


    static LoopType loop_type( const Code code );

    // Helpers for printing
    static const char *format_as_string( Format format );

    static const char *code_type_as_string( CodeType code_type );

    static const char *argument_spec_as_string( ArgumentSpec argument_spec );

    static const char *send_type_as_string( SendType send_type );

    static const char *loop_type_as_string( LoopType loop_type );

    //
    // Send bytecode transitions
    //
    // The following functions return the corresponding interpreted, compiled,
    // polymorphic or megamorphic send bytecode for a given send bytecode.
    //
    // They are used to implement the bytecode transitions during interpreter inline cache misses.
    //

    static Code original_send_code_for( Code code );        // predicted sends keep original code (smi_add -> smi_add)
    static Code interpreted_send_code_for( Code code );     // predicted sends loose their original value (smi_add -> int'_send_1)
    static Code compiled_send_code_for( Code code );

    static Code access_send_code_for( Code code );

    static Code primitive_send_code_for( Code code );

    static Code polymorphic_send_code_for( Code code );

    static Code megamorphic_send_code_for( Code code );


    // Primitive lookup bytecode transitions
    //
    // The following function returns the corresponding primitive call bytecode for a given primitive call lookup bytecode.
    // It is used to implement the bytecode transition during a primitive lookup.

    static Code original_primitive_call_code_for( Code code );

    static Code primitive_call_code_for( Code code );

    // Initialization/debugging
    static void init();

    static void print();
};
