//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/system/platform.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/lprintf.hpp"


bool_t ResourceMark::_enabled = true;


ResourceMark::ResourceMark() {

    if ( not _enabled )
        return;

    _resourceArea      = &resource_area;
    _resourceAreaChunk = _resourceArea->_resourceAreaChunk;
    _top               = _resourceAreaChunk ? _resourceAreaChunk->_firstFree : nullptr;
    _resourceArea->_nestingLevel++;

    st_assert( _resourceArea->_nestingLevel > 0, "_nesting must be positive" );
}


ResourceMark::~ResourceMark() {

    if ( not _enabled )
        return;

    st_assert( _resourceArea->_nestingLevel > 0, "_nesting must be positive" );
    _resourceArea->_nestingLevel--;
    if ( PrintResourceAllocation ) {
        lprintf( "deallocating to mark %#lx\n", _top );
    }

    ResourceAreaChunk * prevc;
    ResourceAreaChunk * c = _resourceArea->_resourceAreaChunk;
    for ( ; c not_eq _resourceAreaChunk; c = prevc ) {
        // deallocate all chunks behind marked _resourceAreaChunk
        prevc = c->_prev;
        resources.addToFreeList( c );
    }

    _resourceArea->_resourceAreaChunk = c;
    if ( c == nullptr ) {
        _top = nullptr;
        return;
    }
    c->freeTo( _top );
    if ( _top == c->_bottom ) {
        // this _resourceAreaChunk is completely unused - deallocate it
        _resourceArea->_resourceAreaChunk = c->_prev;
        resources.addToFreeList( c );
    }
}


FinalResourceMark::FinalResourceMark() {
    _enabled = false;
}


FinalResourceMark::~FinalResourceMark() {
    _enabled = true;
}
