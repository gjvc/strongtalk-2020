
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/RegisterAllocator.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/compiler/Compiler.hpp"


RegisterAllocator *theAllocator;


static std::int32_t compare_pseudoRegisterBegs( PseudoRegister **a, PseudoRegister **b ) {
    return ( *a )->begByteCodeIndex() - ( *b )->begByteCodeIndex();
}


static std::int32_t compare_pseudoRegisterEnds( PseudoRegister **a, PseudoRegister **b ) {
    return ( *a )->endByteCodeIndex() - ( *b )->endByteCodeIndex();
}


RegisterAllocator::RegisterAllocator() : _stackLocs{} {
    theAllocator = this;
    _stackLocs   = new IntegerFreeList( 2 );
}


void RegisterAllocator::preAllocate( PseudoRegister *r ) {
    r->allocateTo( Mapping::localTemporary( _stackLocs->allocate() ) );
}


void RegisterAllocator::allocate( GrowableArray<PseudoRegister *> *globals ) {

    GrowableArray<PseudoRegister *> *regs = new GrowableArray<PseudoRegister *>( globals->length() );

    std::int32_t i = globals->length();

    while ( i-- > 0 ) {
        PseudoRegister *r = globals->at( i );
        st_assert( r->ndefs() + r->nuses() > 0 or r->incorrectDU(), "PseudoRegister is unused" );
        if ( r->_location != Location::UNALLOCATED_LOCATION ) {
            // already allocated
        } else if ( r->isConstPseudoRegister() ) {
            // don't allocate constants for now
        } else {
            // collect for later allocation
            regs->append( r );
        }
    }

    std::int32_t len = regs->length();
    if ( len > 0 ) {

        // sort begByteCodeIndexs & distribute to scopes
        regs->sort( &compare_pseudoRegisterBegs );
        st_assert( regs->isEmpty() or regs->first()->begByteCodeIndex() <= regs->last()->begByteCodeIndex(), "wrong sort order" );
        for ( std::int32_t i = 0; i < len; i++ ) {
            PseudoRegister *r = regs->at( i );
            st_assert( r->begByteCodeIndex() not_eq IllegalByteCodeIndex, "illegal begByteCodeIndex" );
            st_assert( r->endByteCodeIndex() not_eq IllegalByteCodeIndex, "illegal endByteCodeIndex" );
            r->scope()->addToPseudoRegistersBegSorted( r );
        }

        // sort endByteCodeIndexs & distribute to scopes
        regs->sort( &compare_pseudoRegisterEnds );
        st_assert( regs->isEmpty() or regs->first()->endByteCodeIndex() <= regs->last()->endByteCodeIndex(), "wrong sort order" );
        for ( std::int32_t i = 0; i < len; i++ ) {
            PseudoRegister *r = regs->at( i );
            r->scope()->addToPseudoRegistersEndSorted( r );
        }

        // do register allocation
        theCompiler->topScope->allocatePseudoRegisters( _stackLocs );

        // result
        if ( CompilerDebug ) {
            cout( PrintRegAlloc )->print_cr( "%d (-2) stack locations allocated for %d PseudoRegisters", _stackLocs->length(), regs->length() );
        }

    }

}
