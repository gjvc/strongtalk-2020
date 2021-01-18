//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"


bool_t ObjectArrayOopDescriptor::verify() {
    bool_t flag = MemOopDescriptor::verify();
    if ( flag ) {
        int l = length();
        if ( l < 0 ) {
            error( "objArrayOop %#lx has negative length", this );
            flag = false;
        }
    }
    return flag;
}


void ObjectArrayOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    MemOopDescriptor::bootstrap_object( stream );
    stream->read_oop( length_addr() );
    for ( int i = 1; i <= length(); i++ )
        stream->read_oop( objs( i ) );
}


ObjectArrayOop ObjectArrayOopDescriptor::copy_remove( int index, int number ) {
    ObjectArrayOop new_array = oopFactory::new_objArray( length() - number );
    // copy [1..i-1]
    for ( int      i         = 1; i < index; i++ )
        new_array->obj_at_put( i, obj_at( i ) );

    // copy  [index+number..length]
    for ( int i = index; i <= length() - number; i++ )
        new_array->obj_at_put( i, obj_at( i + number ) );

    return new_array;
}


ObjectArrayOop ObjectArrayOopDescriptor::copy() {
    ObjectArrayOop new_array = oopFactory::new_objArray( length() );

    for ( int i = 1; i <= length(); i++ )
        new_array->obj_at_put( i, obj_at( i ) );

    return new_array;
}


ObjectArrayOop ObjectArrayOopDescriptor::copy_add( Oop a ) {
    ObjectArrayOop new_array = oopFactory::new_objArray( length() + 1 );

    int i = 1;
    for ( ; i <= length(); i++ )
        new_array->obj_at_put( i, obj_at( i ) );
    new_array->obj_at_put( i, a );
    return new_array;
}


ObjectArrayOop ObjectArrayOopDescriptor::copy_add_two( Oop a, Oop b ) {
    ObjectArrayOop new_array = oopFactory::new_objArray( length() + 2 );
    int            i         = 1;
    for ( ; i < length(); i++ )
        new_array->obj_at_put( i, obj_at( i ) );
    new_array->obj_at_put( i++, a );
    new_array->obj_at_put( i, b );
    return new_array;
}

// Old Smalltalk code:
//  replaceFrom: start <Int> to: stop <Int> with: other <SeqCltn[E]> startingAt: repStart <Int>
//	"replace the elements of the receiver from start to stop with elements from other,
//	  starting with the element of other with index repStart."
//
//	| otheri <Int> |
//	repStart < start
//		ifFalse: [ otheri := repStart.
//				  start to: stop do:
//					[ :i <Int> |
//						self at: i put: (other at: otheri).
//						otheri := otheri + 1.	]]
//		ifTrue: [ otheri := repStart + (stop - start).
//				stop to: start by: -1 do:
//					[ :i <Int> |
//						self at: i put: (other at: otheri).
//						otheri := otheri - 1.	]]

void ObjectArrayOopDescriptor::replace_from_to( int from, int to, ObjectArrayOop source, int start ) {
    int other_index = start;
    if ( start < to ) {
        // copy up
        for ( int i = from; i <= to; i++ ) {
            source->obj_at_put( other_index++, obj_at( i ) );
        }
    } else {
        // copy down
        for ( int i = to; i >= from; i-- ) {
            source->obj_at_put( other_index--, obj_at( i ) );
        }
    }
}


void ObjectArrayOopDescriptor::replace_and_fill( int from, int start, ObjectArrayOop source ) {
    // Fill the first part
    for ( int i = 1; i < from; i++ ) {
        obj_at_put( i, nilObj );
    }

    // Fill the middle part
    int to = min( source->length() - start + 1, length() );

    for ( int i = from; i <= to; i++ ) {
        obj_at_put( i, source->obj_at( start + i - from ) );
    }

    // Fill the last part
    for ( int i = to + 1; i <= length(); i++ ) {
        obj_at_put( i, nilObj );
    }
}


void WeakArrayOopDescriptor::scavenge_contents_after_registration() {
    Oop *p = objs( 1 );
    int len = length();

    for ( int i = 1; i <= len; i++ )
        scavenge_tenured_oop( p++ );
}


void WeakArrayOopDescriptor::follow_contents_after_registration() {
    Oop *p = objs( 1 );
    int len = length();

    for ( int i = 1; i <= len; i++ )
        scavenge_tenured_oop( p++ );
}
