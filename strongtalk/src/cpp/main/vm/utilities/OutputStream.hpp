
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


protected:
    const std::ostream &_output;




};
