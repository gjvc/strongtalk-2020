//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/DoubleByteArrayKlass.hpp"
#include "vm/oops/DoubleByteArrayOopDescriptor.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


Oop DoubleByteArrayKlass::allocateObject( bool permit_scavenge, bool tenured ) {
    static_cast<void>(permit_scavenge); // unused
    static_cast<void>(tenured); // unused

    st_fatal( "should never call allocateObject in doubleByteArrayKlass" );
    return badOop;
}


Oop DoubleByteArrayKlass::allocateObjectSize( std::int32_t size, bool permit_scavenge, bool tenured ) {
    KlassOop     k        = as_klassOop();
    std::int32_t ni_size  = non_indexable_size();
    std::int32_t obj_size = ni_size + 1 + roundTo( size * 2, OOP_SIZE ) / OOP_SIZE;

    // allocate
    Oop *result = tenured ? Universe::allocate_tenured( obj_size, permit_scavenge ) : Universe::allocate( obj_size, (MemOop *) &k, permit_scavenge );
    if ( result == nullptr )
        return nullptr;

    DoubleByteArrayOop obj = as_doubleByteArrayOop( result );

    // header
    MemOop( obj )->initialize_header( true, k );

    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );

    // indexables
    Oop *base = (Oop *) obj->addr();
    Oop *end  = base + obj_size;
    // %optimized 'obj->set_signed_length(size)'
    base[ ni_size ] = smiOopFromValue( size );
    // %optimized 'for (std::int32_t index = 1; index <= size; index++)
    //               obj->doubleByte_at_put(index, 0)'
    base = &base[ ni_size + 1 ];
    while ( base < end )
        *base++ = (Oop) 0;

    return obj;
}


KlassOop DoubleByteArrayKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::doubleByteArray_klass ) {
        return DoubleByteArrayKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop DoubleByteArrayKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    DoubleByteArrayKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


void setKlassVirtualTableFromDoubleByteArrayKlass( Klass *k ) {
    DoubleByteArrayKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


bool DoubleByteArrayKlass::oop_verify( Oop obj ) {
    st_assert_doubleByteArray( obj, "Argument must be doubleByteArray" );
    return DoubleByteArrayOop( obj )->verify();
}


void DoubleByteArrayKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    st_assert_doubleByteArray( obj, "Argument must be doubleByteArray" );
    DoubleByteArrayOop array = DoubleByteArrayOop( obj );
    std::int32_t       len   = array->length();
    std::int32_t       n     = min( MaxElementPrintSize, len );
    stream->print( "'" );
    for ( std::int32_t i = 1; i <= n; i++ ) {
        std::int32_t c = array->doubleByte_at( i );
        if ( isprint( c ) )
            stream->print( "%c", c );
        else
            stream->print( "\\%o", c );
    }

    if ( n < len )
        stream->print( "..." );

    stream->print( "'" );
}


void DoubleByteArrayKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    std::uint16_t *p  = DoubleByteArrayOop( obj )->doubleBytes();
    Oop           *l  = DoubleByteArrayOop( obj )->length_addr();
    std::int32_t  len = DoubleByteArrayOop( obj )->length();
    MemOopKlass::oop_layout_iterate( obj, blk );
    blk->do_oop( "length", l );
    blk->begin_indexables();
    for ( std::int32_t i = 1; i <= len; i++ ) {
        blk->do_indexable_doubleByte( i, p++ );
    }
    blk->end_indexables();
}


void DoubleByteArrayKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    Oop *l = DoubleByteArrayOop( obj )->length_addr();
    MemOopKlass::oop_oop_iterate( obj, blk );
    blk->do_oop( l );
}


std::int32_t DoubleByteArrayKlass::oop_scavenge_contents( Oop obj ) {
    MemOopKlass::oop_scavenge_contents( obj );
    return object_size( DoubleByteArrayOop( obj )->length() );
}


std::int32_t DoubleByteArrayKlass::oop_scavenge_tenured_contents( Oop obj ) {
    MemOopKlass::oop_scavenge_tenured_contents( obj );
    return object_size( DoubleByteArrayOop( obj )->length() );
}
