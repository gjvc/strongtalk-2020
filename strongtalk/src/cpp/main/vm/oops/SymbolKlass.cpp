
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/oops/SymbolKlass.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


SymbolOop SymbolKlass::allocateSymbol( const char *value, std::int32_t len ) {
    spdlog::info( "%oops-SymbolKlass: SymbolKlass::allocateSymbol: symbol[{}]", value );
    SymbolOop sym = as_symbolOop( Universe::allocate_tenured( object_size( len ) ) );
    sym->init_untagged_contents_mark();
    sym->set_klass_field( Universe::symbolKlassObject() );
    sym->set_length( len );
    initialize_object( sym, value, len );
    return sym;
}


KlassOop SymbolKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::symbol_klass ) {
        return SymbolKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop SymbolKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    SymbolKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


void setKlassVirtualTableFromSymbolKlass( Klass *k ) {
    SymbolKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop SymbolKlass::scavenge( Oop obj ) {
    ShouldNotCallThis(); // shouldn't need to scavenge canonical symbols
    // (should be tenured)
    return nullptr;
}


bool SymbolKlass::verify( Oop obj ) {
    return SymbolOop( obj )->verify();
}


void SymbolKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    st_assert_symbol( obj, "dispatch check" );
    SymbolOop    array = SymbolOop( obj );
    std::int32_t len   = array->length();
    std::int32_t n     = min( MaxElementPrintSize, len );
    stream->print( "#" );
    for ( std::int32_t i = 1; i <= n; i++ ) {
        char c = array->byte_at( i );
        if ( isprint( c ) )
            stream->print( "%c", c );
        else
            stream->print( "\\%o", c );
    }
    if ( n < len )
        stream->print( "..." );
}


void SymbolKlass::print( Oop obj ) {
    st_assert_symbol( obj, "dispatch check" );
    _console->print( "'" );
    SymbolOop( obj )->print_symbol_on();
    _console->print( "' " );
}


Oop SymbolKlass::oop_shallow_copy( Oop obj, bool tenured ) {
    st_assert_symbol( obj, "dispatch check" );
    return obj;
}
