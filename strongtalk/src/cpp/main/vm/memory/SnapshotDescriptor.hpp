//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "allocation.hpp"


// SnapshotDescriptor is the class handling reading and writing of snapshots

class SnapshotDescriptor : StackAllocatedObject {
private:
    FILE *_file;
    bool _has_error;

    // HEADER
    void read_header();

    void write_header();

    void read_sizes();

    void write_sizes();

    void read_revision();

    void write_revision();

    // ROOTS
    void read_roots();

    void write_roots();

    // OBJECTS
    void read_spaces();

    void write_spaces();

    // ZONE
    void read_zone();

    void write_zone();

public:

    SnapshotDescriptor() : _file{ nullptr }, _has_error{ false } {

    }

    virtual ~SnapshotDescriptor() = default;
    SnapshotDescriptor( const SnapshotDescriptor & ) = default;
    SnapshotDescriptor &operator=( const SnapshotDescriptor & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void read_from( const char *name );

    void write_on( const char *name );


    bool has_error() {
        return _has_error;
    }


    SymbolOop error_symbol();

    void error( const char *msg );

    friend class WriteClosure;

    friend class ReadClosure;
};
