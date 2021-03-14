//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/NativeInstruction.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/assembler/Assembler.hpp"
#include "vm/assembler/MacroAssembler.hpp"

//
// Displacement describes the 32bit immediate field of an instruction which
// may be used together with a Label in order to refer to a yet unknown code
// position.
//
// Displacements stored in the instruction stream are used to describe
// the instruction and to chain a list of instructions using the same Label.
// Displacement contains 3 different fields:
//
// next field: position of next displacement in the chain (0 = end of list)
// type field: instruction type
// info field: instruction specific information
//
// A next value of null (0) indicates the end of a chain (note that there can
// be no displacement at position zero, because there is always at least one
// instruction byte before the displacement).
//
// Displacement _data field layout
//
// |31....10|9......8|7......0|
// [  next  |  type  |  info  ]
//

class Displacement : public ValueObject {

private:
    std::int32_t _data{};

    static constexpr std::int32_t info_size = InlineCacheInfo::number_of_flags;   //
    static constexpr std::int32_t type_size = 2;                                  //
    static constexpr std::int32_t next_size = 32 - ( type_size + info_size );     //
    static constexpr std::int32_t info_pos  = 0;                                  //
    static constexpr std::int32_t type_pos  = info_pos + info_size;               //
    static constexpr std::int32_t next_pos  = type_pos + type_size;               //
    static constexpr std::int32_t info_mask = ( 1 << info_size ) - 1;             //
    static constexpr std::int32_t type_mask = ( 1 << type_size ) - 1;             //
    static constexpr std::int32_t next_mask = ( 1 << next_size ) - 1;             //

    enum class Type {             // info field usage
        call,               // unused
        absolute_jump,      // unused
        conditional_jump,   // condition code
        ic_info,            // flags
    };


    void init( const Label &L, Type type, std::int32_t info );

    std::int32_t data() const;

    std::int32_t info() const;

    Type type() const;

    void next( Label &L ) const;

    void link_to( Label &L );

    Displacement( std::int32_t data );

    Displacement( const Label &L, Type type, std::int32_t info );

    void print();

    friend class Assembler;

    friend class MacroAssembler;
};
