
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

void Location::overflow( Mode mode, std::int32_t f1, std::int32_t f2, std::int32_t f3 ) {
    // should handle field overflow somehow - for now: fatal error
    st_fatal( "Location field overflow - please notify the compiler folks" );
}


Location::Location( Mode mode, std::int32_t f ) {
    _loc = (std::int32_t) mode + ( f << _fPos );
}


Location::Location( Mode mode, std::int32_t f1, std::int32_t f2, std::int32_t f3 ) {
    if ( ( f1 & _f1Mask ) not_eq f1 or ( f2 & _f2Mask ) not_eq f2 or ( f3 & _f3Mask ) not_eq f3 )
        overflow( mode, f1, f2, f3 );
    _loc = std::int32_t( mode ) + ( f1 << _f1Pos ) + ( f2 << _f2Pos ) + ( f3 << _f3Pos );
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

bool Location::isTopOfStack() const {
    return *this == topOfStack or *this == topOfFloatStack;
}


bool Location::isTemporaryRegister() const {
    return isRegisterLocation() and Mapping::isTemporaryRegister( *this );
}


bool Location::isTrashedRegister() const {
    return isRegisterLocation() and Mapping::isTrashedRegister( *this );
}


bool Location::isLocalRegister() const {
    return isRegisterLocation() and Mapping::isLocalRegister( *this );
}

// Implementation of IntFreeList

void IntFreeList::grow() {
    _list->append( _first );
    _first = _list->length() - 1;
}


IntFreeList::IntFreeList( std::int32_t size ) {
    _first = -1;
    _list  = new GrowableArray<std::int32_t>( 2 );
    st_assert( _list->length() == 0, "should be zero" );
}


std::int32_t IntFreeList::allocate() {
    if ( _first < 0 )
        grow();
    std::int32_t i = _first;
    _first = _list->at( i );
    _list->at_put( i, -1 ); // for debugging only
    return i;
}


std::int32_t IntFreeList::allocated() {
    std::int32_t n = length();
    std::int32_t i = _first;
    while ( i >= 0 ) {
        i = _list->at( i );
        n--;
    };
    st_assert( n >= 0, "should be >= 0" );
    return n;
}


void IntFreeList::release( std::int32_t i ) {
    st_assert( _list->at( i ) == -1, "should have been allocated before" );
    _list->at_put( i, _first );
    _first = i;
}


std::int32_t IntFreeList::length() {
    return _list->length();
}


void IntFreeList::print() {
    spdlog::info( "FreeList 0x{0:x}:", static_cast<const void *>(this) );
    _list->print();
}
