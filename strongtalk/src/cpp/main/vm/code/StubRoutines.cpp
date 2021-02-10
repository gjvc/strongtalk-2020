
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/system/dll.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/SavedRegisters.hpp"
#include "vm/runtime/uncommonBranch.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/interpreter/Interpreter.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"


constexpr std::int32_t max_fast_allocate_size   = 9;
constexpr std::int32_t max_fast_alien_call_size = 7;

// entry points
const char *StubRoutines::_icNormalLookupEntry          = nullptr;
const char *StubRoutines::_icSuperLookupEntry           = nullptr;
const char *StubRoutines::_zombieNativeMethodEntry      = nullptr;
const char *StubRoutines::_zombieBlockNativeMethodEntry = nullptr;
const char *StubRoutines::_megamorphicIcEntry           = nullptr;
const char *StubRoutines::_compileBlockEntry            = nullptr;
const char *StubRoutines::_continueNonLocalReturnEntry  = nullptr;
const char *StubRoutines::_callSyncDllEntry             = nullptr;
const char *StubRoutines::_callAsyncDllEntry            = nullptr;
const char *StubRoutines::_lookupSyncDllEntry           = nullptr;
const char *StubRoutines::_lookupAsyncDllEntry          = nullptr;
const char *StubRoutines::_recompileStubEntry           = nullptr;
const char *StubRoutines::_usedUncommonTrapEntry        = nullptr;
const char *StubRoutines::_unusedUncommonTrapEntry      = nullptr;
const char *StubRoutines::_verifyContextChainEntry      = nullptr;
const char *StubRoutines::_deoptimizeBlockEntry         = nullptr;
const char *StubRoutines::_callInspectorEntry           = nullptr;
const char *StubRoutines::_callDelta                    = nullptr;
const char *StubRoutines::_return_from_Delta            = nullptr;
const char *StubRoutines::_singleStepStub               = nullptr;
const char *StubRoutines::_single_step_continuation     = nullptr;
const char *StubRoutines::_unpackUnoptimizedFrames      = nullptr;
const char *StubRoutines::_provokeNlrAt                 = nullptr;
const char *StubRoutines::_continueNlrInDelta           = nullptr;
const char *StubRoutines::_handlePascalCallbackStub     = nullptr;
const char *StubRoutines::_handleCCallbackStub          = nullptr;
const char *StubRoutines::_oopifyFloat                  = nullptr;
const char *StubRoutines::_alienCallWithArgsEntry       = nullptr;

const char *StubRoutines::_PolymorphicInlineCache_stub_entries[static_cast<std::int32_t>( PolymorphicInlineCache::Constant::max_nof_entries ) + 1]; // entry 0 ignored
const char *StubRoutines::_allocate_entries[max_fast_allocate_size + 1];
const char *StubRoutines::_alien_call_entries[max_fast_alien_call_size + 1];


//-----------------------------------------------------------------------------------------
// extern "C" Oop call_delta(void* method, Oop receiver, std::int32_t nofArgs, Oop* args)

extern "C" {

extern char         *method_entry_point;
extern bool         have_nlr_through_C;
extern Oop          nlr_result;
extern std::int32_t nlr_home;
extern std::int32_t nlr_home_id;
extern char         *C_frame_return_addr;

extern std::int32_t *last_delta_fp;    // ebp of the last Delta frame before a C call
extern Oop          *last_delta_sp;    // esp of the last Delta frame before a C call

void popStackHandles( const char *nextFrame );

}


// tracing

void StubRoutines::trace_DLL_call_1( dll_func_ptr_t function, Oop *last_argument, std::int32_t nof_arguments ) {

    //
    if ( not TraceDLLCalls ) {
        return; // in case it has been turned off during run-time
    }

    Frame f = DeltaProcess::active()->last_frame();
    if ( f.is_interpreted_frame() ) {
        // called from within interpreter -> Interpreted_DLLCache available
        MethodOop            m      = f.method();
        CodeIterator         it( m, m->byteCodeIndex_from( f.hp() ) );
        Interpreted_DLLCache *cache = it.dll_cache();
        st_assert( cache->entry_point() == function, "inconsistency with Interpreted_DLLCache" );
        st_assert( cache->number_of_arguments() == nof_arguments, "inconsistency with Interpreted_DLLCache" );
        cache->print();
    } else {
        // called from within compiled code -> Compiled_DLLCache available
        Compiled_DLLCache *cache = compiled_DLLCache_from_return_address( f.pc() );
        st_assert( cache->entry_point() == function, "inconsistency with Compiled_DLLCache" );
        cache->print();
    }

    // print arguments
    Oop *arg_ptr = last_argument + ( nof_arguments - 1 );

    for ( std::size_t i = 1; i <= nof_arguments; i++, arg_ptr-- ) {
        Oop arg = *arg_ptr;
        _console->print( "%6d. ", i );
        if ( arg->isSmallIntegerOop() ) {
            SPDLOG_INFO( "small_int_t   value = 0x%08x", static_cast<SmallIntegerOop>( arg )->value() );

        } else {
            SPDLOG_INFO( "proxy   value = 0x%08x  address = 0x%08x", static_cast<const void *>(arg), static_cast<const void *>( static_cast<ProxyOop>( arg )->get_pointer() ) );
        }

//            if ( i == 5 and static_cast<proxyOop>( arg )->get_pointer() == 0x80000000 ) {
//                static_cast<proxyOop>( arg )->set_pointer( 100 );
//            }
//
//            if ( i == 6 and static_cast<proxyOop>( arg )->get_pointer() == 0x80000000 ) {
//                static_cast<proxyOop>( arg )->set_pointer( 100 );
//            }

        static_cast<ProxyOop>( arg )->print();
    }
}


void StubRoutines::trace_DLL_call_2( std::int32_t result ) {
    if ( not TraceDLLCalls )
        return; // in case it has been turned off during run-time
    SPDLOG_INFO( "    result = 0x%08x", result );
}


void StubRoutines::wrong_DLL_call() {
    {
        ResourceMark resourceMark;
        SPDLOG_INFO( "DLL call error: number of arguments probably wrong" );
    }

    if ( DeltaProcess::active()->is_scheduler() ) {
        DeltaProcess::active()->trace_stack();
        st_fatal( "DLL error in scheduler" );
    } else {
        DeltaProcess::active()->suspend( ProcessState::DLL_lookup_error );
    }

    ShouldNotReachHere();
}


// generators

const char *StubRoutines::generate_ic_lookup( MacroAssembler *masm, const char *lookup_routine_entry ) {

    // Stub routine that calls icLookup which patches the icache.
    // After returning from icLookup the send is continued.
    // The icLookupStub is called from empty iCaches within compiled code.
    //
    // Note that so far, the lookup routine doesn't do allocation, therefore the receiver can be saved/restored on the C stack.
    // (This has to be fixed as soon as 'message not understood' is handled correctly from within compiled code.

    // eax: receiver
    // tos: return address
    const char *entry_point = masm->pc();

    masm->set_last_delta_frame_after_call();
    masm->movl( ebx, Address( esp ) );  // get return address (= ic address)
    masm->pushl( eax );                     // save receiver
    masm->pushl( ebx );                     // pass ic
    masm->pushl( eax );                     // pass receiver
    masm->call( lookup_routine_entry, RelocationInformation::RelocationType::runtime_call_type );    // eax = lookup_routine(receiver, ic)
    masm->movl( ebx, eax );                 // ebx = method code
    masm->popl( eax );                      // get rid of receiver argument
    masm->popl( eax );                      // get rid of ic argument
    masm->popl( eax );                      // restore receiver (don't use argument, might be overwritten)
    masm->reset_last_delta_frame();         //
    masm->jmp( ebx );                       // jump to target

    return entry_point;
}


extern "C" {
const char *icNormalLookup( Oop recv, CompiledInlineCache *ic );
const char *icSuperLookup( Oop recv, CompiledInlineCache *ic );
const char *zombie_nativeMethod( const char *return_addr );
}


const char *StubRoutines::generate_ic_normal_lookup( MacroAssembler *masm ) {
    return generate_ic_lookup( masm, (const char *) icNormalLookup );
}


const char *StubRoutines::generate_ic_super_lookup( MacroAssembler *masm ) {
    return generate_ic_lookup( masm, (const char *) icSuperLookup );
}


const char *StubRoutines::generate_zombie_nativeMethod( MacroAssembler *masm ) {

    // Called from zombie nativeMethods immediately after they are called.
    // Does cleanup of interpreted/compiled ic and redoes the send.
    //
    // Note: zombie_nativeMethod() doesn't do scavenge (otherwise this code
    //       has to change - the receiver cannot be saved on the stack).

    // eax  : receiver
    // tos  : return address to zombie NativeMethod (which called this stub)
    // tos-4: return address to caller NativeMethod (which called the zombie NativeMethod)
    const char *entry_point = masm->pc();
    masm->popl( ebx );                // get rid of return address to zombie NativeMethod
    // eax: receiver
    // tos: return address to caller NativeMethod (which called the zombie NativeMethod)
    masm->set_last_delta_frame_after_call();
    masm->movl( ebx, Address( esp ) ); // get return address (= ic address) - don't pop! (needed for correct Delta frame)
    masm->pushl( eax );                // save receiver
    masm->pushl( ebx );                // pass ic
    masm->call( (const char *) zombie_nativeMethod, RelocationInformation::RelocationType::runtime_call_type );    // eax = zombie_nativeMethod(return_address)
    masm->movl( ecx, eax );            // ecx = entry point to redo send
    masm->popl( ebx );                 // get rid of ic argument
    masm->popl( eax );                 // restore receiver
    masm->popl( ebx );                 // get rid of return address
    masm->reset_last_delta_frame();
    masm->jmp( ecx );                  // redo send
    return entry_point;
}

// The code below (validateContextChain(...) and generate_verify_context_chain(...)
// can be removed if it turns out that the new solution (using deoptimize_block()
// and generate_deoptimize_block(...)) is working correctly. See also call site
// (code for PrologueNode). - gri 6/25/96

static bool validateContextChain( BlockClosureOop block ) {
    st_assert( block->is_block(), "must be block" );
    bool is_valid = true;

    {
        ContextOop con      = block->lexical_scope();
        ContextOop prev_con = nullptr;
        // verify entire context chain
        st_assert( con->is_context(), "expecting a context" );

        //while (true) {
        //  if (con->unoptimized_context() not_eq nullptr) {
        //    is_valid = false;
        //    break;
        //  }
        //  if (not con->has_outer_context()) break;
        //  con = con->outer_context();
        //}

        // slr fixup unoptimized context refs as we go
        while ( true ) {
            if ( con->unoptimized_context() not_eq nullptr ) {
                is_valid = false;
                if ( prev_con ) {
                    prev_con->set_parent( con->parent() );
                } else {
                    prev_con = con;
                }
                //break;
            } else {
                prev_con = con;
            }
            if ( not con->has_outer_context() )
                break;
            con = con->outer_context();
        }
        // slr fixup unoptimized context refs as we go
    }

    if ( is_valid )
        return true;

    st_assert( block->isCompiledBlock(), "we should be in a compiled block" );

    // Patch the blockClosure
    MethodOop    method = block->method();
    NativeMethod *nm    = block->jump_table_entry()->block_nativeMethod();

    SPDLOG_INFO( "Deoptimized context in blockClosure -> switch to methodOop 0x{x}", static_cast<const void *>( nm ) );
    {
        block->set_method( method );
        ContextOop con    = block->lexical_scope();
        ContextOop un_con = con->unoptimized_context();
        if ( un_con ) {
            block->set_lexical_scope( un_con );
        } else {
            Unimplemented();
        }
    }
    return false;
}


static void deoptimize_context_and_patch_block( BlockClosureOop block ) {

    st_assert( block->is_block(), "must be block" );
    st_assert( block->isCompiledBlock(), "we should be in a compiled block" );

    // Patch the blockClosure
    MethodOop    method = block->method();
    NativeMethod *nm    = block->jump_table_entry()->block_nativeMethod();

    SPDLOG_INFO( "Deoptimized context in blockClosure -> switch to methodOop 0x%lx", static_cast<const void *>( nm ) );

    ContextOop con = block->lexical_scope();
    block->set_method( method );
    if ( method->expectsContext() ) {
        guarantee( con and con->is_context(), "Optimized context must be present" );

        // we have (nm, con) the deoptimize the context
        ContextOop unoptimized_con = con->unoptimized_context();
        guarantee( unoptimized_con and unoptimized_con->is_context(), "Unoptimized context must be present" );
        block->set_lexical_scope( unoptimized_con );
    } else {
        guarantee( not con->is_context(), "Cannot be a context" );
    }
}

//extern "C" void restart_primitiveValue();
//extern "C" char* restart_primitiveValue;

const char *StubRoutines::generate_zombie_block_nativeMethod( MacroAssembler *masm ) {

    st_assert( Interpreter::_restart_primitiveValue, "restart_primitiveValue must have been generated before generate_zombie_block_nativeMethod" );
    // %hack indirect load

    // Called from zombie nativeMethods immediately after they are called.
    // Does cleanup of interpreted/compiled ic and redoes the send.
    //
    // Note: zombie_nativeMethod() doesn't do scavenge (otherwise this code has to change - the receiver cannot be saved on the stack).

    // eax  : receiver
    // tos  : return address to zombie NativeMethod (which called this stub)
    // tos-4: return address to caller NativeMethod (which called the zombie NativeMethod)
    const char *entry_point = masm->pc();

    masm->set_last_delta_frame_after_call();
    masm->pushl( self_reg ); // pass argument (C calling convention)
    masm->call( (const char *) deoptimize_context_and_patch_block, RelocationInformation::RelocationType::runtime_call_type );
    masm->movl( ebx, eax );
    masm->popl( self_reg );
    masm->reset_last_delta_frame();
    masm->addl( esp, 4 );
    masm->movl( edx, Address( (std::int32_t) Interpreter::restart_primitiveValue(), RelocationInformation::RelocationType::external_word_type ) );
    masm->jmp( edx );
//  masm->jmp(restart_primitiveValue, RelocationInformation::RelocationType::runtime_call_type);
    return entry_point;
}


extern "C" char *o;


const char *StubRoutines::generate_megamorphic_ic( MacroAssembler *masm ) {

    // Called from within a MIC (MEGAMORPHIC inline cache), the special variant of PICs for compiled code (see compiledPIC.hpp/cpp).
    // The MIC layout is as follows:
    //
    // call <this stub routine>
    // selector			<--- return address (tos)
    //
    // Note: Don't use this for MEGAMORPHIC super sends!

    Label isSmallIntegerOop, probe_primary_cache, probe_secondary_cache, call_method, is_methodOop, do_lookup;

    masm->bind( isSmallIntegerOop );                // small_int_t case (assumed to be infrequent)
    masm->movl( ecx, Address( (std::int32_t) &smiKlassObject, RelocationInformation::RelocationType::external_word_type ) );
    masm->jmp( probe_primary_cache );

    // eax    : receiver
    // tos    : return address pointing to selector in MIC
    // tos + 4: return address of MEGAMORPHIC send in compiled code
    // tos + 8: last argument/receiver
    const char *entry_point = masm->pc();
    masm->popl( ebx );                // get return address (MIC cache)
    masm->test( eax, MEMOOP_TAG );            // check if small_int_t
    masm->jcc( Assembler::Condition::zero, isSmallIntegerOop );        // if so, get small_int_t class directly
    masm->movl( ecx, Address( eax, MemOopDescriptor::klass_byte_offset() ) );    // otherwise, load receiver class

    // probe primary cache
    //
    // eax: receiver
    // ebx: MIC cache pointer
    // ecx: receiver klass
    // tos: return address of MEGAMORPHIC send in compiled code (ic)
    masm->bind( probe_primary_cache );        // compute hash value
    masm->movl( edx, Address( ebx ) );        // get selector
    // compute hash value
    masm->movl( edi, ecx );
    masm->xorl( edi, edx );
    masm->andl( edi, ( primary_cache_size - 1 ) << 4 );
    // probe cache
    masm->cmpl( ecx, Address( edi, LookupCache::primary_cache_address() + 0 * OOP_SIZE ) );
    masm->jcc( Assembler::Condition::notEqual, probe_secondary_cache );
    masm->cmpl( edx, Address( edi, LookupCache::primary_cache_address() + 1 * OOP_SIZE ) );
    masm->jcc( Assembler::Condition::notEqual, probe_secondary_cache );
    masm->movl( ecx, Address( edi, LookupCache::primary_cache_address() + 2 * OOP_SIZE ) );

    // call method
    //
    // eax: receiver
    // ecx: methodOop/NativeMethod
    // tos: return address of MEGAMORPHIC send in compiled code (ic)
    masm->bind( call_method );
    masm->test( ecx, MEMOOP_TAG );            // check if methodOop
    masm->jcc( Assembler::Condition::notZero, is_methodOop );    // otherwise
    masm->jmp( ecx );                // call NativeMethod

    // call methodOop - setup registers
    masm->bind( is_methodOop );
    masm->xorl( ebx, ebx );                // clear ebx for interpreter
    masm->movl( edx, Address( std::int32_t( &method_entry_point ), RelocationInformation::RelocationType::external_word_type ) );
    // (Note: cannot use value in method_entry_point directly since interpreter is generated afterwards)
    //
    // eax: receiver
    // ebx: 00000000
    // ecx: methodOop
    // edx: entry point
    // tos: return address of MEGAMORPHIC send in compiled code (ic)
    masm->jmp( edx );                // call method_entry

    // probe secondary cache
    //
    // eax: receiver
    // ebx: MIC cache pointer
    // ecx: receiver klass
    // edx: selector
    // edi: primary cache index
    // tos: return address of MEGAMORPHIC send in compiled code (ic)
    masm->bind( probe_secondary_cache );        // compute hash value
    masm->andl( edi, ( secondary_cache_size - 1 ) << 4 );
    // probe cache
    masm->cmpl( ecx, Address( edi, LookupCache::secondary_cache_address() + 0 * OOP_SIZE ) );
    masm->jcc( Assembler::Condition::notEqual, do_lookup );
    masm->cmpl( edx, Address( edi, LookupCache::secondary_cache_address() + 1 * OOP_SIZE ) );
    masm->jcc( Assembler::Condition::notEqual, do_lookup );
    masm->movl( ecx, Address( edi, LookupCache::secondary_cache_address() + 2 * OOP_SIZE ) );
    masm->jmp( call_method );

    // do lookup
    //
    // eax: receiver
    // ebx: MIC cache pointer
    // ecx: receiver klass
    // edx: selector
    // edi: secondary cache index
    // tos: return address of MEGAMORPHIC send in compiled code (ic)
    masm->bind( do_lookup );
    masm->set_last_delta_frame_after_call();
    masm->pushl( eax );                // save receiver
    masm->pushl( edx );                // pass 2nd argument: selector
    masm->pushl( ecx );                // pass 1st argument: receiver klass
    masm->call( (const char *) LookupCache::normal_lookup, RelocationInformation::RelocationType::runtime_call_type );
    masm->movl( ecx, eax );                // ecx: method
    masm->popl( ebx );                // pop 1st argument
    masm->popl( ebx );                // pop 2nd argument
    masm->popl( eax );                // restore receiver
    masm->reset_last_delta_frame();
    masm->testl( ecx, ecx );            // test if method has been found in lookup cache
    masm->jcc( Assembler::Condition::notZero, call_method );

    // method not found in the lookup cache - full lookup needed (message not understood may happen)
    // eax: receiver
    // ebx: points to MIC cache
    // tos: return address of MEGAMORPHIC send in compiled code
    //
    // Note: This should not happen right now, since normal_lookup always returns a value
    //       if the method exists (and 'message not understood' is not yet supported in
    //       compiled code). However, this should change at some point, and normal_lookup_cache_probe
    //       should be used instead of normal_lookup.
    masm->hlt();

    return entry_point;
}


const char *StubRoutines::generate_compile_block( MacroAssembler *masm ) {
// Stub routine that is called from a jumptable entry for a block closure.
// The recipe:
//   - compile the toplevelblock NativeMethod
//   - patch the jump entry with the entry point of the compiler NativeMethod
//   - jump to the new NativeMethod.
//
// Note that compile_new_block doesn't do allocation,
// therefore the receiver can be saved on the C stack.

    // eax: receiver
    const char *entry_point = masm->pc();
    masm->set_last_delta_frame_after_call();
    masm->pushl( eax );                // save receiver
    masm->pushl( eax );                // pass receiver
    masm->call( (const char *) JumpTable::compile_new_block, RelocationInformation::RelocationType::runtime_call_type );    // eax = block_closure_compile(receiver)
    masm->movl( ebx, eax );                // ebx = block code
    masm->popl( eax );                // get rid of receiver argument
    masm->popl( eax );                // restore receiver (don't use argument, might be overwritten)
    masm->reset_last_delta_frame();
    masm->jmp( ebx );                // jump to target
    return entry_point;
}


const char *StubRoutines::generate_continue_NonLocalReturn( MacroAssembler *masm ) {
// Entry point jumped to from compiled code. Initiates (or continues an ongoing) NonLocalReturn.
// Originally this code has been generated in nativeMethods, using a stub reduces code size.

    Register ret_addr = temp1;
    Register offset   = temp2;

    const char *entry_point = masm->pc();
    masm->leave();                        // remove stack frame
    masm->popl( ret_addr );                        // get (local) return address
    masm->movl( offset, Address( ret_addr, InlineCacheInfo::info_offset ) );    // get ic info
    if ( InlineCacheInfo::number_of_flags > 0 ) {
        masm->sarl( offset, InlineCacheInfo::number_of_flags );        // shift ic info flags out
    }
    masm->addl( ret_addr, offset );                    // compute non-local return address
    masm->jmp( ret_addr );                        // do NonLocalReturn
    return entry_point;
}


const char *StubRoutines::generate_call_DLL( MacroAssembler *masm, bool async ) {

    // The following routine provides the extra frame for DLL calls.
    // Note: 1. Its code has to be *outside* the interpreters code! (see also: DLL calls in interpreter)
    //       2. This routine is also used by the compiler! Make sure to adjust the parameter
    //          passing in the compiler as well (x86_node.cpp, codeGenerator.cpp), when changing this code!
    //
    // Stack layout immediately after calling the DLL:
    // (The DLL state word is used for asynchronous DLL/interrupted DLL calls)
    //
    //	  ...					DLL land
    // esp->[ return addr	] ------------------------------------------------------------------------
    //	[ unboxed arg 1	]			C land
    //	  ...
    // 	[ unboxed arg n	]
    // 	[ ptr to itself	] <----	ptr to itself	to check that the right no. of arguments is used
    // 	[ DLL state	]			used for DLL/interrupted DLL calls
    // 	[ return addr	] ------------------------------------------------------------------------
    // 	[ argument n	] <----	last_delta_sp	Delta land
    // 	[ argument n-1	]
    // 	  ...
    // 	[ argument 1	]
    //	[ return proxy	]
    //	  ...
    // ebp->[ previous ebp	] <----	last_delta_fp
    //
    // The routine expects 3 arguments to be passed in registers as follows:
    //
    // ebx: number of arguments
    // ecx: address of last argument
    // edx: DLL function entry point

    Label loop_entry, no_arguments, smi_argument, next_argument, wrong_call, align_stack, convert_args;

    const char *entry_point = masm->pc();
    masm->set_last_delta_frame_after_call();
    masm->pushl( 0 );                       // initial value for DLL state
    masm->movl( esi, esp );                 // save DLL state address
    masm->pushl( esp );                     // to check that the right no. of arguments is used
    if ( TraceDLLCalls ) {                  // call trace routine (C to C call, no special setup required)
        masm->pushl( esi );                 // save DLL state address
        masm->pushl( ebx );                 // pass arguments in reverse order
        masm->pushl( ecx );
        masm->pushl( edx );
        masm->call( (const char *) trace_DLL_call_1, RelocationInformation::RelocationType::runtime_call_type );
        masm->popl( edx );                  // restore registers
        masm->popl( ecx );
        masm->popl( ebx );
        masm->popl( esi );                  // restore DLL state address
    }

    //slr mod: push a fake stack frame to support cdecl calls
//    masm->enter();
    //slr mod end

//    // following is to allow 16-byte stack alignment for Darwin (OSX)
//    masm->movl( eax, ebx );
//    masm->negl( eax );
//    masm->leal( eax, Address( esp, eax, Address::ScaleFactor::times_4 ) ); // esp - 4 x nargs
//    masm->andl( eax, 0xf ); // padding required for 16-byte alignment
//    masm->bind( align_stack );
//    masm->cmpl( eax, 0 );
//    masm->jcc( MacroAssembler::Condition::lessEqual, convert_args );
//    masm->pushl( 0 );
//    masm->subl( eax, 4 ); // align stack
//    masm->jmp( align_stack );
//    masm->bind( convert_args );
//    // end stack alignment mod

    masm->testl( ebx, ebx );                            // if number of arguments not_eq 0 then
    masm->jcc( MacroAssembler::Condition::notZero, loop_entry );   // convert arguments

    // done with all the arguments
    masm->bind( no_arguments );
    if ( async ) {
        masm->pushl( edx );
        masm->pushl( esi );                             // pass DLL state address
        masm->call( (const char *) DLLs::enter_async_call, RelocationInformation::RelocationType::runtime_call_type );
        masm->popl( esi );                              // discard argument
        masm->popl( edx );                              // restore registers
    }

    // do DLL call
    masm->call( edx );                // eax := dll call (pops arguments itself)

    //slr mod: pop the fake stack frame for cdecl
//    masm->leave();
    //slr mod end

    // check no. of arguments
    masm->popl( ebx );                // must be the same as esp after popping
    masm->cmpl( ebx, esp );
    masm->jcc( Assembler::Condition::notEqual, wrong_call );

    // top of stack contains DLL state
    masm->movl( ebx, esp );             // get DLL state address
    masm->pushl( eax );                 // save result
    masm->pushl( ebx );                 // pass DLL state address
    const char *exit_dll = (const char *) ( async ? DLLs::exit_async_call : DLLs::exit_sync_call );
    masm->call( exit_dll, RelocationInformation::RelocationType::runtime_call_type );
    masm->popl( ebx );                  // discard argument
    masm->popl( eax );                  // restore result

    if ( TraceDLLCalls ) {              // call trace routine (C to C call, no special setup required)
        masm->pushl( eax );             // pass result
        masm->call( (const char *) trace_DLL_call_2, RelocationInformation::RelocationType::runtime_call_type );
        masm->popl( eax );              // restore result
    }
    masm->popl( ebx );                  // discard DLL state word
    masm->reset_last_delta_frame();
    masm->ret( 0 );

    // wrong DLL has been called (no. of popped arguments is incorrect)
    masm->bind( wrong_call );
    masm->call( (const char *) wrong_DLL_call, RelocationInformation::RelocationType::runtime_call_type );
    masm->hlt();                                        // should never reach here

    // small_int_t argument -> convert it to std::int32_t
    masm->bind( smi_argument );
    masm->sarl( eax, TAG_SIZE );                        // convert small_int_t into C std::int32_t

    // next argument
    masm->bind( next_argument );
    masm->pushl( eax );                                 // push converted argument
    masm->addl( ecx, OOP_SIZE );                         // go to previous argument
    masm->decl( ebx );                                  // decrement argument counter
    masm->jcc( MacroAssembler::Condition::zero, no_arguments );    // continue until no arguments

    // loop
    masm->bind( loop_entry );

    // ebx: argument count
    // ecx: current argument address
    masm->movl( eax, Address( ecx ) );                  // get argument
    masm->testb( eax, MEMOOP_TAG );                        // check if small_int_t or proxy
    masm->jcc( MacroAssembler::Condition::zero, smi_argument );

    // boxed argument -> unbox it
    masm->movl( eax, Address( eax, pointer_offset ) );  // unbox proxy
    masm->jmp( next_argument );

    return entry_point;
}


const char *StubRoutines::generate_lookup_DLL( MacroAssembler *masm, bool async ) {
    // Lookup routine called from "empty" DLL caches in compiled code only.
    // Calls a lookup & patch routine which updates the DLL cache and then
    // continues with call_DLL.

    // ebx: number of arguments
    // ecx: address of last argument
    // edx: some initial value for DLL function entry point
    st_assert( call_DLL_entry( async ) not_eq nullptr, "call_DLL_entry must have been generated before" );
    const char *entry_point = masm->pc();
    masm->set_last_delta_frame_after_call();
    masm->pushl( ebx );                // save registers (edx has no valid value yet)
    masm->pushl( ecx );
    masm->call( (const char *) DLLs::lookup_and_patch_Compiled_DLLCache, RelocationInformation::RelocationType::runtime_call_type );    // eax := function entry point
    masm->popl( ecx );                // restore registers
    masm->popl( ebx );
    masm->movl( edx, eax );                // setup edx
    masm->reset_last_delta_frame();
    masm->jmp( call_DLL_entry( async ), RelocationInformation::RelocationType::runtime_call_type );    // now jump to real target
    return entry_point;
}


const char *StubRoutines::generate_recompile_stub( MacroAssembler *masm ) {
// Recompilation; called by NativeMethod prologue of recompilation
// trigger. Sets up stack frame and passes on receiver + caller
// to C. Using the stub reduces the code size of NativeMethod prologues.

    // eax: receiver
    const char *entry_point = masm->pc();
    //masm->int3();
    masm->set_last_delta_frame_after_call();
//  masm->call((char*)SavedRegisters::save_registers, RelocationInformation::RelocationType::runtime_call_type);
    SavedRegisters::generate_save_registers( masm );
    masm->movl( ebx, Address( esp ) );        // get return address (trigger NativeMethod)
    masm->pushl( eax );                // save receiver
    masm->pushl( ebx );                // pass 2nd argument (pc)
    masm->pushl( eax );                // pass 1st argument (recv)
    masm->call( (const char *) Recompilation::nativeMethod_invocation_counter_overflow, RelocationInformation::RelocationType::runtime_call_type );    // eax = nativeMethod_invocation_counter_overflow(receiver, pc)
    masm->movl( ecx, eax );                // save continuation address in ecx
    masm->popl( eax );                // pop 1st argument
    masm->popl( ebx );                // pop 2nd argument
    masm->popl( eax );                // restore receiver
    masm->reset_last_delta_frame();
    masm->leave();                // remove trigger NativeMethod's stack frame
    masm->jmp( ecx );                // continue
    return entry_point;
}


/* Old code - keep around for a while - gri 9/18/96

extern "C" char* recompileNativeMethod(Oop receiver, char* retpc);

char* StubRoutines::generate_recompile_stub(MacroAssembler* masm) {
// Recompilation; called by NativeMethod prologue of recompilation
// trigger. Sets up stack frame and passes on receiver + caller
// to C. Using the stub reduces the code size of NativeMethod prologues.

  char* entry_point = masm->pc();
  masm->movl(ebx, Address(esp));		// get return address (trigger NativeMethod)
  masm->enter();				// create normal stack frame with link (for stub)
  masm->pushl(eax);				// save receiver
  masm->call_C((char*)recompileNativeMethod, eax, ebx);	// eax = recompileNativeMethod(receiver, pc)
  masm->movl(ecx, eax);				// save continuation address in ecx
  masm->popl(eax);				// restore receiver
  masm->leave();				// remove this stack frame
  masm->leave();				// remove trigger's stack frame
  masm->jmp(ecx);				// continue
  return entry_point;
}
*/


const char *StubRoutines::generate_uncommon_trap( MacroAssembler *masm ) {
    const char *entry_point = masm->pc();
    masm->set_last_delta_frame_after_call();
//  masm->call((char*)SavedRegisters::save_registers, RelocationInformation::RelocationType::runtime_call_type);
    SavedRegisters::generate_save_registers( masm );
    masm->call( (const char *) uncommon_trap, RelocationInformation::RelocationType::runtime_call_type );
    masm->reset_last_delta_frame();
    masm->ret( 0 );
    return entry_point;
}


const char *StubRoutines::generate_verify_context_chain( MacroAssembler *masm ) {
// Verify the context chain for a block NativeMethod, if there is
// an unoptimized context in the chain the block must be deoptimized
// and reevaluated.
//
// Called as first instruction for a block NativeMethod if
// the method is expecting a contextOop
//
// Stack at entry:
//   [NativeMethod caller return address]  @ esp - 4
//   [callee NativeMethod return address]  @ esp
//
// Registers at entry:
//   ebp points to NativeMethod callers frame
//   eax contains the block closure.

    Label deoptimize;

    const char *entry_point = masm->pc();
    masm->set_last_delta_frame_after_call();
    masm->pushl( self_reg ); // save self (argument can get corrupted in called function)
    masm->pushl( self_reg ); // pass argument (C calling convention)
    masm->call( (const char *) validateContextChain, RelocationInformation::RelocationType::runtime_call_type );
    masm->movl( ebx, eax );
    masm->popl( self_reg );
    masm->popl( self_reg );
    masm->reset_last_delta_frame();
    masm->testl( ebx, ebx );
    masm->jcc( MacroAssembler::Condition::zero, deoptimize );
    masm->ret( 0 );

    masm->bind( deoptimize );
    masm->addl( esp, 4 );
    masm->jmp( (const char *) Interpreter::restart_primitiveValue(), RelocationInformation::RelocationType::runtime_call_type );
    return entry_point;
}


static BlockClosureOop deoptimize_block( BlockClosureOop block ) {
    VerifyNoAllocation vna;
    st_assert( block->is_block(), "must be a block" );
    st_assert( block->isCompiledBlock(), "we should be in a compiled block" );

    // just checking if the context chain really contains an unoptimized context
    bool has_unoptimized_context = false;
    {
        ContextOop con = block->lexical_scope();
        // verify entire context chain
        st_assert( con->is_context(), "expecting a context" );
        while ( true ) {
            if ( con->unoptimized_context() not_eq nullptr ) {
                has_unoptimized_context = true;
                break;
            }
            if ( not con->has_outer_context() )
                break;
            con = con->outer_context();
        }
    }
    st_assert( has_unoptimized_context, "should have an unoptimized context" );

    block->deoptimize();
    return block;
}


const char *StubRoutines::generate_deoptimize_block( MacroAssembler *masm ) {
// Called if there's an unoptimized context in the incoming context
// chain of a block NativeMethod. The block must be deoptimized and the
// value send has to be restarted.
//
// Called from block nativeMethods with incoming contexts, immediately
// after checking the context chain and before any stack frame
// has been set up.

    // eax: block closure
    // tos: callee NativeMethod return address (returning to caller)
    const char *entry_point = masm->pc();
    masm->set_last_delta_frame_after_call();    // NativeMethod is treated as C routine
    masm->pushl( self_reg );            // pass argument
    masm->call( (const char *) deoptimize_block, RelocationInformation::RelocationType::runtime_call_type );    // eax := deoptimize_block(self_reg)
    masm->popl( ebx );                // get rid of argument
    masm->reset_last_delta_frame();        // return & restart the primitive (eax must contain the block)
    masm->jmp( (const char *) Interpreter::restart_primitiveValue(), RelocationInformation::RelocationType::runtime_call_type );
    return entry_point;
}


const char *StubRoutines::generate_call_inspector( MacroAssembler *masm ) {
// Called for each MacroAssembler::inspect(...) - used for debugging only
    const char *entry_point = masm->pc();
    masm->set_last_delta_frame_after_call();    // just in case somebody wants to look at the stack
    masm->pushad();
    masm->call( (const char *) MacroAssembler::inspector, RelocationInformation::RelocationType::runtime_call_type );
    masm->popad();
    masm->reset_last_delta_frame();
    masm->ret( 0 );
    return entry_point;
}


const char *StubRoutines::generate_call_delta( MacroAssembler *masm ) {

    // This is the general Delta entry point. All code that is calling the interpreter or
    // compiled code is entering via this entry point. In case of an NonLocalReturn leaving Delta code,
    // the global NonLocalReturn variables are set. Note: Needs to be *outside* the interpreters code.
    //
    // method must be a methodOop or a non-zombie NativeMethod (zombie nativeMethods must be filtered
    // out before - calling call_delta with a zombie NativeMethod crashes the system).
    //
    // Note: Shouldn't we preserve some C registers before entering Delta and restore
    // them when coming back? At least that's what C calling convention would
    // require. Might not be a problem because the calling C funtions don't do
    // anything anymore calling call_delta. CHECK THIS! (gri 6/10/96)
    //
    // indeed we have to preserve EDI & ESI or else the debug mode stack
    // check assertions will fail (cmp esp, esi)				-Marc 04/07

    const char *nlr_return_from_delta_entry = generate_nlr_return_from_Delta( masm );

    Label _loop, _no_args, _is_compiled, _return, _nlr_test, _nlr_setup, _stack_ok;

    // extern "C" Oop call_delta(void* method, Oop receiver, std::int32_t nofArgs, Oop* args)
    // incoming arguments
    Address method   = Address( ebp, +2 * OOP_SIZE );
    Address receiver = Address( ebp, +3 * OOP_SIZE );
    Address nofArgs  = Address( ebp, +4 * OOP_SIZE );
    Address args     = Address( ebp, +5 * OOP_SIZE );

    const char *entry_point = masm->pc();

    // setup stack frame
    masm->enter();

    // last_delta_fp & last_delta_sp must be the first two words in
    // the stack frame; i.e. at ebp - 4 and ebp - 8. See also frame.hpp.
    masm->pushl( Address( (std::int32_t) &last_delta_fp, RelocationInformation::RelocationType::external_word_type ) );
    masm->pushl( Address( (std::int32_t) &last_delta_sp, RelocationInformation::RelocationType::external_word_type ) );

    masm->pushl( edi );    // save registers for C calling convetion
    masm->pushl( esi );
    masm->movl( edi, Address( esp, 12 ) );

    // reset last Delta frame
    masm->reset_last_delta_frame();
    // test for stack corruption
    masm->testl( edi, edi );
    masm->jcc( Assembler::Condition::equal, _stack_ok );
    masm->cmpl( esp, edi );
    masm->jcc( Assembler::Condition::less, _stack_ok );
    // break because of stack corruption
    //masm->int3();

    masm->bind( _stack_ok );
    // setup calling stack frame with arguments
    masm->movl( ebx, nofArgs );    // get no. of arguments
    masm->movl( ecx, args );    // pointer to first argument
    masm->testl( ebx, ebx );
    masm->jcc( Assembler::Condition::zero, _no_args );

    masm->bind( _loop );
    masm->movl( edx, Address( ecx ) );    // get argument
    masm->addl( ecx, OOP_SIZE );        // advance to next argument
    masm->pushl( edx );            // push argument on stack
    masm->decl( ebx );            // decrement argument counter
    masm->jcc( Assembler::Condition::notZero, _loop );    // until no arguments

    // call Delta method
    masm->bind( _no_args );
    masm->movl( eax, receiver );
    masm->xorl( ebx, ebx );            // _restore_ebx
    masm->movl( edx, method );
    masm->test( edx, MEMOOP_TAG );
    masm->jcc( Assembler::Condition::zero, _is_compiled );
    masm->movl( ecx, edx );
    masm->movl( edx, Address( (std::int32_t) &method_entry_point, RelocationInformation::RelocationType::external_word_type ) );

    // eax: receiver
    // ebx: 0
    // ecx: methodOop (if not compiled)
    // edx: calling address
    // Note: no zombie nativeMethods possible -> no 2nd ic_info word required
    masm->bind( _is_compiled );
    masm->call( edx );
    _return_from_Delta = masm->pc();
    masm->ic_info( _nlr_test, 0 );
    masm->movl( Address( (std::int32_t) &have_nlr_through_C, RelocationInformation::RelocationType::external_word_type ), 0 );

    masm->bind( _return );
    masm->leal( esp, Address( ebp, -4 * OOP_SIZE ) );
    masm->popl( esi );    // restore registers for C calling convetion
    masm->popl( edi );
    masm->popl( Address( (std::int32_t) &last_delta_sp, RelocationInformation::RelocationType::external_word_type ) ); // reset _last_delta_sp
    masm->popl( Address( (std::int32_t) &last_delta_fp, RelocationInformation::RelocationType::external_word_type ) ); // reset _last_delta_fp
    masm->popl( ebp );
    masm->ret( 0 );    // remove stack frame & return

    // When returning from Delta to C via a NonLocalReturn, the following code
    // sets up the global NonLocalReturn variables and patches the return address
    // of the first C frame in the last_C_chunk of the stack (see below).
    masm->bind( _nlr_test );
    masm->movl( ecx, Address( ebp, -2 * OOP_SIZE ) );    // get pushed value of _last_delta_sp
    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::zero, _nlr_setup );

    masm->movl( edx, Address( ecx, -OOP_SIZE ) ); // get return address of the first C function called
    // store return address for nlr_return_from_Delta
    masm->movl( Address( (std::int32_t) &C_frame_return_addr, RelocationInformation::RelocationType::external_word_type ), edx );
//  masm->hlt();

//  char* nlr_return_from_delta_addr = StubRoutines::nlr_return_from_Delta();
//  assert(nlr_return_from_delta_addr, "nlr_return_from_Delta not initialized yet");
//  masm->movl(Address(ecx, -OOP_SIZE), (std::int32_t)nlr_return_from_delta_addr);  // patch return address
    masm->movl( Address( ecx, -OOP_SIZE ), (std::int32_t) nlr_return_from_delta_entry );  // patch return address
    masm->pushl( eax );
    masm->pushl( ebx );
    masm->pushl( edx );
    masm->pushl( edi );
    masm->pushl( esi );
    masm->pushl( ecx );
    masm->call( (const char *) &popStackHandles, RelocationInformation::RelocationType::external_word_type );
    masm->popl( ecx );
    masm->popl( esi );
    masm->popl( edi );
    masm->popl( edx );
    masm->popl( ebx );
    masm->popl( eax );

    masm->bind( _nlr_setup );
    // setup global NonLocalReturn variables
    masm->movl( Address( (std::int32_t) &have_nlr_through_C, RelocationInformation::RelocationType::external_word_type ), 1 );
    masm->movl( Address( (std::int32_t) &nlr_result, RelocationInformation::RelocationType::external_word_type ), eax );
    masm->movl( Address( (std::int32_t) &nlr_home, RelocationInformation::RelocationType::external_word_type ), edi );
    masm->movl( Address( (std::int32_t) &nlr_home_id, RelocationInformation::RelocationType::external_word_type ), esi );
    masm->jmp( _return );

    return entry_point;
}


const char *StubRoutines::generate_nlr_return_from_Delta( MacroAssembler *masm ) {

    // When returning from C to Delta via a NonLocalReturn, the following code continues an ongoing NonLocalReturn.
    // In case of a NonLocalReturn, the return address of the first C frame in the last_C_chunk of the stack is patched such
    // that C will return to _nlr_setup, which in turn returns to the NonLocalReturn testpoint of the primitive that called C.

    const char *entry_point = masm->pc();

    masm->reset_last_delta_frame();
    masm->movl( eax, Address( (std::int32_t) &nlr_result, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( edi, Address( (std::int32_t) &nlr_home, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( esi, Address( (std::int32_t) &nlr_home_id, RelocationInformation::RelocationType::external_word_type ) );

    // get return address
    masm->movl( ebx, Address( (std::int32_t) &C_frame_return_addr, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( ecx, Address( ebx, InlineCacheInfo::info_offset ) );        // get nlr_offset
    masm->sarl( ecx, InlineCacheInfo::number_of_flags );            // shift ic info flags out
    masm->addl( ebx, ecx );                            // compute NonLocalReturn test point address
    masm->jmp( ebx );                            // return to nlr test point

    return entry_point;
}

//-----------------------------------------------------------------------------------------
// single_step_stub

extern "C" std::int32_t *frame_breakpoint;   // dispatch table
extern "C" doFn         original_table[static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)];
extern "C" void single_step_handler();

void (*StubRoutines::single_step_fn)() = nullptr;
//extern "C" void nlr_single_step_continuation();

//extern "C" char* single_step_handler;
//extern "C" char* nlr_single_step_continuation;

const char *StubRoutines::generate_single_step_stub( MacroAssembler *masm ) {
// slr mod:
//   first the single step continuation
//		- used as the return address of the single step code below
    _single_step_continuation = masm->pc();
    // inline cache for non local return
    std::int32_t offset  = Interpreter::nlr_single_step_continuation_entry() - _single_step_continuation;
    std::int32_t ic_info = ( offset << 8 ) + 0;
    masm->testl( eax, ic_info );
    //masm->ic_info(Interpreter::nlr_single_step_continuation(), 0);

    masm->reset_last_delta_frame();
    masm->popl( eax );
    // restore bytecode pointer
    masm->movl( esi, Address( ebp, -2 * OOP_SIZE ) );
    // reload bytecode
    masm->xorl( ebx, ebx );
    masm->movb( ebx, Address( esi ) );
    // execute bytecode
    masm->jmp( Address( noreg, ebx, Address::ScaleFactor::times_4, (std::int32_t) original_table ) );

//   then the calling stub
// end slr mod
//   parameters:
//     ebx = byte code
//     esi = byte code pointer
//

    Label is_break;

    const char *entry_point = masm->pc();

//  masm->int3();
    masm->cmpl( ebp, Address( (std::int32_t) &frame_breakpoint, RelocationInformation::RelocationType::external_word_type ) );
    masm->jcc( Assembler::Condition::greaterEqual, is_break );
    masm->jmp( Address( noreg, ebx, Address::ScaleFactor::times_4, (std::int32_t) original_table ) );

    masm->bind( is_break );
    masm->movl( Address( ebp, -2 * OOP_SIZE ), esi );    // save esi
    masm->pushl( eax );                // save tos
    masm->set_last_delta_frame_before_call();

//  masm->pushl(Address((std::int32_t)&nlr_single_step_continuation, RelocationInformation::RelocationType::external_word_type));
//  assert(nlr_single_step_continuation, "%fix this");
    // %hack: indirect load
//  masm->int3();
//  masm->hlt();
//  slr mod: masm->movl(edx, (std::int32_t)Interpreter::nlr_single_step_continuation());
//  slr mod: masm->pushl(Address(edx));
    masm->pushl( (std::int32_t) _single_step_continuation );
//  end slr mod

//  assert(single_step_handler, "%fix this");
    masm->jmp( Address( noreg, noreg, Address::ScaleFactor::no_scale, (std::int32_t) &single_step_fn ) );
//  masm->jmp((char*)single_step_handler, RelocationInformation::RelocationType::runtime_call_type);
//  masm->jmp(single_step_handler, RelocationInformation::RelocationType::runtime_call_type);
    //	Should not reach here
    masm->hlt();

    return entry_point;
}
//-----------------------------------------------------------------------------------------
// unpack_unoptimized_frames

extern "C" Oop *setup_deoptimization_and_return_new_sp( Oop *old_sp, std::int32_t *old_fp, ObjectArrayOop frame_array, std::int32_t *current_frame );
extern "C" void unpack_frame_array();
extern "C" bool         nlr_through_unpacking;
extern "C" Oop          result_through_unpacking;


const char *StubRoutines::generate_unpack_unoptimized_frames( MacroAssembler *masm ) {
// Invoked when returning to an unoptimized frame.
// Deoptimizes the frame_array into a stack stretch of interpreter frames
//
// _unpack_unoptimized_frames must look like a compiled inline cache
// so NonLocalReturn works across unoptimized frames.
// Since a NonLocalReturn may have its home inside the optimized frames we have to deoptimize
// and then continue the NonLocalReturn.

    Address real_sender_sp = Address( ebp, -2 * OOP_SIZE );
    Address frame_array    = Address( ebp, -1 * OOP_SIZE );
    Address real_fp        = Address( ebp );

    Register nlr_result_reg  = eax; // holds the result of the method
    Register nlr_home_reg    = edi; // the home frame ptr
    Register nlr_home_id_reg = esi; // used to pass esi

    Label wrapper_for_unpack_frame_array, _return;

    masm->bind( wrapper_for_unpack_frame_array );
    masm->enter();
    masm->call( (const char *) unpack_frame_array, RelocationInformation::RelocationType::runtime_call_type );
    // Restore the nlr state
    masm->cmpl( Address( (std::int32_t) &nlr_through_unpacking, RelocationInformation::RelocationType::external_word_type ), 0 );
    masm->jcc( Assembler::Condition::equal, _return );
    masm->movl( Address( (std::int32_t) &nlr_through_unpacking, RelocationInformation::RelocationType::external_word_type ), 0 );
    masm->movl( nlr_result_reg, Address( (std::int32_t) &nlr_result, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( nlr_home_reg, Address( (std::int32_t) &nlr_home, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( nlr_home_id_reg, Address( (std::int32_t) &nlr_home_id, RelocationInformation::RelocationType::external_word_type ) );

    masm->bind( _return );
    masm->popl( ebp );
    masm->ret( 0 );

    Label common_unpack_unoptimized_frames;

    masm->bind( common_unpack_unoptimized_frames );
    masm->pushl( ebp );            // Push the old   frame pointer
    masm->pushl( frame_array );        // Push the array with the packed frames
    masm->pushl( real_fp );        // Push the frame pointer link
    masm->pushl( real_sender_sp );    // Push the stack pointer of the calling activation

    // Compute the new stack pointer
    masm->call( (const char *) setup_deoptimization_and_return_new_sp, RelocationInformation::RelocationType::runtime_call_type );
    masm->movl( esp, eax );            // Set the new stack pointer
    masm->movl( ebp, real_fp );                // Set the frame pointer to the link
    masm->pushl( -1 );                // Push invalid return address
    masm->jmp( wrapper_for_unpack_frame_array );    // Call the unpacking function

    Label nlr_unpack_unoptimized_frames;

    masm->bind( nlr_unpack_unoptimized_frames );
    masm->movl( Address( (std::int32_t) &nlr_through_unpacking, RelocationInformation::RelocationType::external_word_type ), 1 );
    masm->movl( Address( (std::int32_t) &nlr_result, RelocationInformation::RelocationType::external_word_type ), nlr_result_reg );
    masm->movl( Address( (std::int32_t) &nlr_home, RelocationInformation::RelocationType::external_word_type ), nlr_home_reg );
    masm->movl( Address( (std::int32_t) &nlr_home_id, RelocationInformation::RelocationType::external_word_type ), nlr_home_id_reg );
    masm->jmp( common_unpack_unoptimized_frames );

    const char *entry_point = masm->pc();
    masm->ic_info( nlr_unpack_unoptimized_frames, 0 );
    masm->movl( Address( (std::int32_t) &nlr_through_unpacking, RelocationInformation::RelocationType::external_word_type ), 0 );
    masm->movl( Address( (std::int32_t) &result_through_unpacking, RelocationInformation::RelocationType::external_word_type ), eax );
    masm->jmp( common_unpack_unoptimized_frames );

    return entry_point;
}


const char *StubRoutines::generate_provoke_nlr_at( MacroAssembler *masm ) {
    // extern "C" void provoke_nlr_at(std::int32_t* frame_pointer, Oop* stack_pointer);
    Address old_ret_addr  = Address( esp, -1 * OOP_SIZE );
    Address frame_pointer = Address( esp, +1 * OOP_SIZE );
    Address stack_pointer = Address( esp, +2 * OOP_SIZE );

    Register nlr_result_reg  = eax; // holds the result of the method
    Register nlr_home_reg    = edi; // the home frame ptr
    Register nlr_home_id_reg = esi; // used to pass esi

    const char *entry_point = masm->pc();

    masm->movl( ebp, frame_pointer );            // set new frame pointer
    masm->movl( esp, stack_pointer );            // set new stack pointer
    masm->movl( ebx, old_ret_addr );            // find old return address

    masm->movl( nlr_result_reg, Address( (std::int32_t) &nlr_result, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( nlr_home_reg, Address( (std::int32_t) &nlr_home, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( nlr_home_id_reg, Address( (std::int32_t) &nlr_home_id, RelocationInformation::RelocationType::external_word_type ) );

    masm->movl( ecx, Address( ebx, InlineCacheInfo::info_offset ) );    // get nlr_offset
    masm->sarl( ecx, InlineCacheInfo::number_of_flags );            // shift ic info flags out
    masm->addl( ebx, ecx );                        // compute NonLocalReturn test point address
    masm->jmp( ebx );                            // return to nlr test point

    return entry_point;
}


const char *StubRoutines::generate_continue_nlr_in_delta( MacroAssembler *masm ) {
    // extern "C" void continue_nlr_in_delta(std::int32_t* frame_pointer, Oop* stack_pointer);
    Address old_ret_addr  = Address( esp, -1 * OOP_SIZE );
    Address frame_pointer = Address( esp, +1 * OOP_SIZE );
    Address stack_pointer = Address( esp, +2 * OOP_SIZE );

    Register nlr_result_reg  = eax; // holds the result of the method
    Register nlr_home_reg    = edi; // the home frame ptr
    Register nlr_home_id_reg = esi; // used to pass esi

    const char *entry_point = masm->pc();

    masm->movl( ebp, frame_pointer );            // set new frame pointer
    masm->movl( esp, stack_pointer );            // set new stack pointer
    masm->movl( ebx, old_ret_addr );            // find old return address

    masm->movl( nlr_result_reg, Address( (std::int32_t) &nlr_result, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( nlr_home_reg, Address( (std::int32_t) &nlr_home, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( nlr_home_id_reg, Address( (std::int32_t) &nlr_home_id, RelocationInformation::RelocationType::external_word_type ) );

    masm->jmp( ebx );                        // continue

    return entry_point;
}

//-------------------------------------------------------------------------------
// Stub routines for callbacks (see callBack.cpp)
extern "C" volatile void *handleCallBack( std::int32_t index, std::int32_t params );


const char *StubRoutines::generate_handle_pascal_callback_stub( MacroAssembler *masm ) {
    // Stub routines called from a "Pascal" callBack chunk (see callBack.cpp)
    // Incomming arguments:
    //  ecx, ecx = index           (passed on to Delta)
    //  edx, edx = number of bytes (to be deallocated before returning)

    const char *entry_point = masm->pc();

    // create link
    masm->enter();

    // save registers for Pascal calling convention
    masm->pushl( ebx );
    masm->pushl( edi );
    masm->pushl( esi );

    // save number of bytes in parameter list
    masm->pushl( edx );

    // compute parameter start
    masm->movl( edx, esp );
    masm->addl( edx, 24 );        // (esi, edi, ebx, edx, fp, return address)

    // eax = handleCallBack(index, &params)
    masm->pushl( edx );        // &params
    masm->pushl( ecx );        // index
    masm->call( (const char *) handleCallBack, RelocationInformation::RelocationType::runtime_call_type );
    masm->addl( esp, 2 * OOP_SIZE );    // pop the arguments

    // restore number of bytes in parameter list
    masm->popl( edx );

    // restore registers for Pascal calling convention
    masm->popl( esi );
    masm->popl( edi );
    masm->popl( ebx );

    // destroy link
    masm->leave();

    // get return address
    masm->popl( ecx );

    // deallocate the callers parameters
    masm->addl( esp, edx );

    // jump to caller
    masm->jmp( ecx );

    return entry_point;
}


const char *StubRoutines::generate_handle_C_callback_stub( MacroAssembler *masm ) {
    // Stub routines called from a "C" callBack chunk (see callBack.cpp)
    // Incomming arguments:
    // eax = index               (passed on to Delta)

    Label      stackOK;
    const char *entry_point = masm->pc();

    // create link
    masm->enter();

    // save registers for C calling convention
    masm->pushl( ebx );
    masm->pushl( edi );
    masm->pushl( esi );

    // compute parameter start
    masm->movl( edx, esp );
    masm->addl( edx, 20 ); //  (esi, edi, ebx, fp, return address)

    // eax = handleCallBack(index, &params)
    masm->pushl( esp );
    masm->pushl( edx ); // &params
    masm->pushl( ecx ); // index
    masm->call( (const char *) handleCallBack, RelocationInformation::RelocationType::runtime_call_type );
    masm->addl( esp, 2 * OOP_SIZE );    // pop the arguments
    masm->popl( esi );
    masm->cmpl( esi, esp );
    masm->jcc( Assembler::Condition::equal, stackOK );
    masm->int3();
    masm->bind( stackOK );

    // restore registers for Pascal calling convention
    masm->popl( esi );
    masm->popl( edi );
    masm->popl( ebx );

    // destroy link
    masm->leave();

    // jump to caller
    masm->ret( 0 );

    return entry_point;
}


/*
static Oop oopify_float() {
  double x;
  __asm fstp x							// get top of FPU stack
    BlockScavenge bs;						// because all registers are saved on the stack
  return OopFactory::new_double(x);				// box the FloatValue
}
*/

const char *StubRoutines::generate_oopify_float( MacroAssembler *masm ) {
    const char *entry_point = masm->pc();

    // masm->int3();
    // masm->hlt();
    // masm->call((char*)::oopify_float, RelocationInformation::RelocationType::runtime_call_type);
    masm->enter();
    masm->subl( esp, 8 );
    masm->fstp_d( Address( esp ) );
    masm->incl( Address( (std::int32_t) BlockScavenge::counter_addr(), RelocationInformation::RelocationType::external_word_type ) );
    masm->call( (const char *) OopFactory::new_double, RelocationInformation::RelocationType::runtime_call_type );
    masm->decl( Address( (std::int32_t) BlockScavenge::counter_addr(), RelocationInformation::RelocationType::external_word_type ) );
    masm->leave();
    masm->ret();

    return entry_point;
}


const char *StubRoutines::generate_PolymorphicInlineCache_stub( MacroAssembler *masm, std::int32_t pic_size ) {
// Called from within a PolymorphicInlineCache (POLYMORPHIC inline cache).
// The stub interprets the methodOop section of compiled PICs.
// The methodOop section layout is as follows:
//
// call <this stub routine>
// cached klass 1	<--- return address (tos)
// cached methodOop 1
// cached klass 2
// cached methodOop2
// ...
//
// cached klass n
// cached methodOop n
//
// Note: Don't use this for POLYMORPHIC super sends!

    Label found, loop;

    // entry found at index
    //
    // eax: receiver
    // ebx: PolymorphicInlineCache table pointer
    // ecx: methodOop
    // edx: receiver klass
    // tos: return address of POLYMORPHIC send in compiled code
    masm->bind( found );
    masm->movl( edx, Address( std::int32_t( &method_entry_point ), RelocationInformation::RelocationType::external_word_type ) );
    // (Note: cannot use value in method_entry_point directly since interpreter is generated afterwards)
    masm->xorl( ebx, ebx );
    // eax: receiver
    // ebx: 000000xx
    // ecx: methodOop
    masm->jmp( edx );

    // eax    : receiver
    // tos    : return address pointing to table in PolymorphicInlineCache
    // tos + 4: return address of POLYMORPHIC send in compiled code
    // tos + 8: last argument/receiver
    const char *entry_point = masm->pc();
    masm->popl( ebx );                // get return address (PolymorphicInlineCache table pointer)
    masm->movl( edx, Address( (std::int32_t) &smiKlassObject, RelocationInformation::RelocationType::external_word_type ) );
    masm->test( eax, MEMOOP_TAG );            // check if small_int_t
    masm->jcc( Assembler::Condition::zero, loop );        // if so, class is already in ecx
    masm->movl( edx, Address( eax, MemOopDescriptor::klass_byte_offset() ) );    // otherwise, load receiver class

    // eax: receiver
    // ebx: PolymorphicInlineCache table pointer
    // edx: receiver klass
    // tos: return address of POLYMORPHIC send in compiled code
    masm->bind( loop );
    for ( std::size_t i = 0; i < pic_size; i++ ) {
        // compare receiver klass with klass in PolymorphicInlineCache table at index
        masm->cmpl( edx, Address( ebx, i * static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_entry_size ) + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_klass_offset ) ) );
        masm->movl( ecx, Address( ebx, i * static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_entry_size ) + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_offset ) ) );
        masm->jcc( Assembler::Condition::equal, found );
    }
    st_assert( ic_normal_lookup_entry() not_eq nullptr, "ic_normal_lookup_entry must be generated before" );
    masm->jmp( ic_normal_lookup_entry(), RelocationInformation::RelocationType::runtime_call_type );

    return entry_point;
}


const char *StubRoutines::generate_allocate( MacroAssembler *masm, std::int32_t size ) {
    static_cast<void>(size); // unused

    const char *entry_point = masm->pc();
    masm->hlt();
    return entry_point;
}


void StubRoutines::alien_arg_size( MacroAssembler *masm, Label &nextArg ) {
    Label isPointer, isDirect, isSMI, isUnsafe;

    masm->movl( ecx, Address( eax ) );                        // load current arg
    masm->testl( ecx, MEMOOP_TAG );
    masm->jcc( Assembler::Condition::equal, isSMI );

    masm->movl( esi, Address( ecx, MemOopDescriptor::klass_byte_offset() ) );// get class
    masm->movl( esi, Address( esi, KlassOopDescriptor::nonIndexableSizeOffset() ) );// get non-indexable size
    // start of the byte array's bytes
    masm->leal( ecx, Address( ecx, esi, Address::ScaleFactor::times_1, -MEMOOP_TAG ) );
    masm->testl( ecx, MEMOOP_TAG );
    masm->jcc( Assembler::Condition::notEqual, isUnsafe );
    masm->addl( ecx, 4 );

    masm->movl( esi, Address( ecx ) );                        // load the size
    masm->testl( esi, esi );                                // direct?
    masm->jcc( Assembler::Condition::equal, isPointer );               // pointer == 0
    masm->jcc( Assembler::Condition::greater, isDirect );              // direct > 0

    masm->addl( edx, esi );                                 // indirect so size negative
    masm->jmp( nextArg );

    masm->bind( isDirect );
    masm->subl( edx, esi );
    masm->jmp( nextArg );

    masm->bind( isUnsafe );
    masm->bind( isPointer );
    masm->bind( isSMI );
    masm->subl( edx, 4 );
}


void StubRoutines::push_alignment_spacers( MacroAssembler *masm ) {
    Label pushArgs;
    masm->andl( edx, 0xf );                                   // push spacers to align stack
    masm->jcc( Assembler::Condition::equal, pushArgs );
    masm->subl( edx, 4 );
    masm->pushl( 0 );
    masm->jcc( Assembler::Condition::equal, pushArgs );
    masm->subl( edx, 4 );
    masm->pushl( 0 );
    masm->jcc( Assembler::Condition::equal, pushArgs );
    masm->subl( edx, 4 );
    masm->pushl( 0 );
    masm->bind( pushArgs );                                   // end of stack alignment spacers
}


void StubRoutines::push_alien_arg( MacroAssembler *masm, Label &nextArg ) {
    Label isSMI, isPointer, isDirect, isUnsafe, startMove, moveLoopTest, moveLoopHead;
    masm->testl( eax, MEMOOP_TAG );
    masm->jcc( Assembler::Condition::equal, isSMI );

    masm->movl( esi, Address( eax, sizeof( MemOopDescriptor ) - MEMOOP_TAG ) );
    // load the first ivar
    //   either SmallIntegerOop for normal alien
    //   or MemOop for unsafe alien
    masm->testl( esi, MEMOOP_TAG );
    masm->jcc( Assembler::Condition::notEqual, isUnsafe );

    masm->movl( esi, Address( eax, MemOopDescriptor::klass_byte_offset() ) );
    masm->movl( esi, Address( esi, KlassOopDescriptor::nonIndexableSizeOffset() ) );
    masm->leal( eax, Address( eax, esi, Address::ScaleFactor::times_1, 4 - MEMOOP_TAG ) ); // start of the oops in the array

    masm->movl( esi, Address( eax ) );                        // load the size
    masm->testl( esi, esi );                                // direct?
    masm->jcc( Assembler::Condition::equal, isPointer );               // pointer == 0
    masm->jcc( Assembler::Condition::greater, isDirect );              // direct > 0
    // indirect < 0
    masm->movl( eax, Address( eax, 4 ) );                     // load the indirect start address
    masm->jmp( startMove );

    masm->bind( isPointer );                                // push the pointer
    masm->movl( eax, Address( eax, 4 ) );
    masm->pushl( eax );
    masm->jmp( nextArg );

    masm->bind( isDirect );
    masm->negl( esi );                                      // negate the size to lower esp
    masm->addl( eax, 4 );                                   // start of direct contents

    masm->bind( startMove );
    masm->pushl( 0 );                                       // pad odd sizes with zero
    masm->leal( esp, Address( esp, esi, Address::ScaleFactor::times_1, 4 ) ); // move stack pointer by size of data
    masm->andl( esp, -4 );                                  // ensure stack 4-byte aligned

    masm->negl( esi );                                      // negate size to use as offset
    masm->jmp( moveLoopTest );
    masm->bind( moveLoopHead );
    masm->movl( ecx, Address( eax, esi, Address::ScaleFactor::times_1 ) );
    masm->movl( Address( esp, esi, Address::ScaleFactor::times_1 ), ecx );

    masm->bind( moveLoopTest );                             // continue?
    masm->subl( esi, 4 );
    masm->cmpl( esi, 0 );
    masm->jcc( Assembler::Condition::greaterEqual, moveLoopHead );     // y - next iteration
    // n - less than four bytes remain
    masm->addl( esi, 3 );                                   // adjust offset for byte moves
    masm->jcc( Assembler::Condition::less, nextArg );                  // y - exact multiple of four bytes
    // n - 1-3 bytes need to be moved
    masm->movb( ecx, Address( eax, esi, Address::ScaleFactor::times_1 ) );
    masm->movb( Address( esp, esi, Address::ScaleFactor::times_1 ), ecx );
    masm->decl( esi );
    masm->jcc( Assembler::Condition::less, nextArg );                  // y - one extra byte
    // n - 2-3 extra bytes
    masm->movb( ecx, Address( eax, esi, Address::ScaleFactor::times_1 ) );
    masm->movb( Address( esp, esi, Address::ScaleFactor::times_1 ), ecx );
    masm->decl( esi );
    masm->jcc( Assembler::Condition::less, nextArg );                  // y - two extra bytes
    // n - three extra bytes
    masm->movb( ecx, Address( eax, esi, Address::ScaleFactor::times_1 ) );
    masm->movb( Address( esp, esi, Address::ScaleFactor::times_1 ), ecx );
    masm->jmp( nextArg );                                   // finished

    masm->bind( isUnsafe );

    masm->movl( eax, Address( esi, MemOopDescriptor::klass_byte_offset() ) );
    masm->movl( eax, Address( eax, KlassOopDescriptor::nonIndexableSizeOffset() ) );
    masm->leal( esi, Address( esi, eax, Address::ScaleFactor::times_1, 4 - MEMOOP_TAG ) ); // start of the bytes in the array
    masm->pushl( esi );
    masm->jmp( nextArg );

    masm->bind( isSMI );
    masm->sarl( eax, 2 );
    masm->pushl( eax );

    masm->bind( nextArg );
}


extern "C" {
void __CALLING_CONVENTION enter_async_call( DeltaProcess ** );
void __CALLING_CONVENTION exit_async_call( DeltaProcess ** );
}


const char *StubRoutines::generate_alien_call_with_args( MacroAssembler *masm ) {
    Label no_result, ptr_result, short_ptr_result, short_result, argLoopStart;
    Label isSMI, isDirect, startMove, isPointer, nextArg, moveLoopHead, moveLoopEnd, moveLoopTest, argLoopExit, argLoopTest, pushArgs;
    Label sizeLoopTest, sizeLoopStart;

    Address    fnptr( ebp, 8 );
    Address    result( ebp, 12 );
    Address    argCount( ebp, 16 );
    Address    argArray( ebp, 20 );
    Address    proc( ebp, -16 );
    const char *entry_point = masm->pc();

    masm->enter();
    masm->pushl( esi );                                       // preserve registers
    masm->pushl( edi );
    masm->pushl( ebx );
    masm->subl( esp, 4 );                                     // process pointer

    masm->movl( edi, argArray );                              // start of the oops in the array
    masm->movl( ebx, argCount );
    masm->leal( ebx, Address( edi, ebx, Address::ScaleFactor::times_1 ) );   // upper bounds of array

    masm->movl( edx, esp );                                   // start of size calculation

    //masm->int3();

    masm->movl( eax, ebx );                                   // eax is pointer to current arg
    masm->jmp( sizeLoopTest );
    masm->bind( sizeLoopStart );

    alien_arg_size( masm, sizeLoopTest );

    masm->bind( sizeLoopTest );                             // end of size calculation
    masm->subl( eax, 4 );
    masm->cmpl( eax, edi );
    masm->jcc( Assembler::Condition::greaterEqual, sizeLoopStart );

    push_alignment_spacers( masm );
    //masm->int3();

    masm->jmp( argLoopTest );
    masm->bind( argLoopStart );
    masm->movl( eax, Address( ebx ) );

    push_alien_arg( masm, argLoopTest );

    masm->subl( ebx, 4 );
    masm->cmpl( ebx, edi );
    masm->jcc( Assembler::Condition::less, argLoopExit );

    masm->jmp( argLoopStart );
    masm->bind( argLoopExit );
#ifdef ASYNC_ALIEN
                                                                                                                            masm->leal(eax, proc);
  masm->pushl(eax);
  masm->call((char*)enter_async_call, RelocationInformation::RelocationType::external_word_type);
#endif
    masm->call( fnptr );                            // call the alien function

#ifdef ASYNC_ALIEN
                                                                                                                            masm->pushl(eax);
  masm->leal(eax, proc);
  masm->pushl(edx);
  masm->pushl(eax);
  masm->call((char*) exit_async_call, RelocationInformation::RelocationType::external_word_type);
  masm->popl(edx);
  masm->popl(eax);
#endif
    masm->movl( ecx, result );                      // result alien

    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, no_result );       // is a result required?

    //masm->int3(); //debug
    // get the start of the alien area
    masm->movl( ecx, Address( ecx ) );
    masm->movl( esi, Address( ecx, MemOopDescriptor::klass_byte_offset() ) );

    masm->movl( esi, Address( esi, KlassOopDescriptor::nonIndexableSizeOffset() ) );
    masm->leal( ecx, Address( ecx, esi, Address::ScaleFactor::times_1, 4 - MEMOOP_TAG ) );
    // ecx now points to start of alien contents
    masm->movl( esi, Address( ecx ) );
    masm->testl( esi, esi );
    masm->jcc( Assembler::Condition::less, ptr_result );
    masm->jcc( Assembler::Condition::equal, short_result );

    masm->cmpl( esi, 4 );                           // direct result
    masm->jcc( Assembler::Condition::equal, short_result );
    masm->movl( Address( ecx, 8 ), edx );
    masm->bind( short_result );
    masm->movl( Address( ecx, 4 ), eax );

    masm->jmp( no_result );

    masm->bind( ptr_result );                       // indirect result
    masm->cmpl( esi, -4 );
    masm->movl( esi, Address( ecx, 4 ) );
    masm->jcc( Assembler::Condition::equal, short_ptr_result );
    masm->movl( Address( esi, 4 ), edx );
    masm->bind( short_ptr_result );
    masm->movl( Address( esi ), eax );

    masm->bind( no_result );
    masm->movl( esi, Address( ebp, -4 ) );            // restore registers
    masm->movl( edi, Address( ebp, -8 ) );
    masm->movl( ebx, Address( ebp, -12 ) );
    masm->leave();

    masm->ret( 16 );
    return entry_point;
}


const char *StubRoutines::generate_alien_call( MacroAssembler *masm, std::int32_t args ) {
    Label      no_result, ptr_result, short_ptr_result, short_result, pushArgs;
    Address    fnptr( ebp, 8 );
    Address    result( ebp, 12 );
    Address    proc( ebp, -8 );
    const char *entry_point = masm->pc();

    masm->enter();
    masm->pushl( esi );                             // preserve registers
    masm->subl( esp, 4 );                           // make Space for process pointer

    masm->movl( edx, esp );
    for ( std::int32_t arg = 0; arg < args; arg++ ) {
        Address argAddress( ebp, 16 + ( args - arg - 1 ) * 4 );
        Label   nextArg;
        masm->leal( eax, argAddress );
        alien_arg_size( masm, nextArg );

        masm->bind( nextArg );
    }

    masm->andl( edx, 0xf );
    masm->jcc( Assembler::Condition::equal, pushArgs );
    masm->pushl( 0 );
    masm->subl( edx, 4 );
    masm->jcc( Assembler::Condition::equal, pushArgs );
    masm->pushl( 0 );
    masm->subl( edx, 4 );
    masm->jcc( Assembler::Condition::equal, pushArgs );
    masm->pushl( 0 );
    masm->subl( edx, 4 );
    masm->bind( pushArgs );
    //if (args > 0) masm->int3();

    for ( std::int32_t arg1 = 0; arg1 < args; arg1++ ) {
        Address argAddress( ebp, 16 + ( args - arg1 - 1 ) * 4 );
        Label   moveLoopEnd;
        masm->movl( eax, argAddress );

        push_alien_arg( masm, moveLoopEnd );
    }
#ifdef ASYNC_ALIEN
                                                                                                                            masm->leal(eax, proc);
  masm->pushl(eax);
  masm->call((char*)enter_async_call, RelocationInformation::RelocationType::external_word_type);
#endif

    masm->call( fnptr );                            // call the alien function

#ifdef ASYNC_ALIEN
                                                                                                                            masm->pushl(eax);
  masm->leal(eax, proc);
  masm->pushl(edx);
  masm->pushl(eax);
  masm->call((char*) exit_async_call, RelocationInformation::RelocationType::external_word_type);
  masm->popl(edx);
  masm->popl(eax);
#endif

    masm->movl( ecx, result );                      // result alien

    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, no_result );       // is a result required?

    //masm->int3(); //debug
    // get the start of the alien area
    masm->movl( ecx, Address( ecx ) );
    masm->movl( esi, Address( ecx, MemOopDescriptor::klass_byte_offset() ) );

    masm->movl( esi, Address( esi, KlassOopDescriptor::nonIndexableSizeOffset() ) );
    masm->leal( ecx, Address( ecx, esi, Address::ScaleFactor::times_1, 4 - MEMOOP_TAG ) );
    // ecx now points to start of alien contents
    masm->movl( esi, Address( ecx ) );
    masm->testl( esi, esi );
    masm->jcc( Assembler::Condition::less, ptr_result );
    masm->jcc( Assembler::Condition::equal, short_result );

    masm->cmpl( esi, 4 );                           // direct result
    masm->jcc( Assembler::Condition::equal, short_result );
    masm->movl( Address( ecx, 8 ), edx );
    masm->bind( short_result );
    masm->movl( Address( ecx, 4 ), eax );

    masm->jmp( no_result );

    masm->bind( ptr_result );                       // indirect/pointer result
    masm->cmpl( esi, -4 );
    masm->movl( esi, Address( ecx, 4 ) );
    masm->jcc( Assembler::Condition::equal, short_ptr_result );
    masm->movl( Address( esi, 4 ), edx );
    masm->bind( short_ptr_result );
    masm->movl( Address( esi ), eax );

    masm->bind( no_result );

    masm->movl( esi, Address( ebp, -4 ) );            // restore registers
    masm->leave();
    masm->ret( 8 + ( args << 2 ) );
    return entry_point;
}


// Parametrized accessors

const char *StubRoutines::PolymorphicInlineCache_stub_entry( std::int32_t pic_size ) {
    st_assert( _is_initialized, "StubRoutines not initialized yet" );
    st_assert( 1 <= pic_size and pic_size <= static_cast<std::int32_t>( PolymorphicInlineCache::Constant::max_nof_entries ), "pic size out of range" )
    return _PolymorphicInlineCache_stub_entries[ pic_size ];
}


const char *StubRoutines::allocate_entry( std::int32_t size ) {
    st_assert( _is_initialized, "StubRoutines not initialized yet" );
    st_assert( 0 <= size and size <= max_fast_allocate_size, "size out of range" )
    return _allocate_entries[ size ];
}


const char *StubRoutines::alien_call_entry( std::int32_t args ) {
    st_assert( _is_initialized, "StubRoutines not initialized yet" );
    st_assert( 0 <= args and args <= max_fast_alien_call_size, "size out of range" )
    return _alien_call_entries[ args ];
}


// Initialization

bool       StubRoutines::_is_initialized = false;
const char *StubRoutines::_code          = nullptr;


const char *StubRoutines::generateStubRoutine( MacroAssembler *masm, const char *title, const char *gen( MacroAssembler * ) ) {

    const char *old_pc      = masm->pc();
    const char *entry_point = gen( masm );
    const char *new_pc      = masm->pc();

    SPDLOG_INFO( "%stubroutine-generate[{}], size [0x{08:x}] bytes, entry point address [0x{08:x}]", title, new_pc - old_pc, entry_point );
    if ( PrintStubRoutines ) {
        masm->code()->decode();
        _console->cr();
    }

    return entry_point;
}


const char *StubRoutines::generateStubRoutine( MacroAssembler *masm, const char *title, const char *gen( MacroAssembler *, std::int32_t ), std::int32_t argument ) {

    const char *old_pc      = masm->pc();
    const char *entry_point = gen( masm, argument );
    const char *new_pc      = masm->pc();

    SPDLOG_INFO( "%stubroutine-generate[{}], argument [{}], size [0x{08:x}] bytes, entry point address [0x{08:x}]", title, argument, new_pc - old_pc, entry_point );
    if ( PrintStubRoutines ) {
        masm->code()->decode();
        _console->cr();
    }

    return entry_point;
}


void StubRoutines::init() {

    if ( _is_initialized )
        return;

    _code                = os::exec_memory( _code_size );

    ResourceMark   rm;
    CodeBuffer     *code = new CodeBuffer( _code, _code_size );
    MacroAssembler *masm = new MacroAssembler( code );

    // add generators here
    _icNormalLookupEntry          = generateStubRoutine( masm, "ic_normal_lookup", generate_ic_normal_lookup );
    _icSuperLookupEntry           = generateStubRoutine( masm, "ic_super_lookup", generate_ic_super_lookup );
    _zombieNativeMethodEntry      = generateStubRoutine( masm, "zombie_nativeMethod", generate_zombie_nativeMethod );
    _zombieBlockNativeMethodEntry = generateStubRoutine( masm, "zombie_block_nativeMethod", generate_zombie_block_nativeMethod );
    _megamorphicIcEntry           = generateStubRoutine( masm, "megamorphic_ic", generate_megamorphic_ic );
    _compileBlockEntry            = generateStubRoutine( masm, "compile_block", generate_compile_block );
    _continueNonLocalReturnEntry  = generateStubRoutine( masm, "continue_NonLocalReturn", generate_continue_NonLocalReturn );
    _callSyncDllEntry             = generateStubRoutine( masm, "call_sync_DLL", generate_call_sync_DLL );
    _callAsyncDllEntry            = generateStubRoutine( masm, "call_async_DLL", generate_call_async_DLL );
    _lookupSyncDllEntry           = generateStubRoutine( masm, "lookup_sync_DLL", generate_lookup_sync_DLL );
    _lookupAsyncDllEntry          = generateStubRoutine( masm, "lookup_async_DLL", generate_lookup_async_DLL );
    _recompileStubEntry           = generateStubRoutine( masm, "recompile_stub", generate_recompile_stub );
    _usedUncommonTrapEntry        = generateStubRoutine( masm, "used_uncommon_trap", generate_uncommon_trap );
    _unusedUncommonTrapEntry      = generateStubRoutine( masm, "unused_uncommon_trap", generate_uncommon_trap );
    _verifyContextChainEntry      = generateStubRoutine( masm, "verify_context_chain", generate_verify_context_chain );
    _deoptimizeBlockEntry         = generateStubRoutine( masm, "deoptimize_block", generate_deoptimize_block );
    _callInspectorEntry           = generateStubRoutine( masm, "call_inspector", generate_call_inspector );
    _callDelta                    = generateStubRoutine( masm, "call_delta", generate_call_delta );
    _singleStepStub               = generateStubRoutine( masm, "single_step_stub", generate_single_step_stub );
    _unpackUnoptimizedFrames      = generateStubRoutine( masm, "unpack_unoptimized_frames", generate_unpack_unoptimized_frames );
    _provokeNlrAt                 = generateStubRoutine( masm, "provoke_nlr_at", generate_provoke_nlr_at );
    _continueNlrInDelta           = generateStubRoutine( masm, "continue_nlr_in_delta", generate_continue_nlr_in_delta );
    _handlePascalCallbackStub     = generateStubRoutine( masm, "handle_pascal_callback_stub", generate_handle_pascal_callback_stub );
    _handleCCallbackStub          = generateStubRoutine( masm, "handle_C_callback_stub", generate_handle_C_callback_stub );
    _oopifyFloat                  = generateStubRoutine( masm, "oopify_float", generate_oopify_float );
    _alienCallWithArgsEntry       = generateStubRoutine( masm, "alien_call_with_args", generate_alien_call_with_args );

    for ( std::int32_t pic_size = 1; pic_size <= static_cast<std::int32_t>( PolymorphicInlineCache::Constant::max_nof_entries ); pic_size++ ) {
        _PolymorphicInlineCache_stub_entries[ pic_size ] = generateStubRoutine( masm, "PolymorphicInlineCache stub", generate_PolymorphicInlineCache_stub, pic_size );
    }

    for ( std::int32_t size = 0; size <= max_fast_allocate_size; size++ ) {
        _allocate_entries[ size ] = generateStubRoutine( masm, "allocate", generate_allocate, size );
    }

    for ( std::int32_t args = 0; args <= max_fast_alien_call_size; args++ ) {
        _alien_call_entries[ args ] = generateStubRoutine( masm, "alien_call", generate_alien_call, args );
    }

    masm->finalize();
    st_assert( code->code_size() < _code_size, "Stub routines too large for allocated space" );
    _is_initialized = true;
    SPDLOG_INFO( "%stubroutines-size: [0x{08:x}] bytes", masm->offset() );
}

/*

  %note: initialisation is in InterpreterGenerator now -Marc 04/07

void stubRoutines_init() {
  StubRoutines::init();
}
*/
