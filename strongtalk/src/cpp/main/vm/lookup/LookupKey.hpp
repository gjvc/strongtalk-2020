//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


// LookupKeys are the keys into the code table.
// There should be at most one compiled method for a given LookupKey.
// A LookupKey can take two forms:
// 1) normal send lookup key (is_normal_type() == true). {receiver klass, selector}
// 2) super  send lookup key (is_super_type()  == true). {receiver klass, method}
// 2) fake lookup key for block methods {receiver klass, block method}

// LookupKeys are allocated in different ways:
//   as embedded objects: the LookupCache has LookupKeys as part of an array element.
//   as dynamically allocated resource objects: used by the optimizing compiler.
//      Use "LookupKey::allocate" for allocating a LookupKey in the resource area.


class LookupKey : ValueObject {

protected:
    KlassOop _klass;
    Oop      _selector_or_method;

public:
    LookupKey();
    LookupKey( KlassOop klass, Oop selector_or_method );
    LookupKey( LookupKey *key );

    KlassOop klass() const;
    Oop selector_or_method() const;
    SymbolOop selector() const;
    MethodOop method() const;


    // Lookup type
    bool is_super_type() const;
    bool is_normal_type() const;
    bool is_block_type() const;


    bool equal( const LookupKey *p ) const;
    void initialize( KlassOop klass, Oop selector_or_method );
    void clear();
    std::int32_t hash() const;
    void switch_pointers( Oop from, Oop to );

    void relocate();
    bool verify() const;
    void oops_do( void f( Oop * ) );

    // Printing support output format is:
    //   "class::selector" for normal sends and
    //   "class^^selector" for super sends.
    //   "class->selector {byteCodeIndex}+" for block keys
    void print() const;

    const char *toString() const;

    void print_on( ConsoleOutputStream *stream ) const;

    void print_inlining_database_on( ConsoleOutputStream *stream ) const;

    // For resource allocation.
    static LookupKey *allocate( KlassOop klass, Oop selector_or_method );
};
