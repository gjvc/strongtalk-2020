
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/code/LogicalAddress.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/memory/Array.hpp"
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


RelocationInformation::RelocationInformation( RelocationInformation::RelocationType t, int off ) {

    st_assert( 0 <= static_cast<int>(t) and static_cast<int>(t) < ( 1 << reloc_type_width ), "wrong type" );
    st_assert( off <= nthMask( reloc_offset_width ), "offset out of bounds" );

    _value = ( static_cast<int>(t) << reloc_offset_width ) | off;
}


int RelocationInformation::print( NativeMethod *m, int last_offset ) {

    if ( not isValid() )
        return 0;

    int current_offset = offset() + last_offset;
    int *addr = (int *) ( m->instructionsStart() + current_offset );
    printIndent();
    if ( isOop() ) {
        _console->print( "embedded Oop   @0x%lx = ", addr );
        Oop( *addr )->print_value();
    } else {
        st_assert( isCall(), "must be a call" );
        const char *target = (const char *) ( *addr + (int) addr + oopSize );
        if ( isInlineCache() ) {
            _console->print( "inline cache   @0x%lx", addr );
        } else if ( isPrimitive() ) {
            _console->print( "primitive call @0x%lx = ", addr );
            PrimitiveDescriptor *pd = Primitives::lookup( (primitiveFunctionType) target );
            if ( pd not_eq nullptr ) {
                _console->print( "(%s)", pd->name() );
            } else {
                _console->print( "runtime routine" );
            }
        } else if ( isDLL() ) {
            _console->print( "DLL call @0x%lx = ", addr );
        } else if ( isUncommonTrap() ) {
            _console->print( "uncommon trap @0x%lx", addr );
        } else {
            _console->print( "run-time call @0x%lx", addr );
        }
    }
    return current_offset;
}


RelocationInformationIterator::RelocationInformationIterator( const NativeMethod *nm ) {
    _current = nm->locs() - 1;
    _end     = nm->locsEnd();
    _address = nm->instructionsStart();
}


bool_t RelocationInformationIterator::wasUncommonTrapExecuted() const {
    st_assert( _current->isUncommonTrap(), "not an uncommon branch" );
    return callDestination() == (const char *) StubRoutines::used_uncommon_trap_entry();
}


bool_t RelocationInformationIterator::is_position_dependent() const {
    switch ( type() ) {
        case RelocationInformation::RelocationType::ic_type:
        case RelocationInformation::RelocationType::primitive_type:
        case RelocationInformation::RelocationType::uncommon_type:
        case RelocationInformation::RelocationType::runtime_call_type:
        case RelocationInformation::RelocationType::internal_word_type:
        case RelocationInformation::RelocationType::dll_type:
            return true;
    }
    return false;
}
