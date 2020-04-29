//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/system/asserts.hpp"
#include "vm/lookup/LookupResult.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/interpreter/ByteCodes.hpp"



// The InterpretedInlineCache describes the contents of an inline cache in the
// byte codes of a method. The inline cache holds a combination of
// selector/method and 0/class oops and is read and modified during
// execution.
//
//
// An InterpretedInlineCache can have four different states:
//
// 1) empty					{selector,         0}
// 2) filled with a call to interpreted code	{method,           klass}
// 3) filled with an interpreter PolymorphicInlineCache		{selector,         objArrayObj}
// 4) filled with a call to compiled code	{jump table entry, klass}
//
//
// Layout:
//
//    send byte code			<-----	send_code_addr()
//    ...
//    halt byte codes for alignment
//    ...
// 0: selector/method/jump table entry	<-----	first_word_addr()
// 4: 0/klass/objArray			<-----	second_word_addr()
// 8: ...

class InterpretedInlineCache : ValueObject {

    public:

        enum {
            size               = 8,            // inline cache size in words
            first_word_offset  = 0,            // layout info: first word
            second_word_offset = 4,            // layout info: second word
        } InterpretedICConstants;

        // Conversion (Bytecode* -> InterpretedInlineCache*)
        friend InterpretedInlineCache * as_InterpretedIC( const char * address_of_next_instr );

        // find send bytecode, given address of selector; return nullptr/IllegalByteCodeIndex if not in a send
        static uint8_t * findStartOfSend( uint8_t * selector_addr );

        static int findStartOfSend( MethodOop m, int byteCodeIndex );

    private:
        // field access
        const char * addr_at( int offset ) const {
            return ( const char * ) this + offset;
        }


        Oop * first_word_addr() const {
            return ( Oop * ) addr_at( first_word_offset );
        }


        Oop * second_word_addr() const {
            return ( Oop * ) addr_at( second_word_offset );
        }


        void set( ByteCodes::Code send_code, Oop first_word, Oop second_word );

    public:
        // Raw inline cache access
        uint8_t * send_code_addr() const {
            return findStartOfSend( ( uint8_t * ) this );
        }


        ByteCodes::Code send_code() const {
            return ByteCodes::Code( *send_code_addr() );
        }


        Oop first_word() const {
            return *first_word_addr();
        }


        Oop second_word() const {
            return *second_word_addr();
        }


        // Returns the polymorphic inline cache array. Assert fails if no pic is present.
        ObjectArrayOop pic_array();


        // Inline cache information
        bool_t is_empty() const {
            return second_word() == nullptr;
        }


        SymbolOop selector() const;        // the selector
        JumpTableEntry * jump_table_entry() const;    // only legal to call if compiled send

        int nof_arguments() const;        // the number of arguments
        ByteCodes::SendType send_type() const;    // the send type
        ByteCodes::ArgumentSpec argument_spec() const;// the argument spec

        // Manipulation
        void clear();                    // clears the inline cache
        void cleanup();                               // cleanup the inline cache
        void clear_without_deallocation_pic();        // clears the inline cache without deallocating the pic
        void replace( NativeMethod * nm );            // replaces the appropriate target with a nm
        void replace( LookupResult result, KlassOop receiver_klass ); // replaces the inline cache with a lookup result

        // Debugging
        void print();

        // Cache miss
        static Oop * inline_cache_miss();        // the inline cache miss handler

    private:
        // helpers for inline_cache_miss
        static void update_inline_cache( InterpretedInlineCache * ic, Frame * f, ByteCodes::Code send_code, KlassOop klass, LookupResult result );

        static Oop does_not_understand( Oop receiver, InterpretedInlineCache * ic, Frame * f );

        static void trace_inline_cache_miss( InterpretedInlineCache * ic, KlassOop klass, LookupResult result );
};


InterpretedInlineCache * as_InterpretedIC( const char * address_of_next_instr );


// Interpreter_PICs handles the allocation and deallocation of interpreter PICs.

static constexpr int size_of_smallest_interpreterPIC                   = 2;
static constexpr int size_of_largest_interpreterPIC                    = 5;
static constexpr int number_of_interpreterPolymorphicInlineCache_sizes = size_of_largest_interpreterPIC - size_of_smallest_interpreterPIC + 1;


// An InterpretedInlineCacheIterator is used to iterate through the entries of an inline cache in a methodOop.
//
// Whenever possible, one should use this abstraction instead of the (raw) InterpretedInlineCache


class InterpretedInlineCacheIterator : public InlineCacheIterator {

    private:
        InterpretedInlineCache * _ic;            // the inline cache
        ObjectArrayOop _pic;            // the PolymorphicInlineCache if there is one

        // state machine
        int              _number_of_targets;    // the no. of InlineCache entries
        InlineCacheShape _info;                 // send site information
        int              _index;                // the current entry no.
        KlassOop         _klass;                // the current klass
        MethodOop        _method;               // the current method
        NativeMethod * _nativeMethod;        // current NativeMethod (nullptr if none)

        void set_method( Oop m );               // set _method and _nativeMethod
        void set_klass( Oop k );                // don't assign to _klass directly

    public:
        InterpretedInlineCacheIterator( InterpretedInlineCache * ic );


        // InlineCache information
        int number_of_targets() const {
            return _number_of_targets;
        }


        InlineCacheShape shape() const {
            return _info;
        }


        SymbolOop selector() const {
            return _ic->selector();
        }


        bool_t is_interpreted_ic() const {
            return true;
        }


        bool_t is_super_send() const;


        InterpretedInlineCache * interpreted_ic() const {
            return _ic;
        }


        // Iterating through entries
        void init_iteration();

        void advance();


        bool_t at_end() const {
            return _index >= number_of_targets();
        }


        // Accessing entries
        KlassOop klass() const {
            return _klass;
        }


        // answer whether current target method is compiled or interpreted
        bool_t is_interpreted() const {
            return _nativeMethod == nullptr;
        }


        bool_t is_compiled() const {
            return _nativeMethod not_eq nullptr;
        }


        MethodOop interpreted_method() const;

        NativeMethod * compiled_method() const;

        // Debugging
        void print();
};
