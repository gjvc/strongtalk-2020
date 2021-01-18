
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include <array>

#include "vm/assembler/Location.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/utilities/OutputStream.hpp"


static std::array<const char *, nofSpecialLocations> specialLocNames{
        "illegalLocation",          //
        "unAllocated",              //
        "noRegister",               //
        "topOfStack",               //
        "resultOfNonLocalReturn",   //
        "topOfFloatStack"           //
};


// Constructors

void Location::overflow( Mode mode, int f1, int f2, int f3 ) {
    // should handle field overflow somehow - for now: fatal error
    st_fatal( "Location field overflow - please notify the compiler folks" );
}


Location::Location( Mode mode, int f ) {
    _loc = (int) mode + ( f << _fPos );
}


Location::Location( Mode mode, int f1, int f2, int f3 ) {
    if ( ( f1 & _f1Mask ) not_eq f1 or ( f2 & _f2Mask ) not_eq f2 or ( f3 & _f3Mask ) not_eq f3 )
        overflow( mode, f1, f2, f3 );
    _loc = int( mode ) + ( f1 << _f1Pos ) + ( f2 << _f2Pos ) + ( f3 << _f3Pos );
}


const char *Location::name() const {

    char *s;
    switch ( mode() ) {
        case Mode::specialLoc: {
            const char *name = specialLocNames[ id() ];
            s = new_resource_array<char>( strlen( name ) );
            sprintf( s, name );
            break;
        }
        case Mode::registerLoc: {
            const char *name = Mapping::asRegister( *this ).name();
            s                = new_resource_array<char>( 8 );
            sprintf( s, name );
            break;
        }
        case Mode::stackLoc: {
            s = new_resource_array<char>( 8 );
            sprintf( s, "S%d", offset() );
            break;
        }
        case Mode::contextLoc1: {
            s = new_resource_array<char>( 24 );
            sprintf( s, "C0x%08x,%d(%d)", contextNo(), tempNo(), scopeID() );
            break;
        }
        case Mode::contextLoc2: {
            s = new_resource_array<char>( 24 );
            sprintf( s, "C%d,%d[%d]", contextNo(), tempNo(), scopeOffs() );
            break;
        }
        case Mode::floatLoc: {
            s = new_resource_array<char>( 16 );
            sprintf( s, "F%d(%d)", floatNo(), scopeNo() );
            break;
        }
        default: ShouldNotReachHere();
            break;
    }

    return s;
}

// predicates

bool_t Location::isTopOfStack() const {
    return *this == topOfStack or *this == topOfFloatStack;
}


bool_t Location::isTemporaryRegister() const {
    return isRegisterLocation() and Mapping::isTemporaryRegister( *this );
}


bool_t Location::isTrashedRegister() const {
    return isRegisterLocation() and Mapping::isTrashedRegister( *this );
}


bool_t Location::isLocalRegister() const {
    return isRegisterLocation() and Mapping::isLocalRegister( *this );
}

// Implementation of IntFreeList

void IntFreeList::grow() {
    _list->append( _first );
    _first = _list->length() - 1;
}


IntFreeList::IntFreeList( int size ) {
    _first = -1;
    _list  = new GrowableArray<int>( 2 );
    st_assert( _list->length() == 0, "should be zero" );
}


int IntFreeList::allocate() {
    if ( _first < 0 )
        grow();
    int i = _first;
    _first = _list->at( i );
    _list->at_put( i, -1 ); // for debugging only
    return i;
}


int IntFreeList::allocated() {
    int n = length();
    int i = _first;
    while ( i >= 0 ) {
        i = _list->at( i );
        n--;
    };
    st_assert( n >= 0, "should be >= 0" );
    return n;
}


void IntFreeList::release( int i ) {
    st_assert( _list->at( i ) == -1, "should have been allocated before" );
    _list->at_put( i, _first );
    _first = i;
}


int IntFreeList::length() {
    return _list->length();
}


void IntFreeList::print() {
    _console->print( "FreeList 0x%x:", this );
    _list->print();
}
