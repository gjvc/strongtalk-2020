//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/os.hpp"
#include "vm/code/NativeInstruction.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"

// Support routines for Dynamic Link Libraries (DLLs)

class Interpreted_DLLCache : public ValueObject {
private:
    SymbolOop      _dll_name;
    SymbolOop      _funct_name;
    dll_func_ptr_t _entry_point;
    char           _number_of_arguments;
    // Do not add more instance variables! Layout must correspond to DLL call in ByteCodes!

public:
    SymbolOop dll_name() const {
        return _dll_name;
    }


    SymbolOop funct_name() const {
        return _funct_name;
    }


    dll_func_ptr_t entry_point() const {
        return _entry_point;
    }


    std::int32_t number_of_arguments() const {
        return _number_of_arguments;
    }


    bool async() const;


    void set_entry_point( dll_func_ptr_t f ) {
        _entry_point = f;
    }


    // Debugging
    void verify();

    void print();
};


// Layout of Compiled_DLLCaches
//
// mov  edx, entry_point	<- mov edx
// test eax, dll_name		<- test 1
// test eax, function_name	<- test 2
// call sync_DLL/async_DLL	<- call
// ...				<- this

class Compiled_DLLCache : public NativeCall {
private:
    enum Layout_constants {
        test_2_instruction_offset  = -NativeCall::instruction_size - NativeTest::instruction_size,   //
        test_1_instruction_offset  = test_2_instruction_offset - NativeTest::instruction_size,       //
        mov_edx_instruction_offset = test_1_instruction_offset - NativeMov::instruction_size,       //
    };


    NativeMov *mov_at( std::int32_t offset ) {
        return nativeMov_at( addr_at( offset ) );
    }


    NativeTest *test_at( std::int32_t offset ) {
        return nativeTest_at( addr_at( offset ) );
    }


public:
    SymbolOop dll_name() {
        return SymbolOop( test_at( test_1_instruction_offset )->data() );
    }


    SymbolOop function_name() {
        return SymbolOop( test_at( test_2_instruction_offset )->data() );
    }


    dll_func_ptr_t entry_point() {
        return (dll_func_ptr_t) mov_at( mov_edx_instruction_offset )->data();
    }


    bool async() const;


    void set_entry_point( dll_func_ptr_t f ) {
        mov_at( mov_edx_instruction_offset )->set_data( std::int32_t( f ) );
    }


    // Debugging
    void verify();

    void print();

    // Creation
    friend Compiled_DLLCache *compiled_DLLCache_from_return_address( const char *return_address );

    friend Compiled_DLLCache *compiled_DLLCache_from_relocInfo( const char *displacement_address );
};


class DLLs : AllStatic {
public:
    // Lookup
    static dll_func_ptr_t lookup( SymbolOop name, DLL *library );

    static DLL *load( SymbolOop name );

    static bool unload( DLL *library );

    static dll_func_ptr_t lookup_fail( SymbolOop dll_name, SymbolOop function_name );

    static dll_func_ptr_t lookup( SymbolOop dll_name, SymbolOop function_name );

    static dll_func_ptr_t lookup_and_patch_Interpreted_DLLCache();

    static dll_func_ptr_t lookup_and_patch_Compiled_DLLCache();

    // Support for asynchronous DLL calls
    static void enter_async_call( DeltaProcess **addr );    // called before each asynchronous DLL call
    static void exit_async_call( DeltaProcess **addr );    // called after each asynchronous DLL call
    static void exit_sync_call( DeltaProcess **addr );    // called after each synchronous DLL call
};


Compiled_DLLCache *compiled_DLLCache_from_return_address( const char *return_address );

Compiled_DLLCache *compiled_DLLCache_from_relocInfo( const char *displacement_address );
