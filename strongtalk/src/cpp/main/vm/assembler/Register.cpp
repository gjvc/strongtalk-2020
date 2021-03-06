
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/macros.hpp"
#include "vm/platform/platform.hpp"
#include "vm/assembler/Register.hpp"

#include <array>

std::array<const char *, REGISTER_COUNT> registerNames = {
    "eax",  //
    "ecx",  //
    "edx",  //
    "ebx",  //
    "esp",  //
    "ebp",  //
    "esi",  //
    "edi"   //
};


const char *Register::name() const {
    return (const char *) ( isValid() ? registerNames[ _number ] : "noreg" );
}


Register::Register( void ) :
    _number( -1 ) {
}


Register::Register( std::int32_t number, char f ) :
    _number( number ) {
    st_unused( f ); // unused
}


std::int32_t Register::number() const {
    st_assert( isValid(), "not a register" );
    return _number;
}


bool Register::isValid() const {
    return ( 0 <= _number ) and ( _number < REGISTER_COUNT );
}


bool Register::hasByteRegister() const {
    return 0 <= _number and _number <= 3;
}


bool Register::operator==( const Register &rhs ) const {
    return rhs._number == _number;
}


bool Register::operator!=( const Register &rhs ) const {
    return rhs._number != _number;
}
