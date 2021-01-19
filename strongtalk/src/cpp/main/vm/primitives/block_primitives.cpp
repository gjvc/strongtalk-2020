
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/objectIDTable.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/oops/BlockClosureKlass.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/primitives/block_primitives.hpp"
#include "vm/runtime/UnwindInfo.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/oops/ContextKlass.hpp"


TRACE_FUNC( TraceBlockPrims, "block" )


static inline void inc_calls() {
}


static inline void inc_block_counter() {
    NumberOfBlockAllocations++;
}


static inline void inc_context_counter() {
    NumberOfContextAllocations++;
}


extern "C" BlockClosureOop allocateTenuredBlock( SMIOop nofArgs ) {
    PROLOGUE_1( "allocateBlock", nofArgs );
    BlockClosureOop blk = as_blockClosureOop( Universe::allocate_tenured( sizeof( BlockClosureOopDescriptor ) / oopSize ) );
    blk->init_mark();
    blk->set_klass_field( BlockClosureKlass::blockKlassFor( nofArgs->value() ) );
    inc_block_counter();
    return blk;
}

// TODO: Implement the following function (gri)
extern "C" BlockClosureOop allocateBlock( SMIOop nofArgs ) {
    PROLOGUE_1( "allocateBlock", nofArgs );
    BlockClosureOop blk = as_blockClosureOop( Universe::allocate( sizeof( BlockClosureOopDescriptor ) / oopSize ) );
    blk->init_mark();
    blk->set_klass_field( BlockClosureKlass::blockKlassFor( nofArgs->value() ) );
    inc_block_counter();
    return blk;
}

extern "C" BlockClosureOop allocateBlock0() {
    PROLOGUE_0( "allocateBlock0" );
    BlockClosureOop blk = as_blockClosureOop( Universe::allocate( sizeof( BlockClosureOopDescriptor ) / oopSize ) );
    blk->init_mark();
    blk->set_klass_field( Universe::zeroArgumentBlockKlassObj() );
    inc_block_counter();
    return static_cast<BlockClosureOop>(Oop( blk ));
}

extern "C" BlockClosureOop allocateBlock1() {
    PROLOGUE_0( "allocateBlock1" );
    BlockClosureOop blk = as_blockClosureOop( Universe::allocate( sizeof( BlockClosureOopDescriptor ) / oopSize ) );
    blk->init_mark();
    blk->set_klass_field( Universe::oneArgumentBlockKlassObj() );
    inc_block_counter();
    return static_cast<BlockClosureOop>(Oop( blk ));
}

extern "C" BlockClosureOop allocateBlock2() {
    PROLOGUE_0( "allocateBlock2" );
    BlockClosureOop blk = as_blockClosureOop( Universe::allocate( sizeof( BlockClosureOopDescriptor ) / oopSize ) );
    blk->init_mark();
    blk->set_klass_field( Universe::twoArgumentBlockKlassObj() );
    inc_block_counter();
    return static_cast<BlockClosureOop>(Oop( blk ));
}

extern "C" ContextOop allocateContext( SMIOop nofVars ) {
    PROLOGUE_1( "allocateContext", nofVars );
    ContextKlass *ok = (ContextKlass *) contextKlassObj->klass_part();
    inc_context_counter();
    return static_cast<ContextOop>(ok->allocateObjectSize( nofVars->value() ));
}

extern "C" ContextOop allocateContext0() {
    PROLOGUE_0( "allocateContext0" );
    // allocate
    ContextOop obj = as_contextOop( Universe::allocate( ContextOopDescriptor::header_size() ) );
    // header
    obj->set_klass_field( contextKlassObj );
    //%clean this up later
    //  hash value must by convention be different from 0 (check markOop.hpp)
    obj->set_mark( MarkOopDescriptor::tagged_prototype()->set_hash( 0 + 1 ) );

    inc_context_counter();

    return obj;
}

extern "C" ContextOop allocateContext1() {
    PROLOGUE_0( "allocateContext1" );
    ContextKlass *ok = (ContextKlass *) contextKlassObj->klass_part();
    inc_context_counter();
    return static_cast<ContextOop>(ok->allocateObjectSize( 1 ));
}
extern "C" ContextOop allocateContext2() {
    PROLOGUE_0( "allocateContext2" );
    ContextKlass *ok = (ContextKlass *) contextKlassObj->klass_part();
    inc_context_counter();
    return static_cast<ContextOop>(ok->allocateObjectSize( 2 ));
}


extern "C" bool_t have_nlr_through_C;
extern "C" Oop    nlr_result;


PRIM_DECL_2( unwindprotect, Oop receiver, Oop protectBlock ) {
    PROLOGUE_2( "unwindprotect", receiver, protectBlock );
    Oop block, res;
    {
        PersistentHandle *pb = new PersistentHandle( protectBlock );
        res   = Delta::call( receiver, vmSymbols::value() );
        block = pb->as_oop();
        delete pb;
    }

    if ( have_nlr_through_C ) {
        UnwindInfo enabler;
        PersistentHandle *result = new PersistentHandle( res );
        Delta::call( block, vmSymbols::value(), nlr_result );
        // Now since we have to continue the first non-local-return the nlr_result must be correct.
        res = result->as_oop();
        delete result;
    }

    return res;
}


PRIM_DECL_1( blockRepeat, Oop receiver ) {
    PROLOGUE_1( "blockRepeat", receiver );
    do
        Delta::call( receiver, vmSymbols::value() );
    while ( not have_nlr_through_C );
    return smiOop_zero; // The return value is ignored by the NonLocalReturn
}


PRIM_DECL_1( block_method, Oop receiver ) {
    PROLOGUE_1( "block_method", receiver );
    return BlockClosureOop( receiver )->method();
}


PRIM_DECL_1( block_is_optimized, Oop receiver ) {
    PROLOGUE_1( "blockRepeat", receiver );
    return BlockClosureOop( receiver )->isCompiledBlock() ? trueObj : falseObj;
}


// empty functions, we'll patch them later
static void trap() {
    st_assert( false, "This primitive should be patched" );
}


extern "C" Oop primitiveValue0( Oop blk ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue1( Oop blk, Oop arg1 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue2( Oop blk, Oop arg1, Oop arg2 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue3( Oop blk, Oop arg1, Oop arg2, Oop arg3 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue4( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue5( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue6( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop arg6 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue7( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop arg6, Oop arg7 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue8( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop arg6, Oop arg7, Oop arg8 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

extern "C" Oop primitiveValue9( Oop blk, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop arg6, Oop arg7, Oop arg8, Oop arg9 ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}

/*
extern "C" blockClosureOop allocateBlock0() { trap(); return blockClosureOop(markSymbol(vmSymbols::primitive_trap())); };
extern "C" blockClosureOop allocateBlock1() { trap(); return blockClosureOop(markSymbol(vmSymbols::primitive_trap())); };
extern "C" blockClosureOop allocateBlock2() { trap(); return blockClosureOop(markSymbol(vmSymbols::primitive_trap())); };
*/
extern "C" BlockClosureOop allocateBlock3() {
    trap();
    return BlockClosureOop( markSymbol( vmSymbols::primitive_trap() ) );
}
extern "C" BlockClosureOop allocateBlock4() {
    trap();
    return BlockClosureOop( markSymbol( vmSymbols::primitive_trap() ) );
}
extern "C" BlockClosureOop allocateBlock5() {
    trap();
    return BlockClosureOop( markSymbol( vmSymbols::primitive_trap() ) );
}
extern "C" BlockClosureOop allocateBlock6() {
    trap();
    return BlockClosureOop( markSymbol( vmSymbols::primitive_trap() ) );
}
extern "C" BlockClosureOop allocateBlock7() {
    trap();
    return BlockClosureOop( markSymbol( vmSymbols::primitive_trap() ) );
}
extern "C" BlockClosureOop allocateBlock8() {
    trap();
    return BlockClosureOop( markSymbol( vmSymbols::primitive_trap() ) );
}
extern "C" BlockClosureOop allocateBlock9() {
    trap();
    return BlockClosureOop( markSymbol( vmSymbols::primitive_trap() ) );
}

/*
extern "C" contextOop allocateContext(SMIOop nofVars) { trap(); return contextOop(markSymbol(vmSymbols::primitive_trap())); };
extern "C" contextOop allocateContext0() { trap(); return contextOop(markSymbol(vmSymbols::primitive_trap())); };
extern "C" contextOop allocateContext1() { trap(); return contextOop(markSymbol(vmSymbols::primitive_trap())); };
extern "C" contextOop allocateContext2() { trap(); return contextOop(markSymbol(vmSymbols::primitive_trap())); };
*/
