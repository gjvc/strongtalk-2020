
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/utilities/ObjectIDTable.hpp"
#include "vm/memory/oopFactory.hpp"

// Memory->ObjectIDTable[1.. numberOfIDs] contains the valid entries

static std::int32_t nextID;


ObjectArrayOop ObjectIDTable::array() {
    return Universe::objectIDTable();
}


Oop ObjectIDTable::at( std::int32_t index ) {
    return array()->obj_at( index );
}


bool ObjectIDTable::is_index_ok( std::int32_t index ) {
    return 1 <= index and index <= Universe::objectIDTable()->length();
}


std::int32_t ObjectIDTable::find_index( Oop obj ) {
    std::int32_t len = Universe::objectIDTable()->length();

    for ( std::int32_t i = 1; i <= len; i++ ) {
        if ( at( i ) == obj ) {
            return i;
        }
    }

    return 0;
}


std::int32_t ObjectIDTable::insert( Oop obj ) {
    std::int32_t id = find_index( obj );
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


void ObjectIDTable::cleanup_after_bootstrap() {
    Universe::set_objectIDTable( oopFactory::new_objArray( 200 ) );
    nextID = 1;
}
