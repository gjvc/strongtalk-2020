
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/lookup/LookupCache.hpp"
#include "vm/runtime/DeltaCallCache.hpp"
#include "vm/oop/SymbolOopDescriptor.hpp"


DeltaCallCache::DeltaCallCache() :
    _link{ _root },
    _key{},
    _result{} {
    _root = this;
    clear();
}


void DeltaCallCache::clear() {
    _key.clear();
    _result.clear();
}


void DeltaCallCache::clearAll() {
    DeltaCallCache *p = _root;
    while ( p not_eq nullptr ) {
        p->clear();
        p = p->_link;
    }
}


LookupResult DeltaCallCache::lookup( KlassOop klass, SymbolOop selector ) {
    if ( not match( klass, selector ) ) {
        _result = interpreter_normal_lookup( klass, selector );
        if ( not _result.is_empty() ) {
            _key.initialize( klass, selector );
        }
    }
    return _result;
}


bool DeltaCallCache::match( KlassOop klass, SymbolOop selector ) {
    return Oop( selector ) == _key.selector_or_method() and klass == _key.klass();
}


LookupResult DeltaCallCache::result() {
    return _result;
}
