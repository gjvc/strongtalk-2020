//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/memory/util.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/utilities/lprintf.hpp"


ProgramCounterDescriptor::ProgramCounterDescriptor( uint16_t pc, uint16_t scope, uint16_t byteCode ) {
    _pc       = pc;
    _scope         = scope;
    _byteCodeIndex = byteCode;
}


char * ProgramCounterDescriptor::real_pc( const NativeMethod * nm ) const {
    return nm->instructionsStart() + _pc;
}


ScopeDescriptor * ProgramCounterDescriptor::containingDesc( const NativeMethod * nm ) const {
    return nm->scopes()->at( _scope, real_pc( nm ) );
}


void ProgramCounterDescriptor::print( NativeMethod * nm ) {
    printIndent();
    lprintf( "ProgramCounterDescriptor %#lx: pc: %#lx; scope: %5ld; byte code: %ld\n", this, real_pc( nm ), _scope, ( int32_t ) _byteCodeIndex );
}


bool_t ProgramCounterDescriptor::verify( NativeMethod * nm ) {
    return true;
}
