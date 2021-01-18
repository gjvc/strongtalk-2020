//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"


// ProgramCounterDescriptor maps a physical PC (given as offset from start of NativeMethod) to the corresponding source scope and byte code index.

class ProgramCounterDescriptor : public ValueObject {

    public:
        std::uint16_t _pc;               // offset from start of method (could be std::uint16_t)
        std::uint16_t _scope;            // scope index
        int16_t  _byteCodeIndex;    // can be negative (PrologueByteCodeIndex et al)

    public:
        // Constructor (only used for static in NativeMethod.cpp)
        ProgramCounterDescriptor( std::uint16_t pc, std::uint16_t scope, std::uint16_t byteCode );

        // Returns the scope matching this pc desc
        ScopeDescriptor * containingDesc( const NativeMethod * nm ) const;

        // Returns the real pc
        char * real_pc( const NativeMethod * nm ) const;


        bool_t equals( const ProgramCounterDescriptor * other ) const {
            return _pc == other->_pc and _scope == other->_scope and _byteCodeIndex == other->_byteCodeIndex;
        }


        bool_t operator==( const ProgramCounterDescriptor & rhs ) const {
            return ( _pc == rhs._pc ) and ( _scope == rhs._scope ) and ( _byteCodeIndex == rhs._byteCodeIndex );
        }


        bool_t source_equals( const ProgramCounterDescriptor * other ) const {
            return _scope == other->_scope and _byteCodeIndex == other->_byteCodeIndex;
        }


        bool_t is_prologue() const {
            return _byteCodeIndex == PrologueByteCodeIndex;
        }


        bool_t is_epilogue() const {
            return _byteCodeIndex == EpilogueByteCodeIndex;
        }


        void print( NativeMethod * nm );

        bool_t verify( NativeMethod * nm );
};
