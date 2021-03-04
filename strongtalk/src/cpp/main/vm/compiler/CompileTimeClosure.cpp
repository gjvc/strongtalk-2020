//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/CompileTimeClosure.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/compiler/Compiler.hpp"


bool CompileTimeClosure::verify() const {
    bool ok;
    if ( not _method->is_method() ) {
        ok = false;
        error( "CompileTimeClosure 0x{0:x}: Oop 0x{0:x} is not a method", this, _method );
    }
    return ok and _method->verify();
}


const char *CompileTimeClosure::jump_table_entry() {
    st_assert( _id.is_valid(), "must have valid ID" );
    JumpTableEntry *entry = Universe::code->jump_table()->at( _id );
    st_assert( entry->is_block_closure_stub(), "must be block" );
    return entry->entry_point();
}


NonInlinedBlockScopeNode *CompileTimeClosure::noninlined_block_scope() {
    st_assert( _noninlined_block_scope not_eq nullptr, "debug info not generated" );
    return _noninlined_block_scope;
}


void CompileTimeClosure::generateDebugInfo() {
    st_assert( _noninlined_block_scope == nullptr, "debug info generated twice" );
    _noninlined_block_scope = theCompiler->scopeDescRecorder()->addNonInlinedBlockScope( method(), parent_scope()->getScopeInfo() );
}


void CompileTimeClosure::print() {
    SPDLOG_INFO( "(CompileTimeClosure*)0x{0:x} for method 0x{0:x}: ", static_cast<void *>( this ), static_cast<void *>( _method ) );
    _method->pretty_print();
}
