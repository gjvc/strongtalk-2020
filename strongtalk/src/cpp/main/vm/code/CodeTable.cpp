//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/CodeTable.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/NativeMethod.hpp"


CodeTable::CodeTable( std::int32_t size ) {
    tableSize = size;
    buckets   = new_c_heap_array<CodeTableEntry>( size );
    clear();
}


CodeTableLink *CodeTable::new_link( NativeMethod *nm, CodeTableLink *n ) {
    CodeTableLink *res = new_c_heap_array<CodeTableLink>( 1 );
    res->_nativeMethod = nm;
    res->_next         = n;
    return res;
}


void CodeTable::clear() {
    for ( std::int32_t i = 0; i < tableSize; i++ )
        at( i )->clear();
}


NativeMethod *CodeTable::lookup( const LookupKey *L ) {
    CodeTableEntry *bucket = bucketFor( L->hash() );

    // Empty
    if ( bucket->is_empty() )
        return nullptr;

    // Singleton
    if ( bucket->is_nativeMethod() ) {
        if ( bucket->get_nativeMethod()->_lookupKey.equal( L ) )
            return bucket->get_nativeMethod();
        return nullptr;
    }

    // Bucket
    for ( CodeTableLink *l = bucket->get_link(); l; l = l->_next ) {
        if ( l->_nativeMethod->_lookupKey.equal( L ) )
            return l->_nativeMethod;
    }

    return nullptr;
}


void CodeTable::add( NativeMethod *nm ) {

    if ( lookup( &nm->_lookupKey ) ) {
        st_fatal2( "adding duplicate key to code table: 0x{0:x} and new 0x{0:x}", lookup( &nm->_lookupKey ), nm );
    }
    CodeTableEntry *bucket = bucketFor( nm->_lookupKey.hash() );

    if ( bucket->is_empty() ) {
        bucket->set_nativeMethod( nm );
    } else {
        CodeTableLink *old_link;
        if ( bucket->is_nativeMethod() ) {
            old_link = new_link( bucket->get_nativeMethod() );
        } else {
            old_link = bucket->get_link();
        }
        bucket->set_link( new_link( nm, old_link ) );
    }
}


void CodeTable::addIfAbsent( NativeMethod *nm ) {
    if ( not lookup( &nm->_lookupKey ) )
        add( nm );
}


bool CodeTable::is_present( NativeMethod *nm ) {
    return lookup( &nm->_lookupKey ) == nm;
}


void CodeTable::remove( NativeMethod *nm ) {

    CodeTableEntry *bucket = bucketFor( nm->_lookupKey.hash() );
    if ( bucket->is_empty() ) {
        st_fatal( "trying to remove NativeMethod that is not present" );
    }

    if ( bucket->is_nativeMethod() ) {
        bucket->clear();
        return;
    }

    if ( bucket->get_link()->_nativeMethod == nm ) {
        // is it the first link
        CodeTableLink *disposable_link = bucket->get_link();
        bucket->set_link( disposable_link->_next );
        delete disposable_link;
    } else {
        // the the method must be further down the chain
        CodeTableLink *current = bucket->get_link();
        while ( current->_next ) {
            if ( current->_next->_nativeMethod == nm ) {
                CodeTableLink *disposable_link = current->_next;
                current->_next = disposable_link->_next;
                delete disposable_link;
                return;
            }
            current = current->_next;
        }
        st_fatal( "trying to remove NativeMethod that is not present" );
    }

}


bool CodeTable::verify() {
    bool flag = true;
    return flag;
}


void CodeTable::print() {
    spdlog::info( "CodeTable" );
}


void CodeTable::print_stats() {
#ifdef NOT_IMPLEMENTED
    std::int32_t nmin = 9999999, nmax = 0, total = 0, nonzero = 0;
    constexpr std::int32_t N = 10;
    std::int32_t histo[N];
    for (std::int32_t i = 0; i < N; i++) histo[i] = 0;
    for (nmln* p = buckets;  p < &buckets[tableSize];  ++p) {
      std::int32_t len = 0;
      for (nmln* q = p->next;  q not_eq p;  q = q->next) len++;
      if (len < nmin) nmin = len;
      if (len > nmax) nmax = len;
      if (len) nonzero++;
      total += len;
      histo[min(len, N-1)]++;
    }
    spdlog::info("\ncodeTable statistics: 0x%08x nativeMethods; min chain = 0x%08x, max = 0x%08x, avg = %4.1f", total, nmin, nmax, (float)total / nonzero);
    spdlog::info("histogram:");
    for (std::int32_t i = 0; i < N - 1; i++) spdlog::info("%4d:\t%d", i, histo[i]);
    spdlog::info(">=0x%08x:\t0x%08x", N-1, histo[N-1]);
#endif
}
