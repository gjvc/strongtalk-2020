//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/runtime/ResourceObject.hpp"


// Yet another version of lists, this one allowing efficient insert/delete in the middle.

class GenericSListElem : public ResourceObject {

protected:
    void             *_data;
    GenericSListElem *_next;

public:
    GenericSListElem( void *d, GenericSListElem *n = nullptr ) {
        _data = d;
        _next = n;
    }


    void *dataL() const {
//        return this ? _data : nullptr;
        return _data;
    }


    void setDataL( void *d ) {
        _data = d;
    }


    GenericSListElem *nextL() const {
        return _next;
    }


    void setNextL( GenericSListElem *n ) {
        _next = n;
    }


protected:
    void insertAfterL( void *d ) {
        _next = new GenericSListElem( d, _next );
    }


    friend class GenericSList;
};

typedef bool (*slistFindFn)( void *token, void *elem );

class GenericSList : public PrintableResourceObject {
protected:
    GenericSListElem *_head;
    GenericSListElem *_tail;
    std::int32_t     _len;
public:
    GenericSList() {
        _head = _tail = nullptr;
        _len  = 0;
    }


    void prependL( void *d ) {
        _head     = new GenericSListElem( d, _head );
        if ( _tail == nullptr )
            _tail = _head;
        _len++;
    }


    void appendL( void *d ) {
        GenericSListElem *e = new GenericSListElem( d );
        if ( _tail == nullptr ) {
            _head = _tail = e;
        } else {
            _tail->_next = e;
            _tail = e;
        }
        _len++;
    }


    GenericSListElem *headL() const {
        return _head;
    }


    GenericSListElem *tailL() const {
        return _tail;
    }


    void applyL( void f( void * ) );


    bool isEmpty() const {
        return _head == nullptr;
    }


    bool nonEmpty() const {
        return not isEmpty();
    }


    std::int32_t length() const {
        return _len;
    }


    bool includesL( void *p ) const {
        return findL( p ) not_eq nullptr;
    }


    GenericSListElem *findL( void *p ) const;

    GenericSListElem *findL( void *p, slistFindFn f ) const;

    void *nthL( std::int32_t i ) const;


    void *firstL() const {
        return headL()->dataL();
    }


    void *secondL() const {
        return headL()->nextL()->dataL();
    }


    void *lastL() const {
        return tailL()->dataL();
    }


    void insertAfterL( GenericSListElem *e, void *d );

    void removeL( void *d );

    void removeAfterL( GenericSListElem *e );

protected:
    void *removeHeadL() {
        st_assert( nonEmpty(), "removing from an empty list" );
        void *d = firstL();
        _head     = headL()->nextL();
        if ( --_len == 0 )
            _tail = _head;
        return d;
    }


public:
    void pushL( void *d ) {
        prependL( d );
    }


    void *popL() {
        return removeHeadL();
    }


    void print();

    void print_short();
};


template<typename T>
class SListElem : public GenericSListElem {
public:
    SListElem( T d, SListElem<T> *n = nullptr ) :
        GenericSListElem( (void *) d, (GenericSListElem *) n ) {
    }


    \



    T data() const {
        return (T) GenericSListElem::dataL();
    }


    void setData( T d ) {
        GenericSListElem::setDataL( (void *) d );
    }


    SListElem<T> *next() const {
        return (SListElem<T> *) GenericSListElem::nextL();
    }


    void setNext( SListElem<T> *n ) {
        GenericSListElem::setNextL( n );
    }


    void insertAfter( T d ) {
        GenericSListElem::insertAfterL( (void *) d );
    }
};

template<typename T>
class SList : public GenericSList {
public:
    SList() {
    }


    SListElem<T> *head() const {
        return (SListElem<T> *) GenericSList::headL();
    }


    SListElem<T> *tail() const {
        return (SListElem<T> *) GenericSList::tailL();
    }


    bool includes( T e ) const {
        return GenericSList::includesL( (void *) e );
    }


    SListElem<T> *find( T e ) const {
        return (SListElem<T> *) GenericSList::findL( (void *) e );
    }


    SListElem<T> *find( void *token, slistFindFn f ) const {
        return (SListElem<T> *) GenericSList::findL( token, (slistFindFn) f );
    }


    T first() const {
        return (T) GenericSList::firstL();
    }


    T second() const {
        return (T) GenericSList::secondL();
    }


    T at( std::int32_t i ) const {
        return (T) GenericSList::nthL( i );
    }


    T last() const {
        return (T) GenericSList::lastL();
    }


    void prepend( T d ) {
        GenericSList::prependL( (void *) d );
    }


    void append( T d ) {
        GenericSList::appendL( (void *) d );
    }


    void apply( void f( T ) ) {
        GenericSList::applyL( (void ( * )( void * )) f );
    }


    void insertAfter( SListElem<T> *e, T d ) {
        GenericSList::insertAfterL( (SListElem<T> *) e, (void *) d );
    }


    void removeAfter( SListElem<T> *e, T d ) {
        GenericSList::removeAfterL( (SListElem<T> *) e );
    }


    void remove( T d ) {
        GenericSList::removeL( (void *) d );
    }


    T removeHead() {
        return (T) GenericSList::removeHeadL();
    }


    void push( T d ) {
        GenericSList::pushL( (void *) d );
    }


    T pop() {
        return (T) GenericSList::popL();
    }


    void apply( Closure<T> *c ) {
        SListElem<T>       *nexte;    // to permit removing during iteration
        for ( SListElem<T> *e = head(); e; e = nexte ) {
            nexte = e->next();
            c->do_it( e->data() );
        }
    }
};
