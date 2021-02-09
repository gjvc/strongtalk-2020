
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/primitives/ByteArrayPrimitives.hpp"


#define as_large_integer( value ) \
  ByteArrayPrimitives::largeIntegerFromSmallInteger(smiOopFromValue(value), KlassOop(Universe::find_global("LargeInteger")))
