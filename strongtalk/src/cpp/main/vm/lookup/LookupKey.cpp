
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/lookup/LookupKey.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/utilities/StringOutputStream.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


std::int32_t LookupKey::hash() const {
    return klass()->identity_hash() ^ selector_or_method()->identity_hash();
}


void LookupKey::relocate() {
    RELOCATE_TEMPLATE( &_klass );
    RELOCATE_TEMPLATE( &_selector_or_method );
}


SymbolOop LookupKey::selector() const {
    if ( is_normal_type() ) {
        return SymbolOop( selector_or_method() );
    } else {
        return method()->selector();
    }
}


bool LookupKey::verify() const {
    bool flag = true;
    if ( not klass()->is_klass() ) {
        error( "klass 0x{0:x} isn't a klass", klass() );
        flag = false;
    }
    if ( not selector_or_method()->is_symbol() and not selector_or_method()->is_method() ) {
        spdlog::info( "\tin selector_or_method of LookupKey 0x{0:x}", static_cast<const void *>(this) );
        flag = false;
    }
    return flag;
}


void LookupKey::switch_pointers( Oop from, Oop to ) {
    SWITCH_POINTERS_TEMPLATE( &_klass );
    SWITCH_POINTERS_TEMPLATE( &_selector_or_method );
}


void LookupKey::oops_do( void f( Oop * ) ) {
    f( (Oop *) &_klass );
    f( (Oop *) &_selector_or_method );
}


void LookupKey::print_on( ConsoleOutputStream *stream ) const {
    print_inlining_database_on( stream );
}


void LookupKey::print_inlining_database_on( ConsoleOutputStream *stream ) const {
    if ( klass() ) {
        klass()->klass_part()->print_name_on( stream );
    } else {
        stream->print( "nullptr" );
    }
    if ( is_normal_type() ) {
        stream->print( "::" );
        selector()->print_symbol_on( stream );
    } else if ( is_super_type() ) {
        stream->print( "^^" );
        method()->selector()->print_symbol_on( stream );
    } else if ( is_block_type() ) {
        stream->print( "->" );
        method()->print_inlining_database_on( stream );
    } else {
        ShouldNotReachHere();
    }
}


void LookupKey::print() const {
    print_on( _console );
}


const char *LookupKey::toString() const {
    StringOutputStream *stream = new StringOutputStream( 50 );
    print_on( stream );
    return stream->as_string();
}


LookupKey *LookupKey::allocate( KlassOop klass, Oop selector_or_method ) {
    LookupKey *result = new_resource_array<LookupKey>( 1 );
    result->initialize( klass, selector_or_method );
    return result;
}


LookupKey::LookupKey() : _klass{}, _selector_or_method{} {
    clear();
}


LookupKey::LookupKey( KlassOop klass, Oop selector_or_method ) :
    _klass{ klass },
    _selector_or_method{ selector_or_method } {
}


LookupKey::LookupKey( LookupKey *key ) :
    _klass{ key->klass() },
    _selector_or_method{ key->selector_or_method() } {
}


KlassOop LookupKey::klass() const {
    return _klass;
}


Oop LookupKey::selector_or_method() const {
    return _selector_or_method;
}


MethodOop LookupKey::method() const {
    st_assert( selector_or_method()->is_method(), "Wrong lookup type" );
    return MethodOop( selector_or_method() );
}


bool LookupKey::is_super_type() const {
    return selector_or_method()->is_method() and not MethodOop( selector_or_method() )->is_blockMethod();
}


bool LookupKey::is_normal_type() const {
    return not selector_or_method()->is_method();
}


bool LookupKey::is_block_type() const {
    return selector_or_method()->is_method() and MethodOop( selector_or_method() )->is_blockMethod();
}


bool LookupKey::equal( const LookupKey *p ) const {
    return klass() == p->klass() and selector_or_method() == p->selector_or_method();
}


void LookupKey::initialize( KlassOop klass, Oop selector_or_method ) {
    _klass              = klass;
    _selector_or_method = selector_or_method;
}


void LookupKey::clear() {
    _klass              = nullptr;
    _selector_or_method = nullptr;
}
