//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/utilities/LongInteger64.hpp"
#include "vm/system/os.hpp"
#include "vm/assembler/MacroAssembler.hpp"

//
// StubRoutines contains a set of little assembly run-time routines.
// Instead of relying on an external assembler, these routines are generated during system initialization.
// Note: The StubRoutines are *outside* of the interpreter code.
//
// Steps to add a new stub routine:
//
// 1. add a new entry point (class variable)
// 2. add the corresponding entry point accessor (class method)
// 3. implement the stub code generator
// 4. call the generator in init()
//

class StubRoutines : AllStatic {


private:
    static constexpr std::int32_t _code_size = 1024 * 64;
    static bool_t        _is_initialized;           // true if StubRoutines has been initialized
    static const char *_code;                      // the code buffer for the stub routines
    static void (*single_step_fn)();               // pointer to the current single step function (used by evaluator and ST debugger)
    //  static char _code[_code_size];		        // the code buffer for the stub routines

    // add entry points here
    static const char *_icNormalLookupEntry;
    static const char *_icSuperLookupEntry;
    static const char *_zombieNativeMethodEntry;
    static const char *_zombieBlockNativeMethodEntry;
    static const char *_megamorphicIcEntry;
    static const char *_compileBlockEntry;
    static const char *_continueNonLocalReturnEntry;
    static const char *_callSyncDllEntry;
    static const char *_callAsyncDllEntry;
    static const char *_lookupSyncDllEntry;
    static const char *_lookupAsyncDllEntry;
    static const char *_recompileStubEntry;
    static const char *_usedUncommonTrapEntry;
    static const char *_unusedUncommonTrapEntry;
    static const char *_verifyContextChainEntry;
    static const char *_deoptimizeBlockEntry;
    static const char *_callInspectorEntry;

    static const char *_callDelta;
    static const char *_return_from_Delta;
    static const char *_singleStepStub;
    static const char *_single_step_continuation;
    static const char *_unpackUnoptimizedFrames;
    static const char *_provokeNlrAt;
    static const char *_continueNlrInDelta;
    static const char *_handlePascalCallbackStub;
    static const char *_handleCCallbackStub;
    static const char *_oopifyFloat;

    static const char *_PolymorphicInlineCache_stub_entries[];
    static const char *_allocate_entries[];
    static const char *_alien_call_entries[];
    static const char *_alienCallWithArgsEntry;

    // add tracing routines here
    static void trace_DLL_call_1( dll_func_ptr_t function, Oop *last_argument, std::int32_t nof_arguments );

    static void trace_DLL_call_2( std::int32_t result );

    static void wrong_DLL_call();

    // add generators here
    static const char *generate_ic_lookup( MacroAssembler *masm, const char *lookup_routine_entry );

    static const char *generate_call_DLL( MacroAssembler *masm, bool_t async );

    static const char *generate_lookup_DLL( MacroAssembler *masm, bool_t async );

    static const char *generate_ic_normal_lookup( MacroAssembler *masm );

    static const char *generate_ic_super_lookup( MacroAssembler *masm );

    static const char *generate_zombie_nativeMethod( MacroAssembler *masm );

    static const char *generate_zombie_block_nativeMethod( MacroAssembler *masm );

    static const char *generate_megamorphic_ic( MacroAssembler *masm );

    static const char *generate_compile_block( MacroAssembler *masm );

    static const char *generate_continue_NonLocalReturn( MacroAssembler *masm );


    static const char *generate_call_sync_DLL( MacroAssembler *masm ) {
        return generate_call_DLL( masm, false );
    }


    static const char *generate_call_async_DLL( MacroAssembler *masm ) {
        return generate_call_DLL( masm, true );
    }


    static const char *generate_lookup_sync_DLL( MacroAssembler *masm ) {
        return generate_lookup_DLL( masm, false );
    }


    static const char *generate_lookup_async_DLL( MacroAssembler *masm ) {
        return generate_lookup_DLL( masm, true );
    }


    static const char *generate_recompile_stub( MacroAssembler *masm );
    static const char *generate_uncommon_trap( MacroAssembler *masm );
    static const char *generate_verify_context_chain( MacroAssembler *masm );
    static const char *generate_deoptimize_block( MacroAssembler *masm );
    static const char *generate_call_inspector( MacroAssembler *masm );
    static const char *generate_nlr_return_from_Delta( MacroAssembler *masm );
    static const char *generate_call_delta( MacroAssembler *masm );

    // generate_call_delta assigns _return_from_Delta
    static const char *generate_single_step_stub( MacroAssembler *masm );
    static const char *generate_unpack_unoptimized_frames( MacroAssembler *masm );
    static const char *generate_provoke_nlr_at( MacroAssembler *masm );
    static const char *generate_continue_nlr_in_delta( MacroAssembler *masm );
    static const char *generate_handle_pascal_callback_stub( MacroAssembler *masm );
    static const char *generate_handle_C_callback_stub( MacroAssembler *masm );
    static const char *generate_oopify_float( MacroAssembler *masm );
    static const char *generate_PolymorphicInlineCache_stub( MacroAssembler *masm, std::int32_t pic_size );
    static const char *generate_allocate( MacroAssembler *masm, std::int32_t size );
    static const char *generate_alien_call( MacroAssembler *masm, std::int32_t args );
    static const char *generate_alien_call_with_args( MacroAssembler *masm );

    // helpers for generation
    static const char *generateStubRoutine( MacroAssembler *masm, const char *title, const char *gen( MacroAssembler * ) );
    static const char *generateStubRoutine( MacroAssembler *masm, const char *title, const char *gen( MacroAssembler *, std::int32_t ), std::int32_t argument );
    static void alien_arg_size( MacroAssembler *masm, Label &nextArg );
    static void push_alien_arg( MacroAssembler *masm, Label &nextArg );
    static void push_alignment_spacers( MacroAssembler *masm );

public:
    // add entry point accessors here
    static const char *ic_normal_lookup_entry() {
        return _icNormalLookupEntry;
    }


    static const char *ic_super_lookup_entry() {
        return _icSuperLookupEntry;
    }


    static const char *zombie_NativeMethod_entry() {
        return _zombieNativeMethodEntry;
    }


    static const char *zombie_block_NativeMethod_entry() {
        return _zombieBlockNativeMethodEntry;
    }


    static const char *megamorphic_ic_entry() {
        return _megamorphicIcEntry;
    }


    static const char *compile_block_entry() {
        return _compileBlockEntry;
    }


    static const char *continue_NonLocalReturn_entry() {
        return _continueNonLocalReturnEntry;
    }


    static const char *call_DLL_entry( bool_t async ) {
        return async ? _callAsyncDllEntry : _callSyncDllEntry;
    }


    static const char *lookup_DLL_entry( bool_t async ) {
        return async ? _lookupAsyncDllEntry : _lookupSyncDllEntry;
    }


    static const char *recompile_stub_entry() {
        return _recompileStubEntry;
    }


    static const char *used_uncommon_trap_entry() {
        return _usedUncommonTrapEntry;
    }


    static const char *unused_uncommon_trap_entry() {
        return _unusedUncommonTrapEntry;
    }


    static const char *verify_context_chain() {
        return _verifyContextChainEntry;
    }


    static const char *deoptimize_block_entry() {
        return _deoptimizeBlockEntry;
    }


    static const char *call_inspector_entry() {
        return _callInspectorEntry;
    }


    static const char *call_delta() {
        return _callDelta;
    }


    static const char *return_from_Delta() {
        return _return_from_Delta;
    }


    static const char *single_step_stub() {
        return _singleStepStub;
    }


    static const char *single_step_continuation() {
        return _single_step_continuation;
    }


    static const char *unpack_unoptimized_frames() {
        return _unpackUnoptimizedFrames;
    }


    static const char *provoke_nlr_at() {
        return _provokeNlrAt;
    }


    static const char *continue_nlr_in_delta() {
        return _continueNlrInDelta;
    }


    static const char *handle_pascal_callback_stub() {
        return _handlePascalCallbackStub;
    }


    static const char *handle_C_callback_stub() {
        return _handleCCallbackStub;
    }


    static const char *oopify_float() {
        return _oopifyFloat;
    }


    static const char *alien_call_with_args_entry() {
        return _alienCallWithArgsEntry;
    }


    static const char *PolymorphicInlineCache_stub_entry( std::int32_t pic_size );     // PolymorphicInlineCache interpreter stubs: pic_size is number of entries
    static const char *allocate_entry( std::int32_t size );                            // allocation of memOops: size is words in addition to header
    static const char *alien_call_entry( std::int32_t args );                          // alien call out: args is the number of arguments passed to the function called

    // Support for profiling
    static bool_t contains( const char *pc ) {
        return ( _code <= pc ) and ( pc < &_code[ _code_size ] );
    }


    static void init();                // must be called in system initialization phase
    static void setSingleStepHandler( void (*fn)() ) {
        single_step_fn = fn;
    }
};
