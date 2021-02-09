//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/CallBack.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/runtime/ErrorHandler.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/system/dll.hpp"


void CallBack::initialize( Oop receiver, SymbolOop selector ) {
    st_assert( selector->isSymbol(), "must be symbol" );
    Universe::set_callBack( receiver, selector );
}


static char *store_byte( char *chunk, char b ) {
    *chunk = b;
    return chunk + sizeof( char );
}


static char *store_long( char *chunk, std::int32_t l ) {
    *( (std::int32_t *) chunk ) = l;
    return chunk + sizeof( std::int32_t );
}


// stdcall
void *CallBack::registerPascalCall( std::int32_t index, std::int32_t nofArgs ) {

    void *result = malloc( 15 );
    char *chunk  = (char *) result;

    // MOV ECX, index
    chunk = store_byte( chunk, '\xB9' );
    chunk = store_long( chunk, index );

    // MOV EDX, number of bytes for parameters
    chunk = store_byte( chunk, '\xBA' );
    chunk = store_long( chunk, nofArgs * sizeof( std::int32_t ) );

    // JMP _handleCCallStub
    chunk = store_byte( chunk, '\xE9' );
    chunk = store_long( chunk, ( (std::int32_t) StubRoutines::handle_pascal_callback_stub() ) - ( 4 + (std::int32_t) chunk ) );

    return result;
}


// cdecl
void *CallBack::registerCCall( std::int32_t index ) {
    void *result = malloc( 10 );
    char *chunk  = (char *) result;

    // MOV ECX, index
    chunk = store_byte( chunk, '\xB9' );
    chunk = store_long( chunk, index );

    // JMP _handleCCallStub
    chunk = store_byte( chunk, '\xE9' );
    chunk = store_long( chunk, ( (std::int32_t) StubRoutines::handle_C_callback_stub() ) - ( 4 + (std::int32_t) chunk ) );

    return result;
}


void CallBack::unregister( void *block ) {
    st_assert( block, "block is not valid" );
    free( (char *) block );
}


// handleCallBack is called from the two assembly stubs:
// - handlePascalCallBackStub
// - handleCCallBackStub

typedef void *(__CALLING_CONVENTION *call_out_func_4)( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d );

extern "C" {
extern bool have_nlr_through_C;
}

extern "C" volatile void *handleCallBack( std::int32_t index, std::int32_t params ) {

    DeltaProcess *proc = nullptr;

    if ( Universe::callBack_receiver()->isNilObject() ) {
        SPDLOG_WARN( "callBack receiver is not set" );
    }

    std::int32_t low  = get_unsigned_bitfield( params, 0, 16 );
    std::int32_t high = get_unsigned_bitfield( params, 16, 16 );

    if ( DeltaProcess::active()->thread_id() not_eq os::current_thread_id() ) {
        // We'are now back in a asynchronous DLL call so give up the control
        // Fix this:
        //   remove warning when it has been tested
        proc = Processes::find_from_thread_id( os::current_thread_id() );
        st_assert( proc, "process must be present" );
        DLLs::exit_async_call( &proc );
    }

    DeltaProcess::active()->setIsCallback( true );

    Oop res = Delta::call( Universe::callBack_receiver(), Universe::callBack_selector(), smiOopFromValue( index ), smiOopFromValue( high ), smiOopFromValue( low ) );

    st_assert( DeltaProcess::active()->thread_id() == os::current_thread_id(), "check for process torch" );

    void *result;

    // convert return result

    if ( have_nlr_through_C ) {
        // Continues the NonLocalReturn after at the next Delta frame
        BaseHandle *handle = DeltaProcess::active()->firstHandle();
        if ( handle and ( (const char *) handle < (const char *) DeltaProcess::active()->last_delta_fp() ) )
            handle->pop();

        ErrorHandler::continue_nlr_in_delta();
    }

    if ( res->isSmallIntegerOop() ) {
        result = (void *) SmallIntegerOop( res )->value();
    } else if ( res->is_proxy() ) {
        result = (void *) ProxyOop( res )->get_pointer();
    } else {
        SPDLOG_WARN( "Wrong return type for call back, returning nullptr" );
        result = nullptr;
    }

    // Return value has to be converted before we transfer control to another thread.

    if ( proc ) {
        // We'are now back in a asynchronous DLL call so give up the control
        proc->resetStepping();
        proc->transfer_and_continue();
    }

    // Call Delta level error routine
    return result;
}
