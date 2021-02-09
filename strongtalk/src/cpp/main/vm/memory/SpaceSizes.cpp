
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/flags.hpp"
#include "vm/memory/SpaceSizes.hpp"
#include "vm/memory/Universe.hpp"


static std::int32_t scale_and_adjust( std::int32_t value ) {
    std::int32_t result = roundTo( value * 1024, Universe::page_size() );
    return result;
}


void SpaceSizes::initialize() {
    _reserved_object_size   = scale_and_adjust( ReservedHeapSize );
    _eden_size              = scale_and_adjust( EdenSize );
    _surv_size              = scale_and_adjust( SurvivorSize );
    _old_size               = scale_and_adjust( OldSize );
    _reserved_codes_size    = scale_and_adjust( ReservedCodeSize ); // not used?
    _code_size              = scale_and_adjust( CodeSize );
    _reserved_pic_heap_size = scale_and_adjust( ReservedPICSize );  // not used?
    _pic_heap_size          = scale_and_adjust( PICSize );
    _jump_table_size        = JumpTableSize;
}


static std::int32_t GetNumericEnvironmentVariable( const char *name, std::int32_t factor, std::int32_t def ) {
    char *n = getenv( name );
    if ( n ) {
        std::int32_t l = def;
        if ( sscanf( n, "%d", &l ) == 1 ) {
            def = l * factor;
        } else {
            SPDLOG_WARN( "environment variable[{}] isn't a number", name );
        }
    }
    return def;
}


static std::int32_t getSize( const char *name, std::int32_t def ) {
    const std::int32_t blockSize = 4 * 1024;
    std::int32_t       size      = GetNumericEnvironmentVariable( name, 1024, def );
    return roundTo( size, blockSize );
}


static std::int32_t getSize( std::int32_t def ) {
    const std::int32_t blockSize = 4 * 1024;
    return roundTo( def, blockSize );
}
