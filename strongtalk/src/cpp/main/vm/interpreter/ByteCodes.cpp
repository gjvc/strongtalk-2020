
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/oops/MarkOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/code/NativeCode.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/interpreter/Floats.hpp"


const char *ByteCodes::_entry_point[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];
const char *ByteCodes::_name[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];

ByteCodes::Format           ByteCodes::_format[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];
ByteCodes::CodeType         ByteCodes::_code_type[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];
ByteCodes::ArgumentSpec     ByteCodes::_argument_spec[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];
ByteCodes::SendType         ByteCodes::_send_type[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];
bool                        ByteCodes::_single_step[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];
bool                        ByteCodes::_pop_tos[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];


void ByteCodes::def( Code code ) {
    def( code, "undefined", ByteCodes::Format::UNDEFINED, ByteCodes::CodeType::MISCELLANEOUS, false, ByteCodes::ArgumentSpec::no_args, ByteCodes::SendType::NO_SEND, false );
}


void ByteCodes::def( Code code, const char *name, Format format, CodeType code_type, bool single_step, bool pop_tos ) {
    def( code, name, format, code_type, single_step, ByteCodes::ArgumentSpec::no_args, ByteCodes::SendType::NO_SEND, pop_tos );
}


void ByteCodes::def( Code code, const char *name, Format format, ArgumentSpec argument_spec, SendType send_type, bool pop_tos ) {
    st_assert( send_type not_eq ByteCodes::SendType::NO_SEND, "must be a send" );
    def( code, name, format, ByteCodes::CodeType::MESSAGE_SEND, true, argument_spec, send_type, pop_tos );
}


void ByteCodes::def( Code code, const char *name, Format format, CodeType code_type, bool single_step, ArgumentSpec argument_spec, SendType send_type, bool pop_tos ) {
    st_assert( 0 <= static_cast<std::int32_t>(code) and static_cast<std::int32_t>(code) < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES), "out of bounds" );
    st_assert( _name[ static_cast<std::int32_t>(code) ] == nullptr, "bytecode defined twice" );

    // plausibility checks for arguments (compare with naming convention)
    if ( format == ByteCodes::Format::UNDEFINED ) {
        // bytecode name should be "undefined"
        for ( std::size_t i = 0; i < 9; i++ ) {
            st_assert( name[ i ] == "undefined"[ i ], "inconsistency with naming convention" );
        }
    }

    if ( pop_tos ) {
        // bytecode name unlikely to start with "push_"... and should end with ..."_pop"
        st_assert( name[ 0 ] not_eq 'p' or name[ 1 ] not_eq 'u' or name[ 2 ] not_eq 's' or name[ 3 ] not_eq 'h' or name[ 4 ] not_eq '_', "unlikely naming" );

        std::int32_t i = 0;
        while ( name[ i ] not_eq '\00' )
            i++;
        // Fix this lars
        // assert(name[i-4] == '_' and name[i-3] == 'p' and name[i-2] == 'o' and name[i-1] == 'p', "inconsistency with naming convention");
    }

    if ( argument_spec not_eq ByteCodes::ArgumentSpec::no_args ) {
        // must be a send
        st_assert( send_type not_eq ByteCodes::SendType::NO_SEND, "inconsistency between argument_spec and send_type" );
    }

    if ( code_type == ByteCodes::CodeType::CONTROL_STRUCTURE ) {
        // not intercepted on single step
        st_assert( not single_step, "control structures cannot be single-stepped" );
    }

    if ( code_type == ByteCodes::CodeType::FLOAT_OPERATION ) {
        // bytecode name should start with "float_"
        for ( std::size_t i = 0; i < 6; i++ ) {
            st_assert( name[ i ] == "float_"[ i ], "inconsistency with naming convention" );
        }
    }

    // set table entries
    _entry_point[ static_cast<std::int32_t>(code) ]   = nullptr;
    _name[ static_cast<std::int32_t>(code) ]          = name;
    _format[ static_cast<std::int32_t>(code) ]        = format;
    _code_type[ static_cast<std::int32_t>(code) ]     = code_type;
    _argument_spec[ static_cast<std::int32_t>(code) ] = argument_spec;
    _send_type[ static_cast<std::int32_t>(code) ]     = send_type;
    _single_step[ static_cast<std::int32_t>(code) ]   = single_step;
    _pop_tos[ static_cast<std::int32_t>(code) ]       = pop_tos;
}


extern "C" doFn original_table[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];


void ByteCodes::set_entry_point( Code code, const char *entry_point ) {
    st_assert( is_defined( code ), "undefined byte code" );
    st_assert( entry_point not_eq nullptr, "not a valid entry_point" );
    _entry_point[ static_cast<std::int32_t>(code) ]   = entry_point;
    original_table[ static_cast<std::int32_t>(code) ] = (doFn) entry_point;
}


void ByteCodes::init() {
    // to allow check for complete initialization at end of init
    for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES); i++ ) {
        _name[ static_cast<std::int32_t>( Code( i ) ) ] = nullptr;
    }

    // for better readability
    const bool do_sst = true;
    const bool no_sst = false;
    const bool pop    = true;

    def( ByteCodes::Code::push_temp_0, "push_temp_0", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_1, "push_temp_1", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_2, "push_temp_2", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_3, "push_temp_3", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_4, "push_temp_4", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_5, "push_temp_5", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );

    def( ByteCodes::Code::unimplemented_06 );

    def( ByteCodes::Code::push_temp_n, "push_temp_n", ByteCodes::Format::BB, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_arg_1, "push_arg_1", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_arg_2, "push_arg_2", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_arg_3, "push_arg_3", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );
    def( ByteCodes::Code::push_arg_n, "push_arg_n", ByteCodes::Format::BB, ByteCodes::CodeType::LOCAL_ACCESS, no_sst );

    def( ByteCodes::Code::allocate_temp_1, "allocate_temp_1", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::allocate_temp_2, "allocate_temp_2", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::allocate_temp_3, "allocate_temp_3", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::allocate_temp_n, "allocate_temp_n", ByteCodes::Format::BB, ByteCodes::CodeType::MISCELLANEOUS, no_sst );

    def( ByteCodes::Code::store_temp_0_pop, "store_temp_0_pop", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_1_pop, "store_temp_1_pop", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_2_pop, "store_temp_2_pop", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_3_pop, "store_temp_3_pop", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_4_pop, "store_temp_4_pop", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_5_pop, "store_temp_5_pop", ByteCodes::Format::B, ByteCodes::CodeType::LOCAL_ACCESS, do_sst, pop );

    def( ByteCodes::Code::store_temp_n, "store_temp_n", ByteCodes::Format::BB, ByteCodes::CodeType::LOCAL_ACCESS, do_sst );
    def( ByteCodes::Code::store_temp_n_pop, "store_temp_n_pop", ByteCodes::Format::BB, ByteCodes::CodeType::LOCAL_ACCESS, do_sst, pop );

    def( ByteCodes::Code::push_neg_n, "push_neg_n", ByteCodes::Format::BB, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::push_succ_n, "push_succ_n", ByteCodes::Format::BB, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::push_literal, "push_literal", ByteCodes::Format::BO, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::push_tos, "push_tos", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::push_self, "push_self", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::push_nil, "push_nil", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::push_true, "push_true", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::push_false, "push_false", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, no_sst );

    def( ByteCodes::Code::unimplemented_20 );
    def( ByteCodes::Code::unimplemented_21 );
    def( ByteCodes::Code::unimplemented_22 );
    def( ByteCodes::Code::unimplemented_23 );
    def( ByteCodes::Code::unimplemented_24 );
    def( ByteCodes::Code::unimplemented_25 );
    def( ByteCodes::Code::unimplemented_26 );
    def( ByteCodes::Code::unimplemented_27 );

    def( ByteCodes::Code::return_instVar_name, "return_instVar_name", ByteCodes::Format::BO, ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS, no_sst );
    def( ByteCodes::Code::push_classVar, "push_classVar", ByteCodes::Format::BO, ByteCodes::CodeType::CLASS_VARIABLE_ACCESS, no_sst );
    def( ByteCodes::Code::store_classVar_pop, "store_classVar_pop", ByteCodes::Format::BO, ByteCodes::CodeType::CLASS_VARIABLE_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_classVar, "store_classVar", ByteCodes::Format::BO, ByteCodes::CodeType::CLASS_VARIABLE_ACCESS, do_sst );
    def( ByteCodes::Code::return_instVar, "return_instVar", ByteCodes::Format::BL, ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS, do_sst );
    def( ByteCodes::Code::push_instVar, "push_instVar", ByteCodes::Format::BL, ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS, no_sst );
    def( ByteCodes::Code::store_instVar_pop, "store_instVar_pop", ByteCodes::Format::BL, ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_instVar, "store_instVar", ByteCodes::Format::BL, ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS, do_sst );

    def( ByteCodes::Code::float_allocate, "float_allocate", ByteCodes::Format::BBBB, ByteCodes::CodeType::FLOAT_OPERATION, no_sst );
    def( ByteCodes::Code::float_floatify_pop, "float_floatify_pop", ByteCodes::Format::BB, ByteCodes::CodeType::FLOAT_OPERATION, no_sst, pop );
    def( ByteCodes::Code::float_move, "float_move", ByteCodes::Format::BBB, ByteCodes::CodeType::FLOAT_OPERATION, no_sst );
    def( ByteCodes::Code::float_set, "float_set", ByteCodes::Format::BBO, ByteCodes::CodeType::FLOAT_OPERATION, no_sst );
    def( ByteCodes::Code::float_nullary_op, "float_nullary_op", ByteCodes::Format::BBB, ByteCodes::CodeType::FLOAT_OPERATION, no_sst );
    def( ByteCodes::Code::float_unary_op, "float_unary_op", ByteCodes::Format::BBB, ByteCodes::CodeType::FLOAT_OPERATION, no_sst );
    def( ByteCodes::Code::float_binary_op, "float_binary_op", ByteCodes::Format::BBB, ByteCodes::CodeType::FLOAT_OPERATION, no_sst );
    def( ByteCodes::Code::float_unary_op_to_oop, "float_unary_op_to_oop", ByteCodes::Format::BBB, ByteCodes::CodeType::FLOAT_OPERATION, no_sst );
    def( ByteCodes::Code::float_binary_op_to_oop, "float_binary_op_to_oop", ByteCodes::Format::BBB, ByteCodes::CodeType::FLOAT_OPERATION, no_sst );

    def( ByteCodes::Code::unimplemented_39 );
    def( ByteCodes::Code::unimplemented_3a );
    def( ByteCodes::Code::unimplemented_3b );
    def( ByteCodes::Code::unimplemented_3c );

    def( ByteCodes::Code::push_instVar_name, "push_instVar_name", ByteCodes::Format::BO, ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS, no_sst );
    def( ByteCodes::Code::store_instVar_pop_name, "store_instVar_pop_name", ByteCodes::Format::BO, ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS, no_sst, pop );
    def( ByteCodes::Code::store_instVar_name, "store_instVar_name", ByteCodes::Format::BO, ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS, no_sst );

    def( ByteCodes::Code::push_temp_0_context_0, "push_temp_0_context_0", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_1_context_0, "push_temp_1_context_0", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_2_context_0, "push_temp_2_context_0", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_n_context_0, "push_temp_n_context_0", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::store_temp_0_context_0_pop, "store_temp_0_context_0_pop", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_1_context_0_pop, "store_temp_1_context_0_pop", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_2_context_0_pop, "store_temp_2_context_0_pop", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_n_context_0_pop, "store_temp_n_context_0_pop", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::push_new_closure_context_0, "push_new_closure_context_0", ByteCodes::Format::BO, ByteCodes::CodeType::NEW_CLOSURE, no_sst );
    def( ByteCodes::Code::push_new_closure_context_1, "push_new_closure_context_1", ByteCodes::Format::BO, ByteCodes::CodeType::NEW_CLOSURE, no_sst );
    def( ByteCodes::Code::push_new_closure_context_2, "push_new_closure_context_2", ByteCodes::Format::BO, ByteCodes::CodeType::NEW_CLOSURE, no_sst );
    def( ByteCodes::Code::push_new_closure_context_n, "push_new_closure_context_n", ByteCodes::Format::BBO, ByteCodes::CodeType::NEW_CLOSURE, no_sst );
    def( ByteCodes::Code::install_new_context_method_0, "install_new_context_method_0", ByteCodes::Format::B, ByteCodes::CodeType::NEW_CONTEXT, no_sst );
    def( ByteCodes::Code::install_new_context_method_1, "install_new_context_method_1", ByteCodes::Format::B, ByteCodes::CodeType::NEW_CONTEXT, no_sst );
    def( ByteCodes::Code::install_new_context_method_2, "install_new_context_method_2", ByteCodes::Format::B, ByteCodes::CodeType::NEW_CONTEXT, no_sst );
    def( ByteCodes::Code::install_new_context_method_n, "install_new_context_method_n", ByteCodes::Format::BB, ByteCodes::CodeType::NEW_CONTEXT, no_sst );

    def( ByteCodes::Code::push_temp_0_context_1, "push_temp_0_context_1", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_1_context_1, "push_temp_1_context_1", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_2_context_1, "push_temp_2_context_1", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_n_context_1, "push_temp_n_context_1", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::store_temp_0_context_1_pop, "store_temp_0_context_1_pop", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_1_context_1_pop, "store_temp_1_context_1_pop", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_2_context_1_pop, "store_temp_2_context_1_pop", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_n_context_1_pop, "store_temp_n_context_1_pop", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::push_new_closure_tos_0, "push_new_closure_tos_0", ByteCodes::Format::BO, ByteCodes::CodeType::NEW_CLOSURE, no_sst );
    def( ByteCodes::Code::push_new_closure_tos_1, "push_new_closure_tos_1", ByteCodes::Format::BO, ByteCodes::CodeType::NEW_CLOSURE, no_sst );
    def( ByteCodes::Code::push_new_closure_tos_2, "push_new_closure_tos_2", ByteCodes::Format::BO, ByteCodes::CodeType::NEW_CLOSURE, no_sst );
    def( ByteCodes::Code::push_new_closure_tos_n, "push_new_closure_tos_n", ByteCodes::Format::BBO, ByteCodes::CodeType::NEW_CLOSURE, no_sst );
    def( ByteCodes::Code::only_pop, "only_pop", ByteCodes::Format::B, ByteCodes::CodeType::NEW_CONTEXT, no_sst, pop );
    def( ByteCodes::Code::install_new_context_block_1, "install_new_context_block_1", ByteCodes::Format::B, ByteCodes::CodeType::NEW_CONTEXT, no_sst );
    def( ByteCodes::Code::install_new_context_block_2, "install_new_context_block_2", ByteCodes::Format::B, ByteCodes::CodeType::NEW_CONTEXT, no_sst );
    def( ByteCodes::Code::install_new_context_block_n, "install_new_context_block_n", ByteCodes::Format::BB, ByteCodes::CodeType::NEW_CONTEXT, no_sst );

    def( ByteCodes::Code::push_temp_0_context_n, "push_temp_0_context_n", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_1_context_n, "push_temp_1_context_n", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_2_context_n, "push_temp_2_context_n", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::push_temp_n_context_n, "push_temp_n_context_n", ByteCodes::Format::BBB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::store_temp_0_context_n_pop, "store_temp_0_context_n_pop", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_1_context_n_pop, "store_temp_1_context_n_pop", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_2_context_n_pop, "store_temp_2_context_n_pop", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_temp_n_context_n_pop, "store_temp_n_context_n_pop", ByteCodes::Format::BBB, ByteCodes::CodeType::CONTEXT_ACCESS, do_sst, pop );
    def( ByteCodes::Code::set_self_via_context, "set_self_via_context", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::copy_1_into_context, "copy_1_into_context", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::copy_2_into_context, "copy_2_into_context", ByteCodes::Format::BBB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::copy_n_into_context, "copy_n_into_context", ByteCodes::Format::BBS, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::copy_self_into_context, "copy_self_into_context", ByteCodes::Format::B, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::copy_self_1_into_context, "copy_self_1_into_context", ByteCodes::Format::BB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::copy_self_2_into_context, "copy_self_2_into_context", ByteCodes::Format::BBB, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );
    def( ByteCodes::Code::copy_self_n_into_context, "copy_self_n_into_context", ByteCodes::Format::BBS, ByteCodes::CodeType::CONTEXT_ACCESS, no_sst );

    def( ByteCodes::Code::ifTrue_byte, "ifTrue_byte", ByteCodes::Format::BBB, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::ifFalse_byte, "ifFalse_byte", ByteCodes::Format::BBB, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::and_byte, "and_byte", ByteCodes::Format::BB, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::or_byte, "or_byte", ByteCodes::Format::BB, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::whileTrue_byte, "whileTrue_byte", ByteCodes::Format::BB, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::whileFalse_byte, "whileFalse_byte", ByteCodes::Format::BB, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::jump_else_byte, "jump_else_byte", ByteCodes::Format::BB, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::jump_loop_byte, "jump_loop_byte", ByteCodes::Format::BBB, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::ifTrue_word, "ifTrue_word", ByteCodes::Format::BBL, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::ifFalse_word, "ifFalse_word", ByteCodes::Format::BBL, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::and_word, "and_word", ByteCodes::Format::BL, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::or_word, "or_word", ByteCodes::Format::BL, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::whileTrue_word, "whileTrue_word", ByteCodes::Format::BL, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::whileFalse_word, "whileFalse_word", ByteCodes::Format::BL, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::jump_else_word, "jump_else_word", ByteCodes::Format::BL, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );
    def( ByteCodes::Code::jump_loop_word, "jump_loop_word", ByteCodes::Format::BLL, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );

    def( ByteCodes::Code::interpreted_send_0, "interpreted_send_0", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::INTERPRETED_SEND );
    def( ByteCodes::Code::interpreted_send_1, "interpreted_send_1", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::INTERPRETED_SEND );
    def( ByteCodes::Code::interpreted_send_2, "interpreted_send_2", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::INTERPRETED_SEND );
    def( ByteCodes::Code::interpreted_send_n, "interpreted_send_n", ByteCodes::Format::BBOO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::INTERPRETED_SEND );
    def( ByteCodes::Code::interpreted_send_0_pop, "interpreted_send_0_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::INTERPRETED_SEND, pop );
    def( ByteCodes::Code::interpreted_send_1_pop, "interpreted_send_1_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::INTERPRETED_SEND, pop );
    def( ByteCodes::Code::interpreted_send_2_pop, "interpreted_send_2_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::INTERPRETED_SEND, pop );
    def( ByteCodes::Code::interpreted_send_n_pop, "interpreted_send_n_pop", ByteCodes::Format::BBOO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::INTERPRETED_SEND, pop );
    def( ByteCodes::Code::interpreted_send_self, "interpreted_send_self", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::INTERPRETED_SEND );
    def( ByteCodes::Code::interpreted_send_self_pop, "interpreted_send_self_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::INTERPRETED_SEND, pop );
    def( ByteCodes::Code::interpreted_send_super, "interpreted_send_super", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::INTERPRETED_SEND );
    def( ByteCodes::Code::interpreted_send_super_pop, "interpreted_send_super_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::INTERPRETED_SEND, pop );
    def( ByteCodes::Code::return_tos_pop_0, "return_tos_pop_0", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::return_tos_pop_1, "return_tos_pop_1", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::return_tos_pop_2, "return_tos_pop_2", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::return_tos_pop_n, "return_tos_pop_n", ByteCodes::Format::BB, ByteCodes::CodeType::MISCELLANEOUS, do_sst );

    def( ByteCodes::Code::polymorphic_send_0, "polymorphic_send_0", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::POLYMORPHIC_SEND );
    def( ByteCodes::Code::polymorphic_send_1, "polymorphic_send_1", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::POLYMORPHIC_SEND );
    def( ByteCodes::Code::polymorphic_send_2, "polymorphic_send_2", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::POLYMORPHIC_SEND );
    def( ByteCodes::Code::polymorphic_send_n, "polymorphic_send_n", ByteCodes::Format::BBOO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::POLYMORPHIC_SEND );
    def( ByteCodes::Code::polymorphic_send_0_pop, "polymorphic_send_0_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::POLYMORPHIC_SEND, pop );
    def( ByteCodes::Code::polymorphic_send_1_pop, "polymorphic_send_1_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::POLYMORPHIC_SEND, pop );
    def( ByteCodes::Code::polymorphic_send_2_pop, "polymorphic_send_2_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::POLYMORPHIC_SEND, pop );
    def( ByteCodes::Code::polymorphic_send_n_pop, "polymorphic_send_n_pop", ByteCodes::Format::BBOO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::POLYMORPHIC_SEND, pop );
    def( ByteCodes::Code::polymorphic_send_self, "polymorphic_send_self", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::POLYMORPHIC_SEND );
    def( ByteCodes::Code::polymorphic_send_self_pop, "polymorphic_send_self_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::POLYMORPHIC_SEND, pop );
    def( ByteCodes::Code::polymorphic_send_super, "polymorphic_send_super", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::POLYMORPHIC_SEND );
    def( ByteCodes::Code::polymorphic_send_super_pop, "polymorphic_send_super_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::POLYMORPHIC_SEND, pop );
    def( ByteCodes::Code::return_self_pop_0, "return_self_pop_0", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::return_self_pop_1, "return_self_pop_1", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::return_self_pop_2, "return_self_pop_2", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::return_self_pop_n, "return_self_pop_n", ByteCodes::Format::BB, ByteCodes::CodeType::MISCELLANEOUS, do_sst );

    def( ByteCodes::Code::compiled_send_0, "compiled_send_0", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::COMPILED_SEND );
    def( ByteCodes::Code::compiled_send_1, "compiled_send_1", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::COMPILED_SEND );
    def( ByteCodes::Code::compiled_send_2, "compiled_send_2", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::COMPILED_SEND );
    def( ByteCodes::Code::compiled_send_n, "compiled_send_n", ByteCodes::Format::BBLO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::COMPILED_SEND );
    def( ByteCodes::Code::compiled_send_0_pop, "compiled_send_0_pop", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::COMPILED_SEND, pop );
    def( ByteCodes::Code::compiled_send_1_pop, "compiled_send_1_pop", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::COMPILED_SEND, pop );
    def( ByteCodes::Code::compiled_send_2_pop, "compiled_send_2_pop", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::COMPILED_SEND, pop );
    def( ByteCodes::Code::compiled_send_n_pop, "compiled_send_n_pop", ByteCodes::Format::BBLO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::COMPILED_SEND, pop );
    def( ByteCodes::Code::compiled_send_self, "compiled_send_self", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::COMPILED_SEND );
    def( ByteCodes::Code::compiled_send_self_pop, "compiled_send_self_pop", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::COMPILED_SEND, pop );
    def( ByteCodes::Code::compiled_send_super, "compiled_send_super", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::COMPILED_SEND );
    def( ByteCodes::Code::compiled_send_super_pop, "compiled_send_super_pop", ByteCodes::Format::BLO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::COMPILED_SEND, pop );
    def( ByteCodes::Code::return_tos_zap_pop_n, "return_tos_zap_pop_n", ByteCodes::Format::BB, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::return_self_zap_pop_n, "return_self_zap_pop_n", ByteCodes::Format::BB, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::non_local_return_tos_pop_n, "non_local_return_tos_pop_n", ByteCodes::Format::BB, ByteCodes::CodeType::NONLOCAL_RETURN, do_sst );
    def( ByteCodes::Code::non_local_return_self_pop_n, "non_local_return_self_pop_n", ByteCodes::Format::BB, ByteCodes::CodeType::NONLOCAL_RETURN, do_sst );

    def( ByteCodes::Code::prim_call, "prim_call", ByteCodes::Format::BL, ByteCodes::CodeType::PRIMITIVE_CALL, do_sst );
    def( ByteCodes::Code::predict_primitive_call, "predict_primitive_call", ByteCodes::Format::BL, ByteCodes::CodeType::PRIMITIVE_CALL, do_sst );
    def( ByteCodes::Code::primitive_call_failure, "primitive_call_failure", ByteCodes::Format::BLL, ByteCodes::CodeType::PRIMITIVE_CALL, do_sst );
    def( ByteCodes::Code::predict_primitive_call_failure, "predict_primitive_call_failure", ByteCodes::Format::BLL, ByteCodes::CodeType::PRIMITIVE_CALL, do_sst );
    def( ByteCodes::Code::dll_call_sync, "dll_call_sync", ByteCodes::Format::BOOLB, ByteCodes::CodeType::DLL_CALL, do_sst );
    def( ByteCodes::Code::primitive_call_self, "primitive_call_self", ByteCodes::Format::BL, ByteCodes::CodeType::PRIMITIVE_CALL, do_sst );
    def( ByteCodes::Code::primitive_call_self_failure, "primitive_call_self_failure", ByteCodes::Format::BLL, ByteCodes::CodeType::PRIMITIVE_CALL, do_sst );
    def( ByteCodes::Code::unimplemented_b7 );
    def( ByteCodes::Code::access_send_self, "access_send_self", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::ACCESSOR_SEND );
    def( ByteCodes::Code::primitive_send_0, "primitive_send_0", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::PRIMITIVE_SEND );
    def( ByteCodes::Code::primitive_send_super, "primitive_send_super", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::PRIMITIVE_SEND );
    def( ByteCodes::Code::primitive_send_super_pop, "primitive_send_super_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::PRIMITIVE_SEND, pop );
    def( ByteCodes::Code::unimplemented_bc );
    def( ByteCodes::Code::primitive_send_1, "primitive_send_1", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PRIMITIVE_SEND );
    def( ByteCodes::Code::primitive_send_2, "primitive_send_2", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::PRIMITIVE_SEND );
    def( ByteCodes::Code::primitive_send_n, "primitive_send_n", ByteCodes::Format::BBOO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::PRIMITIVE_SEND );

    def( ByteCodes::Code::primitive_call_lookup, "primitive_call_lookup", ByteCodes::Format::BO, ByteCodes::CodeType::PRIMITIVE_CALL, no_sst );
    def( ByteCodes::Code::predict_primitive_call_lookup, "predict_primitive_call_lookup", ByteCodes::Format::BO, ByteCodes::CodeType::PRIMITIVE_CALL, no_sst );
    def( ByteCodes::Code::primitive_call_failure_lookup, "primitive_call_failure_lookup", ByteCodes::Format::BOL, ByteCodes::CodeType::PRIMITIVE_CALL, no_sst );
    def( ByteCodes::Code::predict_primitive_call_failure_lookup, "predict_primitive_call_failure_lookup", ByteCodes::Format::BOL, ByteCodes::CodeType::PRIMITIVE_CALL, no_sst );
    def( ByteCodes::Code::dll_call_async, "dll_call_async", ByteCodes::Format::BOOLB, ByteCodes::CodeType::DLL_CALL, do_sst );
    def( ByteCodes::Code::primitive_call_self_lookup, "primitive_call_self_lookup", ByteCodes::Format::BO, ByteCodes::CodeType::PRIMITIVE_CALL, no_sst );
    def( ByteCodes::Code::primitive_call_self_failure_lookup, "primitive_call_self_failure_lookup", ByteCodes::Format::BOL, ByteCodes::CodeType::PRIMITIVE_CALL, no_sst );
    def( ByteCodes::Code::unimplemented_c7 );
    def( ByteCodes::Code::access_send_0, "access_send_0", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::ACCESSOR_SEND );
    def( ByteCodes::Code::primitive_send_0_pop, "primitive_send_0_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::PRIMITIVE_SEND, pop );
    def( ByteCodes::Code::primitive_send_self, "primitive_send_self", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::PRIMITIVE_SEND );
    def( ByteCodes::Code::primitive_send_self_pop, "primitive_send_self_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::PRIMITIVE_SEND, pop );
    def( ByteCodes::Code::unimplemented_cc );
    def( ByteCodes::Code::primitive_send_1_pop, "primitive_send_1_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PRIMITIVE_SEND, pop );
    def( ByteCodes::Code::primitive_send_2_pop, "primitive_send_2_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::PRIMITIVE_SEND, pop );
    def( ByteCodes::Code::primitive_send_n_pop, "primitive_send_n_pop", ByteCodes::Format::BBOO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::PRIMITIVE_SEND, pop );

    def( ByteCodes::Code::megamorphic_send_0, "megamorphic_send_0", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::MEGAMORPHIC_SEND );
    def( ByteCodes::Code::megamorphic_send_1, "megamorphic_send_1", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::MEGAMORPHIC_SEND );
    def( ByteCodes::Code::megamorphic_send_2, "megamorphic_send_2", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::MEGAMORPHIC_SEND );
    def( ByteCodes::Code::megamorphic_send_n, "megamorphic_send_n", ByteCodes::Format::BBOO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::MEGAMORPHIC_SEND );
    def( ByteCodes::Code::megamorphic_send_0_pop, "megamorphic_send_0_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_0_args, ByteCodes::SendType::MEGAMORPHIC_SEND, pop );
    def( ByteCodes::Code::megamorphic_send_1_pop, "megamorphic_send_1_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::MEGAMORPHIC_SEND, pop );
    def( ByteCodes::Code::megamorphic_send_2_pop, "megamorphic_send_2_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_2_args, ByteCodes::SendType::MEGAMORPHIC_SEND, pop );
    def( ByteCodes::Code::megamorphic_send_n_pop, "megamorphic_send_n_pop", ByteCodes::Format::BBOO, ByteCodes::ArgumentSpec::recv_n_args, ByteCodes::SendType::MEGAMORPHIC_SEND, pop );
    def( ByteCodes::Code::megamorphic_send_self, "megamorphic_send_self", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::MEGAMORPHIC_SEND );
    def( ByteCodes::Code::megamorphic_send_self_pop, "megamorphic_send_self_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::MEGAMORPHIC_SEND, pop );
    def( ByteCodes::Code::megamorphic_send_super, "megamorphic_send_super", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::MEGAMORPHIC_SEND );
    def( ByteCodes::Code::megamorphic_send_super_pop, "megamorphic_send_super_pop", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::args_only, ByteCodes::SendType::MEGAMORPHIC_SEND, pop );
    def( ByteCodes::Code::unimplemented_dc );
    def( ByteCodes::Code::special_primitive_send_1_hint, "special_primitive_send_1_hint", ByteCodes::Format::BB, ByteCodes::CodeType::MISCELLANEOUS, no_sst );
    def( ByteCodes::Code::unimplemented_de );
    def( ByteCodes::Code::unimplemented_df );

    def( ByteCodes::Code::smi_add, "smi_add", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_sub, "smi_sub", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_mult, "smi_mult", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_div, "smi_div", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_mod, "smi_mod", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_create_point, "smi_create_point", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_equal, "smi_equal", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_not_equal, "smi_not_equal", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_less, "smi_less", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_less_equal, "smi_less_equal", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_greater, "smi_greater", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_greater_equal, "smi_greater_equal", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::objectArray_at, "objectArray_at", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::objectArray_at_put, "objectArray_at_put", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::double_equal, "double_equal", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, do_sst );
    def( ByteCodes::Code::double_tilde, "double_tilde", ByteCodes::Format::B, ByteCodes::CodeType::MISCELLANEOUS, do_sst );

    def( ByteCodes::Code::push_global, "push_global", ByteCodes::Format::BO, ByteCodes::CodeType::GLOBAL_ACCESS, no_sst );
    def( ByteCodes::Code::store_global_pop, "store_global_pop", ByteCodes::Format::BO, ByteCodes::CodeType::GLOBAL_ACCESS, do_sst, pop );
    def( ByteCodes::Code::store_global, "store_global", ByteCodes::Format::BO, ByteCodes::CodeType::GLOBAL_ACCESS, do_sst );
    def( ByteCodes::Code::push_classVar_name, "push_classVar_name", ByteCodes::Format::BO, ByteCodes::CodeType::CLASS_VARIABLE_ACCESS, no_sst );
    def( ByteCodes::Code::store_classVar_pop_name, "store_classVar_pop_name", ByteCodes::Format::BO, ByteCodes::CodeType::CLASS_VARIABLE_ACCESS, no_sst, pop );
    def( ByteCodes::Code::store_classVar_name, "store_classVar_name", ByteCodes::Format::BO, ByteCodes::CodeType::CLASS_VARIABLE_ACCESS, no_sst );
    def( ByteCodes::Code::smi_and, "smi_and", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_or, "smi_or", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_xor, "smi_xor", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::smi_shift, "smi_shift", ByteCodes::Format::BOO, ByteCodes::ArgumentSpec::recv_1_args, ByteCodes::SendType::PREDICTED_SEND );
    def( ByteCodes::Code::unimplemented_fa );
    def( ByteCodes::Code::unimplemented_fb );
    def( ByteCodes::Code::unimplemented_fc );
    def( ByteCodes::Code::unimplemented_fd );
    def( ByteCodes::Code::unimplemented_fe );
    def( ByteCodes::Code::halt, "halt", ByteCodes::Format::B, ByteCodes::CodeType::CONTROL_STRUCTURE, no_sst );

    // check if all bytecodes have been initialized
    for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES); i++ ) {
        st_assert( _name[ static_cast<std::int32_t>( Code( i ) ) ] not_eq nullptr, "bytecode table not fully initialized" );
    }
}


// The dispatchTable controls the dispatch of byte codes

ByteCodes::LoopType ByteCodes::loop_type( Code code ) {
    switch ( code ) {
        case ByteCodes::Code::jump_loop_byte:
            [[fallthrough]];
        case ByteCodes::Code::jump_loop_word:
            return ByteCodes::LoopType::LOOP_START;
        case ByteCodes::Code::whileTrue_byte:
            [[fallthrough]];
        case ByteCodes::Code::whileFalse_byte:
            [[fallthrough]];
        case ByteCodes::Code::whileTrue_word:
            [[fallthrough]];
        case ByteCodes::Code::whileFalse_word:
            return ByteCodes::LoopType::LOOP_END;
        default:
            return ByteCodes::LoopType::NO_LOOP;
    }
    ShouldNotReachHere();
}


const char *ByteCodes::format_as_string( Format format ) {
    switch ( format ) {
        case ByteCodes::Format::B:
            return "B";
        case ByteCodes::Format::BB:
            return "BB";
        case ByteCodes::Format::BBB:
            return "BBB";
        case ByteCodes::Format::BBBB:
            return "BBBB";
        case ByteCodes::Format::BBO:
            return "BBO";
        case ByteCodes::Format::BBL:
            return "BBL";
        case ByteCodes::Format::BO:
            return "BO";
        case ByteCodes::Format::BOO:
            return "BOO";
        case ByteCodes::Format::BLO:
            return "BLO";
        case ByteCodes::Format::BOL:
            return "BOL";
        case ByteCodes::Format::BLL:
            return "BLL";
        case ByteCodes::Format::BL:
            return "BL";
        case ByteCodes::Format::BLB:
            return "BLB";
        case ByteCodes::Format::BBOO:
            return "BBOO";
        case ByteCodes::Format::BBLO:
            return "BBLO";
        case ByteCodes::Format::BOOLB:
            return "BOOLB";
        case ByteCodes::Format::BBS:
            return "BBS";
        case ByteCodes::Format::UNDEFINED:
            return "UNDEFINED";
        default:
            spdlog::error( "unhandled case [{}] in switch", format );
            return nullptr;
    }
    ShouldNotReachHere();
    return nullptr;
}


const char *ByteCodes::send_type_as_string( SendType send_type ) {
    switch ( send_type ) {
        case ByteCodes::SendType::INTERPRETED_SEND:
            return "ByteCodes::SendType::interpreted_send";
        case ByteCodes::SendType::COMPILED_SEND:
            return "compiled_send";
        case ByteCodes::SendType::POLYMORPHIC_SEND:
            return "ByteCodes::SendType::polymorphic_send";
        case ByteCodes::SendType::MEGAMORPHIC_SEND:
            return "megamorphic_send";
        case ByteCodes::SendType::PREDICTED_SEND:
            return "predicted_send";
        case ByteCodes::SendType::ACCESSOR_SEND:
            return "accessor send";
        case ByteCodes::SendType::PRIMITIVE_SEND:
            return "primitive_send";
        case ByteCodes::SendType::NO_SEND:
            return "no_send";
        default:
            spdlog::error( "unhandled case [{}] in switch", send_type );
            return nullptr;
    }
    ShouldNotReachHere();
    return nullptr;
}


const char *ByteCodes::code_type_as_string( CodeType code_type ) {
    switch ( code_type ) {
        case ByteCodes::CodeType::LOCAL_ACCESS:
            return "local_access";
        case ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS:
            return "instVar_access";
        case ByteCodes::CodeType::CONTEXT_ACCESS:
            return "context_access";
        case ByteCodes::CodeType::CLASS_VARIABLE_ACCESS:
            return "classVar_access";
        case ByteCodes::CodeType::GLOBAL_ACCESS:
            return "global_access";
        case ByteCodes::CodeType::NEW_CLOSURE:
            return "new_closure";
        case ByteCodes::CodeType::NEW_CONTEXT:
            return "new_context";
        case ByteCodes::CodeType::CONTROL_STRUCTURE:
            return "control_structure";
        case ByteCodes::CodeType::MESSAGE_SEND:
            return "message_send";
        case ByteCodes::CodeType::NONLOCAL_RETURN:
            return "nonlocal_return";
        case ByteCodes::CodeType::PRIMITIVE_CALL:
            return "primitive_call";
        case ByteCodes::CodeType::DLL_CALL:
            return "dll_call";
        case ByteCodes::CodeType::FLOAT_OPERATION:
            return "float_operation";
        case ByteCodes::CodeType::MISCELLANEOUS:
            return "miscellaneous";
        default:
            spdlog::error( "unhandled case [{}] in switch", code_type );
            return nullptr;
    }
    ShouldNotReachHere();
    return nullptr;
}


const char *ByteCodes::argument_spec_as_string( ArgumentSpec argument_spec ) {
    switch ( argument_spec ) {
        case ByteCodes::ArgumentSpec::recv_0_args:
            return "recv_0_args";
        case ByteCodes::ArgumentSpec::recv_1_args:
            return "recv_1_args";
        case ByteCodes::ArgumentSpec::recv_2_args:
            return "recv_2_args";
        case ByteCodes::ArgumentSpec::recv_n_args:
            return "ByteCodes::ArgumentSpec::recv_n_args";
        case ByteCodes::ArgumentSpec::args_only:
            return "ByteCodes::ArgumentSpec::args_only";
        case ByteCodes::ArgumentSpec::no_args:
            return "no_args";
        default:
            spdlog::error( "unhandled case [{}] in switch", argument_spec );
            return nullptr;
    }
    ShouldNotReachHere();
    return nullptr;
}


const char *ByteCodes::loop_type_as_string( LoopType loop_type ) {
    switch ( loop_type ) {
        case ByteCodes::LoopType::LOOP_START:
            return "loop_start";
        case ByteCodes::LoopType::LOOP_END:
            return "loop_end";
        case ByteCodes::LoopType::NO_LOOP:
            return "no_loop";
        default:
            return nullptr;
    }
    ShouldNotReachHere();
    return nullptr;
}


bool ByteCodes::is_self_send( ByteCodes::Code code ) {
    switch ( code ) {
        case ByteCodes::Code::interpreted_send_self:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_self:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_self:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_self:
            [[fallthrough]];
        case ByteCodes::Code::interpreted_send_self_pop:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_self_pop:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_self_pop:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_self_pop:
            return true;
        default:
            return false;
    }
    ShouldNotReachHere();
    return false;
}


bool ByteCodes::is_super_send( ByteCodes::Code code ) {
    switch ( code ) {
        case ByteCodes::Code::interpreted_send_super:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_super:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_super:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_super:
            [[fallthrough]];
        case ByteCodes::Code::interpreted_send_super_pop:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_super_pop:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_super_pop:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_super_pop:
            return true;
        default:
            return false;
    }
    ShouldNotReachHere();
    return false;
}


bool ByteCodes::has_access_send_code( ByteCodes::Code code ) {
    switch ( code ) {
        case ByteCodes::Code::interpreted_send_0:
            [[fallthrough]];
        case ByteCodes::Code::interpreted_send_self:
            return true;
        default:
            return false;
    }
    ShouldNotReachHere();
    return false;
}


bool ByteCodes::has_predicted_send_code( ByteCodes::Code code ) {
    switch ( code ) {
        case ByteCodes::Code::interpreted_send_1:
            return true;
        default:
            return false;
    }
    ShouldNotReachHere();
    return false;
}


ByteCodes::Code ByteCodes::original_send_code_for( ByteCodes::Code code ) {
    switch ( code ) {
        case ByteCodes::Code::smi_add:
            [[fallthrough]];
        case ByteCodes::Code::smi_sub:
            [[fallthrough]];
        case ByteCodes::Code::smi_mult:
            [[fallthrough]];
        case ByteCodes::Code::smi_div:
            [[fallthrough]];
        case ByteCodes::Code::smi_mod:
            [[fallthrough]];
        case ByteCodes::Code::smi_create_point:
            [[fallthrough]];
        case ByteCodes::Code::smi_equal:
            [[fallthrough]];
        case ByteCodes::Code::smi_not_equal:
            [[fallthrough]];
        case ByteCodes::Code::smi_less:
            [[fallthrough]];
        case ByteCodes::Code::smi_less_equal:
            [[fallthrough]];
        case ByteCodes::Code::smi_greater:
            [[fallthrough]];
        case ByteCodes::Code::smi_greater_equal:
            [[fallthrough]];
        case ByteCodes::Code::smi_and:
            [[fallthrough]];
        case ByteCodes::Code::smi_or:
            [[fallthrough]];
        case ByteCodes::Code::smi_xor:
            [[fallthrough]];
        case ByteCodes::Code::smi_shift:
            [[fallthrough]];
        case ByteCodes::Code::objectArray_at:
            [[fallthrough]];
        case ByteCodes::Code::objectArray_at_put:
            [[fallthrough]];
        case ByteCodes::Code::double_equal:
            [[fallthrough]];
        case ByteCodes::Code::double_tilde:
            return code;
        default:
            return interpreted_send_code_for( code );
    }
    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


ByteCodes::Code ByteCodes::interpreted_send_code_for( ByteCodes::Code code ) {
    switch ( code ) {
        case ByteCodes::Code::interpreted_send_0:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_0:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_0:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_0:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_0:
            return ByteCodes::Code::interpreted_send_0;

        case ByteCodes::Code::interpreted_send_0_pop:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_0_pop:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_0_pop:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_0_pop:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_0_pop:
            return ByteCodes::Code::interpreted_send_0_pop;

        case ByteCodes::Code::interpreted_send_1:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_1:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_1:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_1:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_1:
            return ByteCodes::Code::interpreted_send_1;

        case ByteCodes::Code::interpreted_send_1_pop:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_1_pop:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_1_pop:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_1_pop:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_1_pop:
            return ByteCodes::Code::interpreted_send_1_pop;

        case ByteCodes::Code::interpreted_send_2:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_2:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_2:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_2:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_2:
            return ByteCodes::Code::interpreted_send_2;

        case ByteCodes::Code::interpreted_send_2_pop:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_2_pop:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_2_pop:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_2_pop:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_2_pop:
            return ByteCodes::Code::interpreted_send_2_pop;

        case ByteCodes::Code::interpreted_send_n:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_n:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_n:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_n:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_n:
            return ByteCodes::Code::interpreted_send_n;

        case ByteCodes::Code::interpreted_send_n_pop:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_n_pop:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_n_pop:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_n_pop:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_n_pop:
            return ByteCodes::Code::interpreted_send_n_pop;

        case ByteCodes::Code::interpreted_send_self:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_self:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_self:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_self:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_self:
            return ByteCodes::Code::interpreted_send_self;

        case ByteCodes::Code::interpreted_send_self_pop:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_self_pop:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_self_pop:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_self_pop:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_self_pop:
            return ByteCodes::Code::interpreted_send_self_pop;

        case ByteCodes::Code::interpreted_send_super:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_super:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_super:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_super:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_super:
            return ByteCodes::Code::interpreted_send_super;

        case ByteCodes::Code::interpreted_send_super_pop:
            [[fallthrough]];
        case ByteCodes::Code::compiled_send_super_pop:
            [[fallthrough]];
        case ByteCodes::Code::primitive_send_super_pop:
            [[fallthrough]];
        case ByteCodes::Code::polymorphic_send_super_pop:
            [[fallthrough]];
        case ByteCodes::Code::megamorphic_send_super_pop:
            return ByteCodes::Code::interpreted_send_super_pop;

        case ByteCodes::Code::access_send_0:
            return ByteCodes::Code::interpreted_send_0;
        case ByteCodes::Code::access_send_self:
            return ByteCodes::Code::interpreted_send_self;

        case ByteCodes::Code::smi_add:
            [[fallthrough]];
        case ByteCodes::Code::smi_sub:
            [[fallthrough]];
        case ByteCodes::Code::smi_mult:
            [[fallthrough]];
        case ByteCodes::Code::smi_div:
            [[fallthrough]];
        case ByteCodes::Code::smi_mod:
            [[fallthrough]];
        case ByteCodes::Code::smi_create_point:
            [[fallthrough]];
        case ByteCodes::Code::smi_equal:
            [[fallthrough]];
        case ByteCodes::Code::smi_not_equal:
            [[fallthrough]];
        case ByteCodes::Code::smi_less:
            [[fallthrough]];
        case ByteCodes::Code::smi_less_equal:
            [[fallthrough]];
        case ByteCodes::Code::smi_greater:
            [[fallthrough]];
        case ByteCodes::Code::smi_greater_equal:
            [[fallthrough]];
        case ByteCodes::Code::smi_and:
            [[fallthrough]];
        case ByteCodes::Code::smi_or:
            [[fallthrough]];
        case ByteCodes::Code::smi_xor:
            [[fallthrough]];
        case ByteCodes::Code::smi_shift:
            [[fallthrough]];
        case ByteCodes::Code::double_equal:
            [[fallthrough]];
        case ByteCodes::Code::double_tilde:
            [[fallthrough]];
        case ByteCodes::Code::objectArray_at:
            return ByteCodes::Code::interpreted_send_1;
        case ByteCodes::Code::objectArray_at_put:
            return ByteCodes::Code::interpreted_send_2;

        default                : st_fatal( "not a send bytecode" );
    }
    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


ByteCodes::Code ByteCodes::compiled_send_code_for( ByteCodes::Code code ) {
    switch ( interpreted_send_code_for( code ) ) {
        case ByteCodes::Code::interpreted_send_0:
            return ByteCodes::Code::compiled_send_0;
        case ByteCodes::Code::interpreted_send_1:
            return ByteCodes::Code::compiled_send_1;
        case ByteCodes::Code::interpreted_send_2:
            return ByteCodes::Code::compiled_send_2;
        case ByteCodes::Code::interpreted_send_n:
            return ByteCodes::Code::compiled_send_n;
        case ByteCodes::Code::interpreted_send_self:
            return ByteCodes::Code::compiled_send_self;
        case ByteCodes::Code::interpreted_send_super:
            return ByteCodes::Code::compiled_send_super;
        case ByteCodes::Code::interpreted_send_0_pop:
            return ByteCodes::Code::compiled_send_0_pop;
        case ByteCodes::Code::interpreted_send_1_pop:
            return ByteCodes::Code::compiled_send_1_pop;
        case ByteCodes::Code::interpreted_send_2_pop:
            return ByteCodes::Code::compiled_send_2_pop;
        case ByteCodes::Code::interpreted_send_n_pop:
            return ByteCodes::Code::compiled_send_n_pop;
        case ByteCodes::Code::interpreted_send_self_pop:
            return ByteCodes::Code::compiled_send_self_pop;
        case ByteCodes::Code::interpreted_send_super_pop:
            return ByteCodes::Code::compiled_send_super_pop;

        default                : st_fatal( "no corresponding compiled send code" );
    }
    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


ByteCodes::Code ByteCodes::access_send_code_for( ByteCodes::Code code ) {
    switch ( interpreted_send_code_for( code ) ) {
        case ByteCodes::Code::interpreted_send_0:
            return ByteCodes::Code::access_send_0;
        case ByteCodes::Code::interpreted_send_self:
            return ByteCodes::Code::access_send_self;
        default: st_fatal( "no corresponding access send code" );
    }
    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


ByteCodes::Code ByteCodes::primitive_send_code_for( ByteCodes::Code code ) {

    switch ( interpreted_send_code_for( code ) ) {
        case ByteCodes::Code::interpreted_send_0:
            return ByteCodes::Code::primitive_send_0;
        case ByteCodes::Code::interpreted_send_1:
            return ByteCodes::Code::primitive_send_1;
        case ByteCodes::Code::interpreted_send_2:
            return ByteCodes::Code::primitive_send_2;
        case ByteCodes::Code::interpreted_send_n:
            return ByteCodes::Code::primitive_send_n;
        case ByteCodes::Code::interpreted_send_self:
            return ByteCodes::Code::primitive_send_self;
        case ByteCodes::Code::interpreted_send_super:
            return ByteCodes::Code::primitive_send_super;
        case ByteCodes::Code::interpreted_send_0_pop:
            return ByteCodes::Code::primitive_send_0_pop;
        case ByteCodes::Code::interpreted_send_1_pop:
            return ByteCodes::Code::primitive_send_1_pop;
        case ByteCodes::Code::interpreted_send_2_pop:
            return ByteCodes::Code::primitive_send_2_pop;
        case ByteCodes::Code::interpreted_send_n_pop:
            return ByteCodes::Code::primitive_send_n_pop;
        case ByteCodes::Code::interpreted_send_self_pop:
            return ByteCodes::Code::primitive_send_self_pop;
        case ByteCodes::Code::interpreted_send_super_pop:
            return ByteCodes::Code::primitive_send_super_pop;

        default: st_fatal( "no corresponding primitive send code" );
    }

    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


ByteCodes::Code ByteCodes::polymorphic_send_code_for( ByteCodes::Code code ) {
    switch ( interpreted_send_code_for( code ) ) {
        case ByteCodes::Code::interpreted_send_0:
            return ByteCodes::Code::polymorphic_send_0;
        case ByteCodes::Code::interpreted_send_1:
            return ByteCodes::Code::polymorphic_send_1;
        case ByteCodes::Code::interpreted_send_2:
            return ByteCodes::Code::polymorphic_send_2;
        case ByteCodes::Code::interpreted_send_n:
            return ByteCodes::Code::polymorphic_send_n;
        case ByteCodes::Code::interpreted_send_self:
            return ByteCodes::Code::polymorphic_send_self;
        case ByteCodes::Code::interpreted_send_super:
            return ByteCodes::Code::polymorphic_send_super;
        case ByteCodes::Code::interpreted_send_0_pop:
            return ByteCodes::Code::polymorphic_send_0_pop;
        case ByteCodes::Code::interpreted_send_1_pop:
            return ByteCodes::Code::polymorphic_send_1_pop;
        case ByteCodes::Code::interpreted_send_2_pop:
            return ByteCodes::Code::polymorphic_send_2_pop;
        case ByteCodes::Code::interpreted_send_n_pop:
            return ByteCodes::Code::polymorphic_send_n_pop;
        case ByteCodes::Code::interpreted_send_self_pop:
            return ByteCodes::Code::polymorphic_send_self_pop;
        case ByteCodes::Code::interpreted_send_super_pop:
            return ByteCodes::Code::polymorphic_send_super_pop;

        default: st_fatal( "no corresponding POLYMORPHIC send code" );
    }
    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


ByteCodes::Code ByteCodes::megamorphic_send_code_for( ByteCodes::Code code ) {

    switch ( interpreted_send_code_for( code ) ) {
        case ByteCodes::Code::interpreted_send_0:
            return ByteCodes::Code::megamorphic_send_0;
        case ByteCodes::Code::interpreted_send_1:
            return ByteCodes::Code::megamorphic_send_1;
        case ByteCodes::Code::interpreted_send_2:
            return ByteCodes::Code::megamorphic_send_2;
        case ByteCodes::Code::interpreted_send_n:
            return ByteCodes::Code::megamorphic_send_n;
        case ByteCodes::Code::interpreted_send_self:
            return ByteCodes::Code::megamorphic_send_self;
        case ByteCodes::Code::interpreted_send_super:
            return ByteCodes::Code::megamorphic_send_super;
        case ByteCodes::Code::interpreted_send_0_pop:
            return ByteCodes::Code::megamorphic_send_0_pop;
        case ByteCodes::Code::interpreted_send_1_pop:
            return ByteCodes::Code::megamorphic_send_1_pop;
        case ByteCodes::Code::interpreted_send_2_pop:
            return ByteCodes::Code::megamorphic_send_2_pop;
        case ByteCodes::Code::interpreted_send_n_pop:
            return ByteCodes::Code::megamorphic_send_n_pop;
        case ByteCodes::Code::interpreted_send_self_pop:
            return ByteCodes::Code::megamorphic_send_self_pop;
        case ByteCodes::Code::interpreted_send_super_pop:
            return ByteCodes::Code::megamorphic_send_super_pop;
        default: st_fatal( "no corresponding MEGAMORPHIC send code" );
    }
    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


ByteCodes::Code ByteCodes::original_primitive_call_code_for( Code code ) {
    switch ( code ) {
        case ByteCodes::Code::prim_call:
            [[fallthrough]];
        case ByteCodes::Code::primitive_call_lookup:
            return ByteCodes::Code::primitive_call_lookup;

        case ByteCodes::Code::primitive_call_self:
            [[fallthrough]];
        case ByteCodes::Code::primitive_call_self_lookup:
            return ByteCodes::Code::primitive_call_self_lookup;

        case ByteCodes::Code::primitive_call_failure:
            [[fallthrough]];
        case ByteCodes::Code::primitive_call_failure_lookup:
            return ByteCodes::Code::primitive_call_failure_lookup;

        case ByteCodes::Code::primitive_call_self_failure:
            [[fallthrough]];
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return ByteCodes::Code::primitive_call_self_failure_lookup;

        default: st_fatal( "no corresponding primitive call code" );
    }
    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


ByteCodes::Code ByteCodes::primitive_call_code_for( Code code ) {
    switch ( code ) {
        case ByteCodes::Code::primitive_call_lookup:
            return ByteCodes::Code::prim_call;
        case ByteCodes::Code::primitive_call_self_lookup:
            return ByteCodes::Code::primitive_call_self;
        case ByteCodes::Code::primitive_call_failure_lookup:
            return ByteCodes::Code::primitive_call_failure;
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return ByteCodes::Code::primitive_call_self_failure;
        default: st_fatal( "no corresponding primitive call code" );
    }
    ShouldNotReachHere();
    return ByteCodes::Code::halt;
}


void ByteCodes::print() {
    for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES); i++ ) {
        Code code = Code( i );
        if ( is_defined( code ) ) {
            SPDLOG_INFO( "%s", name( code ) );
            SPDLOG_INFO( "  %s", code_type_as_string( code_type( code ) ) );
            SPDLOG_INFO( "  %s", send_type_as_string( send_type( code ) ) );
            _console->cr();
        }
    }
}


// Smalltalk generation

static void generate_comment() {
    SPDLOG_INFO( "\t\"" );
    SPDLOG_INFO( "\tGenerated method - do not modify manually" );
    SPDLOG_INFO( "\t(use delta +GenerateSmalltalk to generate)." );
    SPDLOG_INFO( "\t\"" );
}


static bool actually_generated( ByteCodes::Code code ) {
    return ByteCodes::is_defined( code ) and ( ByteCodes::send_type( code ) == ByteCodes::SendType::NO_SEND or ByteCodes::send_type( code ) == ByteCodes::SendType::INTERPRETED_SEND or ByteCodes::send_type( code ) == ByteCodes::SendType::PREDICTED_SEND );
}


static void generate_instr_method() {

    SPDLOG_INFO( "instr: i" );
    generate_comment();
    SPDLOG_INFO( "\t| h l |" );
    SPDLOG_INFO( "\th := i // 16r10." );
    SPDLOG_INFO( "\tl := i \\\\ 16r10." );
    for ( std::int32_t h = 0; h < 0x10; h++ ) {
        SPDLOG_INFO( "\th = 16r{0:x} ifTrue: [", h );
        for ( std::int32_t l = 0; l < 0x10; l++ ) {
            ByteCodes::Code code = ByteCodes::Code( h * 0x10 + l );
            if ( actually_generated( code ) ) {
                SPDLOG_INFO( "\t\tl = 16r{0:x}\tifTrue:\t[ ^ '%s' ].", l, ByteCodes::name( code ) );
            }
        }
        SPDLOG_INFO( "\t\t^ ''" );
        SPDLOG_INFO( "\t]." );
    }
    SPDLOG_INFO( "\tself halt" );
    SPDLOG_INFO( "" );
}


static void print_table_entry_for( const char *selector, ByteCodes::Code code ) {
    SPDLOG_INFO( "\tselector = #%s\t\tifTrue: [ ^ 16r%02X ].", selector, code );
}


static void generate_codeForPrimitive_method() {

    SPDLOG_INFO( "codeForPrimitive: selector" );
    generate_comment();
    print_table_entry_for( "primitiveAdd:ifFail:", ByteCodes::Code::smi_add );
    print_table_entry_for( "primitiveSubtract:ifFail:", ByteCodes::Code::smi_sub );
    print_table_entry_for( "primitiveMultiply:ifFail:", ByteCodes::Code::smi_mult );

    print_table_entry_for( "primitiveSmallIntegerEqual:ifFail:", ByteCodes::Code::smi_equal );
    print_table_entry_for( "primitiveSmallIntegerNotEqual:ifFail:", ByteCodes::Code::smi_not_equal );
    print_table_entry_for( "primitiveLessThan:ifFail:", ByteCodes::Code::smi_less );
    print_table_entry_for( "primitiveLessThanOrEqual:ifFail:", ByteCodes::Code::smi_less_equal );
    print_table_entry_for( "primitiveGreaterThan:ifFail:", ByteCodes::Code::smi_greater );
    print_table_entry_for( "primitiveGreaterThanOrEqual:ifFail:", ByteCodes::Code::smi_greater_equal );

    print_table_entry_for( "primitiveBitAnd:ifFail:", ByteCodes::Code::smi_and );
    print_table_entry_for( "primitiveBitOr:ifFail:", ByteCodes::Code::smi_or );
    print_table_entry_for( "primitiveBitXor:ifFail:", ByteCodes::Code::smi_xor );
    print_table_entry_for( "primitiveRawBitShift:ifFail:", ByteCodes::Code::smi_shift );
    SPDLOG_INFO( "\t^ nil" );
    SPDLOG_INFO( "" );

}


static void generate_signature( const char *sig, char separator ) {
    std::int32_t i     = 1;
    std::int32_t b_cnt = 1;
    std::int32_t w_cnt = 1;
    std::int32_t o_cnt = 1;
    std::int32_t s_cnt = 1;
    while ( sig[ i ] not_eq '\0' ) {
        _console->put( separator );
        switch ( sig[ i ] ) {
            case 'B':
                _console->print( "byte: b%d", b_cnt++ );
                break;
            case 'L':
                _console->print( "word: w%d", w_cnt++ );
                break;
            case 'O':
                _console->print( "Oop: o%d", o_cnt++ );
                break;
            case 'S':
                _console->print( "bytes: s%d", s_cnt++ );
                break;
            default : ShouldNotReachHere();
        }
        separator = ' ';
        i++;
    }
}


static bool has_instVar_access( ByteCodes::Code code ) {
    return ByteCodes::code_type( code ) == ByteCodes::CodeType::INSTANCE_VARIABLE_ACCESS;
}


static bool has_classVar_access( ByteCodes::Code code ) {
    return ByteCodes::code_type( code ) == ByteCodes::CodeType::CLASS_VARIABLE_ACCESS;
}


static bool has_inline_cache( ByteCodes::Code code ) {
    return ByteCodes::send_type( code ) not_eq ByteCodes::SendType::NO_SEND;
}


static void generate_gen_method( ByteCodes::Code code ) {
    const char *sig = ByteCodes::format_as_string( ByteCodes::format( code ) );
    _console->print( ByteCodes::name( code ) );
    generate_signature( sig, '_' );
    _console->cr();
    generate_comment();
    _console->print( "\tself byte: 16r%02X", code );
    generate_signature( sig, ' ' );
    SPDLOG_INFO( "." );
    if ( has_instVar_access( code ) )
        SPDLOG_INFO( "\tself has_instVar_access." );
    if ( has_classVar_access( code ) )
        SPDLOG_INFO( "\tself has_classVar_access." );
    if ( has_inline_cache( code ) )
        SPDLOG_INFO( "\tself has_inline_cache." );
    SPDLOG_INFO( "" );
}


static void generate_float_function_constant_method( Floats::Function f ) {
    SPDLOG_INFO( "float_%s", Floats::function_name_for( f ) );
    generate_comment();
    SPDLOG_INFO( "\t^ {}", f );
    SPDLOG_INFO( "" );
}


static void generate_heap_code_methods() {
    SPDLOG_INFO( "not DeltaHCode methods " );

    generate_instr_method();
    generate_codeForPrimitive_method();

    for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES); i++ ) {
        ByteCodes::Code code = ByteCodes::Code( i );
        if ( actually_generated( code ) )
            generate_gen_method( code );
    }

    st_assert( Floats::is_initialized(), "Floats must be initialized" );
    for ( std::size_t i = 0; i < static_cast<std::int32_t>( Floats::Function::number_of_functions ); i++ ) {
        Floats::Function f = Floats::Function( i );
        generate_float_function_constant_method( f );
    }

    SPDLOG_INFO( "" );
}


// HTML generation

class Markup : StackAllocatedObject {

private:
    const char *_tag;

public:

    Markup( const char *tag ) : _tag{ tag } {
        SPDLOG_INFO( "<{}>", _tag );
    }


    Markup() = default;
    Markup( const Markup & ) = default;
    Markup &operator=( const Markup & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    virtual ~Markup() {
        SPDLOG_INFO( "</{}>", _tag );
    }

};


static void markup( const char *tag, const char *text ) {
    SPDLOG_INFO( "<{}>{}</{}>", tag, text, tag );
}


static void print_format( ByteCodes::Format format ) {

    const char *f = ByteCodes::format_as_string( format );

    while ( *f ) {
        switch ( *f ) {
            case 'B':
                SPDLOG_INFO( " byte " );
                break;
            case 'L':
                SPDLOG_INFO( " std::int32_t " );
                break;
            case 'O':
                SPDLOG_INFO( " Oop " );
                break;
            case 'S':
                SPDLOG_INFO( " {byte} " );
                break;
            default: ShouldNotReachHere();
                break;
        }
        f++;
    }
}


static const char *arguments_as_string( ByteCodes::ArgumentSpec spec ) {
    switch ( spec ) {
        case ByteCodes::ArgumentSpec::recv_0_args:
            return "receiver";
        case ByteCodes::ArgumentSpec::recv_1_args:
            return "receiver argument_1";
        case ByteCodes::ArgumentSpec::recv_2_args:
            return "receiver argument_1 argument_2";
        case ByteCodes::ByteCodes::ArgumentSpec::recv_n_args:
            return "receiver argument_1 argument_2 ... argument_n";
        case ByteCodes::ByteCodes::ArgumentSpec::args_only:
            return "argument_1 argument_2 ... argument_n";
        default: ShouldNotReachHere();
    }
    return nullptr;
}


static void generate_HTML_for( ByteCodes::Code code ) {

    _console->print( "<td>%02X<sub>H</sub><td><B>%s</B><td>", std::int32_t( code ), ByteCodes::name( code ) );
    print_format( ByteCodes::format( code ) );
    SPDLOG_INFO( "<td>{}", ByteCodes::single_step( code ) ? "intercepted" : "" );

    //
    if ( ByteCodes::code_type( code ) == ByteCodes::CodeType::MESSAGE_SEND ) {
        _console->print( "<td>%s", ByteCodes::send_type_as_string( ByteCodes::send_type( code ) ) );
        _console->print( "<td>%s", arguments_as_string( ByteCodes::argument_spec( code ) ) );
    }

    SPDLOG_INFO( "<tr>" );
}


static void generate_HTML_for( ByteCodes::CodeType type ) {

    {
        Markup tag( "h3" );
        SPDLOG_INFO( "%s bytecodes", ByteCodes::code_type_as_string( type ) );
    }
    {
        Markup tag( "table" );
        SPDLOG_INFO( "<th>Code</th><th>Name</th><th>Format</th><th>Single step</th>" );
        if ( type == ByteCodes::CodeType::MESSAGE_SEND ) {
            _console->print( "<th>Send type</th><th>Arguments</th>" );
        }
        SPDLOG_INFO( "<tr>" );
        for ( std::size_t i = 0; i < static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES); i++ ) {
            ByteCodes::Code code = ByteCodes::Code( i );
            if ( ByteCodes::is_defined( code ) and ByteCodes::code_type( code ) == type ) {
                generate_HTML_for( code );
            }
        }
        SPDLOG_INFO( "</tr>" );

    }

    SPDLOG_INFO( "<hr/>" );
}


static void generate_HTML_docu() {
    Markup tag( "html" );
    SPDLOG_INFO( "<!-- do not modify - use delta +GenerateHTML to generate -->" );
    {
        Markup tag( "head" );
        markup( "title", "Delta ByteCodes" );
    }
    {
        Markup tag( "body" );
        {
            Markup tag( "h2" );
            SPDLOG_INFO( "Delta ByteCodes (Version {})", ByteCodes::version() );
        }

        for ( std::size_t i = 0; static_cast<ByteCodes::CodeType>(i) < ByteCodes::CodeType::NUMBER_OF_CODE_TYPES; i++ ) {
            generate_HTML_for( ByteCodes::CodeType( i ) );
        }

    }
}


void bytecodes_init() {
    SPDLOG_INFO( "system-init:  bytecodes_init" );

    ByteCodes::init();

    if ( GenerateSmalltalk ) {
        generate_heap_code_methods();
        exit( EXIT_SUCCESS );
    }

    if ( GenerateHTML ) {
        generate_HTML_docu();
        exit( EXIT_SUCCESS );
    }

}
