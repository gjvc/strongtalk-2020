//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/lookup/LookupResult.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/oop/SymbolOopDescriptor.hpp"
#include "vm/runtime/DeltaCallCache.hpp"



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
