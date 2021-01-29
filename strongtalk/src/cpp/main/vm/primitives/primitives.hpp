//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/PrimitiveGroup.hpp"
#include "vm/primitives/primitive_tracing.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"



// The file defines the interface to the internal primitives.

// Primitives are divided into four categories:
//                                                        (canScavenge, NonLocalReturn,   canBeConstantFolded)
// 1) Allocation primitives.                              (true,        false, false)
// 2) Pure primitives.                                    (false,       false, true)
// 3) Function primitives                                 (false,       false, false)
// 4) Primitives who might scavenge, gc, or call delta    (true,        false, false)
//    .

class PrimitiveDescriptor;


class InterpretedPrimitiveCache : public ValueObject {

private:
    std::uint8_t *hp() const {
        return (std::uint8_t *) this;
    }


public:
    SymbolOop name() const;

    std::int32_t number_of_parameters() const;

    PrimitiveDescriptor *pdesc() const;

    bool has_receiver() const;

    bool has_failure_code() const;
};


// The PrimitiveDescriptor structure exposes all properties of a primitive.
// Primitives are like procedures (no dispatching is necessary) and invoked by providing a number of parameters.

class PseudoRegister;

class Expression;

class Node;

#include "vm/primitives/PrimitiveDescriptor.hpp"

class Primitives : AllStatic {

public:
    static void print_table();


    static PrimitiveDescriptor *lookup( SymbolOop selector ) {
        return lookup( (const char *) selector->bytes(), selector->length() );
    }


    static PrimitiveDescriptor *lookup( primitiveFunctionType fn );

    static void lookup_and_patch(); // routine called by interpreter

    // For debugging / profiling
    static void clear_counters();

    static void print_counters();

    static void initialize();

    static void patch( const char *name, const char *entry_point );

private:
    static PrimitiveDescriptor *lookup( const char *selector, std::int32_t selector_length );
    static PrimitiveDescriptor *lookup( const char *selector );


    static PrimitiveDescriptor *verified_lookup( const char *selector ); // Fails if the primitive could not be found

    // Primitives used by the compiler are looked up at startup
    static PrimitiveDescriptor *_new0;
    static PrimitiveDescriptor *_new1;
    static PrimitiveDescriptor *_new2;
    static PrimitiveDescriptor *_new3;
    static PrimitiveDescriptor *_new4;
    static PrimitiveDescriptor *_new5;
    static PrimitiveDescriptor *_new6;
    static PrimitiveDescriptor *_new7;
    static PrimitiveDescriptor *_new8;
    static PrimitiveDescriptor *_new9;

    static PrimitiveDescriptor *_equal;
    static PrimitiveDescriptor *_not_equal;

    static PrimitiveDescriptor *_block_allocate;
    static PrimitiveDescriptor *_block_allocate0;
    static PrimitiveDescriptor *_block_allocate1;
    static PrimitiveDescriptor *_block_allocate2;

    static PrimitiveDescriptor *_context_allocate;
    static PrimitiveDescriptor *_context_allocate0;
    static PrimitiveDescriptor *_context_allocate1;
    static PrimitiveDescriptor *_context_allocate2;

public:
    static PrimitiveDescriptor *new0() {
        return _new0;
    }


    static PrimitiveDescriptor *new1() {
        return _new1;
    }


    static PrimitiveDescriptor *new2() {
        return _new2;
    }


    static PrimitiveDescriptor *new3() {
        return _new3;
    }


    static PrimitiveDescriptor *new4() {
        return _new4;
    }


    static PrimitiveDescriptor *new5() {
        return _new5;
    }


    static PrimitiveDescriptor *new6() {
        return _new6;
    }


    static PrimitiveDescriptor *new7() {
        return _new7;
    }


    static PrimitiveDescriptor *new8() {
        return _new8;
    }


    static PrimitiveDescriptor *new9() {
        return _new9;
    }


    static PrimitiveDescriptor *equal() {
        return _equal;
    }


    static PrimitiveDescriptor *not_equal() {
        return _not_equal;
    }


    static PrimitiveDescriptor *block_allocate() {
        return _block_allocate;
    }


    static PrimitiveDescriptor *block_allocate0() {
        return _block_allocate0;
    }


    static PrimitiveDescriptor *block_allocate1() {
        return _block_allocate1;
    }


    static PrimitiveDescriptor *block_allocate2() {
        return _block_allocate2;
    }


    static PrimitiveDescriptor *context_allocate() {
        return _context_allocate;
    }


    static PrimitiveDescriptor *context_allocate0() {
        return _context_allocate0;
    }


    static PrimitiveDescriptor *context_allocate1() {
        return _context_allocate1;
    }


    static PrimitiveDescriptor *context_allocate2() {
        return _context_allocate2;
    }
};
