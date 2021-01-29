//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/primitives/behavior_primitives.hpp"
#include "vm/assembler/MacroAssembler.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/assembler/Address.hpp"
#include "vm/primitives/PrimitivesGenerator.hpp"


const char *PrimitivesGenerator::allocateBlock( std::int32_t n ) {

    spdlog::info( "PrimitivesGenerator::allocateBlock [{}]", n );

    KlassOopDescriptor **block_klass;

    switch ( n ) {
        case 0:
            block_klass = &::zeroArgumentBlockKlassObject;
            break;
        case 1:
            block_klass = &::oneArgumentBlockKlassObject;
            break;
        case 2:
            block_klass = &::twoArgumentBlockKlassObject;
            break;
        case 3:
            block_klass = &::threeArgumentBlockKlassObject;
            break;
        case 4:
            block_klass = &::fourArgumentBlockKlassObject;
            break;
        case 5:
            block_klass = &::fiveArgumentBlockKlassObject;
            break;
        case 6:
            block_klass = &::sixArgumentBlockKlassObject;
            break;
        case 7:
            block_klass = &::sevenArgumentBlockKlassObject;
            break;
        case 8:
            block_klass = &::eightArgumentBlockKlassObject;
            break;
        case 9:
            block_klass = &::nineArgumentBlockKlassObject;
            break;
    }

    Address block_klass_addr = Address( (std::int32_t) block_klass, RelocationInformation::RelocationType::external_word_type );
    Label   need_scavenge, fill_object;

    const char *entry_point = masm->pc();

    test_for_scavenge( eax, 4 * OOP_SIZE, need_scavenge );

    masm->bind( fill_object );
    masm->movl( ebx, block_klass_addr );
    masm->movl( Address( eax, -4 * OOP_SIZE ), 0x80000003 ); // obj->init_mark()
    masm->movl( Address( eax, -3 * OOP_SIZE ), ebx ); // obj->set_klass(klass)
    masm->movl( Address( eax, -2 * OOP_SIZE ), 0 );    // obj->set_method(nullptr)
    masm->movl( Address( eax, -1 * OOP_SIZE ), 0 ); // obj->set_lexical_scope(nullptr)
    masm->subl( eax, ( 4 * OOP_SIZE ) - 1 );
    masm->ret( 0 );

    masm->bind( need_scavenge );
    scavenge( 4 );
    masm->jmp( fill_object );

    return entry_point;
}


extern "C" void scavenge_and_allocate( std::int32_t size );


const char *PrimitivesGenerator::allocateContext_var() {

    Label need_scavenge, fill_object;
    Label _loop, _loop_end;

    const char *entry_point = masm->pc();

    masm->movl( ecx, Address( esp, +OOP_SIZE ) );    // load length  (remember this is a SMIOop)
    masm->movl( eax, Address( (std::int32_t) &eden_top, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( edx, ecx );
    masm->addl( edx, 3 * OOP_SIZE );
    masm->addl( edx, eax );
// Equals? ==>  masm->leal(edx, Address(ecx, eax, Address::times_1, 3*OOP_SIZE));
    masm->cmpl( edx, Address( (std::int32_t) &eden_end, RelocationInformation::RelocationType::external_word_type ) );
    masm->jcc( Assembler::Condition::greater, need_scavenge );
    masm->movl( Address( (std::int32_t) &eden_top, RelocationInformation::RelocationType::external_word_type ), edx );

    masm->bind( fill_object );
    masm->movl( ebx, contextKlass_addr() );
    masm->addl( ecx, 4 );
    masm->addl( ecx, 0x80000003 );                // obj->init_mark()
    masm->movl( Address( eax ), ecx );
    masm->movl( ecx, nil_addr() );

    masm->movl( Address( eax, 1 * OOP_SIZE ), ebx );        // obj->set_klass(klass)
    masm->movl( Address( eax, 2 * OOP_SIZE ), 0 );        // obj->set_home(nullptr)
    masm->leal( ebx, Address( eax, +3 * OOP_SIZE ) );
    masm->jmp( _loop_end );

    masm->bind( _loop );
    masm->movl( Address( ebx ), ecx );
    masm->addl( ebx, OOP_SIZE );

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
    masm->call( (const char *) &scavenge_and_allocate, RelocationInformation::RelocationType::runtime_call_type );
    masm->addl( esp, 4 );
    masm->reset_last_Delta_frame();
    masm->movl( ecx, Address( esp, +OOP_SIZE ) );    // reload length  (remember this is a SMIOop)
    masm->movl( edx, ecx );
    masm->addl( edx, 3 * OOP_SIZE );
    masm->addl( edx, eax );
    masm->jmp( fill_object );

    return entry_point;
}


const char *PrimitivesGenerator::allocateContext( std::int32_t n ) {

    Label        need_scavenge, fill_object;
    std::int32_t size = n + 3;

    const char *entry_point = masm->pc();

    test_for_scavenge( eax, size * OOP_SIZE, need_scavenge );

    masm->bind( fill_object );
    masm->movl( ebx, contextKlass_addr() );
    masm->movl( ecx, nil_addr() );
    masm->movl( Address( eax, ( -size + 0 ) * OOP_SIZE ), 0x80000003 + ( ( n + 1 ) * 4 ) );// obj->init_mark()
    masm->movl( Address( eax, ( -size + 1 ) * OOP_SIZE ), ebx );             // obj->set_klass(klass)
    masm->movl( Address( eax, ( -size + 2 ) * OOP_SIZE ), 0 );               // obj->set_home(nullptr)
    for ( std::int32_t i = 0; i < n; i++ ) {
        masm->movl( Address( eax, ( -size + 3 + i ) * OOP_SIZE ), ecx );     // obj->obj_at_put(i,nilObject)
    }
    masm->subl( eax, size * OOP_SIZE - 1 );
    masm->ret( 0 );

    masm->bind( need_scavenge );
    scavenge( size );
    masm->jmp( fill_object );

    return entry_point;
}


const char *PrimitivesGenerator::inline_allocation() {
    Address klass_addr = Address( esp, +2 * OOP_SIZE );
    Address count_addr = Address( esp, +1 * OOP_SIZE );

    Label        need_scavenge1, fill_object1, need_scavenge2, fill_object2, loop, loop_test, exit;
    std::int32_t size  = 2;

    const char *entry_point = masm->pc();

    masm->movl( ebx, klass_addr );
    masm->movl( edx, count_addr );
    masm->testl( edx, 1 );
    masm->jcc( Assembler::Condition::notEqual, exit );
    masm->sarl( edx, 3 );
    masm->bind( loop );

    test_for_scavenge( eax, size * OOP_SIZE, need_scavenge1 );
    masm->bind( fill_object1 );
    masm->movl( Address( eax, ( -size + 0 ) * OOP_SIZE ), 0x80000003 );    // obj->init_mark()
    masm->movl( Address( eax, ( -size + 1 ) * OOP_SIZE ), ebx );        // obj->init_mark()

    masm->subl( eax, ( size * OOP_SIZE ) - 1 );

    test_for_scavenge( ecx, size * OOP_SIZE, need_scavenge2 );
    masm->bind( fill_object2 );
    masm->movl( Address( ecx, ( -size + 0 ) * OOP_SIZE ), 0x80000003 );    // obj->init_mark()
    masm->movl( Address( ecx, ( -size + 1 ) * OOP_SIZE ), ebx );        // obj->init_mark()

    masm->subl( ecx, ( size * OOP_SIZE ) - 1 );
    //masm->jmp(loop);
    masm->bind( loop_test );
    masm->decl( edx );
    masm->jcc( Assembler::Condition::notEqual, loop );
    masm->bind( exit );
    masm->ret( 2 * OOP_SIZE );

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
