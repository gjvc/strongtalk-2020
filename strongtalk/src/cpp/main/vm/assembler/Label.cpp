//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/assembler/Label.hpp"


int Label::pos() const {
    if ( _pos < 0 )
        return -_pos - 1;
    if ( _pos > 0 )
        return _pos - 1;
    ShouldNotReachHere();
    return 0;
}


void Label::bind_to( int pos ) {
    st_assert( pos >= 0, "illegal position" );
    _pos = -pos - 1;
}


void Label::link_to( int pos ) {
    st_assert( pos >= 0, "illegal position" );
    _pos = pos + 1;
}


void Label::unuse() {
    _pos = 0;
}


bool_t Label::is_bound() const {
    return _pos < 0;
}


bool_t Label::is_unbound() const {
    return _pos > 0;
}


bool_t Label::is_unused() const {
    return _pos == 0;
}


Label::Label() :
    _pos( 0 ) {
}


Label::~Label() {
    st_assert( not is_unbound(), "unbound label" );
}
