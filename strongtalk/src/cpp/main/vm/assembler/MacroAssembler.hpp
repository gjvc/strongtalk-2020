//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/assembler/Assembler.hpp"
#include "vm/assembler/CodeBuffer.hpp"


// MacroAssembler extends Assembler by a few macros used for generating the interpreter and for compiled code.

class MacroAssembler : public Assembler {

    public:
        MacroAssembler( CodeBuffer * code );


        // Alignment
        void align( int modulus );

        // Test-Instructions optimized for length
        void test( Register dst, int imm8 );        // use testb if possible, testl otherwise

        // Stack frame operations
        void enter();

        void leave();

        // Support for inlined data
        void inline_oop( Oop o );

        // C calls
        void set_last_Delta_frame_before_call();    // assumes that the return address has not been pushed yet
        void set_last_Delta_frame_after_call();    // assumes that the return address has been pushed already
        void reset_last_Delta_frame();

        void call_C( const Label & L );

        void call_C( const Label & L, const Label & nlrTestPoint );

        void call_C( const char * entry, RelocationInformation::RelocationType rtype );

        void call_C( const char * entry, RelocationInformation::RelocationType rtype, Label & nlrTestPoint );

        void call_C( const Register & entry );

        void call_C( const Register & entry, const Label & nlrTestPoint );

        // C calls to run-time routines with arguments (args are not preserved)
        void call_C( const char * entry, const Register &arg1 );

        void call_C( const char * entry, const Register &arg1, const Register &arg2 );

        void call_C( const char * entry, const Register &arg1, const Register &arg2, const Register &arg3 );

        void call_C( const char * entry, const Register &arg1, const Register &arg2, const Register &arg3, const Register &arg4 );

        // Stores
        void store_check( Register obj, Register tmp );

        // Floating-point comparisons
        // To jump conditionally on cc, test FPU status word with mask and
        // jump conditionally using cond.
        static void fpu_mask_and_cond_for( Condition cc, int & mask, Condition & cond );

        // Pop ST (ffree & fincstp combined)
        void fpop();

        // debugging
        static void print_reg( const char * name, Oop obj );

        static void inspector( Oop edi, Oop esi, Oop ebp, Oop esp, Oop ebx, Oop edx, Oop ecx, Oop eax, char * eip );

        void inspect( const char * title = nullptr );
};

extern MacroAssembler * theMacroAssembler;
