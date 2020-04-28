//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "allocation.hpp"
// SnapshotDesc is the class handling reading and writing of snapshots

class SnapshotDescriptor : StackAllocatedObject {
    private:
        FILE * _file;
        bool_t _has_error;

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
        void read_from( const char * name );

        void write_on( const char * name );


        bool_t has_error() {
            return _has_error;
        }


        SymbolOop error_symbol();

        void error( const char * msg );

        friend class WriteClosure;

        friend class ReadClosure;
};

