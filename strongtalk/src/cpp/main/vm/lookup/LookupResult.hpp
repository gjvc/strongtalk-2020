//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/code/JumpTable.hpp"


// The lookup cache is a 2-way associative cache mapping LookupKeys into LookupResult.
// Called when the inline cache fails or when a compile takes place.

// %gc-note
// When a garbage collect occurs the lookup cache has to be recomputed since the has value is dependent on the location of symbols and klasses.

class LookupResult : ValueObject {

protected:
    Oop _result; // methodOop or JumpTableEntry

public:
    LookupResult();


    LookupResult( MethodOop method );


    LookupResult( const NativeMethod *nm );


    void clear();


    bool_t is_empty() const;


    bool_t is_method() const;


    bool_t is_entry() const;


    bool_t matches( MethodOop m ) const; // Checks whether the result is methodOop m.
    bool_t matches( NativeMethod *nm ) const; // Checks whether the result is NativeMethod nm.

    Oop value() const;


    MethodOop method() const;

    MethodOop method_or_null() const;

    JumpTableEntry *entry() const;

    NativeMethod *get_nativeMethod() const;


    void set( MethodOop method );


    void set( const NativeMethod *nm );


    void print_on( ConsoleOutputStream *stream ) const;

    void print_short_on( ConsoleOutputStream *stream ) const;
};
