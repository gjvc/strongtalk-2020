//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ByteArrayKlass.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/system/sizes.hpp"


Oop ByteArrayKlass::allocateObject( bool_t permit_scavenge, bool_t tenured ) {
    st_assert( not can_inline_allocation(), "using nonstandard allocation" );

    // This should not be fatal!
    // This could be called erroneously from ST in which case it should result in an ST level error.
    // Instead return a marked symbol to indicate the failure.
    //fatal("should never call allocateObject in byteArrayKlass");
    return markSymbol( vmSymbols::invalid_klass() );
}


Oop ByteArrayKlass::allocateObjectSize( int size, bool_t permit_scavenge, bool_t permit_tenured ) {
    KlassOop k        = as_klassOop();
    int      ni_size  = non_indexable_size();
    int      obj_size = ni_size + 1 + roundTo( size, oopSize ) / oopSize;
    // allocate
    Oop * result = permit_tenured ? Universe::allocate_tenured( obj_size, false ) : Universe::allocate( obj_size, ( MemOop * ) &k, permit_scavenge );

    if ( not result )
        return nullptr;

    ByteArrayOop obj = as_byteArrayOop( result );
    // header
    MemOop( obj )->initialize_header( true, k );
    // instance variables
    MemOop( obj )->initialize_body( MemOopDescriptor::header_size(), ni_size );
    // indexables
    Oop * base = ( Oop * ) obj->addr();
    Oop * end  = base + obj_size;
    // %optimized 'obj->set_length(size)'
    base[ ni_size ] = smiOopFromValue( size );
    // %optimized 'for (int index = 1; index <= size; index++)
    //               obj->byte_at_put(index, '\000')'
    base = &base[ ni_size + 1 ];
    while ( base < end )
        *base++ = ( Oop ) 0;
    return obj;
}


KlassOop ByteArrayKlass::create_subclass( MixinOop mixin, Format format ) {
    if ( format == Format::mem_klass or format == Format::byteArray_klass ) {
        return ByteArrayKlass::create_class( as_klassOop(), mixin );
    }
    return nullptr;
}


KlassOop ByteArrayKlass::create_class( KlassOop super_class, MixinOop mixin ) {
    ByteArrayKlass o;
    return create_generic_class( super_class, mixin, o.vtbl_value() );
}


void ByteArrayKlass::initialize_object( ByteArrayOop obj, const char * value, int len ) {
    for ( int i = 1; i <= len; i++ ) {
        obj->byte_at_put( i, value[ i - 1 ] );
    }
}


void setKlassVirtualTableFromByteArrayKlass( Klass * k ) {
    ByteArrayKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


bool_t ByteArrayKlass::oop_verify( Oop obj ) {
    st_assert_byteArray( obj, "Argument must be byteArray" );
    return ByteArrayOop( obj )->verify();
}


void ByteArrayKlass::oop_print_value_on( Oop obj, ConsoleOutputStream * stream ) {
    st_assert_byteArray( obj, "Argument must be byteArray" );
    ByteArrayOop array = ByteArrayOop( obj );
    int          len   = array->length();
    int          n     = min( MaxElementPrintSize, len );
    stream->print( "'" );
    for ( int i = 1; i <= n; i++ ) {
        char c = array->byte_at( i );
        if ( isprint( c ) )
            stream->print( "%c", c );
        else
            stream->print( "\\%o", c );
    }
    if ( n < len )
        stream->print( "..." );
    stream->print( "'" );
}


void ByteArrayKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure * blk ) {
    std::uint8_t * p = ByteArrayOop( obj )->bytes();
    Oop     * l = ByteArrayOop( obj )->length_addr();
    int len = ByteArrayOop( obj )->length();
    // header + instance variables
    MemOopKlass::oop_layout_iterate( obj, blk );
    // indexables
    blk->begin_indexables();
    blk->do_oop( "length", l );
    for ( int i = 1; i <= len; i++ ) {
        blk->do_indexable_byte( i, p++ );
    }
    blk->end_indexables();
}


void ByteArrayKlass::oop_oop_iterate( Oop obj, OopClosure * blk ) {
    Oop * l = ByteArrayOop( obj )->length_addr();
    // header + instance variables
    MemOopKlass::oop_oop_iterate( obj, blk );
    blk->do_oop( l );
}


int ByteArrayKlass::oop_scavenge_contents( Oop obj ) {
    // header + instance variables
    MemOopKlass::oop_scavenge_contents( obj );
    return object_size( ByteArrayOop( obj )->length() );
}


int ByteArrayKlass::oop_scavenge_tenured_contents( Oop obj ) {
    // header + instance variables
    MemOopKlass::oop_scavenge_tenured_contents( obj );
    return object_size( ByteArrayOop( obj )->length() );
}
