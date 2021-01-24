//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/runtime/ResourceObject.hpp"


class Array : public ResourceObject {

private:
    std::size_t  _index;     //
    std::size_t  _size;      //
    std::size_t  _offset;    //
    std::int32_t *_values;   //

public:
    Array( std::size_t size );


    int length() {
        return _index;
    }


    void extend( int newSize );

    int insertIfAbsent( int value );  // returns index for value
    void copy_to( std::size_t *&addr );
};


class ByteArray : public ResourceObject {

private:
    std::uint8_t *_array;
    int          _top;
    int          _max;

    void extend();

public:
    std::size_t size() {
        return _top;
    }


    std::uint8_t *start() {
        return _array;
    }


    ByteArray( std::size_t size );


    void appendByte( std::uint8_t p ) {
        if ( _top + (int) sizeof( std::uint8_t ) > _max )
            extend();
        _array[ _top++ ] = p;
    }


    void appendHalf( std::int16_t p );

    void appendWord( int p );


    void putByteAt( std::uint8_t p, int offset ) {
        st_assert( offset < _max, "index out of bound" );
        _array[ offset ] = p;
    }


    void putHalfAt( std::int16_t p, int offset );


    // Cut off some of the generated code.
    void setTop( int offset ) {
        st_assert( _top >= offset, "A smaller top is expected" );
        _top = offset;
    }


    void alignToWord();

    void copy_to( std::size_t *&addr );
};
