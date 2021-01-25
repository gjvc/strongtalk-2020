
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/memory/Closure.hpp"


// A Frame represents a physical stack frame (an activation).  Frames can be
// C or Delta frames, and the Delta frames can be interpreted or compiled.
//
// In contrast, VirtualFrames represent source-level activations, so that one (Delta) frame
// can correspond to multiple deltaVFrames because of inlining.

// Layout of interpreter frame:
//    [locals + expr         ] * <- sp
//    [old frame pointer     ]   <- fp
//    [return pc             ]
//    [previous locals + expr]   <- sender sp

// Layout of deoptimized frame:
//    [&unpack_unoptimized_frame ]		// patched return address
//    [scrap area                ] * <- sp
//    [sender sp                 ]
//    [frame array               ]              // objArrayOop holding the deoptimized frames
//    [old frame                 ]   <- fp	// old frame may skip real frames deoptimized away.
//    [return pc                 ]

const std::int32_t frame_temp_offset          = -3; // For interpreter frames only
const std::int32_t frame_hp_offset            = -2; // For interpreter frames only
const std::int32_t frame_receiver_offset      = -1; // For interpreter frames only
const std::int32_t frame_next_Delta_fp_offset = -1; // For entry frames only; see call_delta in interpreter_asm.asm
const std::int32_t frame_next_Delta_sp_offset = -2; // For entry frames only; see call_delta in interpreter_asm.asm
const std::int32_t frame_link_offset          = 0;
const std::int32_t frame_return_addr_offset   = 1;
const std::int32_t frame_arg_offset           = 2;
const std::int32_t frame_sender_sp_offset     = 2;

const std::int32_t frame_real_sender_sp_offset = -2; // For deoptimized frames only
const std::int32_t frame_frame_array_offset    = -1; // For deoptimized frames only

const std::int32_t interpreted_frame_float_magic_offset = frame_temp_offset - 1;
const std::int32_t compiled_frame_magic_oop_offset      = -1;
const std::int32_t minimum_size_for_deoptimized_frame   = 4;

class InlineCacheIterator;

class InterpretedInlineCache;

class CompiledInlineCache;

class Compiled_DLLCache;

class NativeMethod;

class Frame : ValueObject {

private:
    Oop        *_sp; // stack pointer
    std::int32_t        *_fp; // frame pointer - %TODO should be void ** or similar to allow for 64 bit
    const char *_pc; // program counter

public:
    Frame() {
    }


    Frame( Oop *sp, std::int32_t *fp, const char *pc ) {
        _sp = sp;
        _fp = fp;
        _pc = pc;
    }


    Frame( Oop *sp, std::int32_t *fp ) {
        _sp = sp;
        _fp = fp;
        _pc = (const char *) sp[ -1 ];
    }


    Oop *sp() const {
        return _sp;
    }


    std::int32_t *fp() const {
        return _fp;
    } // should return void **

    const char *pc() const {
        return _pc;
    }


    // patching operations
    void patch_pc( const char *pc ); // patch the return address of the frame below.
    void patch_fp( std::int32_t *fp ); // patch the link of the frame below.

    std::int32_t *addr_at( std::int32_t index ) const {
        return &fp()[ index ];
    } // should return void **

    std::int32_t at( std::int32_t index ) const {
        return *addr_at( index );
    } // should really return void *

private:
    std::int32_t **link_addr() const {
        return (std::int32_t **) addr_at( frame_link_offset );
    }


    const char **return_addr_addr() const {
        return (const char **) addr_at( frame_return_addr_offset );
    }


    // support for interpreter frames
    Oop *receiver_addr() const {
        return (Oop *) addr_at( frame_receiver_offset );
    }


    std::uint8_t **hp_addr() const {
        return (std::uint8_t **) addr_at( frame_hp_offset );
    }


    Oop *arg_addr( std::int32_t off ) const {
        return (Oop *) addr_at( frame_arg_offset + off );
    }


public:
    // returns the stack pointer of the calling frame
    Oop *sender_sp() const {
        return (Oop *) addr_at( frame_sender_sp_offset );
    }


    // Link
    std::int32_t *link() const {
        return *link_addr();
    }


    void set_link( std::int32_t *addr ) {
        *link_addr() = addr;
    }


    // Return address
    const char *return_addr() const {
        return *return_addr_addr();
    }


    void set_return_addr( const char *addr ) {
        *return_addr_addr() = addr;
    }


    // Receiver
    Oop receiver() const {
        return *receiver_addr();
    }


    void set_receiver( Oop recv ) {
        *receiver_addr() = recv;
    }


    // Temporaries
    Oop temp( std::int32_t offset ) const {
        return *temp_addr( offset );
    }


    void set_temp( std::int32_t offset, Oop obj ) {
        *temp_addr( offset ) = obj;
    }


    Oop *temp_addr( std::int32_t offset ) const {
        return (Oop *) addr_at( frame_temp_offset - offset );
    }


    // Arguments
    Oop arg( std::int32_t offset ) const {
        return *arg_addr( offset );
    }


    void set_arg( std::int32_t offset, Oop obj ) {
        *arg_addr( offset ) = obj;
    }


    // Expressions
    Oop expr( std::int32_t index ) const {
        return ( (Oop *) sp() )[ index ];
    }


    // Hybrid Code Pointer (interpreted frames only); corresponds to "current PC", not return address
    std::uint8_t *hp() const;

    void set_hp( std::uint8_t *hp );

    // Returns the method for a valid hp() or nullptr if frame not set up yet (interpreted frames only)
    // Used by the profiler which means we must check for
    // valid frame before using the hp value.
    MethodOop method() const;

    // compiled code (compiled frames only)
    NativeMethod *code() const;

private:
    // Float support
    inline bool_t has_interpreted_float_marker() const;

    bool_t oop_iterate_interpreted_float_frame( OopClosure *blk );

    bool_t follow_roots_interpreted_float_frame();

    inline bool_t has_compiled_float_marker() const;

    bool_t oop_iterate_compiled_float_frame( OopClosure *blk );

    bool_t follow_roots_compiled_float_frame();

public:

    // Accessors for (deoptimized frames only)
    ObjectArrayOop *frame_array_addr() const;

    Oop **real_sender_sp_addr() const;

    ObjectArrayOop frame_array() const;


    void set_frame_array( ObjectArrayOop a ) {
        *frame_array_addr() = a;
    }


    Oop *real_sender_sp() const {
        return *real_sender_sp_addr();
    }


    void set_real_sender_sp( Oop *addr ) {
        *real_sender_sp_addr() = addr;
    }


    // returns the frame size in oops
    std::int32_t frame_size() const {
        return sender_sp() - sp();
    }


    // returns the the sending frame
    Frame sender() const;

    // returns the the sending Delta frame, skipping any intermediate C frames
    // NB: receiver must not be first frame
    Frame delta_sender() const;

    // tells whether there is another chunk of Delta stack above (entry frames only)
    bool_t has_next_Delta_fp() const;

    // returns the next C entry frame (entry frames only)
    std::int32_t *next_Delta_fp() const;

    Oop *next_Delta_sp() const;

    bool_t is_first_frame() const;            // oldest frame? (has no sender)
    bool_t is_first_delta_frame() const;        // same for Delta frame

    // testers
    bool_t is_interpreted_frame() const;

    bool_t is_compiled_frame() const;


    bool_t is_delta_frame() const {
        return is_interpreted_frame() or is_compiled_frame();
    }


    bool_t should_be_deoptimized() const;

    bool_t is_entry_frame() const;        // Delta frame called from C?
    bool_t is_deoptimized_frame() const;

    // inline caches
    InlineCacheIterator *sender_ic_iterator() const;    // sending InlineCache (nullptr if entry frame or if a perform rather than a send)
    InlineCacheIterator *current_ic_iterator() const;    // current InlineCache (will break if not at a send or perform)
    InterpretedInlineCache *current_interpretedIC() const;    // current InlineCache in this frame; nullptr if not is_interpreted_frame
    CompiledInlineCache *current_compiledIC() const;    // current InlineCache in this frame; nullptr if not is_compiled_frame

    // Iterators
    void oop_iterate( OopClosure *blk );

    void layout_iterate( FrameLayoutClosure *blk );

    // For debugging
private:
    const char *print_name() const;

public:
    void verify() const;

    void print() const;

    // Prints the frame in a format useful when debugging deoptimization.
    void print_for_deoptimization( ConsoleOutputStream *stream );

    // Garbage collection operations
    void follow_roots();

    void convert_heap_code_pointer();

    void restore_heap_code_pointer();


    // Returns the size of a number of interpreter frames in words.
    // This is used during deoptimization.
    static std::int32_t interpreter_stack_size( std::int32_t number_of_frames, std::int32_t number_of_temporaries_and_locals ) {
        return number_of_frames * interpreter_frame_size( 0 ) + number_of_temporaries_and_locals;
    }


    // Returns the word size of an interpreter frame
    static std::int32_t interpreter_frame_size( std::int32_t locals ) {
        return frame_return_addr_offset - frame_temp_offset + locals;
    }
};
