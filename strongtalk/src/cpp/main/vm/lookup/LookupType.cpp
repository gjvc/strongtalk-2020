//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/lookup/LookupType.hpp"
#include "vm/utilities/lprintf.hpp"
#include "vm/runtime/ResourceArea.hpp"

#include <cstring>

// This code is unused


void printLookupType( LookupType l ) {
    lprintf( lookupTypeName( l ) );
}


static void addFlag( bool_t &flag, char *name, const char *add ) {
    if ( not flag )
        strcat( name, " { " );
    flag = true;
    strcat( name, add );
}


char *lookupTypeName( LookupType l ) {

    char *name = new_resource_array<char>( 80 );
    switch ( withoutExtraBits( l ) ) {
        case LookupType::NormalLookupType:
            strcpy( name, "NormalLookup" );
            break;
        case LookupType::SelfLookupType:
            strcpy( name, "SelfLookup" );
            break;
        case LookupType::SuperLookupType:
            strcpy( name, "SuperLookup" );
            break;
        default: st_fatal( "Unknown lookupType" );
    }

    bool_t hasFlag = false;
    switch ( countType( l ) ) {
        case CountType::NonCounting:
            break;
        case CountType::Counting:
            addFlag( hasFlag, name, "counting " );
            break;
        case CountType::Comparing:
            addFlag( hasFlag, name, "comparing " );
            break;
        default: st_fatal1( "invalid count type %ld", countType( l ) );
    }
    if ( isBitSet( static_cast<int>(l), DirtySendBit ) )
        addFlag( hasFlag, name, "dirty " );
    if ( isBitSet( static_cast<int>(l), OptimizedSendBit ) )
        addFlag( hasFlag, name, "optimized " );
    if ( isBitSet( static_cast<int>(l), UninlinableSendBit ) )
        addFlag( hasFlag, name, "uninlinable " );
    if ( hasFlag )
        strcat( name, "}" );

    return name;
}


void kinds_init() {
    st_assert( ( LookupTypeMask >> LookupTypeSize ) == 0, "wrong LookupTypeSize" );
    st_assert( ( CountTypeMask >> CountTypeSize ) == 0, "wrong CountTypeSize" );
}
