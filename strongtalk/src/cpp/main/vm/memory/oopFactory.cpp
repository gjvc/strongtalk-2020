//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "allocation.hpp"
#include "vm/memory/oopFactory.hpp"
//#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/VirtualFrameKlass.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/oops/AssociationKlass.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/DoubleKlass.hpp"
#include "vm/oops/ByteArrayKlass.hpp"
#include "vm/oops/ObjectArrayKlass.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/SymbolTable.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/system/sizes.hpp"


ByteArrayOop oopFactory::new_byteArray( int size ) {
    ByteArrayKlass *bk = (ByteArrayKlass *) Universe::byteArrayKlassObj()->klass_part();
    return ByteArrayOop( bk->allocateObjectSize( size ) );
}


ByteArrayOop oopFactory::new_byteArray( const char *name ) {
    int          len    = strlen( name );
    ByteArrayOop result = new_byteArray( len );

    for ( int index = 0; index < len; index++ ) {
        result->byte_at_put( index + 1, name[ index ] );
    }
    return result;
}


ObjectArrayOop oopFactory::new_objArray( int size ) {
    ObjectArrayKlass *ok = (ObjectArrayKlass *) Universe::objArrayKlassObj()->klass_part();
    ObjectArrayOop result = ObjectArrayOop( ok->allocateObjectSize( size ) );
    result->set_length( size );
    return result;
}


ObjectArrayOop oopFactory::new_objArray( GrowableArray<Oop> *array ) {
    BlockScavenge bs;
    FlagSetting( processSemaphore, true );
    int size = array->length();
    ObjectArrayKlass *ok = (ObjectArrayKlass *) Universe::objArrayKlassObj()->klass_part();

    ObjectArrayOop result = ObjectArrayOop( ok->allocateObjectSize( size ) );

    for ( int index = 1; index <= size; index++ ) {
        result->obj_at_put( index, array->at( index - 1 ) );
    }
    return result;
}


DoubleOop oopFactory::new_double( double value ) {
    DoubleOop d = as_doubleOop( Universe::allocate( sizeof( DoubleOopDescriptor ) / oopSize ) );
    d->init_untagged_contents_mark();
    d->set_klass_field( doubleKlassObj );
    d->set_value( value );
    return d;
}


DoubleOop oopFactory::clone_double_to_oldspace( DoubleOop value ) {
    DoubleOop d = as_doubleOop( Universe::allocate_tenured( sizeof( DoubleOopDescriptor ) / oopSize ) );
    d->init_untagged_contents_mark();
    d->set_klass_field( doubleKlassObj );
    d->set_value( value->value() );
    return d;
}


SymbolOop oopFactory::new_symbol( const char *name, int len ) {
    return Universe::symbol_table->lookup( name, len );
}


SymbolOop oopFactory::new_symbol( const char *name ) {
    return new_symbol( name, strlen( name ) );
}


SymbolOop oopFactory::new_symbol( ByteArrayOop b ) {
    return new_symbol( const_cast<char *>( b->chars()), b->length() );
}


AssociationOop oopFactory::new_association( SymbolOop key, Oop value, bool_t is_constant ) {
    AssociationOop as = AssociationOop( Universe::associationKlassObj()->klass_part()->allocateObject() );
    st_assert( as->is_association(), "type check" );
    as->set_key( key );
    as->set_value( value );
    as->set_is_constant( is_constant );
    return as;
}


VirtualFrameOop oopFactory::new_vframe( ProcessOop process, int index ) {
    BlockScavenge bs;
    VirtualFrameKlass *vk = (VirtualFrameKlass *) Universe::vframeKlassObj()->klass_part();

    VirtualFrameOop result = VirtualFrameOop( vk->allocateObject() );

    result->set_process( process );
    result->set_index( index );
    result->set_time_stamp( process->process()->time_stamp() );

    return result;
}


SMIOop oopFactory::new_smi( int value ) {
    return smiOopFromValue( value );
}
