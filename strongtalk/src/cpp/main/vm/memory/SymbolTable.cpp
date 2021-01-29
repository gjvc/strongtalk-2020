//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/SymbolTable.hpp"
#include "vm/oops/SymbolKlass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/MarkSweep.hpp"

#define FOR_ALL_ENTRIES( entry ) \
  for (entry = firstBucket(); entry <= lastBucket(); entry ++)

#define FOR_ALL_SYMBOL_ADDR( bucket, var, code )                        \
    { if (bucket->is_symbol()) {                                        \
         var = (SymbolOop*) bucket; code;                               \
      } else {                                                          \
        for (SymbolTableLink* l = bucket->get_link(); l; l = l->next) { \
          var = &l->symbol; code;                                       \
        }                                                               \
      }                                                                 \
    }


std::int32_t hash( const char *name, std::int32_t len ) {

    // hash on at most 32 characters, evenly spaced
    std::int32_t increment;

    if ( len < 32 ) {
        increment = 1;
    } else {
        increment = len >> 5;
    }

    // hashpjw from Dragon book (ASU p. 436), except increment differently

    st_assert( BitsPerByte * BytesPerWord == 32, "assumes 32-bit words" );
    std::int32_t unsigned h = 0;
    std::int32_t unsigned g;

    const char *s   = name;
    const char *end = s + len;

    for ( ; s < end; s = s + increment ) {

        h = ( h << 4 ) + (std::int32_t unsigned) *s;

        if ( g = h & 0xf0000000 )
            h ^= g | ( g >> 24 );
    }
    return h;
}


SymbolTable::SymbolTable() {
    for ( std::int32_t i = 0; i < symbol_table_size; i++ )
        buckets[ i ].clear();
    free_list = first_free_link = end_block = nullptr;
}


SymbolOop SymbolTable::basic_add( const char *name, std::int32_t len, std::int32_t hashValue ) {
    SymbolKlass *sk = (SymbolKlass *) Universe::symbolKlassObject()->klass_part();
    SymbolOop str = sk->allocateSymbol( name, len );
    basic_add( str, hashValue );
    return str;
}


bool SymbolTable::is_present( SymbolOop sym ) {

    const char *name = (const char *) sym->bytes();
    std::int32_t len       = sym->length();
    std::int32_t hashValue = hash( name, len );
    SymbolTableEntry *bucket = bucketFor( hashValue );

    if ( bucket->is_empty() )
        return false;

    if ( bucket->is_symbol() )
        return bucket->get_symbol()->equals( name, len );

    for ( SymbolTableLink *l = bucket->get_link(); l; l = l->next )
        if ( l->symbol->equals( name, len ) )
            return true;
    return false;
}


SymbolOop SymbolTable::lookup( const char *name, std::int32_t len ) {

    std::int32_t hashValue = hash( name, len );

    SymbolTableEntry *bucket = bucketFor( hashValue );

    if ( not bucket->is_empty() ) {
        if ( bucket->is_symbol() ) {
            if ( bucket->get_symbol()->equals( name, len ) )
                return bucket->get_symbol();
        } else {
            for ( SymbolTableLink *l = bucket->get_link(); l; l = l->next )
                if ( l->symbol->equals( name, len ) )
                    return l->symbol;
        }
    }

    return basic_add( name, len, hashValue );
}


void SymbolTable::add( SymbolOop s ) {
    st_assert( s->is_symbol(), "adding something that's not a symbol to the symbol table" );
    st_assert( s->is_old(), "all symbols should be tenured" );
    std::int32_t hashValue = hash( (const char *) s->bytes(), s->length() );
    basic_add( s, hashValue );
}


void SymbolTable::add_symbol( SymbolOop s ) {
    basic_add( s, hash( (const char *) s->bytes(), s->length() ) );
}


SymbolOop SymbolTable::basic_add( SymbolOop s, std::int32_t hashValue ) {
    st_assert( s->is_symbol(), "adding something that's not a symbol to the symbol table" );
    st_assert( s->is_old(), "all symbols should be tenured" );

    // Add the indentity hash for the new symbol
    s->hash_value();
    st_assert( not s->mark()->has_valid_hash(), "should not have a hash yet" );
    s->set_mark( s->mark()->set_hash( s->hash_value() ) );
    st_assert( s->mark()->has_valid_hash(), "should have a hash now" );

    SymbolTableEntry *bucket = bucketFor( hashValue );

    if ( bucket->is_empty() ) {
        bucket->set_symbol( s );
    } else {
        SymbolTableLink *old_link;
        if ( bucket->is_symbol() ) {
            old_link = Universe::symbol_table->new_link( bucket->get_symbol() );
        } else {
            old_link = bucket->get_link();
        }
        bucket->set_link( Universe::symbol_table->new_link( s, old_link ) );
    }
    return s;
}


void SymbolTable::switch_pointers( Oop from, Oop to ) {
    if ( not from->is_symbol() )
        return;
    st_assert( to->is_symbol(), "cannot replace a symbol with a non-symbol" );

    SymbolTableEntry *e;
    FOR_ALL_ENTRIES( e ) {
        SymbolOop *addr;
        FOR_ALL_SYMBOL_ADDR( e, addr, SWITCH_POINTERS_TEMPLATE( addr ) );
    }
}


void SymbolTable::follow_used_symbols() {
    // throw out unreachable symbols
    SymbolTableEntry *e;
    FOR_ALL_ENTRIES( e ) {
        // If we have a one element list; preserve the symbol but remove the chain
        // This moving around cannot take place after follow_root has been called
        // since follow_root reverse pointers.
        if ( e->get_link() and not e->get_link()->next ) {
            SymbolTableLink *old = e->get_link();
            e->set_symbol( old->symbol );
            delete_link( old );
        }

        if ( e->is_symbol() ) {
            if ( e->get_symbol()->is_gc_marked() )
                MarkSweep::follow_root( (Oop *) e );
            else
                e->clear(); // unreachable; clear entry
        } else {
            SymbolTableLink **p   = (SymbolTableLink **) e;
            SymbolTableLink *link = e->get_link();
            while ( link ) {
                if ( link->symbol->is_gc_marked() ) {
                    MarkSweep::follow_root( (Oop *) &link->symbol );
                    p    = &link->next;
                    link = link->next;
                } else {
                    // unreachable; remove from table
                    SymbolTableLink *old = link;
                    *p = link->next;
                    link = link->next;
                    old->next = nullptr;
                    delete_link( old );
                }
            }
        }
    }
}


void SymbolTableEntry::deallocate() {
    if ( not is_symbol() and get_link() )
        Universe::symbol_table->delete_link( get_link() );
}


bool SymbolTableEntry::verify( std::int32_t i ) {
    bool flag = true;
    if ( is_symbol() ) {
        if ( not get_symbol()->is_symbol() ) {
            error( "entry 0x{0:x} in symbol table isn't a symbol", get_symbol() );
            flag = false;
        }
    } else {
        if ( get_link() )
            flag = get_link()->verify( i );
    }
    return flag;
}


void SymbolTable::verify() {
    for ( std::int32_t i = 0; i < symbol_table_size; i++ )
        if ( not buckets[ i ].verify( i ) )
            spdlog::info( "\tof bucket %ld of symbol table", std::int32_t( i ) );
}


void SymbolTable::relocate() {
    SymbolTableEntry *e;
    FOR_ALL_ENTRIES( e ) {
        SymbolOop *addr;
        FOR_ALL_SYMBOL_ADDR( e, addr, RELOCATE_TEMPLATE( addr ) );
    }
}


bool SymbolTableLink::verify( std::int32_t i ) {
    bool flag = true;
    for ( SymbolTableLink *l = this; l; l = l->next ) {
        if ( not l->symbol->is_symbol() ) {
            error( "entry 0x{0:x} in symbol table isn't a symbol", l->symbol );
            flag = false;
        } else if ( hash( reinterpret_cast<const char *>( l->symbol->bytes() ), l->symbol->length() ) % symbol_table_size not_eq i ) {
            error( "entry 0x{0:x} in symbol table has wrong hash value", l->symbol );
            flag = false;
        } else if ( not l->symbol->is_old() ) {
            error( "entry 0x{0:x} in symbol table isn't tenured", l->symbol );
            flag = false;
        }
    }
    return flag;
}


std::int32_t SymbolTableEntry::length() {
    if ( is_symbol() )
        return 1;
    if ( not get_link() )
        return 0;
    std::int32_t count = 0;
    for ( SymbolTableLink *l = get_link(); l; l = l->next )
        count++;
    return count;
}


SymbolTableLink *SymbolTable::new_link( SymbolOop s, SymbolTableLink *n ) {
    SymbolTableLink *res;
    if ( free_list ) {
        res       = free_list;
        free_list = free_list->next;
    } else {
        const std::int32_t block_size = 500;
        if ( first_free_link == end_block ) {
            first_free_link = new_c_heap_array<SymbolTableLink>( block_size );
            end_block       = first_free_link + block_size;
        }
        res                  = first_free_link++;
    }
    res->symbol = s;
    res->next   = n;
    return res;
}


void SymbolTable::delete_link( SymbolTableLink *l ) {
    // Add the link to the freelist
    SymbolTableLink *end = l;
    while ( end->next )
        end = end->next;
    end->next = free_list;
    free_list = l;
}


// much of this comes from the print_histogram routine in mapTable.c,
// so if bug fixes are made here, also make them in mapTable.cpp.
void SymbolTable::print_histogram() {
    const std::int32_t results_length = 100;
    std::int32_t       results[results_length];

    // initialize results to zero
    std::int32_t j = 0;
    for ( ; j < results_length; j++ ) {
        results[ j ] = 0;
    }

    std::int32_t total        = 0;
    std::int32_t min_symbols  = 0;
    std::int32_t max_symbols  = 0;
    std::int32_t out_of_range = 0;

    for ( std::int32_t i = 0; i < symbol_table_size; i++ ) {

        SymbolTableEntry curr    = buckets[ i ];
        std::int32_t              counter = curr.length();

        total += counter;
        if ( counter < results_length ) {
            results[ counter ]++;
        } else {
            out_of_range++;
        }
        min_symbols = min( min_symbols, counter );
        max_symbols = max( max_symbols, counter );
    }
    spdlog::info( "Symbol Table:" );
    spdlog::info( "%8s %5d", "Total  ", total );
    spdlog::info( "%8s %5d", "Minimum", min_symbols );
    spdlog::info( "%8s %5d", "Maximum", max_symbols );
    spdlog::info( "%8s %3.2f", "Average", ( (float) total / (float) symbol_table_size ) );
    spdlog::info( "%s", "Histogram:" );
    spdlog::info( " %s %29s", "Length", "Number chains that length" );


    for ( std::int32_t i = 0; i < results_length; i++ ) {
        if ( results[ i ] > 0 ) {
            spdlog::info( "%6d %10d", i, results[ i ] );
        }
    }


    std::int32_t line_length = 70;
    spdlog::info( "%s %30s", " Length", "Number chains that length" );
    for ( std::int32_t i = 0; i < results_length; i++ ) {
        if ( results[ i ] > 0 ) {
            spdlog::info( "%4d", i );
            for ( j = 0; ( j < results[ i ] ) and ( j < line_length ); j++ ) {
                spdlog::info( "%1s", "*" );
            }
            if ( j == line_length ) {
                spdlog::info( "%1s", "+" );
            }
            spdlog::info( "" );
        }
    }
    spdlog::info( " %s %d: %d", "Number chains longer than", results_length, out_of_range );
}


SymbolTableEntry *SymbolTable::bucketFor( std::int32_t hashValue ) {
    st_assert( hashValue % symbol_table_size >= 0, "must be positive" );
    return &buckets[ hashValue % symbol_table_size ];
}


SymbolTableEntry *SymbolTable::firstBucket() {
    return &buckets[ 0 ];
}


SymbolTableEntry *SymbolTable::lastBucket() {
    return &buckets[ symbol_table_size - 1 ];
}
