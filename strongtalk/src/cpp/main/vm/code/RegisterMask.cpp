//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/memory/util.hpp"

#if 0

Location pick( RegisterMask & alloc, RegisterMask mask ) {
    Unimplemented();
    unsigned r = mask & ~alloc;
    if ( r == 0 ) return Location::UNALLOCATED_LOCATION;
    for ( std::int32_t reg = 0; not isBitSet( r, 0 ); reg++, r >>= 1 );
    setNthBit( alloc, reg );
    // return Location(ireg, reg); /// fix this
    return Location();
}

void printAllocated( RegisterMask rs ) {
    Unimplemented();
    SPDLOG_INFO( "{" );
    bool    first = true;
    unsigned  r     = rs;        // safer for >>
    for ( std::int32_t d     = 0; r; d++, r >>= 1 ) {
        if ( isBitSet( r, 0 ) ) {
            if ( first ) {
                first = false;
            } else {
                SPDLOG_INFO( "," );
            }
            SPDLOG_INFO( "{}", RegisterNames[ d ] );
            Unimplemented();
            // Location d1 = Location(d); <<< fix this
            Location   d1;
            for ( char c = RegisterNames[ d ][ 0 ];
                  isBitSet( r, 1 ) and c == RegisterNames[ d + 1 ][ 0 ];
                  d++, r >>= 1 );
            if ( d > d1.no() ) SPDLOG_INFO( "-{}", RegisterNames[ d ] );
        }
    }
    SPDLOG_INFO( "}" );
    fflush( stdout );

}


std::int32_t tempToIndex( Location temp ) {
    Unimplemented();
    return 0;
    // return temp - FirstStackLocation + 32;
}

Location indexToTemp( std::int32_t temp ) {
    Unimplemented();
    return Location();
    // return Location(temp + FirstStackLocation - 32);
}

LongRegisterMask::LongRegisterMask() {
    bv = new BitVector( 128 );
}

void LongRegisterMask::allocate( Location l ) {
    if ( l.isRegister() ) {
        bv->add( l.number() );
    } else {
        assert( l.isStackLocation(), "should be stack reg" );
        std::int32_t i = tempToIndex( l );
        if ( i >= bv->length ) grow();
        bv->add( i );
    }
}

void LongRegisterMask::deallocate( Location l ) {
    if ( l.isRegister() ) {
        bv->remove( l.number() );
    } else {
        assert( l.isStackLocation(), "should be stack reg" );
        std::int32_t i = tempToIndex( l );
        bv->remove( i );
    }
}

bool LongRegisterMask::isAllocated( Location l ) {
    if ( l.isRegister() ) {
        return bv->includes( l.number() );
    } else {
        assert( l.isStackLocation(), "should be stack reg" );
        std::int32_t i = tempToIndex( l );
        if ( l.number() < bv->length ) {
            return bv->includes( i );
        } else {
            return false;
        }
    }
}

RegisterMask LongRegisterMask::regs() {
    return bv->bits[ 0 ];
}

void LongRegisterMask::grow() {
    bv = bv->copy( bv->length * 2 );
}

void LongRegisterMask::print() {
}


// find the first bit >= start that is unused in all strings[0..len-1]
std::int32_t findFirstUnused( LongRegisterMask ** masks, std::int32_t len, std::int32_t start ) {
    // currently quite unoptimized
    BitVector * b = masks[ 0 ]->bv->copy( masks[ 0 ]->bv->maxLength );
    for ( std::size_t i = 1; i < len; i++ ) {
        b->unionWith( masks[ i ]->bv );
    }
    std::int32_t       i = start;
    for ( ; i < b->length; i++ ) {
        if ( not b->includes( i ) ) break;
    }
    return i;
}

Location findFirstUnusedTemp( LongRegisterMask ** masks, std::int32_t len ) {
    Unimplemented();
    // std::int32_t i = findFirstUnused(masks, len, tempToIndex(FirstStackLocation));
    // return indexToTemp(i);
    return Location();
}

#endif
