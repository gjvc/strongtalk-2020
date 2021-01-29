//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/PrintObjectClosure.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"


constexpr std::int32_t indent_col = 3;
constexpr std::int32_t value_col  = 16;


PrintObjectClosure::PrintObjectClosure( ConsoleOutputStream *stream ) {
    this->_stream = stream ? stream : _console;
}


void PrintObjectClosure::do_object( MemOop obj ) {
    _memOop = obj;
    obj->print_value();
    if ( WizardMode ) {
        _stream->print( " (size = %d)", obj->size() );
    }
    _stream->cr();
}


void PrintObjectClosure::do_mark( MarkOop *m ) {
    _stream->fill_to( indent_col );
    _stream->print( "mark" );
    _stream->sp();
    _stream->fill_to( value_col );
    ( *m )->print_value();
    _stream->cr();
}


void PrintObjectClosure::do_oop( const char *title, Oop *o ) {
    SymbolOop name = _memOop->blueprint()->inst_var_name_at( o - (Oop *) _memOop->addr() );
    _stream->fill_to( indent_col );
    if ( name ) {
        name->print_symbol_on( _stream );
    } else {
        _stream->print( "%s", title );
    }
    _stream->sp();
    _stream->fill_to( value_col );
    ( *o )->print_value();
    _stream->cr();
}


void PrintObjectClosure::do_byte( const char *title, std::uint8_t *b ) {
    _stream->fill_to( indent_col );
    _stream->print( "%s", title );
    _stream->sp();
    _stream->fill_to( value_col );
    char c = (char) *b;
    if ( isprint( c ) )
        _stream->print_cr( "%c", c );
    else
        _stream->print_cr( "\\%o", c );
}


void PrintObjectClosure::do_long( const char *title, void **p ) {
    _stream->fill_to( indent_col );
    _stream->print( "%s", title );
    _stream->sp();
    _stream->fill_to( value_col );
    _stream->print_cr( "0x{0:x}", *p );
}


void PrintObjectClosure::do_double( const char *title, double *d ) {
    _stream->fill_to( indent_col );
    _stream->print( "%s", title );
    _stream->sp();
    _stream->fill_to( value_col );
    _stream->print_cr( "%.15f", *d );
}


void PrintObjectClosure::begin_indexables() {
}


void PrintObjectClosure::end_indexables() {
}


void PrintObjectClosure::do_indexable_oop( std::int32_t index, Oop *o ) {
    if ( index > MaxElementPrintSize )
        return;
    _stream->fill_to( indent_col );
    _stream->print( "%d", index );
    _stream->sp();
    _stream->fill_to( value_col );
    ( *o )->print_value();
    _stream->cr();
}


void PrintObjectClosure::do_indexable_byte( std::int32_t index, std::uint8_t *b ) {
    if ( index > MaxElementPrintSize )
        return;
    _stream->fill_to( indent_col );
    _stream->print( "%d", index );
    _stream->sp();
    _stream->fill_to( value_col );
    std::int32_t c = (std::int32_t) *b;
    if ( isprint( c ) )
        _stream->print_cr( "%c", c );
    else
        _stream->print_cr( "\\%o", c );
}


void PrintObjectClosure::do_indexable_doubleByte( std::int32_t index, std::uint16_t *b ) {
    if ( index > MaxElementPrintSize )
        return;
    _stream->fill_to( indent_col );
    _stream->print( "%d", index );
    _stream->sp();
    _stream->fill_to( value_col );
    std::int32_t c = (std::int32_t) *b;
    if ( isprint( c ) )
        _stream->print_cr( "%c", c );
    else
        _stream->print_cr( "\\%o", c );
}


void PrintObjectClosure::do_indexable_long( std::int32_t index, std::int32_t *l ) {
    if ( index > MaxElementPrintSize )
        return;
    _stream->fill_to( indent_col );
    _stream->print( "%d", index );
    _stream->sp();
    _stream->fill_to( value_col );
    _stream->print_cr( "0x%lx", *l );
}
