
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/utilities/objectIDTable.hpp"
#include "vm/memory/oopFactory.hpp"

// Memory->objectIDTable[1.. numberOfIDs] contains the valid entries

static int nextID;


ObjectArrayOop objectIDTable::array() {
    return Universe::objectIDTable();
}


Oop objectIDTable::at( int index ) {
    return array()->obj_at( index );
}


bool_t objectIDTable::is_index_ok( int index ) {
    return 1 <= index and index <= Universe::objectIDTable()->length();
}


int objectIDTable::find_index( Oop obj ) {
    int len = Universe::objectIDTable()->length();

    for ( std::size_t i = 1; i <= len; i++ )
        if ( at( i ) == obj )
            return i;

    return 0;
}


int objectIDTable::insert( Oop obj ) {
    int id = find_index( obj );
    if ( not id ) {
        if ( nextID < array()->length() ) {
            id = nextID++;
        } else {
            nextID = 1;
            id     = nextID++;
        }
        array()->obj_at_put( id, obj );
    }
    return id;
}


void objectIDTable::cleanup_after_bootstrap() {
    Universe::set_objectIDTable( oopFactory::new_objArray( 200 ) );
    nextID = 1;
}
