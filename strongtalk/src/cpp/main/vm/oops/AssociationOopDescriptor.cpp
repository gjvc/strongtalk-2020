//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"


void AssociationOopDescriptor::bootstrap_object( Bootstrap * stream ) {
    MemOopDescriptor::bootstrap_header( stream );
    stream->read_oop( ( Oop * ) &addr()->_key );
    stream->read_oop( ( Oop * ) &addr()->_value );
    stream->read_oop( ( Oop * ) &addr()->_is_constant );
}


void AssociationOopDescriptor::set_is_constant( bool_t v ) {
    bool_t was_constant = is_constant();
    STORE_OOP( &addr()->_is_constant, v ? trueObj : falseObj );
    if ( was_constant and not v ) {
        warning( "We need invalidation of code when changing a constant association to variable" );
        // Invalidate(value());
    }
}
