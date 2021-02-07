
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
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
    CodeBuffer *_code;

    const char *_code_begin;     // first byte of code buffer
    const char *_code_limit;     // first byte after code buffer
    const char *_code_pos;       // current code generation position

    Label        _unbound_label;   // the last label to be bound to _binding_pos, if unbound
    std::int32_t _binding_pos;     // the position to which _unbound_label has to be bound, if there

    const char *addr_at( std::int32_t pos ) {
        return _code_begin + pos;
    }


    std::int32_t byte_at( std::int32_t pos ) {
        return *(std::uint8_t *) addr_at( pos );
    }


    void byte_at_put( std::int32_t pos, std::int32_t x ) {
        *(std::uint8_t *) addr_at( pos ) = (std::uint8_t) x;
    }


    std::int32_t long_at( std::int32_t pos ) {
        return *(std::int32_t *) addr_at( pos );
    }


    void long_at_put( std::int32_t pos, std::int32_t x ) {
        *(std::int32_t *) addr_at( pos ) = x;
    }


    bool is8bit( std::int32_t x ) {
        return -0x80 <= x and x < 0x80;
    }


    bool isByte( std::int32_t x ) {
        return 0 <= x and x < 0x100;
    }


    bool isShiftCount( std::int32_t x ) {
        return 0 <= x and x < 32;
    }


    void emit_byte( std::int32_t x );

    void emit_long( std::int32_t x );

    void emit_data( std::int32_t data, RelocationInformation::RelocationType rtype );

    void emit_arith_b( std::int32_t op1, std::int32_t op2, const Register &dst, std::int32_t imm8 );

    void emit_arith( std::int32_t op1, std::int32_t op2, const Register &dst, std::int32_t imm32 );

    void emit_arith( std::int32_t op1, std::int32_t op2, const Register &dst, Oop obj );

    void emit_arith( std::int32_t op1, std::int32_t op2, const Register &dst, const Register &src );

    void emit_operand( const Register &reg, const Register &base, const Register &index, Address::ScaleFactor scale, std::int32_t disp, RelocationInformation::RelocationType rtype );

    void emit_operand( const Register &r, const Address &a );

    void emit_farith( std::int32_t b1, std::int32_t b2, std::int32_t i );

    void print( const Label &L );

    void bind_to( Label &L, std::int32_t pos );

    void link_to( Label &L, Label &appendix );

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

    enum class Constants {
        sizeOfCall = 5      // length of call instruction in bytes
    };

    Assembler( CodeBuffer *code );
    Assembler() = default;
    virtual ~Assembler() = default;
    Assembler( const Assembler & ) = default;
    Assembler &operator=( const Assembler & ) = default;
    void operator delete( void *ptr ) { static_cast<void *>(ptr); }



    void finalize();        // call this before using/copying the code

    CodeBuffer *code() const {
        return _code;
    }


    const char *pc() const {
        return _code_pos;
    }


    std::int32_t offset() const {
        return _code_pos - _code_begin;
    }


    // Stack
    void pushad();

    void popad();

    void pushl( std::int32_t imm32 );

    void pushl( Oop obj );

    void pushl( const Register &src );

    void pushl( const Address &src );

    void popl( const Register &dst );

    void popl( const Address &dst );

    // Moves
    void movb( const Register &dst, const Address &src );

    void movb( const Address &dst, std::int32_t imm8 );

    void movb( const Address &dst, const Register &src );

    void movw( const Register &dst, const Address &src );

    void movw( const Address &dst, const Register &src );

    void movl( const Register &dst, std::int32_t imm32 );

    void movl( const Register &dst, Oop obj );

    void movl( const Register &dst, const Register &src );

    void movl( const Register &dst, const Address &src );

    void movl( const Address &dst, std::int32_t imm32 );

    void movl( const Address &dst, Oop obj );

    void movl( const Address &dst, const Register &src );

    void movsxb( const Register &dst, const Address &src );

    void movsxb( const Register &dst, const Register &src );

    void movsxw( const Register &dst, const Address &src );

    void movsxw( const Register &dst, const Register &src );

    // Conditional moves (P6 only)
    void cmovccl( Condition cc, const Register &dst, std::int32_t imm32 );

    void cmovccl( Condition cc, const Register &dst, Oop obj );

    void cmovccl( Condition cc, const Register &dst, const Register &src );

    void cmovccl( Condition cc, const Register &dst, const Address &src );

    // Arithmetics
    void adcl( const Register &dst, std::int32_t imm32 );

    void adcl( const Register &dst, const Register &src );

    void addl( const Address &dst, std::int32_t imm32 );

    void addl( const Register &dst, std::int32_t imm32 );

    void addl( const Register &dst, const Register &src );

    void addl( const Register &dst, const Address &src );

    void andl( const Register &dst, std::int32_t imm32 );

    void andl( const Register &dst, const Register &src );

    void cmpl( const Address &dst, std::int32_t imm32 );

    void cmpl( const Address &dst, Oop obj );

    void cmpl( const Register &dst, std::int32_t imm32 );

    void cmpl( const Register &dst, Oop obj );

    void cmpl( const Register &dst, const Register &src );

    void cmpl( const Register &dst, const Address &src );

    void decb( const Register &dst );

    void decl( const Register &dst );

    void decl( const Address &dst );

    void idivl( const Register &src );

    void imull( const Register &src );

    void imull( const Register &dst, const Register &src );

    void imull( const Register &dst, const Register &src, std::int32_t value );

    void incl( const Register &dst );

    void incl( const Address &dst );

    void leal( const Register &dst, const Address &src );

    void mull( const Register &src );

    void negl( const Register &dst );

    void notl( const Register &dst );

    void orl( const Register &dst, std::int32_t imm32 );

    void orl( const Register &dst, const Register &src );

    void orl( const Register &dst, const Address &src );

    void rcll( const Register &dst, std::int32_t imm8 );

    void sarl( const Register &dst, std::int32_t imm8 );

    void sarl( const Register &dst );

    void sbbl( const Register &dst, std::int32_t imm32 );

    void sbbl( const Register &dst, const Register &src );

    void shldl( const Register &dst, const Register &src );

    void shll( const Register &dst, std::int32_t imm8 );

    void shll( const Register &dst );

    void shrdl( const Register &dst, const Register &src );

    void shrl( const Register &dst, std::int32_t imm8 );

    void shrl( const Register &dst );

    void subl( const Register &dst, std::int32_t imm32 );

    void subl( const Register &dst, const Register &src );

    void subl( const Register &dst, const Address &src );

    void testb( const Register &dst, std::int32_t imm8 );

    void testl( const Register &dst, std::int32_t imm32 );

    void testl( const Register &dst, const Register &src );

    void xorl( const Register &dst, std::int32_t imm32 );

    void xorl( const Register &dst, const Register &src );

    // Miscellaneous
    void cdq();

    void hlt();

    void int3();

    void nop();

    void ret( std::int32_t imm16 = 0 );

    // Labels
    void bind( Label &L );            // binds an unbound label L to the current code position
    void merge( const Label &L, const Label &with );    // merges L and with, L is the merged label

    // Calls
    void call( Label &L );

    void call( const char *entry, RelocationInformation::RelocationType rtype );

    void call( const Register &reg );

    void call( const Address &adr );

    // Jumps
    void jmp( const char *entry, RelocationInformation::RelocationType rtype );

    void jmp( const Register &reg );

    void jmp( const Address &adr );

    // Label operations & relative jumps (PPUM Appendix D)
    void jmp( Label &L );        // unconditional jump to L

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

    void jcc( Condition cc, const char *dst, RelocationInformation::RelocationType rtype = RelocationInformation::RelocationType::runtime_call_type );

    void jcc( Condition cc, Label &L );

    // Support for inline cache information (see also InlineCacheInfo)
    void ic_info( Label &L, std::int32_t flags );

    // Floating-point operations
    void fld1();

    void fldz();

    // %note: _s 32 bits, _d 64 bits
    void fld_s( const Address &a );

    void fld_d( const Address &a );

    void fstp_s( const Address &a );

    void fstp_d( const Address &a );

    void fild_s( const Address &a );

    void fild_d( const Address &a );

    void fistp_s( const Address &a );

    void fistp_d( const Address &a );

    void fabs();

    void fchs();

    void fadd_d( const Address &a );

    void fsub_d( const Address &a );

    void fmul_d( const Address &a );

    void fdiv_d( const Address &a );

    void fadd( std::int32_t i );

    void fsub( std::int32_t i );

    void fmul( std::int32_t i );

    void fdiv( std::int32_t i );

    void faddp( std::int32_t i = 1 );

    void fsubp( std::int32_t i = 1 );

    void fsubrp( std::int32_t i = 1 );

    void fmulp( std::int32_t i = 1 );

    void fdivp( std::int32_t i = 1 );

    void fprem();

    void fprem1();

    void fxch( std::int32_t i = 1 );

    void fincstp();

    void ffree( std::int32_t i = 0 );

    void ftst();

    void fcompp();

    void fnstsw_ax();

    void fwait();


    // For compatibility with old assembler only - should be removed at some point
    void Load( const Register &base, std::int32_t disp, const Register &dst ) {
        movl( dst, Address( base, disp ) );
    }


    void Store( const Register &src, const Register &base, std::int32_t disp ) {
        movl( Address( base, disp ), src );
    }

};
