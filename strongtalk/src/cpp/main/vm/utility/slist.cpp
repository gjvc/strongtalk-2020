//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "slist.hpp"
#include "vm/runtime/ResourceObject.hpp"


GenericSListElem *GenericSList::findL( void *p ) const {
    for ( GenericSListElem *e = headL(); e; e = e->nextL() ) {
        if ( e->dataL() == p )
            return e;
    }
    return nullptr;
}


GenericSListElem *GenericSList::findL( void *token, slistFindFn f ) const {
    for ( GenericSListElem *e = headL(); e; e = e->nextL() ) {
        if ( f( token, e->dataL() ) )
            return e;
    }
    return nullptr;
}


void *GenericSList::nthL( std::int32_t n ) const {
    st_assert( n < length(), "non-existing element" );
    GenericSListElem   *e = headL();
    for ( std::int32_t i  = 0; i < n; i++, e = e->nextL() );
    return e->dataL();
}


void GenericSList::insertAfterL( GenericSListElem *e, void *d ) {
    if ( e == tailL() ) {
        appendL( d );
    } else if ( e == nullptr ) {
        prependL( d );
    } else {
        GenericSListElem *newe = new GenericSListElem( d, e->nextL() );
        e->setNextL( newe );
        _len++;
    }
}


void GenericSList::removeAfterL( GenericSListElem *e ) {
    if ( e == nullptr ) {
        removeHeadL();
    } else {
        GenericSListElem *deletee = e->nextL();
        e->_next = deletee->nextL();
        if ( deletee == tailL() )
            _tail = e;
        _len--;
    }
}


void GenericSList::removeL( void *p ) {
    GenericSListElem *prev = nullptr;
    GenericSListElem *e    = headL();
    for ( ; e and e->dataL() not_eq p; prev = e, e = e->nextL() );
    if ( e == nullptr ) st_fatal( "not in list" );
    removeAfterL( prev );
    st_assert( not includesL( p ), "remove doesn't work" );
}


void GenericSList::applyL( void f( void * ) ) {

    GenericSListElem *nexte;    // to permit removing during iteration

    for ( GenericSListElem *e = headL(); e; e = nexte ) {
        nexte = e->nextL();
        f( e->dataL() );
    }
}


void GenericSList::print_short() {
    SPDLOG_INFO( "GenericSList 0x{0:x}", static_cast<void *>( this ) );
}


static void print_them( void *p ) {
    ( (PrintableResourceObject *) p )->print_short();
    SPDLOG_INFO( " " );
}


void GenericSList::print() {
    print_short();
    SPDLOG_INFO( ": " );
    ( (GenericSList *) this )->applyL( print_them );
}
