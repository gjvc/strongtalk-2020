
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include <array>

#include "vm/assembler/Location.hpp"
#include "vm/assembler/x86_mapping.hpp"
#include "vm/utility/OutputStream.hpp"


// special locations
Location Location::ILLEGAL_LOCATION           = Location::specialLocation( 0 );
Location Location::UNALLOCATED_LOCATION       = Location::specialLocation( 1 );
Location Location::NO_REGISTER                = Location::specialLocation( 2 );
Location Location::TOP_OF_STACK               = Location::specialLocation( 3 );
Location Location::RESULT_OF_NON_LOCAL_RETURN = Location::specialLocation( 4 );
Location Location::TOP_OF_FLOAT_STACK         = Location::specialLocation( 5 );    // only used if UseFPUStack is true


static std::array<const char *, 6> specialLocationNames{
    "ILLEGAL_LOCATION",             //
    "UNALLOCATED_LOCATION",         //
    "NO_REGISTER",                  //
    "TOP_OF_STACK",                 //
    "RESULT_OF_NON_LOCAL_RETURN",   //
    "TOP_OF_FLOAT_STACK"            //
};


// Constructors

void Location::overflow( LocationMode mode, std::int32_t f1, std::int32_t f2, std::int32_t f3 ) {
    st_unused( mode ); // unused
    st_unused( f1 ); // unused
    st_unused( f2 ); // unused
    st_unused( f3 ); // unused

    // should handle field overflow somehow - for now: fatal error
    st_fatal( "Location field overflow - please notify the compiler folks" );
}


Location::Location( LocationMode mode, std::int32_t f ) :
    _loc{ (std::int32_t) mode + ( f << _fPos ) } {
}


Location::Location( LocationMode mode, std::int32_t f1, std::int32_t f2, std::int32_t f3 ) : _loc{ 0 } {

    if ( ( f1 & _f1Mask ) not_eq f1 or ( f2 & _f2Mask ) not_eq f2 or ( f3 & _f3Mask ) not_eq f3 ) {
        overflow( mode, f1, f2, f3 );
    }

    _loc = std::int32_t( mode ) + ( f1 << _f1Pos ) + ( f2 << _f2Pos ) + ( f3 << _f3Pos );
}


const char *Location::name() const {

    char *s{ nullptr };

    switch ( mode() ) {
        case LocationMode::SPECIAL_LOCATION: {
            const char *name = specialLocationNames[ id() ];
            s = new_resource_array<char>( strlen( name ) );
            sprintf( s, name );
            break;
        }
        case LocationMode::REGISTER_LOCATION: {
            const char *name = Mapping::asRegister( *this ).name();
            s                = new_resource_array<char>( 8 );
            sprintf( s, name );
            break;
        }
        case LocationMode::STACK_LOCATION: {
            s = new_resource_array<char>( 8 );
            sprintf( s, "S%d", offset() );
            break;
        }
        case LocationMode::CONTEXT_LOCATIION_1: {
            s = new_resource_array<char>( 24 );
            sprintf( s, "C0x%08x,%d(%d)", contextNo(), tempNo(), scopeID() );
            break;
        }
        case LocationMode::CONTEXT_LOCATIION_2: {
            s = new_resource_array<char>( 24 );
            sprintf( s, "C%d,%d[%d]", contextNo(), tempNo(), scopeOffs() );
            break;
        }
        case LocationMode::FLOAT_LOCATION: {
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
    return ( *this == TOP_OF_STACK ) or ( *this == TOP_OF_FLOAT_STACK );
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
