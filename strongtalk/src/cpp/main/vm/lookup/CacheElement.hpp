//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/lookup/LookupResult.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/Sweeper.hpp"


class CacheElement { // : ValueObj {

public:
    LookupKey    _lookupKey;
    LookupResult _lookupResult;
    int          _filler;


    CacheElement();
    void verify();
    void clear();
    void initialize( LookupKey *k, LookupResult r );

};
