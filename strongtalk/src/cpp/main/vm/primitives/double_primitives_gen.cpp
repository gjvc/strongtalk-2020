//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/primitives/PrimitivesGenerator.hpp"


const char *PrimitivesGenerator::double_op( arith_op op ) {
    Label need_scavenge, fill_object;

    const char *entry_point = masm->pc();

    // 	Tag test for argument
    masm->movl( ebx, Address( esp, +oopSize ) );
    masm->movl( edx, doubleKlass_addr() );
    masm->testb( ebx, 0x01 );
    masm->jcc( Assembler::Condition::zero, error_first_argument_has_wrong_type );

    // 	klass test for argument
    masm->cmpl( edx, Address( ebx, +3 ) );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );

    // 	allocate_double
    test_for_scavenge( eax, 4 * oopSize, need_scavenge );

    // 	mov	DWORD PTR [eax-(4 * oopSize)], 0A0000003H ; obj->init_mark()
    // 	fld  QWORD PTR [ecx+07]
    // 	mov	DWORD PTR [eax-(3 * oopSize)], edx        ; obj->set_klass(klass)
    //
    // 	op   QWORD PTR [ebx+07]

    masm->bind( fill_object );
    masm->movl( ecx, Address( esp, +2 * oopSize ) );

    masm->movl( edx, doubleKlass_addr() );
    masm->movl( Address( eax, -4 * oopSize ), 0xA0000003 );         // obj->init_mark()
    masm->movl( Address( eax, -3 * oopSize ), edx );                // obj->set_klass(klass)

    masm->fld_d( Address( ecx, ( 2 * oopSize ) - 1 ) );

    switch ( op ) {
        case op_add:
            masm->fadd_d( Address( ebx, ( 2 * oopSize ) - 1 ) );
            break;
        case op_sub:
            masm->fsub_d( Address( ebx, ( 2 * oopSize ) - 1 ) );
            break;
        case op_mul:
            masm->fmul_d( Address( ebx, ( 2 * oopSize ) - 1 ) );
            break;
        case op_div:
            masm->fdiv_d( Address( ebx, ( 2 * oopSize ) - 1 ) );
            break;
    }

    masm->subl( eax, ( 4 * oopSize ) - 1 );

    // 	eax result   DoubleOop
    // 	ecx receiver DoubleOop
    // 	ebx argument DoubleOop
    masm->fstp_d( Address( eax, ( 2 * oopSize ) - 1 ) );
    masm->ret( 8 );

    masm->bind( need_scavenge );
    scavenge( 4 );
    masm->movl( ebx, Address( esp, +oopSize ) );
    masm->movl( edx, doubleKlass_addr() );
    masm->jmp( fill_object );

    return entry_point;
}


const char *PrimitivesGenerator::double_from_smi() {
    Label need_scavenge, fill_object;

    const char *entry_point = masm->pc();

    test_for_scavenge( eax, 4 * oopSize, need_scavenge );

    masm->bind( fill_object );
    masm->movl( ecx, Address( esp, +oopSize ) );
    masm->movl( edx, doubleKlass_addr() );
    masm->sarl( ecx, TAG_SIZE );
    masm->movl( Address( eax, -4 * oopSize ), 0xA0000003 );         // obj->init_mark()
    masm->movl( Address( esp, -oopSize ), ecx );
    masm->movl( Address( eax, -3 * oopSize ), edx );                // obj->set_klass(klass)
    masm->fild_s( Address( esp, -oopSize ) );
    masm->subl( eax, ( 4 * oopSize ) - 1 );

    //	eax result   DoubleOop
    masm->fstp_d( Address( eax, ( 2 * oopSize ) - 1 ) );
    masm->ret( 4 );

    masm->bind( need_scavenge );
    scavenge( 4 );
    masm->jmp( fill_object );

    return entry_point;
}
