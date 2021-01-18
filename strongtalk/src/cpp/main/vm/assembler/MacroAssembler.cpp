
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/assembler/MacroAssembler.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/code/StubRoutines.hpp"


MacroAssembler::MacroAssembler( CodeBuffer *code ) :
        Assembler( code ) {
}


// Implementation of MacroAssembler

void MacroAssembler::align( int modulus ) {
    while ( offset() % modulus not_eq 0 )
        nop();
}


void MacroAssembler::test( Register dst, int imm8 ) {
    if ( not CodeForP6 and dst.hasByteRegister() ) {
        testb( dst, imm8 );
    } else {
        testl( dst, imm8 );
    }
}


void MacroAssembler::enter() {
    pushl( ebp );
    movl( ebp, esp );
}


void MacroAssembler::leave() {
    movl( esp, ebp );
    popl( ebp );
}


// Support for inlined data

void MacroAssembler::inline_oop( Oop o ) {
    emit_byte( 0xA9 );
    emit_data( (int) o, RelocationInformation::RelocationType::oop_type );
}


// Calls to C land
//
// When entering C land, the ebp & esp of the last Delta frame have to be recorded.
// When leaving C land, last_Delta_fp has to be reset to 0. This is required to
// allow proper stack traversal.

void MacroAssembler::set_last_Delta_frame_before_call() {
    movl( Address( (int) &last_Delta_fp, RelocationInformation::RelocationType::external_word_type ), ebp );
    movl( Address( (int) &last_Delta_sp, RelocationInformation::RelocationType::external_word_type ), esp );
}


void MacroAssembler::set_last_Delta_frame_after_call() {
    addl( esp, oopSize );    // sets esp to value before call (i.e., before pushing the return address)
    set_last_Delta_frame_before_call();
    subl( esp, oopSize );    // resets esp to original value
}


void MacroAssembler::reset_last_Delta_frame() {
    movl( Address( (int) &last_Delta_fp, RelocationInformation::RelocationType::external_word_type ), 0 );
}


void MacroAssembler::call_C( const Label &L ) {
    set_last_Delta_frame_before_call();
    call( L );
    reset_last_Delta_frame();
}


void MacroAssembler::call_C( const Label &L, const Label &nlrTestPoint ) {
    set_last_Delta_frame_before_call();
    call( L );
    Assembler::ic_info( nlrTestPoint, 0 );
    reset_last_Delta_frame();
}


void MacroAssembler::call_C( const char *entry, RelocationInformation::RelocationType rtype ) {
    set_last_Delta_frame_before_call();
    call( entry, rtype );
    reset_last_Delta_frame();
}


void MacroAssembler::call_C( const char *entry, RelocationInformation::RelocationType rtype, Label &nlrTestPoint ) {
    set_last_Delta_frame_before_call();
    call( entry, rtype );
    Assembler::ic_info( nlrTestPoint, 0 );
    reset_last_Delta_frame();
}


void MacroAssembler::call_C( const Register &entry ) {
    set_last_Delta_frame_before_call();
    call( entry );
    reset_last_Delta_frame();
}


void MacroAssembler::call_C( const Register &entry, const Label &nlrTestPoint ) {
    set_last_Delta_frame_before_call();
    call( entry );
    Assembler::ic_info( nlrTestPoint, 0 );
    reset_last_Delta_frame();
}


/*
    The following 3 macros implement C run-time calls with arguments. When setting up the
    last Delta frame, the value pushed after last_Delta_sp MUST be a valid return address,
    therefore an additional call to a little stub is required which does the parameter
    passing.

    [return addr] \
    [argument 1 ]  |   extra stub in C land
    ...           |
    [argument n ] /
    [return addr] <=== must be valid return address  \
    [...        ] <--- last_Delta_sp                  |
    ...                                              | last Delta frame in Delta land
    [...        ]                                     |
    [previous fp] <--- last_Delta_fp                 /

    Note: The routines could be implemented slightly more efficient and shorter by
    explicitly pushing/popping a valid return address instead of calling the extra
    stub. However, currently the assembler doesn't support label pushes.

*/


void MacroAssembler::call_C( const char *entry, const Register &arg1 ) {

    Label L1, L2;
    jmp( L1 );

    bind( L2 );
    pushl( arg1 );
    call( entry, RelocationInformation::RelocationType::runtime_call_type );
    addl( esp, 1 * oopSize );
    ret( 0 );

    bind( L1 );
    call_C( L2 );
}


void MacroAssembler::call_C( const char *entry, const Register &arg1, const Register &arg2 ) {
    Label L1, L2;
    jmp( L1 );

    bind( L2 );
    pushl( arg2 );
    pushl( arg1 );
    call( entry, RelocationInformation::RelocationType::runtime_call_type );
    addl( esp, 2 * oopSize );
    ret( 0 );

    bind( L1 );
    call_C( L2 );
}


void MacroAssembler::call_C( const char *entry, const Register &arg1, const Register &arg2, const Register &arg3 ) {
    Label L1, L2;
    jmp( L1 );

    bind( L2 );
    pushl( arg3 );
    pushl( arg2 );
    pushl( arg1 );
    call( entry, RelocationInformation::RelocationType::runtime_call_type );
    addl( esp, 3 * oopSize );
    ret( 0 );

    bind( L1 );
    call_C( L2 );
}


void MacroAssembler::call_C( const char *entry, const Register &arg1, const Register &arg2, const Register &arg3, const Register &arg4 ) {
    Label L1, L2;
    jmp( L1 );

    bind( L2 );
    pushl( arg4 );
    pushl( arg3 );
    pushl( arg2 );
    pushl( arg1 );
    call( entry, RelocationInformation::RelocationType::runtime_call_type );
    addl( esp, 4 * oopSize );
    ret( 0 );

    bind( L1 );
    call_C( L2 );
}


void MacroAssembler::store_check( const Register &obj, const Register &tmp ) {
    // Does a store check for the Oop in register obj.
    // The content of register obj is destroyed afterwards.
    // Note: Could be optimized by hardwiring the byte map base address in the code - however relocation would be necessary whenever the base changes.
    // Advantage: only one instead of two instructions.
    st_assert( obj not_eq tmp, "registers must be different" );
    Label no_store;
    cmpl( obj, (int) Universe::new_gen.boundary() );        // assumes boundary between new_gen and old_gen is fixed
    jcc( Assembler::Condition::less, no_store );                      // avoid marking dirty if target is a new object
    movl( tmp, Address( (int) &byte_map_base, RelocationInformation::RelocationType::external_word_type ) );
    shrl( obj, card_shift );
    movb( Address( tmp, obj, Address::ScaleFactor::times_1 ), 0 );
    bind( no_store );
}


void MacroAssembler::fpu_mask_and_cond_for( const Condition &cc, int &mask, Condition &cond ) {
    switch ( cc ) {
        case Condition::equal:
            mask = 0x4000;
            cond = Condition::notZero;
            break;
        case Condition::notEqual:
            mask = 0x4000;
            cond = Condition::zero;
            break;
        case Condition::less:
            mask = 0x0100;
            cond = Condition::notZero;
            break;
        case Condition::lessEqual:
            mask = 0x4500;
            cond = Condition::notZero;
            break;
        case Condition::greater:
            mask = 0x4500;
            cond = Condition::zero;
            break;
        case Condition::greaterEqual:
            mask = 0x0100;
            cond = Condition::zero;
            break;
        default: Unimplemented();
    };
}


void MacroAssembler::fpop() {
    ffree();
    fincstp();
}


// debugging

void MacroAssembler::print_reg( const char *name, Oop obj ) {
    _console->print( "%s = ", name );
    if ( obj == nullptr ) {
        _console->print_cr( "nullptr" );

    } else if ( obj->is_smi() ) {
        _console->print_cr( "smi_t (%d)", SMIOop( obj )->value() );

    } else if ( obj->is_mem() and Universe::is_heap( (Oop *) obj ) ) {
        // use explicit checks to avoid crashes even in a broken system
        if ( obj == Universe::nilObj() ) {
            _console->print_cr( "nil (0x%08x)", obj );
        } else if ( obj == Universe::trueObj() ) {
            _console->print_cr( "true (0x%08x)", obj );
        } else if ( obj == Universe::falseObj() ) {
            _console->print_cr( "false (0x%08x)", obj );
        } else {
            _console->print_cr( "MemOop (0x%08x)", obj );
        }
    } else {
        _console->print_cr( "0x%08x", obj );
    }
}


void MacroAssembler::inspector( Oop edi, Oop esi, Oop ebp, Oop esp, Oop ebx, Oop edx, Oop ecx, Oop eax, char *eip ) {

    const char *title = (const char *) ( nativeTest_at( eip )->data() );
    if ( title not_eq nullptr )
        _console->print_cr( "%s", title );

    print_reg( "eax", eax );
    print_reg( "ebx", ebx );
    print_reg( "ecx", ecx );
    print_reg( "edx", edx );
    print_reg( "edi", edi );
    print_reg( "esi", esi );
    _console->print_cr( "ebp = 0x%08x", ebp );
    _console->print_cr( "esp = 0x%08x", esp );
    _console->cr();
}


void MacroAssembler::inspect( const char *title ) {
    const char *entry = StubRoutines::call_inspector_entry();
    if ( entry not_eq nullptr ) {
        call( entry, RelocationInformation::RelocationType::runtime_call_type );            // call stub invoking the inspector
        testl( eax, int( title ) );                    // additional info for inspector
    } else {
        const char *s = ( title == nullptr ) ? "" : title;
        _console->print_cr( "cannot call inspector for \"%s\" - no entry point yet", s );
    }
}
