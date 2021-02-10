//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/assembler/Address.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/assembler/MacroAssembler.hpp"

#include "vm/primitives/BehaviorPrimitives.hpp"
#include "vm/primitives/PrimitivesGenerator.hpp"


static bool stop = false;


const char *PrimitivesGenerator::primitiveNew( std::int32_t n ) {
    Address      klass_addr = Address( esp, +2 * OOP_SIZE );
    Label        need_scavenge, fill_object;
    std::int32_t size       = n + 2;

    // %note: it looks like the compiler assumes we spill only eax/ebx here -Marc 04/07

    const char *entry_point = masm->pc();

    test_for_scavenge( eax, size * OOP_SIZE, allocation_failure );
    Address _stop = Address( (std::int32_t) &stop, RelocationInformation::RelocationType::external_word_type );
    Label   _break, no_break;
    masm->bind( fill_object );
    masm->movl( ebx, _stop );
    masm->testl( ebx, ebx );
    masm->jcc( Assembler::Condition::notEqual, _break );
    masm->bind( no_break );
    masm->movl( ebx, klass_addr );
    masm->movl( Address( eax, ( -size + 0 ) * OOP_SIZE ), 0x80000003 );    // obj->init_mark()
    masm->movl( Address( eax, ( -size + 1 ) * OOP_SIZE ), ebx );        // obj->init_mark()

    if ( n > 0 ) {
        masm->movl( ebx, nil_addr() );
        for ( std::size_t i = 0; i < n; i++ ) {
            masm->movl( Address( eax, ( -size + 2 + i ) * OOP_SIZE ), ebx );    // obj->obj_at_put(i,nilObject)
        }
    }

    masm->subl( eax, ( size * OOP_SIZE ) - 1 );
    masm->ret( 2 * OOP_SIZE );

    masm->bind( _break );
    masm->int3();
    masm->jmp( no_break );

    masm->bind( need_scavenge );
    scavenge( size );
    masm->jmp( fill_object );

    return entry_point;
}
