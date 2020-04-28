
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/VirtualFrame.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/oops/BlockClosureKlass.hpp"
#include "vm/primitives/block_primitives.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


// Computes the byte offset from the beginning of an Oop
static inline int byteOffset( int offset ) {
    st_assert( offset >= 0, "bad offset" );
    return offset * sizeof( Oop ) - MEMOOP_TAG;
}


bool_t ContextOopDescriptor::is_dead() const {
    st_assert( not mark()->has_context_forward(), "checking if context is deoptimized" );
    return parent() == Oop( smiOop_zero ) or parent() == nilObj;
}


bool_t ContextOopDescriptor::has_parent_fp() const {
    st_assert( not mark()->has_context_forward(), "checking if context is deoptimized" );
    return parent()->is_smi() and not is_dead();
}


bool_t ContextOopDescriptor::has_outer_context() const {
    st_assert( not mark()->has_context_forward(), "checking if context is deoptimized" );
    return parent()->is_context();
}


ContextOop ContextOopDescriptor::outer_context() const {
    if ( has_outer_context() ) {
        ContextOop con = ContextOop( parent() );
        st_assert( con->is_context(), "must be context" );
        return con;
    }
    return nullptr;
}


void ContextOopDescriptor::set_unoptimized_context( ContextOop con ) {
    st_assert( not mark()->has_context_forward(), "checking if context is deoptimized" );
    st_assert( this not_eq con, "Checking for forward cycle" );
    set_parent( con );
    set_mark( mark()->set_context_forward() );
}


ContextOop ContextOopDescriptor::unoptimized_context() {
    if ( mark()->has_context_forward() ) {
        ContextOop con = ContextOop( parent() );
        st_assert( con->is_context(), "must be context" );
        return con;
    }
    return nullptr;
}


int ContextOopDescriptor::chain_length() const {

    int                        size   = 1;
    GrowableArray <ContextOop> * path = new GrowableArray <ContextOop>( 10 );

    for ( ContextOop cc = ContextOop( this ); cc->has_outer_context(); cc = cc->outer_context() ) {
        st_assert( path->find( cc ) < 0, "cycle detected in context chain" )
        path->append( cc );
    }

    for ( ContextOop con = ContextOop( this ); con->has_outer_context(); con = con->outer_context() ) {
        size++;
    }
    return size;
}


void ContextOopDescriptor::print_home_on( ConsoleOutputStream * stream ) {
    if ( mark()->has_context_forward() ) {
        stream->print( "deoptimized to (" );
        unoptimized_context()->print_value();
        stream->print( ")" );
    } else if ( has_parent_fp() ) {
        stream->print( "frame 0x%lx", parent_fp() );
    } else if ( has_outer_context() ) {
        stream->print( "outer context " );
        outer_context()->print_value_on( stream );
    } else {
        st_assert( is_dead(), "context must be dead" );
        stream->print( "dead" );
    }
}


int ContextOopDescriptor::parent_word_offset() {
    return 2; // word offset of parent context
}


int ContextOopDescriptor::temp0_word_offset() {
    return 3; // word offset of first context temp
}


int ContextOopDescriptor::parent_byte_offset() {
    return byteOffset( parent_word_offset() );
}


int ContextOopDescriptor::temp0_byte_offset() {
    return byteOffset( temp0_word_offset() );
}
