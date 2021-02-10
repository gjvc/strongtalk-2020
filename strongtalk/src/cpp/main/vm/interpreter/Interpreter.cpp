
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/DeltaProcess.hpp"
#include "vm/runtime/VMProcess.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/utilities/disassembler.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/runtime/ResourceMark.hpp"


//
// Interpreter stack frame
//
// An interpreter stack frame always provides Space for at least one temporary (temp_0).
// This speeds up method activation for methods with less than 2 temporaries.
//
// More temporaries are allocated via special ByteCodes.
//
// Furthermore, since there is always
// (at least) one temporary, which in case of context extensions holds the pointer to
// the heap-allocated context, non-local returns become simpler, because they can assume
// at least temp_0 to be there (if there is a chance of having no temporaries and a self
// send without args, the temp0 location would contain the return address of the callee
// (a non-Oop), if there were no temp_0 always).
//

static constexpr std::int32_t float_0_offset  = OOP_SIZE * ( frame_temp_offset - 3 );
static constexpr std::int32_t temp_1_offset   = OOP_SIZE * ( frame_temp_offset - 1 );
static constexpr std::int32_t temp_0_offset   = OOP_SIZE * frame_temp_offset;
static constexpr std::int32_t esi_offset      = OOP_SIZE * frame_hp_offset;
static constexpr std::int32_t self_offset     = OOP_SIZE * frame_receiver_offset;
static constexpr std::int32_t link_offset     = OOP_SIZE * frame_link_offset;
static constexpr std::int32_t ret_addr_offset = OOP_SIZE * frame_return_addr_offset;
static constexpr std::int32_t arg_n_offset    = OOP_SIZE * ( frame_arg_offset - 1 );

static constexpr std::int32_t max_nof_temps  = 256;
static constexpr std::int32_t max_nof_floats = 256;


bool       Interpreter::_is_initialized   = false;
const char *Interpreter::_code_begin_addr = nullptr;
const char *Interpreter::_code_end_addr   = nullptr;

std::int32_t Interpreter::_interpreter_loop_counter       = 0;
std::int32_t Interpreter::_interpreter_loop_counter_limit = 0;


bool Interpreter::contains( const char *pc ) {
    return ( _code_begin_addr <= pc and pc < _code_end_addr ) or ( pc == StubRoutines::single_step_continuation() );
}

//extern "C" const char * InterpreterCodeStatus() { return "\x01\x00\x00\x00\x00\x00"; }
extern "C" const char *InterpreterCodeStatus() {
    return "\x00\x01\x01\x01\x01\x01";
}


bool Interpreter::is_optimized() {
    return InterpreterCodeStatus()[ 0 ] == 1;
}


bool Interpreter::can_trace_bytecodes() {
    return InterpreterCodeStatus()[ 1 ] == 1;
}


bool Interpreter::can_trace_sends() {
    return InterpreterCodeStatus()[ 2 ] == 1;
}


bool Interpreter::has_assertions() {
    return InterpreterCodeStatus()[ 3 ] == 1;
}


bool Interpreter::has_stack_checks() {
    return InterpreterCodeStatus()[ 4 ] == 1;
}


bool Interpreter::has_timers() {
    return InterpreterCodeStatus()[ 5 ] == 1;
}


void Interpreter::print_code_status() {

    SPDLOG_INFO( "interpreter-status-optimized[{}]", is_optimized() ? "yes" : "no" );
    SPDLOG_INFO( "interpreter-trace-bytecodes[{}]", can_trace_bytecodes() ? "yes" : "no" );
    SPDLOG_INFO( "interpreter-trace-sends[{}]", can_trace_sends() ? "yes" : "no" );
    SPDLOG_INFO( "interpreter-trace-assertions[{}]", has_assertions() ? "yes" : "no" );
    SPDLOG_INFO( "interpreter-trace-stack_checks[{}]", has_stack_checks() ? "yes" : "no" );
    SPDLOG_INFO( "interpreter-trace-timers[{}]", has_timers() ? "yes" : "no" );

}


// Loops

void Interpreter::loop_counter_overflow() {

    const bool debug  = false;
    MethodOop  method = DeltaProcess::active()->last_frame().method();
    method->set_invocation_count( method->invocation_count() + loop_counter_limit() );

    if ( debug ) {
        ResourceMark resourceMark;
        SPDLOG_INFO( "loop_counter_overflow: loop counter [{}] exceeds loop counter limit [{}], method[{}]",
                      loop_counter(), loop_counter_limit(), method->print_value_string() );
    }

    reset_loop_counter();
}


std::int32_t Interpreter::loop_counter() {
    return Interpreter::_interpreter_loop_counter;
}


void Interpreter::reset_loop_counter() {
    Interpreter::_interpreter_loop_counter = 0;
}


std::int32_t Interpreter::loop_counter_limit() {
    return Interpreter::_interpreter_loop_counter_limit;
}


void Interpreter::set_loop_counter_limit( std::int32_t limit ) {
    st_assert( 0 <= limit, "loop counter limit must be positive" );
    Interpreter::_interpreter_loop_counter_limit = limit;
}


// Runtime routines called from interpreter_asm.asm
//
// The following routine is for inline_cache_miss calls from within interpreter_asm.asm.
// Can go away as soon as not needed anymore (because interpreter is generated).

extern "C" void inline_cache_miss() {
    InterpretedInlineCache::inline_cache_miss();
}


extern "C" void verifyPIC( Oop pic ) {
    if ( not Universe::is_heap( (Oop *) pic ) ) st_fatal( "pic should be in heap" );
    if ( not pic->isObjectArray() ) st_fatal( "pic should be an objectArray" );
    std::size_t length = ObjectArrayOop( pic )->length();
    if ( not( 2 * size_of_smallest_interpreterPIC <= length and length <= 2 * size_of_largest_interpreterPIC ) ) st_fatal( "pic has wrong length field" );
}



// Runtime routines called from the generated interpreter

void Interpreter::trace_bytecode() {
    if ( TraceInterpreterFramesAt ) {
        if ( TraceInterpreterFramesAt < NumberOfBytecodesExecuted ) {
            Frame f = DeltaProcess::active()->last_frame();
            SPDLOG_INFO( "Frame: fp = 0x{0:x}, sp = 0x{0:x}]", static_cast<void *>( f.fp() ), static_cast<void *>( f.sp() ) );

            for ( Oop    *p    = f.sp(); p <= f.temp_addr( 0 ); p++ ) {
                SPDLOG_INFO( "\t[0x{0:x}]: ", static_cast<void *>( p ) );
                ( *p )->print_value();
                SPDLOG_INFO( "" );
            }
            std::uint8_t *ip   = DeltaProcess::active()->last_frame().hp();
            const char   *name = ByteCodes::name( (ByteCodes::Code) *ip );
            SPDLOG_INFO( "%9d 0x{0:x}: %02x %s", NumberOfBytecodesExecuted, ip, *ip, name );
        }
    } else if ( TraceBytecodes ) {
        std::uint8_t *ip   = DeltaProcess::active()->last_frame().hp();
        const char   *name = ByteCodes::name( (ByteCodes::Code) *ip );
        SPDLOG_INFO( "%9d 0x{0:x}: %02x %s", NumberOfBytecodesExecuted, ip, *ip, name );
    }
}


void Interpreter::warning_illegal( std::int32_t ebx, std::int32_t esi ) {
    SPDLOG_WARN( "illegal instruction (ebx = 0x{0:x}, esi = 0x{0:x})", ebx, esi );
}


void Interpreter::wrong_eax() {
    st_fatal( "interpreter bug: eax doesn't contain the right value" );
}


void Interpreter::wrong_esp() {
    st_fatal( "interpreter bug: esp doesn't contain the right value" );
}


void Interpreter::wrong_ebx() {
    st_fatal( "interpreter bug: high 3 bytes of ebx # 0" );
}


void Interpreter::wrong_obj() {
    st_fatal( "interpreter bug: register doesn't contain a valid Oop" );
}


void Interpreter::wrong_primitive_result() {
    st_fatal( "interpreter bug: primitive failed that is not supposed to fail" );
}


DoubleOop Interpreter::oopify_FloatValue() {
    // Called from float_oopify. Get the float argument by inspecting the stack and the argument of the Floats::oopify operation.
    Frame f = DeltaProcess::active()->last_frame();
    st_assert( *( f.hp() - 3 ) == static_cast<std::int32_t>(ByteCodes::Code::float_unary_op_to_oop) and *( f.hp() - 1 ) == static_cast<std::int32_t>( Floats::Function::oopify ), "not called by Floats::oopify" );
    std::int32_t float_index = *( f.hp() - 2 );
    st_assert( 0 <= float_index and float_index < max_nof_floats, "illegal float index" );
    double *float_address = (double *) ( (const char *) f.fp() + ( float_0_offset - ( max_nof_floats - 1 ) * SIZEOF_FLOAT ) + float_index * SIZEOF_FLOAT );
    return OopFactory::new_double( *float_address );
}


std::int32_t *Interpreter::_invocation_counter_addr = nullptr;


void Interpreter::set_invocation_counter_limit( std::int32_t new_limit ) {
    st_assert( _invocation_counter_addr not_eq nullptr, "invocation counter address unknown" );
    st_assert( 0 <= new_limit and new_limit <= MethodOopDescriptor::_invocation_count_max, "illegal counter limit" );
    st_assert( *( (std::uint8_t *) _invocation_counter_addr - 2 ) == 0x81, "not a cmp edx, imm32 instruction anymore?" )
    *_invocation_counter_addr = new_limit << MethodOopDescriptor::_invocation_count_offset;
}


std::int32_t Interpreter::get_invocation_counter_limit() {
    st_assert( _invocation_counter_addr not_eq nullptr, "invocation counter address unknown" );
    return get_unsigned_bitfield( *_invocation_counter_addr, MethodOopDescriptor::_invocation_count_offset, MethodOopDescriptor::_invocation_count_width );
}


static std::int32_t *loop_counter_addr() {
    return nullptr;
}


static std::int32_t *loop_counter_limit_addr();

// entry points accessors

const char *Interpreter::access( const char *entry_point ) {
    st_assert( entry_point, "code not generated yet" );
    return entry_point;
}


extern "C" void restart_primitiveValue();
extern "C" void nlr_single_step_continuation();
extern "C" void redo_bytecode_after_deoptimization();
extern "C" void illegal();


const char *Interpreter::redo_send_entry() {
    return access( _redo_send_entry );
}


//char *Interpreter::restart_primitiveValue() { return access( (char *) ::restart_primitiveValue ); }


const char *Interpreter::nlr_single_step_continuation_entry() {
    return access( Interpreter::_nlr_single_step_continuation_entry );
}

//char* Interpreter::redo_bytecode_after_deoptimization()		{ return access((char*)::redo_bytecode_after_deoptimization); }
//char* Interpreter::illegal()					{ return access((char*)::illegal); }

const char *Interpreter::restart_primitiveValue() {
    return access( _restart_primitiveValue );
}


Label &Interpreter::nlr_single_step_continuation() {
    st_assert( _nlr_single_step_continuation.is_bound(), "code not generated yet" );
    return _nlr_single_step_continuation;
}


const char *Interpreter::redo_bytecode_after_deoptimization() {
    return access( _redo_bytecode_after_deoptimization );
}


const char *Interpreter::illegal() {
    return access( _illegal );
}


const char *Interpreter::deoptimized_return_from_send_without_receiver() {
    return access( _dr_from_send_without_receiver );
}


const char *Interpreter::deoptimized_return_from_send_without_receiver_restore() {
    return access( _dr_from_send_without_receiver_restore );
}


const char *Interpreter::deoptimized_return_from_send_without_receiver_pop() {
    return access( _dr_from_send_without_receiver_pop );
}


const char *Interpreter::deoptimized_return_from_send_without_receiver_pop_restore() {
    return access( _dr_from_send_without_receiver_pop_restore );
}


const char *Interpreter::deoptimized_return_from_send_with_receiver() {
    return access( _dr_from_send_with_receiver );
}


const char *Interpreter::deoptimized_return_from_send_with_receiver_restore() {
    return access( _dr_from_send_with_receiver_restore );
}


const char *Interpreter::deoptimized_return_from_send_with_receiver_pop() {
    return access( _dr_from_send_with_receiver_pop );
}


const char *Interpreter::deoptimized_return_from_send_with_receiver_pop_restore() {
    return access( _dr_from_send_with_receiver_pop_restore );
}


const char *Interpreter::deoptimized_return_from_primitive_call_without_failure_block() {
    return access( _dr_from_primitive_call_without_failure_block );
}


const char *Interpreter::deoptimized_return_from_primitive_call_without_failure_block_restore() {
    return access( _dr_from_primitive_call_without_failure_block_restore );
}


const char *Interpreter::deoptimized_return_from_primitive_call_with_failure_block() {
    return access( _dr_from_primitive_call_with_failure_block );
}


const char *Interpreter::deoptimized_return_from_primitive_call_with_failure_block_restore() {
    return access( _dr_from_primitive_call_with_failure_block_restore );
}


const char *Interpreter::deoptimized_return_from_dll_call() {
    return access( _dr_from_dll_call );
}


const char *Interpreter::deoptimized_return_from_dll_call_restore() {
    return access( _dr_from_dll_call_restore );
}


/*
extern "C" void deoptimized_return_from_send_without_receiver();
extern "C" void deoptimized_return_from_send_without_receiver_restore();
extern "C" void deoptimized_return_from_send_without_receiver_pop();
extern "C" void deoptimized_return_from_send_without_receiver_pop_restore();
extern "C" void deoptimized_return_from_send_with_receiver();
extern "C" void deoptimized_return_from_send_with_receiver_restore();
extern "C" void deoptimized_return_from_send_with_receiver_pop();
extern "C" void deoptimized_return_from_send_with_receiver_pop_restore();
extern "C" void deoptimized_return_from_primitive_call_without_failure_block();
extern "C" void deoptimized_return_from_primitive_call_without_failure_block_restore();
extern "C" void deoptimized_return_from_primitive_call_with_failure_block();
extern "C" void deoptimized_return_from_primitive_call_with_failure_block_restore();
extern "C" void deoptimized_return_from_dll_call();
extern "C" void deoptimized_return_from_dll_call_restore();

char* Interpreter::deoptimized_return_from_send_without_receiver() 				{ return access((char*)::deoptimized_return_from_send_without_receiver); }
char* Interpreter::deoptimized_return_from_send_without_receiver_restore() 			{ return access((char*)::deoptimized_return_from_send_without_receiver_restore); }
char* Interpreter::deoptimized_return_from_send_without_receiver_pop() 				{ return access((char*)::deoptimized_return_from_send_without_receiver_pop); }
char* Interpreter::deoptimized_return_from_send_without_receiver_pop_restore() 			{ return access((char*)::deoptimized_return_from_send_without_receiver_pop_restore); }
char* Interpreter::deoptimized_return_from_send_with_receiver() 				{ return access((char*)::deoptimized_return_from_send_with_receiver); }
char* Interpreter::deoptimized_return_from_send_with_receiver_restore() 			{ return access((char*)::deoptimized_return_from_send_with_receiver_restore); }
char* Interpreter::deoptimized_return_from_send_with_receiver_pop() 				{ return access((char*)::deoptimized_return_from_send_with_receiver_pop); }
char* Interpreter::deoptimized_return_from_send_with_receiver_pop_restore() 			{ return access((char*)::deoptimized_return_from_send_with_receiver_pop_restore); }
char* Interpreter::deoptimized_return_from_primitive_call_without_failure_block() 		{ return access((char*)::deoptimized_return_from_primitive_call_without_failure_block); }
char* Interpreter::deoptimized_return_from_primitive_call_without_failure_block_restore() 	{ return access((char*)::deoptimized_return_from_primitive_call_without_failure_block_restore); }
char* Interpreter::deoptimized_return_from_primitive_call_with_failure_block() 			{ return access((char*)::deoptimized_return_from_primitive_call_with_failure_block); }
char* Interpreter::deoptimized_return_from_primitive_call_with_failure_block_restore() 		{ return access((char*)::deoptimized_return_from_primitive_call_with_failure_block_restore); }
char* Interpreter::deoptimized_return_from_dll_call() 						{ return access((char*)::deoptimized_return_from_dll_call); }
char* Interpreter::deoptimized_return_from_dll_call_restore() 					{ return access((char*)::deoptimized_return_from_dll_call_restore); }
*/


// Interpreter initialization

void Interpreter::init() {
    if ( _is_initialized ) {
        return;
    }

    reset_loop_counter();
    set_loop_counter_limit( LoopCounterLimit );
    set_invocation_counter_limit( InvocationCounterLimit );

    _is_initialized = true;
}
