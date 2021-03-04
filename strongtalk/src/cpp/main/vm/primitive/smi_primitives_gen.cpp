//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/assembler/Address.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/assembler/MacroAssembler.hpp"
#include "vm/primitive/PrimitivesGenerator.hpp"


const char *PrimitivesGenerator::smiOopPrimitives_add() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );
    Label   _overflow;

    const char *entry_point = masm->pc();

    masm->movl( eax, receiver );
    masm->addl( eax, argument );
    masm->jcc( Assembler::Condition::overflow, _overflow );
    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->ret( 8 );

    masm->bind( _overflow );
    masm->movl( eax, argument );
    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->jmp( error_overflow );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_subtract() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );
    Label   _overflow;

    const char *entry_point = masm->pc();

    masm->movl( eax, receiver );
    masm->subl( eax, argument );
    masm->jcc( Assembler::Condition::overflow, _overflow );
    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->ret( 8 );

    masm->bind( _overflow );
    masm->movl( eax, argument );
    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->jmp( error_overflow );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_multiply() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );

    const char *entry_point = masm->pc();

    // masm->int3();
    masm->movl( edx, argument );
    masm->movl( eax, receiver );
    masm->testb( edx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->sarl( edx, 2 );
    masm->imull( edx );
    masm->jcc( Assembler::Condition::overflow, error_overflow );
    masm->ret( 8 );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_mod() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );
    Label   _equal, _positive;

    const char *entry_point = masm->pc();

    // PUBLIC _smiOopPrimitives_mod@8
    //
    // ; Intel definition of mod delivers:
    // ;   0 <= |x%y| < |y|
    // ;
    // ; Standard definition requires:
    // ;   y>0:
    // ;     0 <= x mod y < y
    // ;   y<0:
    // ;     y <  x mod y <= 0
    // ;
    // ; Conversion:
    // ;
    // ;   sgn(y)=sgn(x%y):
    // ;     x mod y = x%y
    // ;
    // ;   sgn(y)#sgn(x%y):
    // ;     x mod y = x%y + y
    // ;

//  masm->int3();
    masm->movl( eax, receiver );
    masm->movl( ecx, argument );
    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, error_division_by_zero );

    masm->testb( ecx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );

    masm->sarl( ecx, 2 );
    masm->sarl( eax, 2 );
    masm->cdq();
    masm->idivl( ecx );
    masm->jcc( Assembler::Condition::overflow, error_overflow );

    masm->movl( eax, edx );
    masm->testl( eax, eax );
    masm->jcc( Assembler::Condition::equal, _equal );

    masm->xorl( edx, ecx );
    masm->jcc( Assembler::Condition::negative, _positive );

    masm->bind( _equal );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    masm->bind( _positive );
    masm->addl( eax, ecx );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_div() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );
    Label   _equal, _positive;

    const char *entry_point = masm->pc();

    // ; Intel definition of div delivers:
    // ;   x = (x/y)*y + (x%y)
    // ;
    // ; Standard definition requires:
    // ;   x = (x div y)*y + (x mod y)
    // ;
    // ; Conversion:
    // ;
    // ;   sgn(y)=sgn(x%y):
    // ;     x div y = x/y
    // ;
    // ;   sgn(y)#sgn(x%y):
    // ;     x div y = x/y-1
    // ;
    //

    masm->movl( ecx, argument );
    masm->movl( eax, receiver );
    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, error_division_by_zero );

    masm->testb( ecx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );

    masm->sarl( ecx, 2 );
    masm->sarl( eax, 2 );
    masm->cdq();
    masm->idivl( ecx );

    masm->jcc( Assembler::Condition::overflow, error_overflow );

    masm->testl( edx, edx );
    masm->jcc( Assembler::Condition::equal, _equal );

    masm->xorl( ecx, edx );
    masm->jcc( Assembler::Condition::negative, _positive );

    masm->bind( _equal );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    masm->bind( _positive );
    masm->decl( eax );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_quo() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );

    const char *entry_point = masm->pc();

    masm->movl( ecx, argument );
    masm->movl( eax, receiver );

    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_receiver_has_wrong_type );

    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, error_division_by_zero );

    masm->testb( ecx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );

    masm->sarl( ecx, 2 );
    masm->sarl( eax, 2 );
    masm->cdq();
    masm->idivl( ecx );

    masm->jcc( Assembler::Condition::overflow, error_overflow );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_remainder() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );

    const char *entry_point = masm->pc();

    masm->movl( ecx, argument );
    masm->movl( eax, receiver );
    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, error_division_by_zero );
    masm->testb( ecx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->sarl( ecx, 2 );
    masm->sarl( eax, 2 );
    masm->cdq();
    masm->idivl( ecx );
    masm->jcc( Assembler::Condition::overflow, error_overflow );
    masm->movl( eax, edx );
    masm->sarl( eax, 2 );
    masm->ret( 8 );

    return entry_point;
}
