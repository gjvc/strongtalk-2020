//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/memory/util.hpp"
#include "vm/code/NativeMethodScopes.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/ResourceMark.hpp"


ScopeDescriptor *NativeMethodScopes::at( std::int32_t offset, const char *pc ) const {

    // Read the first byte and decode the ScopeDescriptor type at the location.
    st_assert( offset >= 0, "illegal desc offset" );
    ScopeDescriptorHeaderByte b;
    b.unpack( peek_next_char( offset ) );
    switch ( b.code() ) {
        case METHOD_CODE:
            return new MethodScopeDescriptor( (NativeMethodScopes *) this, offset, pc );
        case TOP_LEVEL_BLOCK_CODE:
            return new TopLevelBlockScopeDescriptor( (NativeMethodScopes *) this, offset, pc );
        case BLOCK_CODE:
            return new BlockScopeDescriptor( (NativeMethodScopes *) this, offset, pc );
        case NON_INLINED_BLOCK_CODE:
            return nullptr;
    }
    st_fatal( "Unknown ScopeDescriptor code in NativeMethodScopes" );
    return nullptr;
}


NonInlinedBlockScopeDescriptor *NativeMethodScopes::noninlined_block_scope_at( std::int32_t offset ) const {
    // Read the first byte and decode the ScopeDescriptor type at the location.
    st_assert( offset > 0, "illegal desc offset" );
    ScopeDescriptorHeaderByte b;
    b.unpack( peek_next_char( offset ) );
    if ( b.code() not_eq NON_INLINED_BLOCK_CODE ) {
        st_fatal( "Not an noninlined scope desc as expected" );
    }
    return new NonInlinedBlockScopeDescriptor( (NativeMethodScopes *) this, offset );
}


std::int16_t NativeMethodScopes::get_next_half( std::int32_t &offset ) const {
    std::int16_t v;
    v = get_next_char( offset ) << BYTE_WIDTH;
    v = addBits( v, get_next_char( offset ) );
    return v;
}


std::uint8_t NativeMethodScopes::getIndexAt( std::int32_t &offset ) const {
    return get_next_char( offset );
}


Oop NativeMethodScopes::unpackOopFromIndex( std::uint8_t index, std::int32_t &offset ) const {
    if ( index == 0 )
        return nullptr;
    if ( index < EXTENDED_INDEX )
        return oop_at( index - 1 );
    return oop_at( get_next_half( offset ) - 1 );
}


std::int32_t NativeMethodScopes::unpackValueFromIndex( std::uint8_t index, std::int32_t &offset ) const {
    if ( index <= MAX_INLINE_VALUE )
        return index;
    if ( index < EXTENDED_INDEX )
        return value_at( index - ( MAX_INLINE_VALUE + 1 ) );
    return value_at( get_next_half( offset ) - ( MAX_INLINE_VALUE + 1 ) );
}


Oop NativeMethodScopes::unpackOopAt( std::int32_t &offset ) const {
    std::uint8_t index = getIndexAt( offset );
    return unpackOopFromIndex( index, offset );
}


std::int32_t NativeMethodScopes::unpackValueAt( std::int32_t &offset ) const {
    std::uint8_t index = getIndexAt( offset );
    return unpackValueFromIndex( index, offset );
}


NameDescriptor *NativeMethodScopes::unpackNameDescAt( std::int32_t &offset, bool &is_last, const char *pc ) const {

    std::int32_t       startOffset = offset;
    nameDescHeaderByte b;
    b.unpack( get_next_char( offset ) );
    is_last = b.is_last();

    NameDescriptor *nd{ nullptr };
    if ( b.is_illegal() ) {
        nd = new IllegalNameDescriptor;
    } else if ( b.is_termination() ) {
        return nullptr;
    } else {
        std::uint8_t index;
        index = b.has_index() ? b.index() : getIndexAt( offset );

        switch ( b.code() ) {
            case LOCATION_CODE: {
                Location l = Location( unpackValueFromIndex( index, offset ) );
                nd = new LocationNameDescriptor( l );
                break;
            }
            case VALUE_CODE: {
                Oop v = unpackOopFromIndex( index, offset );
                nd    = new ValueNameDescriptor( v );
                break;
            }
            case BLOCKVALUE_CODE: {
                Oop blkMethod = unpackOopFromIndex( index, offset );
                st_assert( blkMethod->is_method(), "must be a method" );
                std::int32_t    parent_scope_offset = unpackValueAt( offset );
                ScopeDescriptor *parent_scope       = at( parent_scope_offset, pc );
                nd                                  = new BlockValueNameDescriptor( MethodOop( blkMethod ), parent_scope );
                break;
            }
            case MEMOIZEDBLOCK_CODE: {
                Location l         = Location( unpackValueFromIndex( index, offset ) );
                Oop      blkMethod = unpackOopAt( offset );
                st_assert( blkMethod->is_method(), "must be a method" );
                std::int32_t    parent_scope_offset = unpackValueAt( offset );
                ScopeDescriptor *parent_scope       = at( parent_scope_offset, pc );
                nd                                  = new MemoizedBlockNameDescriptor( l, MethodOop( blkMethod ), parent_scope );
                break;
            }
            default: st_fatal1( "no such name desc (code 0x%08x)", b.code() );
        }
    }

    nd->offset = startOffset;
    return nd;
}


void NativeMethodScopes::iterate( std::int32_t &offset, UnpackClosure *closure ) const {
    char           *pc = my_nativeMethod()->instructionsStart();
    bool           is_last;
    NameDescriptor *nd = unpackNameDescAt( offset, is_last, ScopeDescriptor::invalid_pc );
    if ( nd == nullptr )
        return;        // if at termination byte
    closure->nameDescAt( nd, pc );
    while ( not is_last ) {
        nd = unpackNameDescAt( offset, is_last, ScopeDescriptor::invalid_pc );
        pc += unpackValueAt( offset );
        closure->nameDescAt( nd, pc );
    }
}


NameDescriptor *NativeMethodScopes::unpackNameDescAt( std::int32_t &offset, const char *pc ) const {
    std::int32_t   pc_offset         = pc - my_nativeMethod()->instructionsStart();
    std::int32_t   current_pc_offset = 0;
    bool           is_last;
    NameDescriptor *result           = unpackNameDescAt( offset, is_last, pc );
    if ( result == nullptr )
        return nullptr;    // if at termination byte
    while ( not is_last ) {
        NameDescriptor *current = unpackNameDescAt( offset, is_last, pc );
        current_pc_offset += unpackValueAt( offset );
        if ( current_pc_offset <= pc_offset ) {
            result = current;
        }
    }
    return result;
}


#define FOR_EACH_OOPADDR( VAR )                              \
    for (Oop* VAR = oops(), *CONC(VAR, _end) = oops() + oops_size();          \
         VAR < CONC(VAR, _end); VAR++)


void NativeMethodScopes::verify() {
    // Verify all oops
    FOR_EACH_OOPADDR( addr ) {
        VERIFY_TEMPLATE( addr );
    }

    // Verify all scopedesc
    FOR_EACH_SCOPE( this, s ) {
        if ( not s->verify() )
            spdlog::info( "\t\tin NativeMethod at 0x{0:x} (scopes)", static_cast<void *>( my_nativeMethod() ) );
    }
}


void NativeMethodScopes::scavenge_contents() {
    FOR_EACH_OOPADDR( addr ) {
        SCAVENGE_TEMPLATE( addr );
    }
}


void NativeMethodScopes::switch_pointers( Oop from, Oop to, GrowableArray<NativeMethod *> *nativeMethods_to_invalidate ) {

    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    static_cast<void>(nativeMethods_to_invalidate); // unused


//  This is tricky!
//  First, since some inlined methods are not included in scopes (those that generate no code such as asSmallInteger),
//  you might think that this would not be needed, since memory is swept and dependencies flushed (see Space::switch_pointers_in_region).
//
//  But, when nativeMethods are converted on the stack, zombie nativeMethods are produced. These are obsolete nativeMethods that carry on the execution of
//  active but no longer referenced methods on the stack. Since they have no dependencies, they are not found from the heap.
//  That is why this code is needed anyway.
//
//  Now, since this info describes the code, you cannot change it,
//  instead, you invalidate the NativeMethod if it has a ref to from in its scopes.
//  For example, the method oops in the scope determine the activations
//  that might be on the stack, and you can't change these, because
//  activations are clones of methods. This may be the last reference to
//  a currently executing method, must keep it around.
//
//  However, you may be confused by the fact that locs (embedded literals)
//  in the code in the method are changed. But, those are just object literals,
//  maps of inlined stuff, and are different than the scope oops.
//
//  Although they could possibly be the same, by invalidating any
//  match (beyond just the method holder and method) we are safe.


#ifdef NOT_IMPLEMENTED
    if ( my_nativeMethod()->isInvalid() ) return;

    FOR_EACH_OOPADDR( addr ) {
        if ( *addr == from ) {
            nativeMethods_to_invalidate->append( my_nativeMethod() );
            return;
        }
    }
#endif
}


void NativeMethodScopes::oops_do( void f( Oop * ) ) {
    Oop       *end = oops() + oops_size();
    for ( Oop *p   = oops(); p < end; p++ ) {
        f( p );
    }
}


void NativeMethodScopes::relocate() {
    FOR_EACH_OOPADDR( addr ) {
        RELOCATE_TEMPLATE( addr );
    }
}


bool NativeMethodScopes::is_new() const {
//    bool result = false;
    FOR_EACH_OOPADDR( addr ) {
        if ( ( *addr )->is_new() )
            return true;
    }
    return false;
}


void NativeMethodScopes::print() {
    ResourceMark m;    // in case methods get printed via debugger
    printIndent();
    spdlog::info( "scopes:" );
    Indent++;
    FOR_EACH_SCOPE( this, d )d->print();
    Indent--;
}


void NativeMethodScopes::print_partition() {

    std::int32_t d_size = dependent_length() * sizeof( Oop );
    std::int32_t o_size = oops_size() * sizeof( Oop ) - d_size;
    std::int32_t p_size = (std::int32_t) pcsEnd() - (std::int32_t) pcs();
    std::int32_t v_size = value_size() * sizeof( std::int32_t );
    std::int32_t total  = v_size + p_size + o_size + d_size;

    spdlog::info( "{deps {}%%, oops {}%%, bytes {}%%, pcs %d%%}", d_size * 100 / total, o_size * 100 / total, v_size * 100 / total, p_size * 100 / total );
}
