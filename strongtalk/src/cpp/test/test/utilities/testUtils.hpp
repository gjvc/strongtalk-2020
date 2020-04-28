
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/primitives/byteArray_primitives.hpp"


#define as_large_integer( value ) \
  byteArrayPrimitives::largeIntegerFromSmallInteger(smiOopFromValue(value), KlassOop(Universe::find_global("LargeInteger")))
