
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


Array::Array( std::int32_t sz ) {
    _size   = sz;
    _index  = 0;
    _values = new_resource_array<std::int32_t>( sz );
}


std::int32_t Array::insertIfAbsent( std::int32_t value ) {
    for ( std::int32_t i = 0; i < _index; i++ )
        if ( _values[ i ] == value )
            return i;
    if ( _index == _size )
        extend( _size * 2 );
    _values[ _index ] = value;
    return _index++;
}


void Array::extend( std::int32_t newSize ) {
    std::int32_t *newValues = new_resource_array<std::int32_t>( newSize );

    for ( std::int32_t i = 0; i < _index; i++ )
        newValues[ i ] = _values[ i ];

    _values = newValues;
    _size   = newSize;
}


void Array::copy_to( std::int32_t *&addr ) {
    for ( std::int32_t i = 0; i < length(); i++ ) {
        *addr++ = _values[ i ];
    }
}


ByteArray::ByteArray( std::int32_t size ) {
    _array = new_resource_array<std::uint8_t>( size );
    _max   = size;
    _top   = 0;
}


void ByteArray::extend() {
    std::int32_t newMax    = _max * 2;
    std::uint8_t *newArray = new_resource_array<std::uint8_t>( newMax );

    for ( std::int32_t i = 0; i < _top; i++ ) {
        newArray[ i ] = _array[ i ];
    }

    _array = newArray;
    _max   = newMax;
}


void ByteArray::appendHalf( std::int16_t p ) {

    if ( _top + (std::int32_t) sizeof( std::int16_t ) > _max ) {
        extend();
    }

    // Saving the half as two bytes to avoid alignment problem.
    _array[ _top++ ] = p >> BYTE_WIDTH;
    _array[ _top++ ] = (std::uint8_t) lowerBits( p, 8 );
}


void ByteArray::putHalfAt( std::int16_t p, std::int32_t offset ) {
    // Saving the half as two bytes to avoid alignment problem.
    _array[ offset ]     = p >> BYTE_WIDTH;
    _array[ offset + 1 ] = (std::uint8_t) lowerBits( p, 8 );
}


void ByteArray::appendWord( std::int32_t p ) {

    if ( _top + sizeof( std::int32_t ) > _max ) {
        extend();
    }

    st_assert( size() % sizeof( std::int32_t ) == 0, "Not word aligned" );
    std::int32_t *s = (std::int32_t *) &_array[ _top ];
    *s = p;
    _top += sizeof( std::int32_t );
}


void ByteArray::alignToWord() {
    std::int32_t fill_size = ( sizeof( std::int32_t ) - ( size() % sizeof( std::int32_t ) ) ) % sizeof( std::int32_t );

    for ( std::int32_t i = 0; i < fill_size; i++ ) {
        appendByte( 0 );
    }

}


void ByteArray::copy_to( std::int32_t *&addr ) {
    std::int32_t *fromAddr = (std::int32_t *) start();
    std::int32_t len       = size() / sizeof( std::int32_t );

    for ( std::int32_t i = 0; i < len; i++ ) {
        *addr++ = *fromAddr++;
    }
}
