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
    _lookupKey{},
    _filler{},
    _lookupResult{} {
}


void CacheElement::verify() {
    st_assert( sizeof( CacheElement ) == 16, "checking structure layout" );
    st_assert( (std::int32_t) &_lookupKey - (std::int32_t) this == 0, "checking structure layout" );
    st_assert( (std::int32_t) &_lookupResult - (std::int32_t) this == 8, "checking structure layout" );

    if ( _lookupKey.klass() or _lookupKey.selector_or_method() ) {
        if ( _lookupResult.is_empty() ) {
            spdlog::info( "Verify failed in LookupCache: " );
            spdlog::info( "  element = ({}::{})", _lookupKey.klass()->print_value_string(), _lookupKey.selector_or_method()->print_value_string() );
//            spdlog::info( "result = ({})", _lookupResult.print_value_string() );
            st_fatal( "LookupCache verify failed" );
        }
        const NativeMethod *nm = Universe::code->lookup( &_lookupKey );
        if ( _lookupResult.is_method() and nm ) {
            error( "key %s has interpreted method in lookupTable but should have NativeMethod 0x{0:x}", _lookupKey.toString(), nm );
        } else if ( _lookupResult.is_entry() and _lookupResult.get_nativeMethod() not_eq nm ) {
            error( "key %s: NativeMethod does not match codeTable NativeMethod", _lookupKey.toString() );
        }
        if ( UseInliningDatabaseEagerly and _lookupResult.is_method() and InliningDatabase::lookup( &_lookupKey ) ) {
            error( "key %s: interpreted method in lookupTable despite inlining DB entry", _lookupKey.toString() );
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
