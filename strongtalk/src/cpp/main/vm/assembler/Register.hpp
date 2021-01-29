
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"


constexpr std::int32_t REGISTER_COUNT = 8;    // total number of registers


class Register : public ValueObject {

private:
    std::int32_t _number;

public:
    Register( void );

    explicit Register( std::int32_t number, char f );    // f is only to make sure that an std::int32_t is not accidentally converted into a Register...

    // attributes
    std::int32_t number() const;


    bool isValid() const;


    bool hasByteRegister() const;


    bool operator==( const Register &rhs ) const;


    bool operator!=( const Register &rhs ) const;


    // debugging
    const char *name() const;
};


// Available registers
const Register eax = Register( 0, ' ' );
const Register ecx = Register( 1, ' ' );
const Register edx = Register( 2, ' ' );
const Register ebx = Register( 3, ' ' );
const Register esp = Register( 4, ' ' );
const Register ebp = Register( 5, ' ' );
const Register esi = Register( 6, ' ' );
const Register edi = Register( 7, ' ' );
const Register noreg; // Dummy register used in Load, LoadAddr, and Store.
