//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/asserts.hpp"


// -----------------------------------------------------------------------------

// Concatenates a and b (which are not macro-expanded)
#define CONC( a, b )        a##b

// Concatenates a, b, and c (which are not macro-expanded)
#define CONC3( a, b, c )    a##b##c

// Makes a string of the argument (which is not macro-expanded)
#define STR( a )            #a

// Makes a character of the argument (which is not macro-expanded)
#define CHAR( a )           ((#a)[0])

// Concatenates the macro expansions of a and b
#define XCONC( a, b )       CONC(a,b)

// Makes a string of the macro expansion of a
#define XSTR( a )           STR(a)

// Makes a character of the macro expansion of a
#define XCHAR( a )          CHAR(a)


// ------------------------ Type and value checking macros --------------------

// Macro to verify the type of an Oop, and create a requalified Oop.
// E.g. CHECKOOPTYPE(host, isByteArray, ByteArrayOop, host1) expands to:
//      if(not host->isByteArray()) return primitive_error(BADTYPEERROR);
//      ByteArrayOop host1 = ByteArrayOop(host);
// Warning: does not wrap in '{' and '}'!

#define CHECKOOPTYPE( ref, typePredicate, newType, newRef ) \
    if (not ref->typePredicate()) \
      return primitive_error(BADTYPEERROR); \
    newType newRef = newType(ref);


// Check that ref is a SmallIntegerOop, and set up variable to hold value.
// Warning: does not wrap in '{' and '}'!

#define CHECKOOPSMI( ref, val ) \
    if (not ref->isSmallIntegerOop()) \
      return primitive_error(BADTYPEERROR); \
    smi val = SmallIntegerOop(ref)->value();


// Check that ref is either trueObject or falseObject. Set up boolean var corresp.
// Warning: does not wrap in '{' and '}'!

#define CHECKOOPBOOL( ref, val ) \
    bool val; \
    if (ref == falseObject) \
      val = false; \
    else if (ref == trueObject) \
      val = true; \
    else \
      return primitive_error(BADTYPEERROR);
