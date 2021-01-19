//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/macros.hpp"
#include "vm/memory/util.hpp"
#include "allocation.hpp"
#include "vm/code/LogicalAddress.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/ScopeDescriptorNode.hpp"
#include "vm/code/MethodScopeNode.hpp"
#include "vm/memory/Array.hpp"


Array::Array( int sz ) {
    _size   = sz;
    _index  = 0;
    _values = new_resource_array<int>( sz );
}


int Array::insertIfAbsent( int value ) {
    for ( std::size_t i = 0; i < _index; i++ )
        if ( _values[ i ] == value )
            return i;
    if ( _index == _size )
        extend( _size * 2 );
    _values[ _index ] = value;
    return _index++;
}


void Array::extend( int newSize ) {
    int *newValues = new_resource_array<int>( newSize );

    for ( std::size_t i = 0; i < _index; i++ )
        newValues[ i ] = _values[ i ];

    _values = newValues;
    _size   = newSize;
}


void Array::copy_to( int *&addr ) {
    for ( std::size_t i = 0; i < length(); i++ ) {
        *addr++ = _values[ i ];
    }
}


ByteArray::ByteArray( int size ) {
    _array = new_resource_array<std::uint8_t>( size );
    _max   = size;
    _top   = 0;
}


void ByteArray::extend() {
    int newMax = _max * 2;
    std::uint8_t *newArray = new_resource_array<std::uint8_t>( newMax );

    for ( std::size_t i = 0; i < _top; i++ )
        newArray[ i ] = _array[ i ];

    _array = newArray;
    _max   = newMax;
}


void ByteArray::appendHalf( std::int16_t p ) {

    if ( _top + (int) sizeof( std::int16_t ) > _max )
        extend();

    // Saving the half as two bytes to avoid alignment problem.
    _array[ _top++ ] = p >> BYTE_WIDTH;
    _array[ _top++ ] = (std::uint8_t) lowerBits( p, 8 );
}


void ByteArray::putHalfAt( std::int16_t p, int offset ) {
    // Saving the half as two bytes to avoid alignment problem.
    _array[ offset ]     = p >> BYTE_WIDTH;
    _array[ offset + 1 ] = (std::uint8_t) lowerBits( p, 8 );
}


void ByteArray::appendWord( int p ) {
    if ( _top + sizeof( int ) > _max )
        extend();
    st_assert( size() % sizeof( int ) == 0, "Not word aligned" );
    int *s = (int *) &_array[ _top ];
    *s = p;
    _top += sizeof( int );
}


void ByteArray::alignToWord() {
    int fill_size = ( sizeof( int ) - ( size() % sizeof( int ) ) ) % sizeof( int );

    for ( std::size_t i = 0; i < fill_size; i++ )
        appendByte( 0 );
}


void ByteArray::copy_to( int *&addr ) {
    int *fromAddr = (int *) start();
    int len = size() / sizeof( int );

    for ( std::size_t i = 0; i < len; i++ ) {
        *addr++ = *fromAddr++;
    }
}
