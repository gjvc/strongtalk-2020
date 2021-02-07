//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/lookup/LookupResult.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"


// A DeltaCallCache caches methods for Delta calls.
// They are primarily used to speed up performs.
//
// %todo:
//   clear all DeltaCallCache at garbage collection

class DeltaCallCache : public AllStatic {

private:
    static DeltaCallCache *_root;    // root of all DeltaCallCaches
    DeltaCallCache        *_link;    // all DeltaCallCaches are linked

    // cache
    LookupKey    _key;
    LookupResult _result;

public:
    DeltaCallCache();           //
    void clear();               // clears the cache
    static void clearAll();     // clears all DeltaCallCaches (called by GC, etc.)

    virtual ~DeltaCallCache() = default;
    DeltaCallCache( const DeltaCallCache & ) = default;
    DeltaCallCache &operator=( const DeltaCallCache & ) = default;
    void operator delete( void *ptr ) { (void)ptr; }


    bool match( KlassOop klass, SymbolOop selector ) {
        return Oop( selector ) == _key.selector_or_method() and klass == _key.klass();
    }


    LookupResult lookup( KlassOop klass, SymbolOop selector ) {
        if ( not match( klass, selector ) ) {
            _result = interpreter_normal_lookup( klass, selector );
            if ( not _result.is_empty() ) {
                _key.initialize( klass, selector );
            }
        }
        return _result;
    }


    LookupResult result() {
        return _result;
    }
};


// Delta provides the following entry points for calling Delta methods

class Delta : public AllStatic {

public:
    // The following functions return a marked Oop if the selector is not a SymbolOop
    static Oop call_generic( DeltaCallCache *ic, Oop receiver, Oop selector, std::int32_t nofArgs, Oop *args );

    static Oop call( Oop receiver, Oop selector );

    static Oop call( Oop receiver, Oop selector, Oop arg1 );

    static Oop call( Oop receiver, Oop selector, Oop arg1, Oop arg2 );

    static Oop call( Oop receiver, Oop selector, Oop arg1, Oop arg2, Oop arg3 );

    static Oop call( Oop receiver, Oop selector, ObjectArrayOop args );

    static Oop does_not_understand( Oop receiver, SymbolOop selector, std::int32_t nofArgs, Oop *argArray );
};
