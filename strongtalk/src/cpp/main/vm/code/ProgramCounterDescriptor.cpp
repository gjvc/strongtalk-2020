//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/util.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"


ProgramCounterDescriptor::ProgramCounterDescriptor( std::uint16_t pc, std::uint16_t scope, std::uint16_t byteCode ) {
    _pc            = pc;
    _scope         = scope;
    _byteCodeIndex = byteCode;
}


char *ProgramCounterDescriptor::real_pc( const NativeMethod *nm ) const {
    return nm->instructionsStart() + _pc;
}


ScopeDescriptor *ProgramCounterDescriptor::containingDesc( const NativeMethod *nm ) const {
    return nm->scopes()->at( _scope, real_pc( nm ) );
}


void ProgramCounterDescriptor::print( NativeMethod *nm ) {
    printIndent();
    spdlog::info( "ProgramCounterDescriptor {0:x}: pc: 0x{0:x}; scope: %5ld; byte code: %ld", static_cast<void *>(this), real_pc( nm ), _scope, (std::int32_t) _byteCodeIndex );
}


bool ProgramCounterDescriptor::verify( NativeMethod *nm ) {
    static_cast<void>(nm); // unused
    return true;
}
