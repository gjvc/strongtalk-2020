//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/system/bits.hpp"
#include "vm/compiler/BitVector.hpp"


bool BitVector::unionWith( BitVector *other ) {

    while ( length < other->length ) {
        _bits[ length++ ] = 0;
    }

    st_assert( length <= _maxLength, "grew too much" );

    bool changed = false;

    for ( std::size_t i = indexFromNumber( other->length - 1 ); i >= 0; i-- ) {
        std::size_t old = _bits[ i ];
        _bits[ i ] |= other->_bits[ i ];
        changed |= ( old not_eq _bits[ i ] );
    }

    return changed;
}


bool BitVector::intersectWith( BitVector *other ) {
    bool changed = false;

    for ( std::size_t i = indexFromNumber( min( length, other->length ) - 1 ); i >= 0; i-- ) {
        std::size_t old = _bits[ i ];
        _bits[ i ] &= other->_bits[ i ];
        changed |= ( old not_eq _bits[ i ] );
    }

    return changed;
}


bool BitVector::isDisjointFrom( BitVector *other ) {

    for ( std::size_t i = indexFromNumber( min( length, other->length ) - 1 ); i >= 0; i-- ) {
        if ( ( _bits[ i ] & other->_bits[ i ] ) not_eq 0 ) {
            return false;
        }
    }

    return true;
}


void BitVector::addFromTo( std::size_t first, std::size_t last ) {

    // mark bits [first..last]
    st_assert( first >= 0 and first < length, "wrong index" );
    st_assert( last >= 0 and last < length, "wrong index" );

    std::size_t startIndex = indexFromNumber( first );
    std::size_t endIndex   = indexFromNumber( last );

    if ( startIndex == endIndex ) {
        st_assert( last - first < BITS_PER_WORD, "oops" );
        std::size_t mask = nthMask( last - first + 1 );
        _bits[ startIndex ] |= mask << offsetFromNumber( first );
    } else {
        _bits[ startIndex ] |= AllBitsSet << offsetFromNumber( first );

        for ( std::size_t i = startIndex + 1; i < endIndex; i++ ) {
            _bits[ i ] = AllBitsSet;
        }

        _bits[ endIndex ] |= nthMask( offsetFromNumber( last ) + 1 );
    }

    for ( std::size_t i = first; i <= last; i++ ) {
        st_assert( includes( i ), "bit should be set" );
    }
}


void BitVector::removeFromTo( std::size_t first, std::size_t last ) {

    st_assert( first >= 0 and first < length, "wrong index" );
    st_assert( last >= 0 and last < length, "wrong index" );

    std::size_t startIndex = indexFromNumber( first );
    std::size_t endIndex   = indexFromNumber( last );

    if ( startIndex == endIndex ) {
        st_assert( last - first < BITS_PER_WORD, "oops" );
        std::size_t mask = ~nthMask( last - first + 1 );
        _bits[ startIndex ] &= mask << offsetFromNumber( first );
    } else {
        _bits[ startIndex ] &= ~( AllBitsSet << offsetFromNumber( first ) );

        for ( std::size_t i = startIndex + 1; i < endIndex; i++ ) {
            _bits[ i ] = 0;
        }

        _bits[ endIndex ] &= ~nthMask( offsetFromNumber( last ) + 1 );
    }

    for ( std::size_t i = first; i <= last; i++ ) {
        st_assert( not includes( i ), "bit shouldn't be set" );
    }

}


void BitVector::print_short() {
    SPDLOG_INFO( "BitVector {0:x}", static_cast<void *>(this) );
}


void BitVector::doForAllOnes( intDoFn f ) {

    for ( std::size_t i = indexFromNumber( length - 1 ); i >= 0; i-- ) {

        std::size_t b = _bits[ i ];

        for ( std::size_t j = 0; j < BITS_PER_WORD; j++ ) {
            if ( isBitSet( b, j ) ) {
                f( i * BITS_PER_WORD + j );
                clearNthBit( b, j );
                if ( not b ) {
                    break;
                }
            }
        }
    }

}


void BitVector::print() {

    print_short();
    SPDLOG_INFO( ": {" );
    std::size_t last = -1;
    std::size_t i    = 0;

    for ( ; i < length; i++ ) {
        if ( includes( i ) ) {
            if ( last < 0 ) {
                SPDLOG_INFO( " %ld", i );    // first bit after string of 0s
                last = i;
            }
        } else {
            if ( last >= 0 ) {
                SPDLOG_INFO( "..%ld", i - 1 );    // ended a group
            }
            last = -1;
        }
    }

    if ( last >= 0 ) {
        SPDLOG_INFO( "..%ld", i - 1 );
    }

    SPDLOG_INFO( " }" );
}
