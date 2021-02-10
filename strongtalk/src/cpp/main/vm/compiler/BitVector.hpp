
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/runtime/ResourceObject.hpp"

class LongRegisterMask;
// General bit vectors; Space is allocated for maxLength bits, but only the portion up to length bits is used.
// SimpleBitVector: simple version that fits into a word (for speed)

typedef void (*intDoFn)( std::size_t i );

// a pseudo-class -- std::size_t(this) actually holds the bits, so there's no allocation
class SimpleBitVector : public ValueObject {

    std::size_t _bits;

public:
    SimpleBitVector( std::size_t b = 0 ) :
        _bits{ b } {
    }


    SimpleBitVector allocate( std::size_t l ) {
        st_assert( l < BITS_PER_WORD, "need longer bit vector" );
        return SimpleBitVector( addNthBit( _bits, l ) );
    }


    SimpleBitVector deallocate( std::size_t l ) {
        st_assert( l < BITS_PER_WORD, "need longer bit vector" );
        return SimpleBitVector( subNthBit( _bits, l ) );
    }


    bool isAllocated( std::size_t l ) {
        st_assert( l < BITS_PER_WORD, "need longer bit vector" );
        return isBitSet( _bits, l );
    }


    bool isEmpty() {
        return _bits == 0;
    }

};


class BitVector : public PrintableResourceObject {

protected:
    std::size_t _maxLength; // max # bits
    std::size_t length;     // number of bits, not words
    std::size_t *_bits;     // array containing the bits

    std::size_t indexFromNumber( std::size_t i ) {
        return i >> LOG_2_BITS_PER_WORD;
    }


    std::size_t offsetFromNumber( std::size_t i ) {
        return lowerBits( i, LOG_2_BITS_PER_WORD );
    }


    bool getBitInWord( std::size_t i, std::size_t o ) {
        return isBitSet( _bits[ i ], o );
    }


    void setBitInWord( std::size_t i, std::size_t o ) {
        setNthBit( _bits[ i ], o );
    }


    void clearBitInWord( std::size_t i, std::size_t o ) {
        clearNthBit( _bits[ i ], o );
    }


    std::size_t bitsLength( std::size_t l ) {
        return indexFromNumber( l - 1 ) + 1;
    }


    std::size_t *createBitString( std::size_t l ) {
        std::size_t blen = bitsLength( l );
        std::size_t *bs  = new_resource_array<std::size_t>( blen );
        set_words( bs, blen, 0 );
        return bs;
    }


    std::size_t *copyBitString( std::size_t len ) {

        st_assert( len >= _maxLength, "can't shorten" );

        std::size_t blen    = bitsLength( len );
        std::size_t *bs     = new_resource_array<std::size_t>( blen );
        std::size_t blength = bitsLength( _maxLength );
        copy_words( _bits, bs, blength );
        if ( blength < blen ) {
            set_words( bs + blength, blen - blength, 0 );
        }

        return bs;
    }


public:
    BitVector( std::size_t l ) :
        _maxLength{ l },
        length{ l },
        _bits{ createBitString( l ) } {
        st_assert( l > 0, "should have some length" );
    }


    BitVector() = default;
    virtual ~BitVector() = default;
    BitVector( const BitVector & ) = default;
    BitVector &operator=( const BitVector & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



protected:
    BitVector( std::size_t l, std::size_t ml, std::size_t *bs ) :
        _maxLength{ ml },
        length{ l },
        _bits{ bs } {
    }


public:
    BitVector *copy( std::size_t len ) {
        return new BitVector( length, len, copyBitString( len ) );
    }


    bool includes( std::size_t i ) {
        //st_assert( this, "shouldn't be a null pointer" );
        st_assert( i < length, "not in range" );
        bool b = getBitInWord( indexFromNumber( i ), offsetFromNumber( i ) );
        return b;
    }


    void add( std::size_t i ) {
        //st_assert( this, "shouldn't be a null pointer" );
        st_assert( i < length, "not in range" );
        setBitInWord( indexFromNumber( i ), offsetFromNumber( i ) );
    }


    void addFromTo( std::size_t first, std::size_t last );    // set bits [first..last]

    void remove( std::size_t i ) {
        //st_assert( this, "shouldn't be a null pointer" );
        st_assert( i < length, "not in range" );
        clearBitInWord( indexFromNumber( i ), offsetFromNumber( i ) );
    }


    void removeFromTo( std::size_t first, std::size_t last );    // clear bits [first..last]

    // union/intersect return true if receiver has changed
    bool unionWith( BitVector *other );          // this |= other
    bool intersectWith( BitVector *other );      // this &= other
    bool isDisjointFrom( BitVector *other );     // (this & other) == {}

    void doForAllOnes( intDoFn f );  // call f for all 1 bits

    void setLength( std::size_t l ) {
        st_assert( l < _maxLength, "too big" );
        length = l;
    }


    void clear() {
        set_words( _bits, bitsLength( length ), 0 );
    }


    void print_short();

    void print();

    friend class LongRegisterMask;

    friend std::size_t findFirstUnused( LongRegisterMask **strings, std::size_t len, std::size_t start );
};
