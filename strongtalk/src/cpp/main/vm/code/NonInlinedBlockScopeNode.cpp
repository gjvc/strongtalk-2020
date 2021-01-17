//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/NonInlinedBlockScopeNode.hpp"
#include "vm/memory/Array.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/lprintf.hpp"


void NonInlinedBlockScopeNode::generate( ScopeDescriptorRecorder * rec ) {

    _offset = rec->_codes->size();
    rec->genScopeDescHeader( code(), false, false, false, false, false, false, false );
    rec->genOop( _method );
    rec->genValue( _offset - _parent->_offset );

    if ( WizardMode )
        lprintf( "generating NonInlinedBlockScopeNode at %d\n", _offset );
}
