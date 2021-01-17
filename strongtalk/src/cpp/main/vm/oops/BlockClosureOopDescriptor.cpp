
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/primitives/block_primitives.hpp"
#include "vm/runtime/StackChunkBuilder.hpp"
#include "vm/oops/BlockClosureKlass.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


// Computes the byte offset from the beginning of an Oop
static inline int byteOffset( int offset ) {
    st_assert( offset >= 0, "bad offset" );
    return offset * sizeof( Oop ) - MEMOOP_TAG;
}


MethodOop BlockClosureOopDescriptor::method() const {
    Oop m = addr()->_methodOrJumpAddr;

    if ( isCompiledBlock() ) {
        JumpTableEntry * e = ( JumpTableEntry * ) m;
        st_assert( e->is_block_closure_stub(), "must be block stub" );
        return e->block_method();
    }

    return MethodOop( m );
}


int BlockClosureOopDescriptor::number_of_arguments() {
    return ( ( BlockClosureKlass * ) klass() )->number_of_arguments();
}


bool_t BlockClosureOopDescriptor::is_pure() const {
    return not lexical_scope()->is_context();
}


void BlockClosureOopDescriptor::verify() {
    MemOopDescriptor::verify();
    Oop m = addr()->_methodOrJumpAddr;
    if ( isCompiledBlock() ) {
        JumpTableEntry * e = ( JumpTableEntry * ) m;
        e->verify();
        if ( not e->is_block_closure_stub() )
            error( "stub %#x of block %#x isn't a closure stub", e, this );
    } else {
        m->verify();
    }
}


BlockClosureOop BlockClosureOopDescriptor::create_clean_block( int nofArgs, const char * entry_point ) {
    BlockClosureOop blk = allocateTenuredBlock( smiOopFromValue( nofArgs ) );
    blk->set_lexical_scope( ( ContextOop ) nilObj );
    blk->set_jumpAddr( entry_point );
    return blk;
}


void BlockClosureOopDescriptor::deoptimize() {
    if ( not isCompiledBlock() )
        return; // do nothing if unoptimized

    ContextOop con = lexical_scope();
    if ( con == nilObj )
        return;     // do nothing if lexical scope is nil

    int index;
    NativeMethod                   * nm    = jump_table_entry()->parent_nativeMethod( index );
    NonInlinedBlockScopeDescriptor * scope = nm->noninlined_block_scope_at( index );

    LOG_EVENT1( "Deoptimized context in blockClosure -> switch to methodOop [0x%lx]", nm );
    st_assert( nm, "NativeMethod must be present" );

    st_assert( not StackChunkBuilder::is_deoptimizing(), "you cannot be in deoptimization mode" );
    StackChunkBuilder::begin_deoptimization();

    // Patch the block closure to unoptimized form
    set_method( scope->method() );
    con = CompiledVirtualFrame::compute_canonical_context( scope->parent(), nullptr, con );
    set_lexical_scope( con );

    StackChunkBuilder::end_deoptimization();
}


JumpTableEntry * BlockClosureOopDescriptor::jump_table_entry() const {
    st_assert( isCompiledBlock(), "must be compiled" );
    return ( JumpTableEntry * ) addr()->_methodOrJumpAddr;
}
