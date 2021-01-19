//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/oops/WeakArrayKlass.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/oops/MemOopKlass.hpp"


KlassOop WeakArrayKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::weakArray_klass ) {
        return WeakArrayKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop WeakArrayKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    WeakArrayKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


void setKlassVirtualTableFromWeakArrayKlass( Klass *k ) {
    WeakArrayKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


void WeakArrayKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    // Retrieve length information in case the iterator mutates the object
    Oop *p = ObjectArrayOop( obj )->objs( 0 );
    int len = ObjectArrayOop( obj )->length();
    // header + instance variables
    MemOopKlass::oop_layout_iterate( obj, blk );
    // indexables
    blk->do_oop( "length", p++ );
    blk->begin_indexables();
    for ( std::size_t i = 1; i <= len; i++ ) {
        blk->do_indexable_oop( i, p++ );
    }
    blk->end_indexables();
}


void WeakArrayKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    // Retrieve length information in case the iterator mutates the object
    Oop *p = WeakArrayOop( obj )->objs( 0 );
    int len = WeakArrayOop( obj )->length();
    // header + instance variables
    MemOopKlass::oop_oop_iterate( obj, blk );
    // indexables
    blk->do_oop( p++ );
    for ( std::size_t i = 1; i <= len; i++ ) {
        blk->do_oop( p++ );
    }
}


int WeakArrayKlass::oop_scavenge_contents( Oop obj ) {
    // header + instance variables
    MemOopKlass::oop_scavenge_contents( obj );
    // indexables
    WeakArrayOop o = WeakArrayOop( obj );
    if ( not WeakArrayRegister::scavenge_register( o ) ) {
        Oop *base = o->objs( 1 );
        Oop *end  = base + o->length();
        while ( base <= end ) {
            scavenge_oop( base++ );
        }
    }
    return object_size( o->length() );
}


int WeakArrayKlass::oop_scavenge_tenured_contents( Oop obj ) {
    // header + instance variables
    MemOopKlass::oop_scavenge_tenured_contents( obj );
    // indexables
    WeakArrayOop o = WeakArrayOop( obj );
    if ( not WeakArrayRegister::scavenge_register( o ) ) {
        Oop *base = o->objs( 1 );
        Oop *end  = base + o->length();
        while ( base <= end )
            scavenge_tenured_oop( base++ );
    }
    return object_size( o->length() );
}


void WeakArrayKlass::oop_follow_contents( Oop obj ) {
    // indexables
    if ( not WeakArrayRegister::mark_sweep_register( WeakArrayOop( obj ), non_indexable_size() ) ) {
        Oop *base = WeakArrayOop( obj )->objs( 1 );
        Oop *end  = base + WeakArrayOop( obj )->length();
        while ( base <= end )
            MarkSweep::reverse_and_follow( base++ );
    }

    // header + instance variables
    MemOopKlass::oop_follow_contents( obj );
}


// WeakArrayRegister
// - static variables
bool_t                       WeakArrayRegister::during_registration = false;
GrowableArray<WeakArrayOop> *WeakArrayRegister::weakArrays = nullptr;
GrowableArray<int>          *WeakArrayRegister::nis        = nullptr;


// - Scavenge operations
void WeakArrayRegister::begin_scavenge() {
    during_registration = true;
    weakArrays          = new GrowableArray<WeakArrayOop>( 10 );
}


bool_t WeakArrayRegister::scavenge_register( WeakArrayOop obj ) {
    if ( during_registration )
        weakArrays->push( obj );
    return during_registration;
}


void WeakArrayRegister::check_and_scavenge_contents() {
    scavenge_check_for_dying_objects();
    scavenge_contents();
    during_registration = false;
    weakArrays          = nullptr;
}


void WeakArrayRegister::scavenge_contents() {
    for ( std::size_t i = 0; i < weakArrays->length(); i++ )
        weakArrays->at( i )->scavenge_contents_after_registration();
}


bool_t WeakArrayRegister::scavenge_is_near_death( Oop obj ) {
    // must be MemOop and unmarked (no forward pointer)
    return obj->is_new() and not MemOop( obj )->is_forwarded();
}


void WeakArrayRegister::scavenge_check_for_dying_objects() {

    NotificationQueue::mark_elements();
    for ( std::size_t i = 0; i < weakArrays->length(); i++ ) {
        WeakArrayOop w                            = weakArrays->at( i );
        bool_t       encounted_near_death_objects = false;

        for ( std::size_t j = 1; j <= w->length(); j++ ) {
            Oop obj = w->obj_at( j );
            if ( scavenge_is_near_death( obj ) ) {
                encounted_near_death_objects = true;
                MemOop( obj )->mark_as_dying();
            }
        }
        if ( encounted_near_death_objects and not w->is_queued() )
            NotificationQueue::put( w );
    }

    NotificationQueue::clear_elements();
}

// - Mark sweep operations

void WeakArrayRegister::begin_mark_sweep() {
    during_registration = true;
    weakArrays          = new GrowableArray<WeakArrayOop>( 100 );
    nis                 = new GrowableArray<int>( 100 );
}


bool_t WeakArrayRegister::mark_sweep_register( WeakArrayOop obj, int non_indexable_size ) {
    if ( during_registration ) {
        weakArrays->push( obj );
        nis->push( non_indexable_size );
    }
    return during_registration;
}


void WeakArrayRegister::check_and_follow_contents() {
    mark_sweep_check_for_dying_objects();
    follow_contents();
    during_registration = false;
    weakArrays          = nullptr;
    nis                 = nullptr;
}


void WeakArrayRegister::follow_contents() {

    for ( std::size_t i = 0; i < weakArrays->length(); i++ ) {

        WeakArrayOop w                            = weakArrays->at( i );
        int          non_indexable_size           = nis->at( i );
        bool_t       encounted_near_death_objects = false;
        int          length                       = SMIOop( w->raw_at( non_indexable_size ) )->value();

        for ( std::size_t j = 1; j <= length; j++ ) {
            MarkSweep::reverse_and_follow( w->oops( non_indexable_size + j ) );
        }
    }

}


bool_t WeakArrayRegister::mark_sweep_is_near_death( Oop obj ) {
    return obj->is_mem() and not MemOop( obj )->is_gc_marked();
}


void WeakArrayRegister::mark_sweep_check_for_dying_objects() {

    for ( std::size_t i = 0; i < weakArrays->length(); i++ ) {

        WeakArrayOop w                            = weakArrays->at( i );
        int          non_indexable_size           = nis->at( i );
        bool_t       encounted_near_death_objects = false;
        int          length                       = SMIOop( w->raw_at( non_indexable_size ) )->value();

        for ( std::size_t j = 1; j <= length; j++ ) {
            Oop obj = w->raw_at( non_indexable_size + j );
            if ( mark_sweep_is_near_death( obj ) ) {
                encounted_near_death_objects = true;
                MemOop( obj )->mark_as_dying();
            }
        }

        if ( encounted_near_death_objects )
            NotificationQueue::put_if_absent( w );
    }
}

// NotificationQueue

Oop *NotificationQueue::array = nullptr;
std::size_t  NotificationQueue::size  = 100;
int  NotificationQueue::first = 0;
int  NotificationQueue::last  = 0;


bool_t NotificationQueue::is_empty() {
    return first == last;
}


int NotificationQueue::succ( int index ) {
    return ( index + 1 ) % size;
}


Oop NotificationQueue::get() {
    st_assert( not is_empty(), "must contain elements" );
    Oop result = array[ first ];
    first = succ( first );
    return result;
}


void NotificationQueue::put( Oop obj ) {
    if ( array == nullptr )
        array = new_c_heap_array<Oop>( size );
    if ( succ( last ) == first ) {
        int new_size = size * 2;
        int new_last = 0;
        // allocate new_array
        Oop *new_array = new_c_heap_array<Oop>( new_size );
        // copy from array to new_array
        for ( std::size_t i = first; i not_eq last; i = succ( i ) )
            new_array[ new_last++ ] = array[ i ];
        free( array );
        // replace array
        array = new_array;
        size  = new_size;
        first = 0;
        last  = new_last;
    }
    array[ last ] = obj;
    last = succ( last );
}


void NotificationQueue::put_if_absent( Oop obj ) {
    for ( std::size_t i = first; i not_eq last; i = succ( i ) ) {
        if ( array[ i ] == obj )
            return;
    }
    put( obj );
}


void NotificationQueue::oops_do( void f( Oop * ) ) {
    for ( std::size_t i = first; i not_eq last; i = succ( i ) )
        f( &array[ i ] );
}


void NotificationQueue::mark_elements() {
    for ( std::size_t i = first; i not_eq last; i = succ( i ) ) {
        Oop obj = array[ i ];
        if ( obj->is_mem() )
            MemOop( obj )->set_queued();
    }
}


void NotificationQueue::clear_elements() {
    for ( std::size_t i = first; i not_eq last; i = succ( i ) ) {
        Oop obj = array[ i ];
        if ( obj->is_mem() )
            MemOop( obj )->clear_queued();
    }
}
