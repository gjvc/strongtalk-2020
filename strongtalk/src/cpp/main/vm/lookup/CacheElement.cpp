//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/utilities/OutputStream.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/Sweeper.hpp"
#include "vm/lookup/CacheElement.hpp"


CacheElement::CacheElement() :
        _lookupKey(), _lookupResult() {
}


void CacheElement::verify() {
    st_assert( sizeof( CacheElement ) == 16, "checking structure layout" );
    st_assert( (int) &_lookupKey - (int) this == 0, "checking structure layout" );
    st_assert( (int) &_lookupResult - (int) this == 8, "checking structure layout" );

    if ( _lookupKey.klass() or _lookupKey.selector_or_method() ) {
        if ( _lookupResult.is_empty() ) {
            _console->print( "Verify failed in LookupCache: " );
            _console->cr();
            _console->print( "  element = (" );
            _lookupKey.klass()->print_value_on( _console );
            _console->print( "::" );
            _lookupKey.selector_or_method()->print_value_on( _console );
            _console->print( ")" );
            _console->cr();
            _console->print( "  result = (" );
            _lookupResult.print_on( _console );
            _console->print_cr( ")" );
            st_fatal( "LookupCache verify failed" );
        }
        const NativeMethod *nm = Universe::code->lookup( &_lookupKey );
        if ( _lookupResult.is_method() and nm ) {
            error( "key %s has interpreted method in lookupTable but should have NativeMethod %#x", _lookupKey.print_string(), nm );
        } else if ( _lookupResult.is_entry() and _lookupResult.get_nativeMethod() not_eq nm ) {
            error( "key %s: NativeMethod does not match codeTable NativeMethod", _lookupKey.print_string() );
        }
        if ( UseInliningDatabaseEagerly and _lookupResult.is_method() and InliningDatabase::lookup( &_lookupKey ) ) {
            error( "key %s: interpreted method in lookupTable despite inlining DB entry", _lookupKey.print_string() );
        }
    }
}


void CacheElement::clear() {
    _lookupKey.initialize( nullptr, nullptr );
    _lookupResult.clear();
}


void CacheElement::initialize( LookupKey *k, LookupResult r ) {
    _lookupKey    = *k;
    _lookupResult = r;
}
