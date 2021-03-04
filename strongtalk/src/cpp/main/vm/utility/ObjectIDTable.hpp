
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"


class ObjectIDTable : AllStatic {

private:
    static ObjectArrayOop array();

public:
    static std::int32_t insert( Oop obj );

    static Oop at( std::int32_t index );

    static std::int32_t find_index( Oop obj );

    static bool is_index_ok( std::int32_t index );

    static void allocateSize( std::int32_t size );

    static void cleanup_after_bootstrap();

};
