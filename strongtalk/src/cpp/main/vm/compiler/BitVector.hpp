//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/runtime/ResourceObject.hpp"

class LongRegisterMask;
// General bit vectors; Space is allocated for maxLength bits, but only the portion up to length bits is used.
// SimpleBitVector: simple version that fits into a word (for speed)

typedef void (*intDoFn)( int i );

// a pseudo-class -- int(this) actually holds the bits, so there's no allocation
class SimpleBitVector : public ValueObject {
    int bits;
public:
    SimpleBitVector( int b = 0 ) {
        bits = b;
    }


    SimpleBitVector allocate( int l ) {
        st_assert( l >= 0 and l < BitsPerWord, "need longer bit vector" );
        return SimpleBitVector( addNthBit( bits, l ) );
    }


    SimpleBitVector deallocate( int l ) {
        st_assert( l >= 0 and l < BitsPerWord, "need longer bit vector" );
        return SimpleBitVector( subNthBit( bits, l ) );
    }


    bool_t isAllocated( int l ) {
        st_assert( l >= 0 and l < BitsPerWord, "need longer bit vector" );
        return isBitSet( bits, l );
    }


    bool_t isEmpty() {
        return bits == 0;
    }
};


class BitVector : public PrintableResourceObject {
protected:
    int _maxLength;  // max # bits
    int length;     // number of bits, not words
    int *_bits;     // array containing the bits

    int indexFromNumber( int i ) {
        return i >> LogBitsPerWord;
    }


    int offsetFromNumber( int i ) {
        return lowerBits( i, LogBitsPerWord );
    }


    bool_t getBitInWord( int i, int o ) {
        return isBitSet( _bits[ i ], o );
    }


    void setBitInWord( int i, int o ) {
        setNthBit( _bits[ i ], o );
    }


    void clearBitInWord( int i, int o ) {
        clearNthBit( _bits[ i ], o );
    }


    int bitsLength( int l ) {
        return indexFromNumber( l - 1 ) + 1;
    }


    int *createBitString( int l ) {
        int blen = bitsLength( l );
        int *bs = new_resource_array<int>( blen );
        set_words( bs, blen, 0 );
        return bs;
    }


    int *copyBitString( int len ) {
        st_assert( len >= _maxLength, "can't shorten" );
        int blen = bitsLength( len );
        int *bs = new_resource_array<int>( blen );
        int blength = bitsLength( _maxLength );
        copy_words( _bits, bs, blength );
        if ( blength < blen )
            set_words( bs + blength, blen - blength, 0 );
        return bs;
    }


public:
    BitVector( int l ) {
        st_assert( l > 0, "should have some length" );
        length = _maxLength = l;
        _bits  = createBitString( l );
    }


protected:
    BitVector( int l, int ml, int *bs ) {
        _maxLength = ml;
        length     = l;
        _bits      = bs;
    }


public:
    BitVector *copy( int len ) {
        return new BitVector( length, len, copyBitString( len ) );
    }


    bool_t includes( int i ) {
        st_assert( this, "shouldn't be a null pointer" );
        st_assert( i >= 0 and i < length, "not in range" );
        bool_t b = getBitInWord( indexFromNumber( i ), offsetFromNumber( i ) );
        return b;
    }


    void add( int i ) {
        st_assert( this, "shouldn't be a null pointer" );
        st_assert( i >= 0 and i < length, "not in range" );
        setBitInWord( indexFromNumber( i ), offsetFromNumber( i ) );
    }


    void addFromTo( int first, int last );    // set bits [first..last]

    void remove( int i ) {
        st_assert( this, "shouldn't be a null pointer" );
        st_assert( i >= 0 and i < length, "not in range" );
        clearBitInWord( indexFromNumber( i ), offsetFromNumber( i ) );
    }


    void removeFromTo( int first, int last );    // clear bits [first..last]

    // union/intersect return true if receiver has changed
    bool_t unionWith( BitVector *other );          // this |= other
    bool_t intersectWith( BitVector *other );      // this &= other
    bool_t isDisjointFrom( BitVector *other );     // (this & other) == {}

    void doForAllOnes( intDoFn f );  // call f for all 1 bits

    void setLength( int l ) {
        st_assert( l < _maxLength, "too big" );
        length = l;
    }


    void clear() {
        set_words( _bits, bitsLength( length ), 0 );
    }


    void print_short();

    void print();

    friend class LongRegisterMask;

    friend int findFirstUnused( LongRegisterMask **strings, int len, int start );
};
