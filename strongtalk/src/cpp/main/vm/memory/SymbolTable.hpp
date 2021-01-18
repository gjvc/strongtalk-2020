//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <array>

#include "vm/system/platform.hpp"
#include "allocation.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"


//
// The symbol table (Memory->symbol_table) holds all canonical symbols.
// It is implemented as an open hash table with a fixed number of buckets.
//
// A bucket (SymbolTableEntry) is a union containing either:
//
//   nullptr            => bucket is empty.
//   SymbolOop          => bucket has one element.
//   SymbolTableLink*   => bucket has more than one element.
//
//
// SPACE HACK:
//  - symbolTableLinks are allocated in blocks to reduce the malloc overhead.
//

constexpr int symbol_table_size = 20011;

int hash( const char *name, int len );

struct SymbolTableLink {
    // instance variable
    SymbolOop symbol;
    SymbolTableLink *next;

    // memory operations
    bool_t verify( int i );
};

struct SymbolTableEntry {
    void *symbol_or_link;


    bool_t is_empty() {
        return symbol_or_link == nullptr;
    }


    bool_t is_symbol() {
        return Oop( symbol_or_link )->is_mem();
    }


    void clear() {
        symbol_or_link = nullptr;
    }


    SymbolOop get_symbol() {
        return SymbolOop( symbol_or_link );
    }


    void set_symbol( SymbolOop s ) {
        symbol_or_link = (void *) s;
    }


    SymbolTableLink *get_link() {
        return (SymbolTableLink *) symbol_or_link;
    }


    void set_link( SymbolTableLink *l ) {
        symbol_or_link = (void *) l;
    }


    // memory operations
    bool_t verify( int i );

    void deallocate();

    int length();
};

class SymbolTable : public CHeapAllocatedObject {

private:
    std::array<SymbolTableEntry, symbol_table_size> buckets;
    SymbolTableLink *free_list;
    SymbolTableLink *first_free_link;
    SymbolTableLink *end_block;

public:
    SymbolTable();

    // operations
    SymbolOop lookup( const char *name, int len );

    // Used in Bootstrap for checking
    bool_t is_present( SymbolOop sym );

protected:
    void add_symbol( SymbolOop s ); // Only used by Bootstrap

    SymbolOop basic_add( const char *name, int len, int hashValue );

    SymbolOop basic_add( SymbolOop s, int hashValue );


    SymbolTableEntry *bucketFor( int hashValue );


    SymbolTableEntry *firstBucket();


    SymbolTableEntry *lastBucket();


public:
    void add( SymbolOop s );

    // memory operations
    void follow_used_symbols(); // Used during phase1 of garbage collection

    void switch_pointers( Oop from, Oop to );

    void relocate();

    void verify();

    // memory management for symbolTableLinks
    SymbolTableLink *new_link( SymbolOop s, SymbolTableLink *n = nullptr );

    void delete_link( SymbolTableLink *l );

    // histogram
    void print_histogram();

    friend class Bootstrap;
};
