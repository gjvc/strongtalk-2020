//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/lookup/LookupKey.hpp"

class NativeMethod;


// The code table is used to find nativeMethods in the zone.
// It is a hash table, where each bucket contains a list of nativeMethods.

//%note
// Should implement free list like SymbolTable (Lars 2/10-95)

constexpr int codeTableSize  = 2048;
constexpr int debugTableSize = 256;

//struct CodeTableEntry;

struct CodeTableLink : public CHeapAllocatedObject {

    // instance variable
    NativeMethod  * _nativeMethod;
    CodeTableLink * _next;

    // memory operations
    bool_t verify( int i );
};

struct CodeTableEntry : ValueObject {

    // methods are tagged, links are not.
    void * _nativeMethodOrLink;


    bool_t is_empty() {
        return _nativeMethodOrLink == nullptr;
    }


    bool_t is_nativeMethod() {
        return ( int ) _nativeMethodOrLink & 1;
    }


    void clear() {
        _nativeMethodOrLink = nullptr;
    }


    NativeMethod * get_nativeMethod() {
        return ( NativeMethod * ) ( ( int ) _nativeMethodOrLink - 1 );
    }


    void set_nativeMethod( const NativeMethod * nm ) {
        assert_oop_aligned( nm );
        _nativeMethodOrLink = ( void * ) ( ( int ) nm + 1 );
    }


    CodeTableLink * get_link() {
        return ( CodeTableLink * ) _nativeMethodOrLink;
    }


    void set_link( CodeTableLink * l ) {
        assert_oop_aligned( l );
        _nativeMethodOrLink = ( void * ) l;
    }


    // memory operations
    void deallocate();

    int length();   // returns the number of NativeMethod in this bucket.

    bool_t verify( int i );
};

class CodeTable : public PrintableCHeapAllocatedObject {
    protected:
        int tableSize;
        CodeTableEntry * buckets;


        CodeTableEntry * at( int index ) {
            return &buckets[ index ];
        }


        CodeTableEntry * bucketFor( int hash ) {
            return at( hash & ( tableSize - 1 ) );
        }


        CodeTableLink * new_link( NativeMethod * nm, CodeTableLink * n = nullptr );

    public:
        CodeTable( int size );

        void clear();

        NativeMethod * lookup( const LookupKey * L );

        bool_t verify();

        void print();

        void print_stats();

        // Tells whether a NativeMethod is present
        bool_t is_present( NativeMethod * nm );

        // Removes a NativeMethod from the table
        void remove( NativeMethod * nm );

    protected:
        // should always add through zone->addToCodeTable()
        void add( NativeMethod * nm );

        void addIfAbsent( NativeMethod * nm ); // add only if not there yet

        friend class Zone;
};

