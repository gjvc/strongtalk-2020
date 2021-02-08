
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/system/dll.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/interpreter/CodeIterator.hpp"


extern Compiler *theCompiler;

// Interpreted_DLLCache implementation

void Interpreted_DLLCache::verify() {
    // check oops
    if ( not dll_name()->is_symbol() ) st_fatal( "dll name is not a SymbolOop" );
    if ( not funct_name()->is_symbol() ) st_fatal( "function name is not a SymbolOop" );
    if ( number_of_arguments() < 0 ) st_fatal( "illegal number of arguments" );
}


void Interpreted_DLLCache::print() {
    _console->print( "DLL call " );
    dll_name()->print_value();
    _console->print( "::" );
    funct_name()->print_value();
    SPDLOG_INFO( " (0x{0:x}, %s, interpreted)", reinterpret_cast<void *>(entry_point()), async() ? "asynchronous" : "synchronous" );
}


// Compiled_DLLCache implementation

bool Compiled_DLLCache::async() const {
    char *d = destination();
    return d == StubRoutines::lookup_DLL_entry( true ) or d == StubRoutines::call_DLL_entry( true );
}


void Compiled_DLLCache::verify() {
    // check layout
    mov_at( mov_edx_instruction_offset )->verify();
    test_at( test_1_instruction_offset )->verify();
    test_at( test_2_instruction_offset )->verify();
    NativeCall::verify();
    // check oops
    if ( not dll_name()->is_symbol() ) st_fatal( "dll name is not a SymbolOop" );
    if ( not function_name()->is_symbol() ) st_fatal( "function name is not a SymbolOop" );
    // check destination
    char *d = destination();
    if ( d not_eq StubRoutines::lookup_DLL_entry( true ) and d not_eq StubRoutines::lookup_DLL_entry( false ) and d not_eq StubRoutines::call_DLL_entry( true ) and d not_eq StubRoutines::call_DLL_entry( false ) ) {
        st_fatal1( "Compiled_DLLCache destination 0x{0:x} incorrect", d );
    }
}


void Compiled_DLLCache::print() {
    _console->print( "DLL call " );
    dll_name()->print_value();
    _console->print( "::" );
    function_name()->print_value();
    SPDLOG_INFO( " (0x{0:x}, %s, compiled)", reinterpret_cast<const void *>(entry_point()), async() ? "asynchronous" : "synchronous" );
}


// DLLs implementation

dll_func_ptr_t DLLs::lookup( SymbolOop name, DLL *library ) {
    char buffer[200];
    st_assert( not name->copy_null_terminated( buffer, 200 ), "DLL function name longer than 200 chars - truncated" );
    return os::dll_lookup( buffer, library );
}


DLL *DLLs::load( SymbolOop name ) {
    char buffer[200];
    st_assert( not name->copy_null_terminated( buffer, 200 ), "DLL library name longer than 200 chars - truncated" );
    return os::dll_load( buffer );
}


bool DLLs::unload( DLL *library ) {
    return os::dll_unload( library );
}


dll_func_ptr_t DLLs::lookup_fail( SymbolOop dll_name, SymbolOop function_name ) {
    // Call backs to Delta
    if ( Universe::dll_lookup_receiver()->is_nil() ) {
        spdlog::warn( "dll lookup receiver is not set" );
    }
    st_assert( theCompiler == nullptr, "Cannot handle call back during compilation" );

    Oop res = Delta::call( Universe::dll_lookup_receiver(), Universe::dll_lookup_selector(), function_name, dll_name );

    return res->is_proxy() ? (dll_func_ptr_t) ProxyOop( res )->get_pointer() : nullptr;
}


dll_func_ptr_t DLLs::lookup( SymbolOop dll_name, SymbolOop function_name ) {

    dll_func_ptr_t result = lookup_fail( dll_name, function_name );
    if ( result ) {
        if ( TraceDLLLookup ) {
            SPDLOG_INFO( "address [0x%lx], DLL name[{}], function name[{}]", reinterpret_cast<void *>(result), dll_name->print_value_string(), function_name->print_value_string() );
        }
        return result;
    } else {
        if ( TraceDLLLookup ) {
            SPDLOG_INFO( "could not find function name[{}] in DLL name[{}]", function_name->print_value_string(), dll_name->print_value_string() );
        }
    }

    DeltaProcess::active()->suspend( ProcessState::DLL_lookup_error );
    return nullptr;
}


dll_func_ptr_t DLLs::lookup_and_patch_Interpreted_DLLCache() {
    // get DLL call info
    Frame        f = DeltaProcess::active()->last_frame();
    MethodOop    m = f.method();
    CodeIterator it( m, m->byteCodeIndex_from( f.hp() ) );

    Interpreted_DLLCache *cache = it.dll_cache();
    st_assert( cache->entry_point() == nullptr, "should not be set yet" );

    // do lookup, patch & return entry point
    dll_func_ptr_t function = lookup( cache->dll_name(), cache->funct_name() );
    cache->set_entry_point( function );

    return function;
}


dll_func_ptr_t DLLs::lookup_and_patch_Compiled_DLLCache() {
    // get DLL call info
    Frame             f      = DeltaProcess::active()->last_frame();
    Compiled_DLLCache *cache = compiled_DLLCache_from_return_address( f.pc() );
    st_assert( cache->entry_point() == nullptr, "should not be set yet" );
    // do lookup, patch & return entry point
    dll_func_ptr_t function = lookup( cache->dll_name(), cache->function_name() );
    cache->set_entry_point( function );
    cache->set_destination( StubRoutines::call_DLL_entry( cache->async() ) );
    return function;
}


void DLLs::enter_async_call( DeltaProcess **addr ) {
    DeltaProcess *proc = DeltaProcess::active();
    *addr = proc; // proc will be retrieved in dll_enter_async_call
    proc->resetStepping();
    proc->transfer_and_continue();
}


void DLLs::exit_async_call( DeltaProcess **addr ) {
    DeltaProcess *proc = *addr;
    proc->wait_for_control();
    proc->applyStepping();
}


void DLLs::exit_sync_call( DeltaProcess **addr ) {
    static_cast<void>(addr); // unused
    // nothing to do here for now
}


extern "C" {

void __CALLING_CONVENTION enter_async_call( DeltaProcess **addr ) {
    DLLs::enter_async_call( addr );
}

void __CALLING_CONVENTION exit_async_call( DeltaProcess **addr ) {
    DLLs::exit_async_call( addr );
}

}


Compiled_DLLCache *compiled_DLLCache_from_return_address( const char *return_address ) {
    Compiled_DLLCache *cache = (Compiled_DLLCache *) ( nativeCall_from_return_address( return_address ) );
    cache->verify();
    return cache;
}


Compiled_DLLCache *compiled_DLLCache_from_relocInfo( const char *displacement_address ) {
    return (Compiled_DLLCache *) nativeCall_from_relocInfo( displacement_address );
}
