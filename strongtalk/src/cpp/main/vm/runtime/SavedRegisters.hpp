//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/assembler/MacroAssembler.hpp"



// SavedRegisters is used for storing the values of data registers
// when entering the vm at special entry points like uncommon_trap
// where the current scope has values in the registers.
//
// Note: stack pointer and frame pointer are not saved - all other
//       (user) register are saved and their value is not destroyed.

class SavedRegisters : AllStatic {

    public:
        static Oop fetch( int register_number, int * frame_pointer );

        static void clear();

//  static void save_registers();
        static void generate_save_registers( MacroAssembler * masm );

};
