//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/primitives/PrimitivesGenerator.hpp"
#include "vm/system/sizes.hpp"


const char * PrimitivesGenerator::allocateBlock( int n ) {

    KlassOopDescriptor ** block_klass;

    switch ( n ) {
        case 0:
            block_klass = &::zeroArgumentBlockKlassObj;
            break;
        case 1:
            block_klass = &::oneArgumentBlockKlassObj;
            break;
        case 2:
            block_klass = &::twoArgumentBlockKlassObj;
            break;
        case 3:
            block_klass = &::threeArgumentBlockKlassObj;
            break;
        case 4:
            block_klass = &::fourArgumentBlockKlassObj;
            break;
        case 5:
            block_klass = &::fiveArgumentBlockKlassObj;
            break;
        case 6:
            block_klass = &::sixArgumentBlockKlassObj;
            break;
        case 7:
            block_klass = &::sevenArgumentBlockKlassObj;
            break;
        case 8:
            block_klass = &::eightArgumentBlockKlassObj;
            break;
        case 9:
            block_klass = &::nineArgumentBlockKlassObj;
            break;
    }

    Address block_klass_addr = Address( ( int ) block_klass, RelocationInformation::RelocationType::external_word_type );
    Label   need_scavenge, fill_object;

    const char * entry_point = masm->pc();

    test_for_scavenge( eax, 4 * oopSize, need_scavenge );

    masm->bind( fill_object );
    masm->movl( ebx, block_klass_addr );
    masm->movl( Address( eax, -4 * oopSize ), 0x80000003 ); // obj->init_mark()
    masm->movl( Address( eax, -3 * oopSize ), ebx );        // obj->set_klass(klass)
//    masm->movl( Address( eax, -2 * oopSize ), 0 );			// obj->set_method(nullptr)
//    masm->movl( Address( eax, -1 * oopSize ), 0 );			// obj->set_lexical_scope(nullptr)
    masm->subl( eax, ( 4 * oopSize ) - 1 );
    masm->ret( 0 );

    masm->bind( need_scavenge );
    scavenge( 4 );
    masm->jmp( fill_object );

    return entry_point;
}


extern "C" void scavenge_and_allocate( int size );


const char * PrimitivesGenerator::allocateContext_var() {

    Label need_scavenge, fill_object;
    Label _loop, _loop_end;

    const char * entry_point = masm->pc();

    masm->movl( ecx, Address( esp, +oopSize ) );    // load length  (remember this is a SMIOop)
    masm->movl( eax, Address( ( int ) &eden_top, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( edx, ecx );
    masm->addl( edx, 3 * oopSize );
    masm->addl( edx, eax );
// Equals? ==>  masm->leal(edx, Address(ecx, eax, Address::times_1, 3*oopSize));
    masm->cmpl( edx, Address( ( int ) &eden_end, RelocationInformation::RelocationType::external_word_type ) );
    masm->jcc( Assembler::Condition::greater, need_scavenge );
    masm->movl( Address( ( int ) &eden_top, RelocationInformation::RelocationType::external_word_type ), edx );

    masm->bind( fill_object );
    masm->movl( ebx, contextKlass_addr() );
    masm->addl( ecx, 4 );
    masm->addl( ecx, 0x80000003 );                // obj->init_mark()
    masm->movl( Address( eax ), ecx );
    masm->movl( ecx, nil_addr() );

    masm->movl( Address( eax, 1 * oopSize ), ebx );        // obj->set_klass(klass)
    masm->movl( Address( eax, 2 * oopSize ), 0 );        // obj->set_home(nullptr)
    masm->leal( ebx, Address( eax, +3 * oopSize ) );
    masm->jmp( _loop_end );

    masm->bind( _loop );
    masm->movl( Address( ebx ), ecx );
    masm->addl( ebx, oopSize );

    masm->bind( _loop_end );
    masm->cmpl( ebx, edx );
    masm->jcc( Assembler::Condition::below, _loop );

    masm->incl( eax );
    masm->ret( 0 );

    masm->bind( need_scavenge );
    masm->set_last_Delta_frame_after_call();
    masm->shrl( ecx, TAG_SIZE );            // SMIOop->value()
    masm->addl( ecx, 3 );
    masm->pushl( ecx );
    masm->call( ( const char * ) &scavenge_and_allocate, RelocationInformation::RelocationType::runtime_call_type );
    masm->addl( esp, 4 );
    masm->reset_last_Delta_frame();
    masm->movl( ecx, Address( esp, +oopSize ) );    // reload length  (remember this is a SMIOop)
    masm->movl( edx, ecx );
    masm->addl( edx, 3 * oopSize );
    masm->addl( edx, eax );
    masm->jmp( fill_object );

    return entry_point;
}


const char * PrimitivesGenerator::allocateContext( int n ) {

    Label need_scavenge, fill_object;
    int   size = n + 3;

    const char * entry_point = masm->pc();

    test_for_scavenge( eax, size * oopSize, need_scavenge );

    masm->bind( fill_object );
    masm->movl( ebx, contextKlass_addr() );
    masm->movl( ecx, nil_addr() );
    masm->movl( Address( eax, ( -size + 0 ) * oopSize ), 0x80000003 + ( ( n + 1 ) * 4 ) );// obj->init_mark()
    masm->movl( Address( eax, ( -size + 1 ) * oopSize ), ebx );             // obj->set_klass(klass)
    masm->movl( Address( eax, ( -size + 2 ) * oopSize ), 0 );               // obj->set_home(nullptr)
    for ( int i = 0; i < n; i++ ) {
        masm->movl( Address( eax, ( -size + 3 + i ) * oopSize ), ecx );     // obj->obj_at_put(i,nilObj)
    }
    masm->subl( eax, size * oopSize - 1 );
    masm->ret( 0 );

    masm->bind( need_scavenge );
    scavenge( size );
    masm->jmp( fill_object );

    return entry_point;
}
