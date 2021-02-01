
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/runtime/ResourceObject.hpp"

#include <vector>

typedef void (*voidDoFn)( void *p );

typedef bool (*growableArrayFindFn)( void *token, void *elem );

constexpr std::size_t INITIAL_ARRAY_SIZE{ 32 };

class GenericGrowableArray : public PrintableResourceObject {

protected:
    std::int32_t _length;                    // current length
    std::int32_t _maxLength;                 // maximum length
    void         **_data;                    // data array
    bool         _allocatedOnSystemHeap;     // is data allocated on C heap?

    void grow( std::int32_t j );     // grow data array (double length until j is a valid index)

    bool raw_contains( const void *p ) const;

    std::int32_t raw_find( const void *p ) const;

    std::int32_t raw_find( void *token, growableArrayFindFn f ) const;

    void raw_remove( const void *p );

    void raw_apply( voidDoFn f ) const;

    void *raw_at_grow( std::int32_t i, const void *fill );

    void raw_at_put_grow( std::int32_t i, const void *p, const void *fill );

    void raw_appendAll( GenericGrowableArray *l );

    GenericGrowableArray *raw_copy() const;

    void raw_sort( std::int32_t f( const void *, const void * ) );

    GenericGrowableArray( std::int32_t initial_size, bool on_C_heap = false );

    GenericGrowableArray( std::int32_t initial_size, std::int32_t initial_len, void *filler, bool on_C_heap = false );

public:
    void clear();

    std::int32_t length() const;

    std::int32_t capacity() const;

    bool isEmpty() const;

    bool nonEmpty() const;

    bool isFull() const;

    void **data_addr() const;    // for sorting
    void print_short();

    void print();
};


template<typename T>
class GrowableArray : public GenericGrowableArray {
private:
    std::vector<T>                    _vector; //
    std::array<T, INITIAL_ARRAY_SIZE> _array; //
public:

    GrowableArray( std::int32_t initial_size, bool on_C_heap = false ) :
        GenericGrowableArray( initial_size, on_C_heap ) {
    }


    GrowableArray( std::int32_t initial_size, std::int32_t initial_len, T filler, bool on_C_heap = false ) :
        GenericGrowableArray( initial_size, initial_len, (void *) filler, on_C_heap ) {
    }


    GrowableArray() :
        GenericGrowableArray( 2 ) {
    }


    void append( const T elem ) {
        if ( _length == _maxLength ) {
            grow( _length );
        }
        _data[ _length++ ] = (void *) ( elem );
    }


    T at( std::int32_t i ) const {
        st_assert( 0 <= i, "i not greater than 0" );
        return reinterpret_cast<T>( _data[ i ] );
    }


    T first() const {
        st_assert( _length > 0, "empty list" );
        return reinterpret_cast<T>( _data[ 0 ] );
    }


    T last() const {
        st_assert( _length > 0, "empty list" );
        return reinterpret_cast<T>( _data[ _length - 1 ] );
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


    void at_put( std::int32_t i, const T elem ) {
        st_assert( 0 <= i and i < _length, "illegal index (at_put)" );
        _data[ i ] = (void *) elem;
    }


    T at_grow( std::int32_t i ) {
        st_assert( 0 <= i, "negative index" );
        return reinterpret_cast<T> (raw_at_grow( i, nullptr ));
    }


    void at_put_grow( std::int32_t i, const T elem ) {
        st_assert( 0 <= i, "negative index" );
        raw_at_put_grow( i, (void *) elem, nullptr );
    }


    void apply( Closure<T> *c ) const {
        for ( std::int32_t i = 0; i < _length; i++ ) {
            c->do_it( (T) _data[ i ] );
        }
    }


    bool contains( const T elem ) const {
        return raw_contains( (const void *) elem );
    }


    std::int32_t find( const T elem ) const {
        return raw_find( (const void *) elem );
    }


    std::int32_t find( void *token, bool f( void *, T ) ) const {
        return raw_find( token, (growableArrayFindFn) f );
    }


    void remove( const T elem ) {
        raw_remove( (const void *) elem );
    }


    void apply( void f( T ) ) const {
        raw_apply( (voidDoFn) f );
    }


    GrowableArray<T> *copy() const {
        return static_cast<GrowableArray<T> *>( raw_copy() );
    }


    void appendAll( GrowableArray<T> *l ) {
        raw_appendAll( static_cast<GenericGrowableArray *>( l ) );
    }


    void sort( std::int32_t f( T *, T * ) ) {
        raw_sort( (std::int32_t ( * )( const void *, const void * )) f );
    }

};
