//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/AgeTable.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/utilities/objectIDTable.hpp"


MemOop as_memOop( void *p ) {
    st_assert( ( int( p ) & TAG_MASK ) == 0, "not an aligned C pointer" );
    return MemOop( int( p ) + MEMOOP_TAG );
}


int MemOopDescriptor::scavenge_contents() {
    return blueprint()->oop_scavenge_contents( this );
}


void MemOopDescriptor::layout_iterate_body( ObjectLayoutClosure *blk, int begin, int end ) {
    Oop *p = (Oop *) addr();
    Oop *q = p + end;
    p += begin;
    while ( p < q ) {
        int       offset = p - (Oop *) addr();
        // Compute the instance variable name at the current offset
        SymbolOop name   = blueprint()->inst_var_name_at( offset );
        const char *n = "instVar?";
        if ( name ) {
            StringOutputStream stream( 50 );
            name->print_symbol_on( &stream );
            n = stream.as_string();
        }
        blk->do_oop( n, p++ );
    }
}


Oop MemOopDescriptor::scavenge() {
    st_assert( ( not Universe::should_scavenge( this ) ) == ( ( (const char *) this > Universe::old_gen._lowBoundary ) or Universe::new_gen.to()->contains( this ) ), "just checking" );

    if ( ( (const char *) this > Universe::old_gen._lowBoundary ) or Universe::new_gen.to()->contains( this ) ) {
        return this;
    } else if ( this->is_forwarded() ) {
        return Oop( this->forwardee() );
    } else {
        return copy_to_survivor_space();
    }
}


void MemOopDescriptor::follow_contents() {
    st_assert( is_gc_marked(), "pointer reversal should have taken place" );
    // lprintf("[%s, 0x%lx, 0x%lx]", blueprint()->name(), this, klass());
    blueprint()->oop_follow_contents( this );
}


Oop MemOopDescriptor::copy_to_survivor_space() {
    int s = size();
    st_assert( Universe::should_scavenge( this ) and not is_forwarded(), "shouldn't be scavenging" );
    bool_t is_new;
    Oop *x = Universe::allocate_in_survivor_space( this, s, is_new );

#ifdef VERBOSE_SCAVENGING
    lprintf("{copy %s %#lx -> %#lx (%d)}\n", blueprint()->name(), oops(), x, s);
#endif

    MemOop p = as_memOop( x );
    copy_oops( oops(), x, s );

    if ( is_new ) {
        p->set_mark( p->mark()->incr_age() );
        Universe::age_table->add( p, s );
    } else {
# ifdef VERBOSE_SCAVENGING
        lprintf("{tenuring %s %#lx -> %#lx (%d)}\n", blueprint()->name(), oops(), x, s);
# endif
    }
    forward_to( p );
    return p;
}


void MemOopDescriptor::oop_iterate( OopClosure *blk ) {
    blueprint()->oop_oop_iterate( this, blk );
}


void MemOopDescriptor::layout_iterate( ObjectLayoutClosure *blk ) {
    blueprint()->oop_layout_iterate( this, blk );
}


bool_t MemOopDescriptor::verify() {
    bool_t flag = true;
    if ( flag ) {
        MarkOop m = mark();
        if ( not Oop( m )->is_mark() ) {
            error( "mark of MemOop %#lx isn't a markOop", this );
            if ( not m->verify() )
                error( " mark of MemOop %#lx isn't even a legal Oop", this );
            flag = false;
        }
        KlassOop p = klass();
        if ( not p->is_klass() ) {
            error( "map of MemOop %#lx isn't a klassOop", this );
            flag = false;
        }
    }
    return flag;
}


void MemOopDescriptor::set_identity_hash( smi_t h ) {
    set_mark( mark()->set_hash( h ) );
}


void MemOopDescriptor::bootstrap_header( Bootstrap *stream ) {
    if ( stream->new_format() ) {
        stream->read_oop( (Oop *) &addr()->_klass_field );
        set_mark( blueprint()->has_untagged_contents() ? MarkOopDescriptor::untagged_prototype() : MarkOopDescriptor::tagged_prototype() );
    } else {
        stream->read_mark( &addr()->_mark );
        stream->read_oop( (Oop *) &addr()->_klass_field );
    }
}


void MemOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    bootstrap_header( stream );
    bootstrap_body( stream, header_size() );
}


void MemOopDescriptor::bootstrap_body( Bootstrap *stream, int h_size ) {
    int offset = h_size;
    int s      = blueprint()->non_indexable_size();
    while ( offset < s ) {
        stream->read_oop( (Oop *) addr() + offset );
        offset++;
    }
}


bool_t MemOopDescriptor::is_within_instVar_bounds( int index ) {
    return index >= blueprint()->oop_header_size() and index < blueprint()->non_indexable_size();
}


Oop MemOopDescriptor::instVarAt( int index ) {
    return raw_at( index );
}


Oop MemOopDescriptor::instVarAtPut( int index, Oop value ) {
    raw_at_put( index, value );
    return this;
}


void MemOopDescriptor::print_on( ConsoleOutputStream *stream ) {
    blueprint()->oop_print_on( this, stream );
}


void MemOopDescriptor::print_id_on( ConsoleOutputStream *stream ) {
    int id;
    if ( garbageCollectionInProgress or not( id = objectIDTable::insert( MemOop( this ) ) ) )
        stream->print( "(%#-6lx)", addr() );
    else
        stream->print( "%d", id );
}


smi_t MemOopDescriptor::identity_hash() {
    // don't clean up the addr()->_mark below to mark(), since hash_markOop can modify its argument
    return hash_markOop( addr()->_mark );
}


void MemOopDescriptor::scavenge_header() {
    // Not needed since klas is in old Space
    //   scavenge_oop(klass_addr());
    // this may not be correct if running with the recompiler. Investigate. slr 29/09/2008
}


void MemOopDescriptor::scavenge_body( int begin, int end ) {
    Oop *p = (Oop *) addr();
    Oop *q = p + end;
    p += begin;
    while ( p < q )
        scavenge_oop( p++ );
}


void MemOopDescriptor::scavenge_tenured_body( int begin, int end ) {
    Oop *p = (Oop *) addr();
    Oop *q = p + end;
    p += begin;
    while ( p < q )
        scavenge_tenured_oop( p++ );
}


void MemOopDescriptor::follow_header() {
    MarkSweep::reverse_and_push( klass_addr() );
}


void MemOopDescriptor::follow_body( int begin, int end ) {
    Oop *p = (Oop *) addr();
    Oop *q = p + end;
    p += begin;
    while ( p < q )
        MarkSweep::reverse_and_push( p++ );
}


void MemOopDescriptor::layout_iterate_header( ObjectLayoutClosure *blk ) {
    blk->do_mark( &addr()->_mark );
    blk->do_oop( "klass", (Oop *) &addr()->_klass_field );
}


void MemOopDescriptor::oop_iterate_header( OopClosure *blk ) {
    blk->do_oop( (Oop *) &addr()->_klass_field );
}


void MemOopDescriptor::oop_iterate_body( OopClosure *blk, int begin, int end ) {
    Oop *p = (Oop *) addr();
    Oop *q = p + end;
    p += begin;
    while ( p < q )
        blk->do_oop( p++ );
}


void MemOopDescriptor::initialize_header( bool_t has_untagged_contents, KlassOop klass ) {
    set_klass_field( klass );
    if ( has_untagged_contents )
        init_untagged_contents_mark();
    else
        init_mark();
}


void MemOopDescriptor::initialize_body( int begin, int end ) {
    Oop value = nilObj;
    Oop *p = (Oop *) addr();
    Oop *q = p + end;
    p += begin;
    while ( p < q )
        Universe::store( p++, value, false );
}


void MemOopDescriptor::raw_at_put( int which, Oop contents, bool_t cs ) {
    Universe::store( oops( which ), contents, cs );
}


std::size_t MemOopDescriptor::size() const {
    return blueprint()->oop_size( MemOop( this ) );
}


int MemOopDescriptor::scavenge_tenured_contents() {
    return blueprint()->oop_scavenge_tenured_contents( this );
}


// Store object size in age field and remembered set
void MemOopDescriptor::gc_store_size() {
    int s = size();
    if ( s < MarkOopDescriptor::max_age ) {
        // store size in age field
        set_mark( mark()->set_age( s ) );
        st_assert( mark()->age() == s, "Check size" );
    } else {
        // store size in remembered set
        // first set overflow value in age field
        set_mark( mark()->set_age( 0 ) );
        st_assert( mark()->age() == 0, "Check size" );

        Universe::remembered_set->set_size( this, s );
        st_assert( Universe::remembered_set->get_size( this ) == s, "Check size" );
    }
}


// Retrieve object size from age field and remembered set
int MemOopDescriptor::gc_retrieve_size() {
    if ( mark()->age() == 0 )
        return Universe::remembered_set->get_size( this );
    return mark()->age();
}
