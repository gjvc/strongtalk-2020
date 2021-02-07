
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <fstream>

#include "vm/runtime/ResourceObject.hpp"


class OutputStream : public ResourceObject {

public:
    OutputStream();
    virtual ~OutputStream() = default;
    OutputStream( const OutputStream & ) = default;
    OutputStream &operator=( const OutputStream & ) = default;


    void operator delete( void *ptr ) { (void)ptr; }


protected:
    const std::ostream &_output;


};
