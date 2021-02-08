//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"


bool ObjectArrayOopDescriptor::verify() {
    bool flag = MemOopDescriptor::verify();
    if ( flag ) {
        std::int32_t l = length();
        if ( l < 0 ) {
            error( "objArrayOop 0x{0:x} has negative length", this );
            flag = false;
        }
    }
    return flag;
}


void ObjectArrayOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    SPDLOG_INFO( "ObjectArrayOopDescriptor::bootstrap_object" );
    MemOopDescriptor::bootstrap_object( stream );
    stream->read_oop( length_addr() );
    for ( std::int32_t i = 1; i <= length(); i++ ) {
        stream->read_oop( objs( i ) );
    }
}


ObjectArrayOop ObjectArrayOopDescriptor::copy_remove( std::int32_t index, std::int32_t number ) {

    ObjectArrayOop new_array = oopFactory::new_objArray( length() - number );
    // copy [1..i-1]

    for ( std::int32_t i = 1; i < index; i++ )
        new_array->obj_at_put( i, obj_at( i ) );

    // copy  [index+number..length]
    for ( std::int32_t i = index; i <= length() - number; i++ )
        new_array->obj_at_put( i, obj_at( i + number ) );

    return new_array;
}


ObjectArrayOop ObjectArrayOopDescriptor::copy() {
    ObjectArrayOop new_array = oopFactory::new_objArray( length() );

    for ( std::int32_t i = 1; i <= length(); i++ )
        new_array->obj_at_put( i, obj_at( i ) );

    return new_array;
}


ObjectArrayOop ObjectArrayOopDescriptor::copy_add( Oop a ) {
    ObjectArrayOop new_array = oopFactory::new_objArray( length() + 1 );

    std::int32_t i = 1;
    for ( ; i <= length(); i++ )
        new_array->obj_at_put( i, obj_at( i ) );

    new_array->obj_at_put( i, a );
    return new_array;
}


ObjectArrayOop ObjectArrayOopDescriptor::copy_add_two( Oop a, Oop b ) {

    ObjectArrayOop new_array = oopFactory::new_objArray( length() + 2 );

    std::int32_t i = 1;

    for ( ; i < length(); i++ )
        new_array->obj_at_put( i, obj_at( i ) );
    new_array->obj_at_put( i++, a );
    new_array->obj_at_put( i, b );
    return new_array;
}

// Old Smalltalk code:
//  replaceFrom: start <std::int32_t> to: stop <std::int32_t> with: other <SeqCltn[E]> startingAt: repStart <std::int32_t>
//	"replace the elements of the receiver from start to stop with elements from other,
//	  starting with the element of other with index repStart."
//
//	| otheri <std::int32_t> |
//	repStart < start
//		ifFalse: [ otheri := repStart.
//				  start to: stop do:
//					[ :i <std::int32_t> |
//						self at: i put: (other at: otheri).
//						otheri := otheri + 1.	]]
//		ifTrue: [ otheri := repStart + (stop - start).
//				stop to: start by: -1 do:
//					[ :i <std::int32_t> |
//						self at: i put: (other at: otheri).
//						otheri := otheri - 1.	]]


void ObjectArrayOopDescriptor::replace_from_to( std::int32_t from, std::int32_t to, ObjectArrayOop source, std::int32_t start ) {
    std::int32_t other_index = start;
    if ( start < to ) {
        // copy up
        for ( std::int32_t i = from; i <= to; i++ ) {
            source->obj_at_put( other_index++, obj_at( i ) );
        }
    } else {
        // copy down
        for ( std::int32_t i = to; i >= from; i-- ) {
            source->obj_at_put( other_index--, obj_at( i ) );
        }
    }
}


void ObjectArrayOopDescriptor::replace_and_fill( std::int32_t from, std::int32_t start, ObjectArrayOop source ) {
    // Fill the first part
    for ( std::int32_t i = 1; i < from; i++ ) {
        obj_at_put( i, nilObject );
    }

    // Fill the middle part
    std::int32_t to = min( source->length() - start + 1, length() );

    for ( std::int32_t i = from; i <= to; i++ ) {
        obj_at_put( i, source->obj_at( start + i - from ) );
    }

    // Fill the last part
    for ( std::int32_t i = to + 1; i <= length(); i++ ) {
        obj_at_put( i, nilObject );
    }
}


void WeakArrayOopDescriptor::scavenge_contents_after_registration() {
    Oop          *p  = objs( 1 );
    std::int32_t len = length();

    for ( std::int32_t i = 1; i <= len; i++ )
        scavenge_tenured_oop( p++ );
}


void WeakArrayOopDescriptor::follow_contents_after_registration() {
    Oop          *p  = objs( 1 );
    std::int32_t len = length();

    for ( std::int32_t i = 1; i <= len; i++ )
        scavenge_tenured_oop( p++ );
}
