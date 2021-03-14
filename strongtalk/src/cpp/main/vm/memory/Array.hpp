//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/runtime/ResourceObject.hpp"


class Array : public ResourceObject {

private:
    std::size_t  _index;     //
    std::int32_t _size;      //
    std::int32_t _offset;    //
    std::int32_t *_values;   //

public:
    Array( std::int32_t size );
    Array() = default;
    virtual ~Array() = default;
    Array( const Array & ) = default;
    Array &operator=( const Array & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    std::size_t length() {
        return _index;
    }


    void extend( std::int32_t newSize );

    std::int32_t insertIfAbsent( std::int32_t value );  // returns index for value
    void copy_to( std::size_t *&addr );
};


class ByteArray : public ResourceObject {

private:
    std::uint8_t *_array;
    std::int32_t _top;
    std::int32_t _max;

    void extend();

public:
    std::int32_t size() {
        return _top;
    }


    std::uint8_t *start() {
        return _array;
    }


    ByteArray( std::int32_t size );
    ByteArray() = default;
    virtual ~ByteArray() = default;
    ByteArray( const ByteArray & ) = default;
    ByteArray &operator=( const ByteArray & ) = default;


    void operator delete( void *ptr ) { (void) ptr; }


    void appendByte( std::uint8_t p ) {
        if ( _top + (std::int32_t) sizeof( std::uint8_t ) > _max ) {
            extend();
        }
        _array[ _top++ ] = p;
    }


    void appendHalf( std::int16_t p );

    void appendWord( std::int32_t p );


    void putByteAt( std::uint8_t p, std::int32_t offset ) {
        st_assert( offset < _max, "index out of bound" );
        _array[ offset ] = p;
    }


    void putHalfAt( std::int16_t p, std::int32_t offset );


    // Cut off some of the generated code.
    void setTop( std::int32_t offset ) {
        st_assert( _top >= offset, "A smaller top is expected" );
        _top = offset;
    }


    void alignToWord();

    void copy_to( std::size_t *&addr );
};
