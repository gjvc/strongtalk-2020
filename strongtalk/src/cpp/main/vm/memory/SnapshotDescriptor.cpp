
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/memory/SnapshotDescriptor.hpp"
#include "vm/memory/Universe.hpp"


class ReadClosure : public OopClosure {

private:
    void do_oop( Oop *o ) {
        static_cast<void>(o); // unused
    }


    SnapshotDescriptor *_snapshotDescriptor;

public:
    ReadClosure( SnapshotDescriptor *s ) {
        _snapshotDescriptor = s;
    }
};


class WriteClosure : public OopClosure {

private:
    void do_oop( Oop *o ) {
        fprintf( _snapshotDescriptor->_file, "%#X\n", reinterpret_cast<unsigned int>(o) );
    }


    SnapshotDescriptor *_snapshotDescriptor;

public:
    WriteClosure( SnapshotDescriptor *s ) {
        _snapshotDescriptor = s;
    }
};


void SnapshotDescriptor::read_header() {
    read_revision();
    read_sizes();
}


void SnapshotDescriptor::write_header() {
    write_revision();
    write_sizes();
}


void SnapshotDescriptor::read_sizes() {

}


void SnapshotDescriptor::write_sizes() {

}


static const char *revision_format = "Delta snapshot revision: %d.%d\n";


void SnapshotDescriptor::read_revision() {
    fprintf( _file, revision_format, Universe::major_version(), Universe::snapshot_version() );
}


void SnapshotDescriptor::write_revision() {
    std::int32_t major, snap;
    if ( fscanf( _file, revision_format, &major, &snap ) not_eq 2 )
        error( "reading revision" );

    if ( Universe::major_version() not_eq major )
        error( "major revision number conflict" );

    if ( Universe::snapshot_version() not_eq snap )
        error( "snapshot revision number conflict" );
}


void SnapshotDescriptor::read_roots() {
    ReadClosure blk( this );
    Universe::root_iterate( &blk );
}


void SnapshotDescriptor::write_roots() {
    WriteClosure blk( this );
    Universe::root_iterate( &blk );
}


void SnapshotDescriptor::read_spaces() {

}


void SnapshotDescriptor::write_spaces() {

}


void SnapshotDescriptor::read_zone() {

}


void SnapshotDescriptor::write_zone() {

}


void SnapshotDescriptor::read_from( const char *name ) {
    static_cast<void>(name); // unused

    read_header();
    read_spaces();
    read_roots();
    read_zone();
}


void SnapshotDescriptor::write_on( const char *name ) {
    static_cast<void>(name); // unused

    write_header();
    write_spaces();
    write_roots();
    write_zone();
}


void SnapshotDescriptor::error( const char *msg ) {
    st_fatal( msg );
}


SymbolOop SnapshotDescriptor::error_symbol() {
    return nullptr;
}
