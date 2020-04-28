//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/assembler/CodeBuffer.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/assembler/Address.hpp"
#include "vm/assembler/x86_registers.hpp"
#include "vm/runtime/ResourceObject.hpp"


class Assembler : public ResourceObject {

    protected:
        CodeBuffer * _code;

        const char * _code_begin;     // first byte of code buffer
        const char * _code_limit;     // first byte after code buffer
        const char * _code_pos;       // current code generation position

        Label _unbound_label;   // the last label to be bound to _binding_pos, if unbound
        int   _binding_pos;     // the position to which _unbound_label has to be bound, if there

        const char * addr_at( int pos ) {
            return _code_begin + pos;
        }


        int byte_at( int pos ) {
            return *( uint8_t * ) addr_at( pos );
        }


        void byte_at_put( int pos, int x ) {
            *( uint8_t * ) addr_at( pos ) = ( uint8_t ) x;
        }


        int long_at( int pos ) {
            return *( int * ) addr_at( pos );
        }


        void long_at_put( int pos, int x ) {
            *( int * ) addr_at( pos ) = x;
        }


        bool_t is8bit( int x ) {
            return -0x80 <= x and x < 0x80;
        }


        bool_t isByte( int x ) {
            return 0 <= x and x < 0x100;
        }


        bool_t isShiftCount( int x ) {
            return 0 <= x and x < 32;
        }


        void emit_byte( int x );

        void emit_long( int x );

        void emit_data( int data, RelocationInformation::RelocationType rtype );

        void emit_arith_b( int op1, int op2, Register dst, int imm8 );

        void emit_arith( int op1, int op2, Register dst, int imm32 );

        void emit_arith( int op1, int op2, Register dst, Oop obj );

        void emit_arith( int op1, int op2, Register dst, Register src );

        void emit_operand( Register reg, Register base, Register index, Address::ScaleFactor scale, int disp, RelocationInformation::RelocationType rtype );

        void emit_operand( Register r, Address a );

        void emit_farith( int b1, int b2, int i );

        void print( Label & L );

        void bind_to( Label & L, int pos );

        void link_to( Label & L, Label & appendix );

    public:
        enum class Condition {
            zero         = 0x4, //
            notZero      = 0x5, //
            equal        = 0x4, //
            notEqual     = 0x5, //
            less         = 0xc, //
            lessEqual    = 0xe, //
            greater      = 0xf, //
            greaterEqual = 0xd, //
            below        = 0x2, //
            belowEqual   = 0x6, //
            above        = 0x7, //
            aboveEqual   = 0x3, //
            overflow     = 0x0, //
            noOverflow   = 0x1, //
            carrySet     = 0x2, //
            carryClear   = 0x3, //
            negative     = 0x8, //
            positive     = 0x9, //
        };

        enum Constants {
            sizeOfCall = 5            // length of call instruction in bytes
        };

        Assembler( CodeBuffer * code );

        void finalize();        // call this before using/copying the code
        CodeBuffer * code() const {
            return _code;
        }


        const char * pc() const {
            return _code_pos;
        }


        int offset() const {
            return _code_pos - _code_begin;
        }


        // Stack
        void pushad();

        void popad();

        void pushl( int imm32 );

        void pushl( Oop obj );

        void pushl( Register src );

        void pushl( Address src );

        void popl( Register dst );

        void popl( Address dst );

        // Moves
        void movb( Register dst, Address src );

        void movb( Address dst, int imm8 );

        void movb( Address dst, Register src );

        void movw( Register dst, Address src );

        void movw( Address dst, Register src );

        void movl( Register dst, int imm32 );

        void movl( Register dst, Oop obj );

        void movl( Register dst, Register src );

        void movl( Register dst, Address src );

        void movl( Address dst, int imm32 );

        void movl( Address dst, Oop obj );

        void movl( Address dst, Register src );

        void movsxb( Register dst, Address src );

        void movsxb( Register dst, Register src );

        void movsxw( Register dst, Address src );

        void movsxw( Register dst, Register src );

        // Conditional moves (P6 only)
        void cmovccl( Condition cc, Register dst, int imm32 );

        void cmovccl( Condition cc, Register dst, Oop obj );

        void cmovccl( Condition cc, Register dst, Register src );

        void cmovccl( Condition cc, Register dst, Address src );

        // Arithmetics
        void adcl( Register dst, int imm32 );

        void adcl( Register dst, Register src );

        void addl( Address dst, int imm32 );

        void addl( Register dst, int imm32 );

        void addl( Register dst, Register src );

        void addl( Register dst, Address src );

        void andl( Register dst, int imm32 );

        void andl( Register dst, Register src );

        void cmpl( Address dst, int imm32 );

        void cmpl( Address dst, Oop obj );

        void cmpl( Register dst, int imm32 );

        void cmpl( Register dst, Oop obj );

        void cmpl( Register dst, Register src );

        void cmpl( Register dst, Address src );

        void decb( Register dst );

        void decl( Register dst );

        void decl( Address dst );

        void idivl( Register src );

        void imull( Register src );

        void imull( Register dst, Register src );

        void imull( Register dst, Register src, int value );

        void incl( Register dst );

        void incl( Address dst );

        void leal( Register dst, Address src );

        void mull( Register src );

        void negl( Register dst );

        void notl( Register dst );

        void orl( Register dst, int imm32 );

        void orl( Register dst, Register src );

        void orl( Register dst, Address src );

        void rcll( Register dst, int imm8 );

        void sarl( Register dst, int imm8 );

        void sarl( Register dst );

        void sbbl( Register dst, int imm32 );

        void sbbl( Register dst, Register src );

        void shldl( Register dst, Register src );

        void shll( Register dst, int imm8 );

        void shll( Register dst );

        void shrdl( Register dst, Register src );

        void shrl( Register dst, int imm8 );

        void shrl( Register dst );

        void subl( Register dst, int imm32 );

        void subl( Register dst, Register src );

        void subl( Register dst, Address src );

        void testb( Register dst, int imm8 );

        void testl( Register dst, int imm32 );

        void testl( Register dst, Register src );

        void xorl( Register dst, int imm32 );

        void xorl( Register dst, Register src );

        // Miscellaneous
        void cdq();

        void hlt();

        void int3();

        void nop();

        void ret( int imm16 = 0 );

        // Labels

        void bind( Label & L );            // binds an unbound label L to the current code position
        void merge( Label & L, Label & with );    // merges L and with, L is the merged label

        // Calls
        void call( Label & L );

        void call( const char * entry, RelocationInformation::RelocationType rtype );

        void call( Register reg );

        void call( Address adr );

        // Jumps
        void jmp( const char * entry, RelocationInformation::RelocationType rtype );

        void jmp( Register reg );

        void jmp( Address adr );

        // Label operations & relative jumps (PPUM Appendix D)
        void jmp( Label & L );        // unconditional jump to L

        // jccI is the generic conditional branch generator to run-time routines, while jcc is used for branches to labels.
        // jcc takes a branch opcode (cc) and a label (L) and generates either a backward branch or a forward branch and links it to the label fixup chain.
        //
        // Usage:
        //
        // Label L;		// unbound label
        // jcc(cc, L);	// forward branch to unbound label
        // bind(L);		// bind label to the current pc
        // jcc(cc, L);	// backward branch to bound label
        // bind(L);		// illegal: a label may be bound only once
        //
        // Note: The same Label can be used for forward and backward branches but it may be bound only once.
        //

        void jcc( Condition cc, const char * dst, RelocationInformation::RelocationType rtype = RelocationInformation::RelocationType::runtime_call_type );

        void jcc( Condition cc, Label & L );

        // Support for inline cache information (see also InlineCacheInfo)
        void ic_info( Label & L, int flags );

        // Floating-point operations
        void fld1();

        void fldz();

        // %note: _s 32 bits, _d 64 bits
        void fld_s( Address a );

        void fld_d( Address a );

        void fstp_s( Address a );

        void fstp_d( Address a );

        void fild_s( Address a );

        void fild_d( Address a );

        void fistp_s( Address a );

        void fistp_d( Address a );

        void fabs();

        void fchs();

        void fadd_d( Address a );

        void fsub_d( Address a );

        void fmul_d( Address a );

        void fdiv_d( Address a );

        void fadd( int i );

        void fsub( int i );

        void fmul( int i );

        void fdiv( int i );

        void faddp( int i = 1 );

        void fsubp( int i = 1 );

        void fsubrp( int i = 1 );

        void fmulp( int i = 1 );

        void fdivp( int i = 1 );

        void fprem();

        void fprem1();

        void fxch( int i = 1 );

        void fincstp();

        void ffree( int i = 0 );

        void ftst();

        void fcompp();

        void fnstsw_ax();

        void fwait();


        // For compatibility with old assembler only - should be removed at some point
        void Load( Register base, int disp, Register dst ) {
            movl( dst, Address( base, disp ) );
        }


        void Store( Register src, Register base, int disp ) {
            movl( Address( base, disp ), src );
        }

};
