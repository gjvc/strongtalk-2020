//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/utilities/lprintf.hpp"


GenericGrowableArray::GenericGrowableArray( int initial_size, bool_t c_heap ) {
    _length    = 0;
    _maxLength = initial_size * 4;
    st_assert( _length <= _maxLength, "initial_size too small" );
    _allocatedOnSystemHeap = c_heap;
    if ( c_heap ) {
        _data = (void **) AllocateHeap( _maxLength * sizeof( void * ), "bounded list" );
    } else {
        _data = new_resource_array<void *>( _maxLength );
    }
}


GenericGrowableArray::GenericGrowableArray( int initial_size, int initial_len, void *filler, bool_t c_heap ) {
    _length    = initial_len;
    _maxLength = initial_size;
    st_assert( _length <= _maxLength, "initial_len too big" );
    _allocatedOnSystemHeap = c_heap;
    if ( c_heap ) {
        _data = (void **) AllocateHeap( _maxLength * sizeof( void * ), "bounded list" );
    } else {
        _data = new_resource_array<void *>( _maxLength );
    }
    for ( std::size_t i = 0; i < _length; i++ )
        _data[ i ] = filler;
}


void GenericGrowableArray::grow( int j ) {
    void **newData;
    int oldMax = _maxLength;
    if ( _maxLength == 0 ) st_fatal( "cannot grow array with max = 0" ); // for debugging - should create such arrays with max > 0
    while ( j >= _maxLength )
        _maxLength = _maxLength * 2;
    // _console->print_cr( "GenericGrowableArray::grow from [%d] to [%d]", oldMax, max );
    // j < max
    if ( _allocatedOnSystemHeap ) {
        newData = (void **) AllocateHeap( _maxLength * sizeof( void * ), "bounded list" );
    } else {
        newData = new_resource_array<void *>( _maxLength );
    }
    for ( std::size_t i = 0; i < _length; i++ )
        newData[ i ] = _data[ i ];
    _data = newData;
}


bool_t GenericGrowableArray::raw_contains( const void *elem ) const {
    for ( std::size_t i = 0; i < _length; i++ ) {
        if ( _data[ i ] == elem )
            return true;
    }
    return false;
}


GenericGrowableArray *GenericGrowableArray::raw_copy() const {
    GenericGrowableArray *copy = new GenericGrowableArray( _maxLength, _length, nullptr );
    for ( std::size_t i = 0; i < _length; i++ ) {
        copy->_data[ i ] = _data[ i ];
    }
    return copy;
}


void GenericGrowableArray::raw_appendAll( GenericGrowableArray *l ) {
    for ( std::size_t i = 0; i < l->_length; i++ ) {
        raw_at_put_grow( _length, l->_data[ i ], nullptr );
    }
}


int GenericGrowableArray::raw_find( const void *elem ) const {
    for ( std::size_t i = 0; i < _length; i++ ) {
        if ( _data[ i ] == elem )
            return i;
    }
    return -1;
}


int GenericGrowableArray::raw_find( void *token, growableArrayFindFn f ) const {
    for ( std::size_t i = 0; i < _length; i++ ) {
        if ( f( token, _data[ i ] ) )
            return i;
    }
    return -1;
}


void GenericGrowableArray::raw_remove( const void *elem ) {
    for ( std::size_t i = 0; i < _length; i++ ) {
        if ( _data[ i ] == elem ) {
            for ( int j = i + 1; j < _length; j++ )
                _data[ j - 1 ] = _data[ j ];
            _length--;
            return;
        }
    }
    ShouldNotReachHere();
}


void GenericGrowableArray::raw_apply( voidDoFn f ) const {
    for ( std::size_t i = 0; i < _length; i++ )
        f( _data[ i ] );
}


void *GenericGrowableArray::raw_at_grow( std::size_t i, const void *fill ) {
    if ( i >= _length ) {
        if ( i >= _maxLength )
            grow( i );
        for ( int j = _length; j <= i; j++ )
            _data[ j ] = (void *) fill;
        _length = i + 1;
    }
    return _data[ i ];
}


void GenericGrowableArray::raw_at_put_grow( std::size_t i, const void *p, const void *fill ) {
    if ( i >= _length ) {
        if ( i >= _maxLength )
            grow( i );
        for ( int j = _length; j < i; j++ )
            _data[ j ] = (void *) fill;
        _length = i + 1;
    }
    _data[ i ] = (void *) p;
}


void GenericGrowableArray::print() {
    print_short();
    lprintf( ": length %ld (max %ld) { ", _length, _maxLength );
    for ( std::size_t i = 0; i < _length; i++ )
        lprintf( "%#lx ", (std::int32_t) _data[ i ] );
    lprintf( "}\n" );
}


void GenericGrowableArray::raw_sort( int f( const void *, const void * ) ) {
    qsort( _data, length(), sizeof( void * ), f );
}


void GenericGrowableArray::print_short() {
    _console->print( "Growable Array %#lx", this );
}


void GenericGrowableArray::clear() {
    _length = 0;
}


int GenericGrowableArray::length() const {
    return _length;
}


int GenericGrowableArray::capacity() const {
    return _maxLength;
}


bool_t GenericGrowableArray::isEmpty() const {
    return _length == 0;
}


bool_t GenericGrowableArray::nonEmpty() const {
    return _length not_eq 0;
}


bool_t GenericGrowableArray::isFull() const {
    return _length == _maxLength;
}


void **GenericGrowableArray::data_addr() const {
    return _data;
}
