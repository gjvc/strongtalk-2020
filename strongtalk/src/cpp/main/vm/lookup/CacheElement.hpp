
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/lookup/LookupResult.hpp"


class CacheElement {

public:
    LookupKey    _lookupKey;
    LookupResult _lookupResult;
    std::int32_t _filler;


    CacheElement() :
        _lookupKey{},
        _lookupResult{},
        _filler{} {
    }


    void verify();
    void clear();
    void initialize( LookupKey *k, LookupResult r );

};
