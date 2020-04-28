//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/bits.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/SpaceSizes.hpp"
#include "vm/memory/Universe.hpp"


static int scale_and_adjust( int value ) {
    int result = roundTo( value * 1024, Universe::page_size() );
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


static int GetNumericEnvironmentVariable( const char *name, int factor, int def ) {
    char *n = getenv( name );
    if ( n ) {
        int l = def;
        if ( sscanf( n, "%ld", & l ) == 1 ) {
            def = l * factor;
        } else {
            warning( "environment variable [%s] isn't a number", name );
        }
    }
    return def;
}


static int getSize( const char *name, int def ) {
    const int blockSize = 4 * 1024;
    int size = GetNumericEnvironmentVariable( name, 1024, def );
    return roundTo( size, blockSize );
}


static int getSize( int def ) {
    const int blockSize = 4 * 1024;
    return roundTo( def, blockSize );
}

