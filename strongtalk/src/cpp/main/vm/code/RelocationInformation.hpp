//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/code/CompiledInlineCache.hpp"


// These hold enough information to read or write a value embedded
// in the instructions of an NativeMethod.  They're used to update:
//
//   1) embedded oops     (isOop()          == true)
//   2) inline caches     (isIC()           == true)
//   3) primitive caches  (isPrimitive()    == true)
//   4) uncommom traps    (isUncommonTrap() == true)
//   5) runtime calls     (isRuntimeCall()  == true)
//   6) internal word ref (isInternalWord() == true)
//   7) external word ref (isExternalWord() == true)
//
// when objects move (GC) or if code moves (compacting the code heap)
//
// A RelocationInformation is represented in 16 bits:
//   3 bits indicating the relocation type
//  13 bits indicating the byte offset from the previous RelocationInformation address

constexpr std::size_t reloc_type_width   = 3;
constexpr std::size_t reloc_offset_width = 13;


class RelocationInformation : ValueObject {

protected:
    std::uint16_t _value;

public:

    enum class RelocationType {
        none               = -1,    //
        oop_type           = 0,     // embedded Oop (non-smi_t)
        ic_type            = 1,     // inline cache
        primitive_type     = 2,     // primitive call
        runtime_call_type  = 3,     // Relative reference to external segment
        external_word_type = 4,     // Absolute reference to external segment
        internal_word_type = 5,     // Absolute reference to local segment
        uncommon_type      = 6,     // uncommon branch
        dll_type           = 7      // DLL call

    };

    RelocationInformation( RelocationInformation::RelocationType type, std::size_t offset );


    std::size_t offset() const {
        return get_unsigned_bitfield( (std::size_t) _value, 0, reloc_offset_width );
    }


    RelocationInformation::RelocationType type() const {
        return static_cast<RelocationInformation::RelocationType>( get_unsigned_bitfield( (std::size_t) _value, reloc_offset_width, reloc_type_width ) );
    }


    bool_t isInlineCache() const {
        return type() == RelocationInformation::RelocationType::ic_type;
    }


    bool_t isPrimitive() const {
        return type() == RelocationInformation::RelocationType::primitive_type;
    }


    bool_t isUncommonTrap() const {
        return type() == RelocationInformation::RelocationType::uncommon_type;
    }


    bool_t isOop() const {
        return type() == RelocationInformation::RelocationType::oop_type;
    }


    bool_t isRuntimeCall() const {
        return type() == RelocationInformation::RelocationType::runtime_call_type;
    }


    bool_t isInternalWord() const {
        return type() == RelocationInformation::RelocationType::internal_word_type;
    }


    bool_t isExternalWord() const {
        return type() == RelocationInformation::RelocationType::external_word_type;
    }


    bool_t isDLL() const {
        return type() == RelocationInformation::RelocationType::dll_type;
    }


    bool_t isCall() const {
        return not isOop();
    }


    // If the offset is 0 the RelocationInformation is invalid.
    // Only used for padding (RelocationInformation array is Oop aligned in NativeMethod).
    bool_t isValid() const {
        return offset() not_eq 0;
    }


    // prints the relocation with retrieved information from the NativeMethod.
    std::size_t print( NativeMethod *c, std::size_t last_offset );
};


CompiledInlineCache *CompiledIC_from_return_addr( const char *return_addr );

CompiledInlineCache *CompiledIC_from_relocInfo( const char *displacement_address );

PrimitiveInlineCache *PrimitiveIC_from_relocInfo( const char *displacement_address );

Compiled_DLLCache *compiled_DLLCache_from_relocInfo( const char *displacement_address );


// A RelocationInformationIterator iterates through the relocation information of a NativeMethod.
// Usage:
//    RelocationInformationIterator iter(nm);
//    while (iter.next()) {
//      switch (iter.type()) {
//       case RelocationInformation::RelocationType::oop_type:
//       case RelocationInformation::RelocationType::ic_type:
//       case RelocationInformation::RelocationType::primitive_type:
//       case RelocationInformation::RelocationType::uncommon_type:
//       case RelocationInformation::RelocationType::runtime_call_type:
//       case RelocationInformation::RelocationType::internal_word_type:
//       case RelocationInformation::RelocationType::external_word_type:
//      }
//    }

class RelocationInformationIterator : StackAllocatedObject {

private:
    char                  *_address;
    RelocationInformation *_current;
    RelocationInformation *_end;

public:

    RelocationInformationIterator( const NativeMethod *nm );


    bool_t next() {
        _current++;
        if ( _current == _end )
            return false;
        _address += _current->offset();
        return _current->isValid();
    }


    // accessors
    RelocationInformation::RelocationType type() const {
        return _current->type();
    }


    bool_t is_call() const {
        return _current->isCall();
    }


    std::size_t *word_addr() const {
        return (std::size_t *) _address;
    }


    Oop *oop_addr() const {
        st_assert( type() == RelocationInformation::RelocationType::oop_type, "must be Oop" );
        return (Oop *) _address;
    }


    CompiledInlineCache *ic() const {
        st_assert( type() == RelocationInformation::RelocationType::ic_type, "must be inline cache" );
        return CompiledIC_from_relocInfo( _address );
    }


    PrimitiveInlineCache *primIC() const {
        st_assert( type() == RelocationInformation::RelocationType::primitive_type, "must be inline cache" );
        return PrimitiveIC_from_relocInfo( _address );
    }


    Compiled_DLLCache *DLLInlineCache() const {
        st_assert( type() == RelocationInformation::RelocationType::dll_type, "must be inline cache" );
        return compiled_DLLCache_from_relocInfo( _address );
    }


    char *call_end() const {
        st_assert( type() not_eq RelocationInformation::RelocationType::oop_type, "must be call" );
        return _address + 4;    // INTEL-SPECIFIC
    }


    char *callDestination() const {
        st_assert( type() not_eq RelocationInformation::RelocationType::oop_type, "must be call" );
        return *(char **) _address + std::size_t( _address ) + 4;    // INTEL-SPECIFIC
    }


    // for uncommon traps only: was it ever executed?
    bool_t wasUncommonTrapExecuted() const;

    // is current reloc position dependent
    bool_t is_position_dependent() const;
};
