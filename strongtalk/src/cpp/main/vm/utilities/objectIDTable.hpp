
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"


class objectIDTable : AllStatic {

private:
    static ObjectArrayOop array();

public:
    static std::size_t insert( Oop obj );

    static Oop at( int index );

    static std::size_t find_index( Oop obj );

    static bool_t is_index_ok( int index );

    static void allocateSize( std::size_t size );

    static void cleanup_after_bootstrap();

};
