//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/code/LogicalAddress.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/memory/Array.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/code/PseudoRegisterMapping.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/utilities/disassembler.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/system/dll.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/runtime/StackChunkBuilder.hpp"
#include "vm/code/NameDescriptor.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/utilities/StringOutputStream.hpp"


void LocationNameDescriptor::print() {
    SPDLOG_INFO( "[@{} = {}]  0x{0:x}", location().name(), offset );

//    _console->print( "@%s  (0x%08x)", location().name(), offset );
}


void ValueNameDescriptor::print() {
    SPDLOG_INFO( "={}  0x{0:x}", value()->print_value_string(), offset );
//    _console->print( "=" );
//    value()->print_value();
//    _console->print( " (0x%08x)", offset );
}


void BlockValueNameDescriptor::print() {
    SPDLOG_INFO( "[={}]  0x{0:x}", block_method()->print_value_string(), offset );

//    _console->print( "[=" );
//    block_method()->print_value();
//    _console->print( "]" );
//    _console->print( " (0x%08x)", offset );
}


void MemoizedBlockNameDescriptor::print() {
    SPDLOG_INFO( "[@{} = {}]  0x{0:x}", location().name(), block_method()->print_value_string() );

//    _console->print( "[@%s =", location().name() );
//    block_method()->print_value();
//    _console->print( "]" );
//    _console->print( " (0x%08x)", offset );
}


void IllegalNameDescriptor::print() {
    SPDLOG_INFO( "###illegal###" );
    SPDLOG_INFO( " (0x{0:x})", offset );
//    _console->print( "###illegal###" );
//    _console->print( " (0x%08x)", offset );
}


bool LocationNameDescriptor::equal( NameDescriptor *other ) const {
    if ( other->isLocation() ) {
        return location() == ( (LocationNameDescriptor *) other )->location();
    }
    return false;
}


bool ValueNameDescriptor::equal( NameDescriptor *other ) const {
    if ( other->isValue() ) {
        return value() == ( (ValueNameDescriptor *) other )->value();
    }
    return false;
}


bool BlockValueNameDescriptor::equal( NameDescriptor *other ) const {
    if ( other->isBlockValue() ) {
        return block_method() == ( (BlockValueNameDescriptor *) other )->block_method() and parent_scope()->s_equivalent( ( (BlockValueNameDescriptor *) other )->parent_scope() );
    }
    return false;
}


bool MemoizedBlockNameDescriptor::equal( NameDescriptor *other ) const {
    if ( other->isMemoizedBlock() ) {
        return location() == ( (MemoizedBlockNameDescriptor *) other )->location() and block_method() == ( (MemoizedBlockNameDescriptor *) other )->block_method() and parent_scope()->s_equivalent( ( (MemoizedBlockNameDescriptor *) other )->parent_scope() );
    }
    return false;
}


bool IllegalNameDescriptor::equal( NameDescriptor *other ) const {
    return other->isIllegal();
}


extern "C" BlockClosureOop allocateBlock( SMIOop nofArgs );


Oop BlockValueNameDescriptor::value( const Frame *fr ) const {
    // create a block closure
    if ( MaterializeEliminatedBlocks or StackChunkBuilder::is_deoptimizing() ) {
        BlockClosureOop blk = BlockClosureOop( allocateBlock( smiOopFromValue( block_method()->number_of_arguments() ) ) );
        blk->set_method( block_method() );

        CompiledVirtualFrame *vf = CompiledVirtualFrame::new_vframe( fr, parent_scope(), 0 );
        blk->set_lexical_scope( vf->canonical_context() );
        return blk;
    } else {
        ResourceMark       resourceMark;
        StringOutputStream stream( 50 );
        stream.print( "eliminated [] in " );
        block_method()->home()->selector()->print_symbol_on( &stream );
        return oopFactory::new_symbol( stream.as_string() );
    }
}


Oop MemoizedBlockNameDescriptor::value( const Frame *fr ) const {
    // check if the block has been created
    CompiledVirtualFrame *vf = fr ? CompiledVirtualFrame::new_vframe( fr, parent_scope(), 0 ) : nullptr;

    Oop stored_value = CompiledVirtualFrame::resolve_location( location(), vf );
    if ( stored_value not_eq uncreatedBlockValue() ) {
        return stored_value;
    }

    // otherwise do the same as for a BlockValueNameDescriptor
    if ( MaterializeEliminatedBlocks or StackChunkBuilder::is_deoptimizing() ) {
        BlockClosureOop blk = BlockClosureOop( allocateBlock( smiOopFromValue( block_method()->number_of_arguments() ) ) );
        blk->set_method( block_method() );

        blk->set_lexical_scope( vf->canonical_context() );
        return blk;
    } else {
        ResourceMark       resourceMark;
        StringOutputStream stream( 50 );
        stream.print( "memoized [] in " );
        block_method()->home()->selector()->print_symbol_on( &stream );
        return oopFactory::new_symbol( stream.as_string() );
    }
}
