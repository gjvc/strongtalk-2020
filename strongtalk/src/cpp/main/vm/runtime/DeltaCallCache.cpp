
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/Delta.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"


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
