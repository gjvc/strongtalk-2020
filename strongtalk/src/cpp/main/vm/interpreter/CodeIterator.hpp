//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"


class InterpretedPrimitiveCache;

class Interpreted_DLLCache;

class InterpretedInlineCache;

// CodeIterator is a simple but fast iterator for scanning byte code instructions in a methodOop.

class CodeIterator : public StackAllocatedObject {

private:
    MethodOop    _methodOop;
    std::uint8_t *_current;
    std::uint8_t *_end;

    void align();

    std::uint8_t *align( std::uint8_t *p ) const;


public:
    CodeIterator( MethodOop method, std::int32_t startByteCodeIndex = 1 );

    CodeIterator( std::uint8_t *hp );


    // Advance to next instruction
    // Returns false if we passed the end.
    bool advance() {
        _current = next_hp();
        return _current < _end;
    }


    // accessors
    std::uint8_t byte_at( std::int32_t offset_from_instruction ) {
        return _current[ offset_from_instruction ];
    }


    Oop oop_at( std::int32_t offset_from_instruction ) {
        return *aligned_oop( offset_from_instruction );
    }


    std::int32_t word_at( std::int32_t offset_from_instruction ) {
        return (std::int32_t) *aligned_oop( offset_from_instruction );
    }


    ByteCodes::Code code() const {
        return ByteCodes::Code( *_current );
    }


    ByteCodes::CodeType code_type() const {
        return ByteCodes::code_type( code() );
    }


    ByteCodes::Format format() const {
        return ByteCodes::format( code() );
    }


    ByteCodes::SendType send() const {
        return ByteCodes::send_type( code() );
    }


    ByteCodes::ArgumentSpec argumentsType() const {
        return ByteCodes::argument_spec( code() );
    }


    bool pop_result() const {
        return ByteCodes::pop_tos( code() );
    }


    ByteCodes::LoopType loopType() const {
        return ByteCodes::loop_type( code() );
    }


    std::int32_t byteCodeIndex() const;

    std::int32_t next_byteCodeIndex() const;


    std::uint8_t *hp() const {
        return _current;
    }


    std::uint8_t *next_hp() const;

    // FOR DEOPTIMIZATION
    // Returns the interpreter return point for the current byte code.
    const char *interpreter_return_point( bool restore_value = false ) const;

    void set_byteCodeIndex( std::int32_t byteCodeIndex );


    // returns the location of an aligned Oop
    Oop *aligned_oop( std::int32_t offset_from_instruction ) {
        return (Oop *) align( _current + offset_from_instruction );
    }


    bool is_message_send() const {
        return ByteCodes::code_type( code() ) == ByteCodes::CodeType::MESSAGE_SEND;
    }


    bool is_primitive_call() const {
        return ByteCodes::code_type( code() ) == ByteCodes::CodeType::PRIMITIVE_CALL;
    }


    bool is_dll_call() const {
        return ByteCodes::code_type( code() ) == ByteCodes::CodeType::DLL_CALL;
    }


    // Returns the address of the block method if the current butecode is a push closure, nullptr otherwise.
    Oop *block_method_addr();

    // Returns the block method if the current butecode is a push closure, nullptr otherwise.
    MethodOop block_method();

    // Customization
    void customize_class_var_code( KlassOop to_klass );

    void recustomize_class_var_code( KlassOop from_klass, KlassOop to_klass );

    void uncustomize_class_var_code( KlassOop from_klass );

    void customize_inst_var_code( KlassOop to_klass );

    void recustomize_inst_var_code( KlassOop from_klass, KlassOop to_klass );

    void uncustomize_inst_var_code( KlassOop from_klass );

    // Returns the inline cache iff the current instruction is a send
    InterpretedInlineCache *ic();

    InterpretedPrimitiveCache *primitive_cache();

    Interpreted_DLLCache *dll_cache();


    // For byte code manipulation
    void set_code( std::uint8_t code ) {
        *_current = code;
    }


    void set_code( ByteCodes::Code code ) {
        *_current = static_cast<std::uint8_t>( code );
    }

};
