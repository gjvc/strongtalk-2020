
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/lookup/LookupResult.hpp"


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


    void operator delete( void *ptr ) { (void) ptr; }


    bool match( KlassOop klass, SymbolOop selector );


    LookupResult lookup( KlassOop klass, SymbolOop selector );


    LookupResult result();
};
