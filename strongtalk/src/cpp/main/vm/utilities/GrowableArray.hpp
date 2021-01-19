//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/runtime/ResourceObject.hpp"

#include <vector>

typedef void   (*voidDoFn)( void *p );

typedef bool_t (*growableArrayFindFn)( void *token, void *elem );


class GenericGrowableArray : public PrintableResourceObject {

protected:
    int                 _length;                    // current length
    int                 _maxLength;                 // maximum length
    void                **_data;                    // data array
    bool_t              _allocatedOnSystemHeap;     // is data allocated on C heap?
    std::vector<void *> _vector;                    //

    void grow( int j );     // grow data array (double length until j is a valid index)

    bool_t raw_contains( const void *p ) const;

    int raw_find( const void *p ) const;

    int raw_find( void *token, growableArrayFindFn f ) const;

    void raw_remove( const void *p );

    void raw_apply( voidDoFn f ) const;

    void *raw_at_grow( std::size_t i, const void *fill );

    void raw_at_put_grow( std::size_t i, const void *p, const void *fill );

    void raw_appendAll( GenericGrowableArray *l );

    GenericGrowableArray *raw_copy() const;

    void raw_sort( int f( const void *, const void * ) );

    GenericGrowableArray( int initial_size, bool_t on_C_heap = false );

    GenericGrowableArray( int initial_size, int initial_len, void *filler, bool_t on_C_heap = false );

public:
    void clear();

    int length() const;

    int capacity() const;

    bool_t isEmpty() const;

    bool_t nonEmpty() const;

    bool_t isFull() const;

    void **data_addr() const;    // for sorting
    void print_short();

    void print();
};


template<typename T>
class GrowableArray : public GenericGrowableArray {

public:
    GrowableArray( int initial_size, bool_t on_C_heap = false ) :
            GenericGrowableArray( initial_size, on_C_heap ) {
    }


    GrowableArray( int initial_size, int initial_len, T filler, bool_t on_C_heap = false ) :
            GenericGrowableArray( initial_size, initial_len, (void *) filler, on_C_heap ) {
    }


    GrowableArray() :
            GenericGrowableArray( 2 ) {
    }


    void append( const T elem ) {
        if ( _length == _maxLength )
            grow( _length );
        _data[ _length++ ] = (void *) ( elem );
    }


    T at( std::size_t i ) const {
        st_assert( 0 <= i, "i not greater than 0" );
        return reinterpret_cast<T> (_data[ i ]);
    }


    T first() const {
        st_assert( _length > 0, "empty list" );
        return reinterpret_cast<T> (_data[ 0 ]);
    }


    T last() const {
        st_assert( _length > 0, "empty list" );
        return reinterpret_cast<T> (_data[ _length - 1 ]);
    }


    void push( const T elem ) {
        append( elem );
    }


    T pop() {
        st_assert( _length > 0, "empty list" );
        return reinterpret_cast<T> (_data[ --_length ]);
    }


    T top() const {
        return last();
    }


    void at_put( std::size_t i, const T elem ) {
        st_assert( 0 <= i and i < _length, "illegal index" );
        _data[ i ] = (void *) elem;
    }


    T at_grow( std::size_t i ) {
        st_assert( 0 <= i, "negative index" );
        return reinterpret_cast<T> (raw_at_grow( i, nullptr ));
    }


    void at_put_grow( std::size_t i, const T elem ) {
        st_assert( 0 <= i, "negative index" );
        raw_at_put_grow( i, (void *) elem, nullptr );
    }


    void apply( Closure<T> *c ) const {
        for ( std::size_t i = 0; i < _length; i++ )
            c->do_it( (T) _data[ i ] );
    }


    bool_t contains( const T elem ) const {
        return raw_contains( (const void *) elem );
    }


    int find( const T elem ) const {
        return raw_find( (const void *) elem );
    }


    int find( void *token, bool_t f( void *, T ) ) const {
        return raw_find( token, (growableArrayFindFn) f );
    }


    void remove( const T elem ) {
        raw_remove( (const void *) elem );
    }


    void apply( void f( T ) ) const {
        raw_apply( (voidDoFn) f );
    }


    GrowableArray<T> *copy() const {
        return (GrowableArray<T> *) raw_copy();
    }


    void appendAll( GrowableArray<T> *l ) {
        raw_appendAll( (GenericGrowableArray *) l );
    }


    void sort( int f( T *, T * ) ) {
        raw_sort( (int ( * )( const void *, const void * )) f );
    }
};
