//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/HeapCodeBuffer.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/system/sizes.hpp"



void HeapCodeBuffer::align() {
    while ( not isAligned() )
        _bytes->append( 0xFF );
}


bool_t HeapCodeBuffer::isAligned() {
    return ( _bytes->length() % oopSize ) == 0;
}


void HeapCodeBuffer::pushByte( uint8_t op ) {
    if ( isAligned() )
        _oops->append( smiOopFromValue( 0 ) );
    _bytes->append( op );
}



void HeapCodeBuffer::pushOop( Oop arg ) {
    align();
    _bytes->append( 0 );
    _bytes->append( 0 );
    _bytes->append( 0 );
    _bytes->append( 0 );

    _oops->append( arg );
}


ByteArrayOop HeapCodeBuffer::bytes() {
    BlockScavenge bs;
    align();
    Klass        * klass = Universe::byteArrayKlassObj()->klass_part();
    ByteArrayOop result = ByteArrayOop( klass->allocateObjectSize( byteLength() ) );

    for ( int i = 0; i < byteLength(); i++ )
        result->byte_at_put( i + 1, ( uint8_t ) _bytes->at( i ) );

    return result;
}


ObjectArrayOop HeapCodeBuffer::oops() {
    BlockScavenge bs;
    Klass         * klass = Universe::objArrayKlassObj()->klass_part();
    ObjectArrayOop result = ObjectArrayOop( klass->allocateObjectSize( oopLength() ) );

    for ( int i = 0; i < oopLength(); i++ )
        result->obj_at_put( i + 1, _oops->at( i ) );

    return result;
}
