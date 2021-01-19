
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"


class objectIDTable : AllStatic {

private:
    static ObjectArrayOop array();

public:
    static int insert( Oop obj );

    static Oop at( int index );

    static int find_index( Oop obj );

    static bool_t is_index_ok( int index );

    static void allocateSize( int size );

    static void cleanup_after_bootstrap();

};
