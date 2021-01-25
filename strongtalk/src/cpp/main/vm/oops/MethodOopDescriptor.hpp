//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/interpreter/MissingMethodBuilder.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/GrowableArray.hpp"



// A methodOop is a method with byte codes.

const std::int32_t method_size_mask_bitno = 2;
const std::int32_t method_size_mask_size  = 18;

const std::int32_t method_args_mask_bitno = method_size_mask_bitno + method_size_mask_size;
const std::int32_t method_args_mask_size  = 4;

const std::int32_t method_flags_mask_bitno = method_args_mask_bitno + method_args_mask_size;
const std::int32_t method_flags_mask_size  = 8;

class InterpretedInlineCache;

class MethodOopDescriptor : public MemOopDescriptor {
private:
    ObjectArrayOop _debugInfo;             //
    Oop            _selector_or_method;    // selector for normal methods, enclosing method for blocks
    std::int32_t            _counters;              // invocation counter and sharing counter
    SMIOop         _size_and_flags;        //


    // [flags (8 bits),  nofArgs (4 bits), size in oops (18 bits), tag (2 bits)]
    MethodOopDescriptor *addr() const {
        return (MethodOopDescriptor *) MemOopDescriptor::addr();
    }


    // returns the header size of a methodOop
    static std::int32_t header_size() {
        return sizeof( MethodOopDescriptor ) / oopSize;
    }


public:
    // offsets for code generation
    static std::int32_t selector_or_method_byte_offset() {
        return std::int32_t( &( ( (MethodOopDescriptor *) nullptr )->_selector_or_method ) ) - MEMOOP_TAG;
    }


    static std::int32_t counters_byte_offset() {
        return std::int32_t( &( ( (MethodOopDescriptor *) nullptr )->_counters ) ) - MEMOOP_TAG;
    }


    static std::int32_t codes_byte_offset() {
        return sizeof( MethodOopDescriptor ) - MEMOOP_TAG;
    }


    SMIOop size_and_flags() const {
        return addr()->_size_and_flags;
    }


    void set_size_and_flags( std::int32_t size, std::int32_t nofArgs, std::int32_t flags ) {
        addr()->_size_and_flags = (SMIOop) ( ( flags << method_flags_mask_bitno ) + ( nofArgs << method_args_mask_bitno ) + ( size << method_size_mask_bitno ) );
    }


    std::int32_t flags() const {
        return get_unsigned_bitfield( (std::int32_t) size_and_flags(), method_flags_mask_bitno, method_flags_mask_size );
    }


    void set_flags( std::int32_t flags ) {
        set_size_and_flags( size_of_codes(), nofArgs(), flags );
    }


    std::int32_t nofArgs() const {             // number of arguments (excluding receiver)
        return get_unsigned_bitfield( (std::int32_t) size_and_flags(), method_args_mask_bitno, method_args_mask_size );
    }


public:
    friend MethodOop as_methodOop( void *p );

    void bootstrap_object( Bootstrap *stream );


    // Tester
    bool_t is_blockMethod() const {
        return not selector_or_method()->is_symbol();
    }


    ObjectArrayOop debugInfo() const {
        return addr()->_debugInfo;
    }


    void set_debugInfo( ObjectArrayOop d ) {
        addr()->_debugInfo = d;
    }


    SymbolOop selector() const;

    MethodOop parent() const;        // returns the enclosing block or method (for blocks), or nullptr
    MethodOop home() const;        // returns the enclosing method (for blocks), or itself

    Oop selector_or_method() const {
        return addr()->_selector_or_method;
    }


    void set_selector_or_method( Oop value ) {
        addr()->_selector_or_method = value;
    }


    // returns the enclosing method's selector (block methods only)
    SymbolOop enclosing_method_selector() const;

    // returns the source if the debugInfo is present, otherwise nullptr.
    ByteArrayOop source();

    // returns the tempInfo if the debugInfo is present, otherwise nullptr.
    ObjectArrayOop tempInfo();

    // Generates an array with fileout information.
    // The array contains pair of information:
    // (true,  SmallInteger) means character
    // (false, Oop)          means Oop
    ObjectArrayOop fileout_body();

    // Accessor operaions on counters.
    //   invocation counter - tells how many times this methodOop has been executed (may decay over time)
    //   sharing counter    - tells how many callers this methodOop has.

    enum {
        _short_size              = 16, //
        _invocation_count_offset = _short_size, //
        _invocation_count_width  = _short_size, //
        _invocation_count_max    = ( 1 << _short_size ) - 1, //
        _sharing_count_offset    = 0, //
        _sharing_count_width     = _short_size, //
        _sharing_count_max       = ( 1 << _short_size ) - 1 //
    };


    std::int32_t counters() const {
        return addr()->_counters;
    }


    void set_counters( std::int32_t inv, std::int32_t share ) {
        addr()->_counters = ( inv << _invocation_count_offset ) | share;
    }


    // Invocation counter
    std::int32_t invocation_count() const {
        return get_unsigned_bitfield( counters(), _invocation_count_offset, _invocation_count_width );
    }


    void set_invocation_count( std::int32_t value ) {
        set_counters( value, sharing_count() );
    }


    void decay_invocation_count( double decay_factor );


    // Sharing counter (number of callers)
    std::int32_t sharing_count() const {
        return get_unsigned_bitfield( counters(), _sharing_count_offset, _sharing_count_width );
    }


    void set_sharing_count( std::int32_t value ) {
        set_counters( invocation_count(), value );
    }


    void inc_sharing_count();

    void dec_sharing_count();

    bool_t was_never_executed();        // was method never executed? (count = 0, empty inline caches)

    std::int32_t size_of_codes() const {        // size of byte codes in words
        return get_unsigned_bitfield( (std::int32_t) size_and_flags(), method_size_mask_bitno, method_size_mask_size );
    }


    void set_size_of_code( std::int32_t size ) {
        set_size_and_flags( size, nofArgs(), flags() );
    }


    // Returns a pointer to the hybrid code at 'offset'
    std::uint8_t *codes( std::int32_t offset = 1 ) const {
        return (std::uint8_t *) addr() + sizeof( MethodOopDescriptor ) + offset - 1;
    }


    std::uint8_t *codes_end() const {
        return codes() + size_of_codes() * oopSize;
    }


    // find methodOop given an hcode pointer
    static MethodOop methodOop_from_hcode( std::uint8_t *hp );


    std::uint8_t byte_at( std::int32_t offset ) const {
        return *codes( offset );
    }


    void byte_at_put( std::int32_t offset, std::uint8_t c ) {
        *codes( offset ) = c;
    }


    std::int32_t word_at( std::int32_t offset ) const {
        return *(std::int32_t *) codes( offset );
    }


    void word_at_put( std::int32_t offset, std::uint32_t w ) {
        *(std::int32_t *) codes( offset ) = w;
    }


    Oop oop_at( std::int32_t offset ) const {
        return *(Oop *) codes( offset );
    }


    void oop_at_put( std::int32_t offset, Oop obj ) {
        *(Oop *) codes( offset ) = obj;
    }


    // Returns the next byte code index based on hp.
    std::int32_t next_byteCodeIndex_from( std::uint8_t *hp ) const;

    // Returns the current byte code index based on hp (points to the next byte code)
    std::int32_t byteCodeIndex_from( std::uint8_t *hp ) const;

    std::int32_t number_of_arguments() const;

    // Returns the number of temporaries allocated by the interpreter
    // (excluding receiver & float temporaries, which may come afterwards).
    std::int32_t number_of_stack_temporaries() const;


    // Method with hardwired floating-point operations
    bool_t has_float_temporaries() const {
        return *codes( 1 ) == static_cast<std::uint8_t>(ByteCodes::Code::float_allocate);
    }


    std::int32_t number_of_float_temporaries() const {
        return has_float_temporaries() ? *codes( 3 ) : 0;
    }


    std::int32_t float_expression_stack_size() const {
        return has_float_temporaries() ? *codes( 4 ) : 0;
    }


    std::int32_t total_number_of_floats() const {
        return number_of_float_temporaries() + float_expression_stack_size();
    }


    // Stack frame layout if there's a float section (offset & size in oops relative to ebp)
    std::int32_t float_offset( std::int32_t float_no ) const;


    std::int32_t float_section_start_offset() const {
        return frame_temp_offset - number_of_stack_temporaries();
    }


    std::int32_t float_section_size() const {
        return total_number_of_floats() * SIZEOF_FLOAT / oopSize;
    }


    // Testers
    bool_t is_accessMethod() const {
        return *codes() == static_cast<std::uint8_t>(ByteCodes::Code::return_instVar);
    }


    bool_t is_primitiveMethod() const;


    // For predicted sends (smi_t +, -, *, etc.)
    bool_t is_special_primitiveMethod() const {
        return *codes( 1 ) == static_cast<std::uint8_t>(ByteCodes::Code::special_primitive_send_1_hint);
    }


    ByteCodes::Code special_primitive_code() const;


    // Information needed by the optimizing compiler
    //
    // Incoming_Info describes the possible Oop kinds that a methodOop gets 'from' the
    // outside (via the closure) when invoked. Currently, the incoming Oop is stored in
    // the recv and temp0 stack location for blocks, and in the recv location only for
    // methods (ordinary send). This should change at some point and the incoming Oop
    // should be stored in the recv location only (allows better stack usage).
    //
    // Temporaries_Info is used to compute the number of stack-allocated temporaries. If
    // more than two temporaries are needed, the very first bytecode is an allocate temp
    // byte code (bytecode invariant).

    enum Flags {
        // general flags
        containsNonLocalReturnFlag = 0, //
        allocatesContextFlag       = 1, //
        mustBeCustomizedFlag       = 2, //
        isCustomizedFlag           = 3, //

        // method specific flags (overlapping with block specific flags)
        methodInfoFlags = isCustomizedFlag + 1, //
        methodInfoSize  = 2,                    //

        // block specific flags (overlapping with method specific flags)
        blockInfoFlags = methodInfoFlags,       //
        blockInfoSize  = methodInfoSize         //
    };

    // Flags for inlining
    enum Method_Inlining_Info {
        never_inline  = 0,  //
        normal_inline = 1,  //
        always_inline = 2,  //
    };

    Method_Inlining_Info method_inlining_info() const;

    void set_method_inlining_info( Method_Inlining_Info info );

    enum Block_Info {
        expects_nil       = 0,      // 'clean' block
        expects_self      = 1,      // 'copying' block
        expects_parameter = 2,      // 'copying' block
        expects_context   = 3       // 'full' block
    };

    Block_Info block_info() const;


    // Tells if an activation of this method has a context stored as temp 0.
    bool_t activation_has_context() const {
        return allocatesInterpretedContext() or ( is_blockMethod() and expectsContext() );
    }


    // Tells if byteCodeIndex is a context allocation
    bool_t in_context_allocation( std::int32_t byteCodeIndex ) const;


    bool_t containsNonLocalReturn() const {
        return isBitSet( flags(), containsNonLocalReturnFlag );
    }


    bool_t allocatesInterpretedContext() const {
        return isBitSet( flags(), allocatesContextFlag );
    }


    bool_t mustBeCustomizedToClass() const {
        return isBitSet( flags(), mustBeCustomizedFlag );
    }


    bool_t expectsContext() const {
        return block_info() == expects_context;
    }


    bool_t hasNestedBlocks() const;


    bool_t is_clean_block() const {
        return block_info() == expects_nil;
    }


    bool_t is_copying_block() const {
        return block_info() == expects_self or block_info() == expects_parameter;
    }


    bool_t is_full_block() const {
        return block_info() == expects_context;
    }


    // Method customization
    bool_t has_instVar_access() const {
        return true;
    } // for now - conservative - FIX THIS
    bool_t has_classVar_access() const {
        return true;
    } // for now - conservative - FIX THIS
    bool_t has_inlineCache() const {
        return true;
    } // for now - conservative - FIX THIS
    bool_t is_customized() const {
        return isBitSet( flags(), isCustomizedFlag );
    }


    bool_t should_be_customized() const {
        return has_instVar_access() or has_classVar_access() or has_inlineCache();
    }


    // Returns a deep copy of the methodOop
    MethodOop copy_for_customization() const;

    // Customize method to klass
    void customize_for( KlassOop klass, MixinOop mixin );

    void uncustomize_for( MixinOop mixin );

    // Uplevel accesses via contexts
    std::int32_t lexicalDistance( std::int32_t contextNo );    // for uplevel accesses; see comment in .c file
    std::int32_t contextNo( std::int32_t lexicalDistance );    // inverse of lexicalDistance()

    // Computes the number of interpreter contexts from here up to the home method
    std::int32_t context_chain_length() const;

    // Clears all the inline caches in the method.
    void clear_inline_caches();

    // Cleanup all inline caches
    void cleanup_inline_caches();

    // Computes the estimated cost of a method by summing
    // cost of all byte codes (see code_cost in methodOop.cpp).
    std::int32_t estimated_inline_cost( KlassOop receiverKlass );

    // Finds the byteCodeIndex based on the next byteCodeIndex
    // Returns -1 if the search failed.
    std::int32_t find_byteCodeIndex_from( std::int32_t nbyteCodeIndex ) const;

    // Returns the next byteCodeIndex
    std::int32_t next_byteCodeIndex( std::int32_t byteCodeIndex ) const;

    // Returns the end byteCodeIndex (excluding the padding)
    std::int32_t end_byteCodeIndex() const;

    // Returns the inline cache associated with the send at byteCodeIndex.
    InterpretedInlineCache *ic_at( std::int32_t byteCodeIndex ) const;

    // Returns an array of byte code indecies contributing to the expression stack
    GrowableArray<std::int32_t> *expression_stack_mapping( std::int32_t byteCodeIndex );

    // For debugging only
    void print_codes();

    void pretty_print();

    // Printing support
    void print_value_for( KlassOop receiver_klass, ConsoleOutputStream *stream = nullptr );

    // Inlining database support
    void print_inlining_database_on( ConsoleOutputStream *stream );

    std::int32_t byteCodeIndex_for_block_method( MethodOop inner );

    MethodOop block_method_at( std::int32_t byteCodeIndex );

    // Returns the numbers of temporaries allocated in a context.
    // self_in_context tells if self is copied into context.
    // Note: This function is extremely slow, so please use it for
    //       verify and debug code only.
    std::int32_t number_of_context_temporaries( bool_t *self_in_context = nullptr );

    // Checks if the context matches this method
    void verify_context( ContextOop con );

    // Query primitives
    ObjectArrayOop referenced_instance_variable_names( MixinOop mixin ) const;

    ObjectArrayOop referenced_class_variable_names() const;

    ObjectArrayOop referenced_global_names() const;

    ObjectArrayOop senders() const;

    friend class MethodKlass;
};


inline MethodOop as_methodOop( void *p ) {
    return MethodOop( as_memOop( p ) );
}


class StopInSelector : public ValueObject {
private:
    static bool_t ignored;
    bool_t        enable;
    bool_t        stop;
    FlagSetting   oldFlag;

public:
    StopInSelector( const char *class_name, const char *name, KlassOop klass, Oop method_or_selector, bool_t &fl = StopInSelector::ignored, bool_t stop = true );
};
