//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/lookup/LookupResult.hpp"


LookupResult::LookupResult() {
    clear();
}


MethodOop LookupResult::method() const {
    st_assert( not is_empty(), "cannot be empty" );
    if ( is_method() )
        return MethodOop( _result );
    return get_nativeMethod()->method();
}


MethodOop LookupResult::method_or_null() const {
    if ( is_empty() )
        return nullptr;
    if ( is_method() )
        return MethodOop( _result );
    return get_nativeMethod()->method();
}


JumpTableEntry * LookupResult::entry() const {
    st_assert( not is_empty(), "cannot be empty" );
    return is_entry() ? ( JumpTableEntry * ) _result : nullptr;
}


NativeMethod * LookupResult::get_nativeMethod() const {
    JumpTableEntry * e = entry();
    return e ? e->method() : nullptr;
}


bool_t LookupResult::matches( MethodOop m ) const {
    if ( is_empty() )
        return false;
    return is_method() ? method() == m : false;
}


bool_t LookupResult::matches( NativeMethod * nm ) const {
    if ( is_empty() )
        return false;
    return is_entry() ? entry()->method() == nm : false;
}


void LookupResult::print_on( ConsoleOutputStream * stream ) const {
    if ( is_empty() ) {
        stream->print( "empty" );
    } else {
        method()->print_value_on( stream );
        stream->print( ", 0x%lx", value() );
    }
}


void LookupResult::print_short_on( ConsoleOutputStream * stream ) const {
    if ( is_empty() ) {
        stream->print( "empty" );
    } else {
        stream->print( "0x%lx", value() );
    }
}


LookupResult::LookupResult( MethodOop method ) {
    set( method );
}


LookupResult::LookupResult( const NativeMethod * nm ) {
    set( nm );
}


void LookupResult::clear() {
    _result = nullptr;
}


bool_t LookupResult::is_empty() const {
    return _result == nullptr;
}


bool_t LookupResult::is_method() const {
    return _result->is_mem();
}


bool_t LookupResult::is_entry() const {
    return not is_method();
}


Oop LookupResult::value() const {
    return _result;
}


void LookupResult::set( MethodOop method ) {
    st_assert( method->is_method(), "must be method" );
    _result = Oop( method );
}


void LookupResult::set( const NativeMethod * nm ) {
    st_assert( Oop(nm)->is_smi(), "NativeMethod must be aligned" );
    _result = Oop( nm->jump_table_entry()->entry_point() );
}
