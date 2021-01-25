//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/memory/util.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/assembler/MacroAssembler.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/memory/Universe.hpp"


constexpr std::int32_t float_0_offset  = oopSize * ( frame_temp_offset - 3 );
constexpr std::int32_t temp_1_offset   = oopSize * ( frame_temp_offset - 1 );
constexpr std::int32_t temp_0_offset   = oopSize * frame_temp_offset;
constexpr std::int32_t esi_offset      = oopSize * frame_hp_offset;
constexpr std::int32_t self_offset     = oopSize * frame_receiver_offset;
constexpr std::int32_t link_offset     = oopSize * frame_link_offset;
constexpr std::int32_t ret_addr_offset = oopSize * frame_return_addr_offset;
constexpr std::int32_t arg_n_offset    = oopSize * ( frame_arg_offset - 1 );

constexpr std::int32_t max_nof_temps  = 256;
constexpr std::int32_t max_nof_floats = 256;


// The InterpreterGenerator contains the functionality to generate the interpreter during the system initialization phase.

class InterpreterGenerator : StackAllocatedObject {
private:
    MacroAssembler *_macroAssembler;    // used to generate code
    bool_t _debug;              // indicates debug mode

    bool_t _stack_check;            //

    Label _method_entry;            // entry point to activate method execution
    Label _block_entry;             // entry point to activate block execution (primitiveValue)
    Label _inline_cache_miss;       // inline cache misses handling
    Label _smi_send_failure;        // handles predicted smi_t send failures
    Label _issue_NonLocalReturn;    // the starting point for NonLocalReturns in interpreted code
    Label _nlr_testpoint;           // the return point for NonLocalReturns in interpreted sends
    Label _C_nlr_testpoint;         // the return point for NonLocalReturns in C

    Label _boolean_expected;                // boolean expected error
    Label _float_expected;                  // float expected error
    Label _NonLocalReturn_to_dead_frame;    // NonLocalReturn error
    Label _halted;                          // halt executed

    Label _stack_misaligned;       // assertion errors
    Label _ebx_wrong;               //
    Label _obj_wrong;               //
    Label _last_Delta_fp_wrong;     //
    Label _primitive_result_wrong;  //
    const char *_illegal;           //

    // Debugging
    void check_ebx();

    void check_oop( Register reg );

    void should_not_reach_here();

    void stack_check_push();

    void stack_check_pop();

    // Arguments, temporaries & instance variables
    Address arg_addr( std::int32_t i );

    Address arg_addr( Register arg_no );

    Address temp_addr( std::int32_t i );

    Address temp_addr( Register temp_no );

    Address float_addr( Register float_no );

    Address field_addr( Register obj, std::int32_t i );

    Address field_addr( Register obj, Register smi_offset );

    // Instruction sequencing
    void skip_words( std::int32_t n );

    void advance_aligned( std::int32_t n );

    void load_ebx();

    void next_ebx();

    void jump_ebx();

    void load_edi();

    void jump_edi();

    const char *entry_point();


    // Frame addresses
    Address self_addr() {
        return Address( ebp, self_offset );
    }


    Address esi_addr() {
        return Address( ebp, esi_offset );
    }


    Address context_addr() {
        return Address( ebp, temp_0_offset );
    }


    void save_esi() {
        _macroAssembler->movl( esi_addr(), esi );
    }


    void restore_esi() {
        _macroAssembler->movl( esi, esi_addr() );
    }


    void restore_ebx() {
        _macroAssembler->xorl( ebx, ebx );
    }


    // Constant addresses
    Address nil_addr() {
        return Address( std::int32_t( &nilObject ), RelocationInformation::RelocationType::external_word_type );
    }


    Address true_addr() {
        return Address( std::int32_t( &trueObject ), RelocationInformation::RelocationType::external_word_type );
    }


    Address false_addr() {
        return Address( std::int32_t( &falseObject ), RelocationInformation::RelocationType::external_word_type );
    }


    Address smiKlass_addr() {
        return Address( std::int32_t( &smiKlassObject ), RelocationInformation::RelocationType::external_word_type );
    }


    Address doubleKlass_addr() {
        return Address( std::int32_t( &doubleKlassObject ), RelocationInformation::RelocationType::external_word_type );
    }


    Address contextKlass_addr() {
        return Address( std::int32_t( &contextKlassObject ), RelocationInformation::RelocationType::external_word_type );
    }


    // C calls
    void call_C( Label &L );

    void call_C( const char *entry );

    void call_C( Register entry );

    // Parameter passing/returns
    void load_recv( ByteCodes::ArgumentSpec arg_spec );

    void return_tos( ByteCodes::ArgumentSpec arg_spec );

    void zap_context();

    // Debugging
    void generateStopInterpreterAt();

    // Instructions
    const char *push_temp( std::int32_t i );

    const char *push_temp_n();

    const char *push_arg( std::int32_t i );

    const char *push_arg_n();

    const char *push_smi( bool_t negative );

    const char *push_literal();

    const char *push_tos();

    const char *push_self();

    const char *push_const( Address obj_addr );

    const char *push_global();

    const char *push_instVar();

    const char *return_instVar();

    const char *only_pop();

    const char *store_temp( std::int32_t i, bool_t pop = false );

    const char *store_temp_n( bool_t pop = false );

    const char *store_global( bool_t pop = false );

    const char *store_instVar( bool_t pop = false );

    const char *allocate_temps( std::int32_t n );

    const char *allocate_temps_n();

    const char *set_self_via_context();

    const char *with_context_temp( bool_t store, std::int32_t tempNo, std::int32_t contextNo );

    const char *copy_params_into_context( bool_t self, std::int32_t paramsCount );

    const char *float_allocate();

    const char *float_floatify();

    const char *float_oopify();

    const char *float_move();

    const char *float_set();

    const char *float_op( std::int32_t nof_args, bool_t returns_float = false );

    const char *push_closure( std::int32_t nofArgs, bool_t use_context );

    const char *install_context( std::int32_t nofArgs, bool_t for_method );

    const char *predict_prim( bool_t canFail );

    const char *lookup_primitive();

    const char *call_primitive();

    const char *call_primitive_can_fail();

    const char *call_DLL( bool_t async );

    void call_method();

    void call_native( Register entry );

    void generate_error_handler_code();

    void generate_nonlocal_return_code();

    void generate_method_entry_code();

    void generate_inline_cache_miss_handler();

    void generate_predicted_smi_send_failure_handler();

    void generate_redo_send_code();

    void generate_deoptimized_return_restore();

    void generate_deoptimized_return_code();

    void generate_primitiveValue( std::int32_t i );

    void generate_forStubRoutines();

    const char *normal_send( ByteCodes::Code code, bool_t allow_methodOop, bool_t allow_nativeMethod, bool_t primitive_send = false );

    const char *control_cond( ByteCodes::Code code );

    const char *control_while( ByteCodes::Code code );

    const char *control_jump( ByteCodes::Code code );

    const char *access_send( bool_t self );

    const char *primitive_send( ByteCodes::Code code );

    const char *interpreted_send( ByteCodes::Code code );

    const char *compiled_send( ByteCodes::Code code );

    const char *polymorphic_send( ByteCodes::Code code );

    const char *megamorphic_send( ByteCodes::Code code );

    void check_smi_tags();

    const char *smi_add();

    const char *smi_sub();

    const char *smi_mul();

    const char *smi_compare_op( ByteCodes::Code code );

    const char *smi_logical_op( ByteCodes::Code code );

    const char *smi_shift();

    const char *objArray_size();

    const char *objArray_at();

    const char *objArray_at_put();

    const char *compare( bool_t equal );

    const char *special_primitive_send_hint();

    const char *halt();


    const char *illegal() {
        return _illegal;
    }


    const char *local_return( bool_t push_self, std::int32_t nofArgs, bool_t zap = false );

    // Non-local returns
    const char *nonlocal_return_tos();

    const char *nonlocal_return_self();


public:
    InterpreterGenerator( CodeBuffer *code, bool_t debug );

    // Instruction generation
    const char *generate_instruction( ByteCodes::Code code );

    // Generation helper
    void info( const char *name );

    // Generation
    void generate_all();

};
