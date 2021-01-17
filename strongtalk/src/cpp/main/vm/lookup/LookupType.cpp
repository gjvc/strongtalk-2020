//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/system/platform.hpp"
#include "vm/system/bits.hpp"
#include "vm/system/asserts.hpp"
#include "vm/lookup/LookupType.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/runtime/ResourceArea.hpp"

#include <cstring>

// This code is unused


void printLookupType( LookupType l ) {
    lprintf( lookupTypeName( l ) );
}


static void addFlag( bool_t & flag, char * name, const char * add ) {
    if ( not flag )
        strcat( name, " { " );
    flag = true;
    strcat( name, add );
}


char * lookupTypeName( LookupType l ) {

    char * name = new_resource_array <char>( 80 );
    switch ( withoutExtraBits( l ) ) {
        case NormalLookupType:
            strcpy( name, "NormalLookup" );
            break;
        case SelfLookupType:
            strcpy( name, "SelfLookup" );
            break;
        case SuperLookupType:
            strcpy( name, "SuperLookup" );
            break;
        default: st_fatal( "Unknown lookupType" );
    }

    bool_t hasFlag = false;
    switch ( countType( l ) ) {
        case NonCounting:
            break;
        case Counting:
            addFlag( hasFlag, name, "counting " );
            break;
        case Comparing:
            addFlag( hasFlag, name, "comparing " );
            break;
        default: st_fatal1( "invalid count type %ld", countType( l ) );
    }
    if ( isBitSet( l, DirtySendBit ) )
        addFlag( hasFlag, name, "dirty " );
    if ( isBitSet( l, OptimizedSendBit ) )
        addFlag( hasFlag, name, "optimized " );
    if ( isBitSet( l, UninlinableSendBit ) )
        addFlag( hasFlag, name, "uninlinable " );
    if ( hasFlag )
        strcat( name, "}" );

    return name;
}


void kinds_init() {
    st_assert( ( LookupTypeMask >> LookupTypeSize ) == 0, "wrong LookupTypeSize" );
    st_assert( ( CountTypeMask >> CountTypeSize ) == 0, "wrong CountTypeSize" );
}
