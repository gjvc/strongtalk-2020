//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"


class Interpreter : AllStatic {

public:
    static bool_t _is_initialized;              // true if Interpreter has been initialized
    static const char *_code_begin_addr;            // the first byte of the interpreter's code
    static const char *_code_end_addr;              // the first byte after the interpreter's code
    static std::int32_t        *_invocation_counter_addr;    // the address of the invocation counter (used in method entry code)

    // entry points
    static const char *_redo_send_entry;    // entry point to redo an interpreted send that called a zombie NativeMethod

    // deoptimized returns
    static const char *_dr_from_send_without_receiver;
    static const char *_dr_from_send_without_receiver_restore;

    static const char *_dr_from_send_without_receiver_pop;
    static const char *_dr_from_send_without_receiver_pop_restore;

    static const char *_dr_from_send_with_receiver;
    static const char *_dr_from_send_with_receiver_restore;

    static const char *_dr_from_send_with_receiver_pop;
    static const char *_dr_from_send_with_receiver_pop_restore;

    static const char *_dr_from_primitive_call_without_failure_block;
    static const char *_dr_from_primitive_call_without_failure_block_restore;

    static const char *_dr_from_primitive_call_with_failure_block;
    static const char *_dr_from_primitive_call_with_failure_block_restore;

    static const char *_dr_from_dll_call;
    static const char *_dr_from_dll_call_restore;

    static const char *_restart_primitiveValue;
    static const char *_nlr_single_step_continuation_entry;
    static Label _nlr_single_step_continuation; // used by single step stub routine
    static const char *_redo_bytecode_after_deoptimization;
    static const char *_illegal;

    // Run-time routines
    static void trace_bytecode();

    static void warning_illegal( std::int32_t ebx, std::int32_t esi );

    static void wrong_eax();        // called in debug mode only
    static void wrong_esp();        // called in debug mode only

    // Floats
    static DoubleOop oopify_FloatValue();

public:
    // Note: The variable below has been introduced for debugging only: It seems that sometimes a NativeMethod
    //       is called from the interpreter (last time it was via a megamorphic self send) that is invalid,
    //       i.e., the NativeMethod reports a cache miss by calling the lookup routines. In order to backtrack
    //       called entry point is stored. Remove this variable if the bug has been found! (gri 7-24-96)
    static char *_last_native_called;    // last NativeMethod entry point called by the interpreter

    static bool_t contains( const char *pc );    // true if pc is within interpreter code; false otherwise

    // Properties of the interpreter code (depends on defines in the assembler code)
    static bool_t is_optimized();

    static bool_t can_trace_bytecodes();

    static bool_t can_trace_sends();

    static bool_t has_assertions();

    static bool_t has_stack_checks();

    static bool_t has_timers();

    static void print_code_status();

    // Loops
    static void loop_counter_overflow();            // this routine gets called when the loop counter overflows
    static std::int32_t loop_counter();                      // the number of loop iterations since the last reset
    static void reset_loop_counter();               // resets the loop counter to 0
    static std::int32_t loop_counter_limit();                // the loop counter limit
    static void set_loop_counter_limit( std::int32_t limit );

    static std::int32_t *loop_counter_addr();

    static std::int32_t *loop_counter_limit_addr();

    // Invocation counters
    static void set_invocation_counter_limit( std::int32_t new_limit );  // set invocation limit
    static std::int32_t get_invocation_counter_limit();            // return invocation limit

    // entry points accessors
    static const char *access( const char *entry_point );

    static const char *redo_send_entry();

    static const char *restart_primitiveValue();

    static const char *nlr_single_step_continuation_entry();

    static Label &nlr_single_step_continuation();

    static const char *redo_bytecode_after_deoptimization();

    static const char *illegal();

    static const char *deoptimized_return_from_send_without_receiver();

    static const char *deoptimized_return_from_send_without_receiver_restore();

    static const char *deoptimized_return_from_send_without_receiver_pop();

    static const char *deoptimized_return_from_send_without_receiver_pop_restore();

    static const char *deoptimized_return_from_send_with_receiver();

    static const char *deoptimized_return_from_send_with_receiver_restore();

    static const char *deoptimized_return_from_send_with_receiver_pop();

    static const char *deoptimized_return_from_send_with_receiver_pop_restore();

    static const char *deoptimized_return_from_primitive_call_without_failure_block();

    static const char *deoptimized_return_from_primitive_call_without_failure_block_restore();

    static const char *deoptimized_return_from_primitive_call_with_failure_block();

    static const char *deoptimized_return_from_primitive_call_with_failure_block_restore();

    static const char *deoptimized_return_from_dll_call();

    static const char *deoptimized_return_from_dll_call_restore();


    static std::int32_t _interpreter_loop_counter;
    static std::int32_t _interpreter_loop_counter_limit;


    // Initialization
    static bool_t is_initialized() {
        return _is_initialized;
    }


    static void init();

    friend class InterpreterGenerator;
};
