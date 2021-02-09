
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
#include "vm/memory/OopFactory.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/compiler/BasicBlock.hpp"
#include "vm/primitives/primitives.hpp"


RelocationInformation::RelocationInformation( RelocationInformation::RelocationType t, std::int32_t offset ) :
    _value{} {

    st_assert( 0 <= static_cast<std::int32_t>(t) and static_cast<std::int32_t>(t) < ( 1 << reloc_type_width ), "wrong type" );
    st_assert( offset <= nthMask( reloc_offset_width ), "offset out of bounds" );

    _value = ( static_cast<std::int32_t>(t) << reloc_offset_width ) | offset;
}


std::int32_t RelocationInformation::print( NativeMethod *m, std::int32_t last_offset ) {

    if ( not isValid() )
        return 0;

    std::int32_t current_offset = offset() + last_offset;
    std::int32_t *addr          = (std::int32_t *) ( m->instructionsStart() + current_offset );
    printIndent();
    if ( isOop() ) {
        _console->print( "embedded Oop   @0x%lx = ", addr );
        Oop( *addr )->print_value();
    } else {
        st_assert( isCall(), "must be a call" );
        const char *target = (const char *) ( *addr + (std::int32_t) addr + OOP_SIZE );
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


RelocationInformationIterator::RelocationInformationIterator( const NativeMethod *nm ) :
    _address{ nullptr },
    _current{ nullptr },
    _end{ nullptr } {

    _address = nm->instructionsStart();
    _current = nm->locs() - 1;
    _end     = nm->locsEnd();
}


bool RelocationInformationIterator::wasUncommonTrapExecuted() const {
    st_assert( _current->isUncommonTrap(), "not an uncommon branch" );
    return callDestination() == (const char *) StubRoutines::used_uncommon_trap_entry();
}


bool RelocationInformationIterator::is_position_dependent() const {
    switch ( type() ) {
        case RelocationInformation::RelocationType::ic_type:
        case RelocationInformation::RelocationType::primitive_type:
        case RelocationInformation::RelocationType::uncommon_type:
        case RelocationInformation::RelocationType::runtime_call_type:
        case RelocationInformation::RelocationType::internal_word_type:
        case RelocationInformation::RelocationType::dll_type:
            return true;
        default:
            nullptr;
    }
    return false;
}
