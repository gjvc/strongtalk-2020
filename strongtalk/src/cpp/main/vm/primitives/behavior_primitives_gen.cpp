//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/assembler/Address.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/assembler/MacroAssembler.hpp"

#include "vm/primitives/behavior_primitives.hpp"
#include "vm/primitives/PrimitivesGenerator.hpp"
#include "vm/system/sizes.hpp"


static bool_t stop = false;


const char * PrimitivesGenerator::primitiveNew( int n ) {
    Address klass_addr = Address( esp, +2 * oopSize );
    Label   need_scavenge, fill_object;
    int     size       = n + 2;

    // %note: it looks like the compiler assumes we spill only eax/ebx here -Marc 04/07

    const char * entry_point = masm->pc();

    test_for_scavenge( eax, size * oopSize, allocation_failure );
    Address _stop = Address( ( int ) &stop, RelocationInformation::RelocationType::external_word_type );
    Label   _break, no_break;
    masm->bind( fill_object );
    masm->movl( ebx, _stop );
    masm->testl( ebx, ebx );
    masm->jcc( Assembler::notEqual, _break );
    masm->bind( no_break );
    masm->movl( ebx, klass_addr );
    masm->movl( Address( eax, ( -size + 0 ) * oopSize ), 0x80000003 );    // obj->init_mark()
    masm->movl( Address( eax, ( -size + 1 ) * oopSize ), ebx );        // obj->init_mark()

    if ( n > 0 ) {
        masm->movl( ebx, nil_addr() );
        for ( int i = 0; i < n; i++ ) {
            masm->movl( Address( eax, ( -size + 2 + i ) * oopSize ), ebx );    // obj->obj_at_put(i,nilObj)
        }
    }

    masm->subl( eax, ( size * oopSize ) - 1 );
    masm->ret( 2 * oopSize );

    masm->bind( _break );
    masm->int3();
    masm->jmp( no_break );

    masm->bind( need_scavenge );
    scavenge( size );
    masm->jmp( fill_object );

    return entry_point;
}


const char * PrimitivesGenerator::inline_allocation() {
    Address klass_addr = Address( esp, +2 * oopSize );
    Address count_addr = Address( esp, +1 * oopSize );

    Label need_scavenge1, fill_object1, need_scavenge2, fill_object2, loop, loop_test, exit;
    int   size         = 2;

    const char * entry_point = masm->pc();

    masm->movl( ebx, klass_addr );
    masm->movl( edx, count_addr );
    masm->testl( edx, 1 );
    masm->jcc( Assembler::notEqual, exit );
    masm->sarl( edx, 3 );
    masm->bind( loop );

    test_for_scavenge( eax, size * oopSize, need_scavenge1 );
    masm->bind( fill_object1 );
    masm->movl( Address( eax, ( -size + 0 ) * oopSize ), 0x80000003 );    // obj->init_mark()
    masm->movl( Address( eax, ( -size + 1 ) * oopSize ), ebx );        // obj->init_mark()

    masm->subl( eax, ( size * oopSize ) - 1 );

    test_for_scavenge( ecx, size * oopSize, need_scavenge2 );
    masm->bind( fill_object2 );
    masm->movl( Address( ecx, ( -size + 0 ) * oopSize ), 0x80000003 );    // obj->init_mark()
    masm->movl( Address( ecx, ( -size + 1 ) * oopSize ), ebx );        // obj->init_mark()

    masm->subl( ecx, ( size * oopSize ) - 1 );
    //masm->jmp(loop);
    masm->bind( loop_test );
    masm->decl( edx );
    masm->jcc( Assembler::notEqual, loop );
    masm->bind( exit );
    masm->ret( 2 * oopSize );

    masm->bind( need_scavenge1 );
    masm->pushl( ebx );
    masm->pushl( edx );
    scavenge( size );
    masm->popl( edx );
    masm->popl( ebx );
    masm->jmp( fill_object1 );

    masm->bind( need_scavenge2 );
    masm->pushl( ebx );
    masm->pushl( edx );
    scavenge( size );
    masm->movl( ecx, eax );
    masm->popl( edx );
    masm->popl( ebx );
    masm->jmp( fill_object2 );

    return entry_point;
}
