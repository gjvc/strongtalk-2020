//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/runtime/ReservedSpace.hpp"


// VirtualSpace is a data structure for reserving a contiguous chunk of memory and then committing to using the reserved chunk bit by bit.
// Perfect for implementing growable stack without relocation.

class VirtualSpace : public ValueObject {

private:
    // Reserved area
    const char *_low_boundary;
    const char *_high_boundary;

    // Committed area
    const char *_low;
    const char *_high;

    // Grow direction
    bool_t _low_to_high;

    VirtualSpace *next;

    friend class VirtualSpaces;

public:
    const char *low() const {
        return _low;
    }


    const char *high() const {
        return _high;
    }


    const char *low_boundary() const {
        return _low_boundary;
    }


    const char *high_boundary() const {
        return _high_boundary;
    }


public:
    VirtualSpace( int reserved_size, int committed_size, bool_t low_to_high = true );

    VirtualSpace( ReservedSpace reserved, int committed_size, bool_t low_to_high = true );

    VirtualSpace();

    void initialize( ReservedSpace reserved, int committed_size, bool_t low_to_high = true );

    ~VirtualSpace();

    // testers
    int committed_size() const;

    int reserved_size() const;

    int uncommitted_size() const;

    bool_t contains( void *p ) const;

    bool_t low_to_high() const;

    // operations
    void expand( std::size_t size );

    void shrink( std::size_t size );

    void release();

    // debugging
    void print();


    // page faults
    virtual void page_fault() {
    }
};

class VirtualSpaces : AllStatic {
private:
    static VirtualSpace *head;

    static void add( VirtualSpace *sp );

    static void remove( VirtualSpace *sp );

    friend class VirtualSpace;

public:
    static int committed_size();

    static int reserved_size();

    static int uncommitted_size();

    static void print();

    static void test();
};
