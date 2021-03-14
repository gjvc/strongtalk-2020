
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/oop/BlockClosureOopDescriptor.hpp"
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/oop/SmallIntegerOopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


class ContextOopDescriptor : public MemOopDescriptor {


private:
    SmallIntegerOop _parent;


    //
    // %note: Robert please describe the parent states in excruciating details.
    //        The description below is far from complete (Lars, 1/9/96).
    //
    // Contains either:
    //  - the frame   	(if the activation creating the block is alive and a first-level block)
    //  - smiOop_zero 	(when the activation creating the block is dead)
    //  - outer context 	(if the corresponding method is a block method??)
    //
    // The transition from frame to smiOop_zero happens when the block is zapped
    // by the epilog code of the method or a non local return.
    // NOTE: the frame is needed in case of a non local return.
    //

    ContextOop addr() const {
        return ContextOop( MemOopDescriptor::addr() );
    }


public:
    friend ContextOop as_contextOop( void *p );


    void set_parent( Oop h ) {
        STORE_OOP( &addr()->_parent, h );
    }


    Oop parent() const {
        return addr()->_parent;
    }


    // Test operations on home
    bool is_dead() const;

    bool has_parent_fp() const;

    bool has_outer_context() const;


    std::int32_t *parent_fp() const {
        return has_parent_fp() ? (std::int32_t *) parent() : nullptr;
    }


    void set_home_fp( std::int32_t *fp ) { // this should be void ** or similar to allow for 64-bit
        st_assert( Oop(fp)->isSmallIntegerOop(), "checking alignment" );
        set_parent( Oop( fp ) );
    }


    // Returns the outer context if any
    ContextOop outer_context() const;     // nullptr if is_dead or has_frame

    // Sets the home to smiOop_zero
    void kill() {
        set_parent( smiOop_zero );
    }


    static std::int32_t header_size() {
        return sizeof( ContextOopDescriptor ) / OOP_SIZE;
    }


    std::int32_t object_size() {
        return header_size() + length();
    }


    Oop *obj_addr_at( std::int32_t index ) {
        return oops( header_size() + index );
    }


    Oop obj_at( std::int32_t index ) {
        return raw_at( header_size() + index );
    }


    void obj_at_put( std::int32_t index, Oop value ) {
        raw_at_put( header_size() + index, value );
    }


    std::size_t length() {
        return mark()->hash() - 1;
    }


    // constants for code generation -- make this an enum
    static std::int32_t parent_word_offset();

    static std::int32_t temp0_word_offset();

    static std::int32_t parent_byte_offset();

    static std::int32_t temp0_byte_offset();



//
//    std::int32_t ContextOopDescriptor::parent_word_offset() {
//        return 2; // word offset of parent context
//    }
//
//
//    std::int32_t ContextOopDescriptor::temp0_word_offset() {
//        return 3; // word offset of first context temp
//    }
//
//
//    std::int32_t ContextOopDescriptor::parent_byte_offset() {
//        return byteOffset( parent_word_offset() );
//    }
//
//
//    std::int32_t ContextOopDescriptor::temp0_byte_offset() {
//        return byteOffset( temp0_word_offset() );
//    }



    // Accessors for storing and reading the forward reference
    // to the unoptimized context (Used during deoptimization).
    void set_unoptimized_context( ContextOop con );

    ContextOop unoptimized_context();

    // Returns the length of the context chain.
    std::int32_t chain_length() const;

    // Print the contents of home
    void print_home_on( ConsoleOutputStream *stream );

    friend class ContextKlass;
};


inline ContextOop as_contextOop( void *p ) {
    return ContextOop( as_memOop( p ) );
}


// Contexts contain the heap-allocated local variables of a method, i.e., the locals
// and arguments that are uplevel-accessed by blocks.  They are variable-length, what's
// shown below is just the common prefix which is followed by the words containing the
// actual data.
