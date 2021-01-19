//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/memory/SpaceSizes.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/assembler/Displacement.hpp"
#include "vm/assembler/Label.hpp"


Displacement::Displacement( int data ) {
    _data = data;
}


Displacement::Type Displacement::type() const {
    return Type( ( _data >> type_pos ) & type_mask );
}


int Displacement::info() const {
    return ( ( _data >> info_pos ) & info_mask );
}


int Displacement::data() const {
    return _data;
}


void Displacement::next( const Label &L ) const {
    int n = ( ( _data >> next_pos ) & next_mask );
    n > 0 ? L.link_to( n ) : L.unuse();
}


void Displacement::link_to( const Label &L ) {
    init( L, type(), info() );
}


Displacement::Displacement( const Label &L, Displacement::Type type, int info ) {
    init( L, type, info );
}


void Displacement::print() {
    const char *s;
    switch ( type() ) {
        case Displacement::Type::call:
            s = "call";
            break;
        case Displacement::Type::absolute_jump:
            s = "jmp ";
            break;
        case Displacement::Type::conditional_jump:
            s = "jcc ";
            break;
        case Displacement::Type::ic_info:
            s = "nlr ";
            break;
        default:
            s = "????";
            break;
    }
    _console->print( "%s (info = 0x%x)", s, info() );
}


void Displacement::init( const Label &L, Displacement::Type type, int info ) {
    st_assert( not L.is_bound(), "label is bound" );
    int next = 0;
    if ( L.is_unbound() ) {
        next = L.pos();
        st_assert( next > 0, "Displacements must be at positions > 0" );
    }
    st_assert( ( next & ~next_mask ) == 0, "next field too small" );
    st_assert( ( static_cast<std::size_t>(type) & ~type_mask ) == 0, "type field too small" );
    st_assert( ( info & ~info_mask ) == 0, "info field too small" );
    _data = ( next << next_pos ) | ( static_cast<std::size_t>(type) << static_cast<std::size_t>(type_pos) ) | ( info << info_pos );
}
