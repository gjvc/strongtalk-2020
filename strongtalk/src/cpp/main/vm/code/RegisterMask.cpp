//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/memory/util.hpp"

#if 0

Location pick( RegisterMask & alloc, RegisterMask mask ) {
    Unimplemented();
    unsigned r = mask & ~alloc;
    if ( r == 0 ) return unAllocated;
    for ( int reg = 0; not isBitSet( r, 0 ); reg++, r >>= 1 );
    setNthBit( alloc, reg );
    // return Location(ireg, reg); /// fix this
    return Location();
}

void printAllocated( RegisterMask rs ) {
    Unimplemented();
    printf( "{" );
    bool_t    first = true;
    unsigned  r     = rs;        // safer for >>
    for ( int d     = 0; r; d++, r >>= 1 ) {
        if ( isBitSet( r, 0 ) ) {
            if ( first ) {
                first = false;
            } else {
                printf( "," );
            }
            printf( "%s", RegisterNames[ d ] );
            Unimplemented();
            // Location d1 = Location(d); <<< fix this
            Location   d1;
            for ( char c = RegisterNames[ d ][ 0 ];
                  isBitSet( r, 1 ) and c == RegisterNames[ d + 1 ][ 0 ];
                  d++, r >>= 1 );
            if ( d > d1.no() ) printf( "-%s", RegisterNames[ d ] );
        }
    }
    printf( "}" );
    fflush( stdout );

}


int tempToIndex( Location temp ) {
    Unimplemented();
    return 0;
    // return temp - FirstStackLocation + 32;
}

Location indexToTemp( int temp ) {
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
        std::size_t i = tempToIndex( l );
        if ( i >= bv->length ) grow();
        bv->add( i );
    }
}

void LongRegisterMask::deallocate( Location l ) {
    if ( l.isRegister() ) {
        bv->remove( l.number() );
    } else {
        assert( l.isStackLocation(), "should be stack reg" );
        std::size_t i = tempToIndex( l );
        bv->remove( i );
    }
}

bool_t LongRegisterMask::isAllocated( Location l ) {
    if ( l.isRegister() ) {
        return bv->includes( l.number() );
    } else {
        assert( l.isStackLocation(), "should be stack reg" );
        std::size_t i = tempToIndex( l );
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
int findFirstUnused( LongRegisterMask ** masks, int len, int start ) {
    // currently quite unoptimized
    BitVector * b = masks[ 0 ]->bv->copy( masks[ 0 ]->bv->maxLength );
    for ( std::size_t i = 1; i < len; i++ ) {
        b->unionWith( masks[ i ]->bv );
    }
    int       i = start;
    for ( ; i < b->length; i++ ) {
        if ( not b->includes( i ) ) break;
    }
    return i;
}

Location findFirstUnusedTemp( LongRegisterMask ** masks, int len ) {
    Unimplemented();
    // std::size_t i = findFirstUnused(masks, len, tempToIndex(FirstStackLocation));
    // return indexToTemp(i);
    return Location();
}

#endif
