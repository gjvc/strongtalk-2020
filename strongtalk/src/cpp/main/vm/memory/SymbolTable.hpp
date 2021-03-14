
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include <array>

#include "vm/platform/platform.hpp"
#include "allocation.hpp"
#include "vm/oop/SymbolOopDescriptor.hpp"


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

constexpr std::int32_t symbol_table_size = 20011;

std::uint32_t hash( const char *name, std::int32_t len );


class SymbolTableLink {

public:

    SymbolOop       _symbol;    //
    SymbolTableLink *_next;     //

    bool verify( std::int32_t i );

};


class SymbolTableEntry {
public:

    void *symbol_or_link;


    bool is_empty() {
        return symbol_or_link == nullptr;
    }


    bool is_symbol() {
        return Oop( symbol_or_link )->isMemOop();
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
    bool verify( std::int32_t i );

    void deallocate();

    std::size_t length();
};


class SymbolTable : public CHeapAllocatedObject {

private:
    std::array<SymbolTableEntry, symbol_table_size> buckets;
    SymbolTableLink                                 *free_list;
    SymbolTableLink                                 *first_free_link;
    SymbolTableLink                                 *end_block;

public:
    SymbolTable();
    virtual ~SymbolTable() = default;
    SymbolTable( const SymbolTable & ) = default;
    SymbolTable &operator=( const SymbolTable & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    // operations
    SymbolOop lookup( const char *name, std::int32_t len );

    // Used in Bootstrap for checking
    bool is_present( SymbolOop sym );

protected:
    void add_symbol( SymbolOop s ); // Only used by Bootstrap

    SymbolOop basic_add( const char *name, std::int32_t len, std::int32_t hashValue );

    SymbolOop basic_add( SymbolOop s, std::int32_t hashValue );


    SymbolTableEntry *bucketFor( std::int32_t hashValue );


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
