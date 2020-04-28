//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/memory/oopFactory.hpp"


SymbolOop vm_symbols[terminating_enum];

#define VMSYMBOL_INIT( name, string ) \
  vm_symbols[ VMSYMBOL_ENUM_NAME( name ) ] = oopFactory::new_symbol( string );


void vmSymbols::initialize() {
    VMSYMBOLS( VMSYMBOL_INIT )
}


void vmSymbols::switch_pointers( Oop from, Oop to ) {
    for ( int i = 0; i < terminating_enum; i++ ) {
        Oop * p = ( Oop * ) &vm_symbols[ i ];
        SWITCH_POINTERS_TEMPLATE( p )
    }
}


void vmSymbols::follow_contents() {
    for ( int i = 0; i < terminating_enum; i++ ) {
        MarkSweep::follow_root( ( Oop * ) &vm_symbols[ i ] );
    }
}


void vmSymbols::relocate() {
    for ( int i = 0; i < terminating_enum; i++ ) {
        Oop * p = ( Oop * ) &vm_symbols[ i ];
        RELOCATE_TEMPLATE( p );
    }
}


void vmSymbols::verify() {

}
