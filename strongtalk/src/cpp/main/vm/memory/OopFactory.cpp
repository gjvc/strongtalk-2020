//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/klass/VirtualFrameKlass.hpp"
#include "vm/oop/AssociationOopDescriptor.hpp"
#include "vm/klass/AssociationKlass.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/klass/DoubleKlass.hpp"
#include "vm/klass/ByteArrayKlass.hpp"
#include "vm/klass/ObjectArrayKlass.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/SymbolTable.hpp"
#include "vm/oop/ProcessOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"


ByteArrayOop OopFactory::new_byteArray( std::int32_t size ) {
    ByteArrayKlass *bk = (ByteArrayKlass *) Universe::byteArrayKlassObject()->klass_part();
    return ByteArrayOop( bk->allocateObjectSize( size ) );
}


ByteArrayOop OopFactory::new_byteArray( const char *name ) {
    std::int32_t len    = strlen( name );
    ByteArrayOop result = new_byteArray( len );

    for ( std::size_t index = 0; index < len; index++ ) {
        result->byte_at_put( index + 1, name[ index ] );
    }
    return result;
}


ObjectArrayOop OopFactory::new_objectArray( std::int32_t size ) {
    ObjectArrayKlass *ok    = (ObjectArrayKlass *) Universe::objectArrayKlassObject()->klass_part();
    ObjectArrayOop   result = ObjectArrayOop( ok->allocateObjectSize( size ) );
    result->set_length( size );
    return result;
}


ObjectArrayOop OopFactory::new_objectArray( GrowableArray<Oop> *array ) {
    BlockScavenge bs;
    FlagSetting( processSemaphore, true );
    std::int32_t     size = array->length();
    ObjectArrayKlass *ok  = (ObjectArrayKlass *) Universe::objectArrayKlassObject()->klass_part();

    ObjectArrayOop result = ObjectArrayOop( ok->allocateObjectSize( size ) );

    for ( std::int32_t index = 1; index <= size; index++ ) {
        result->obj_at_put( index, array->at( index - 1 ) );
    }
    return result;
}


DoubleOop OopFactory::new_double( double value ) {
    DoubleOop d = as_doubleOop( Universe::allocate( sizeof( DoubleOopDescriptor ) / OOP_SIZE ) );
    d->init_untagged_contents_mark();
    d->set_klass_field( doubleKlassObject );
    d->set_value( value );
    return d;
}


DoubleOop OopFactory::clone_double_to_oldspace( DoubleOop value ) {
    DoubleOop d = as_doubleOop( Universe::allocate_tenured( sizeof( DoubleOopDescriptor ) / OOP_SIZE ) );
    d->init_untagged_contents_mark();
    d->set_klass_field( doubleKlassObject );
    d->set_value( value->value() );
    return d;
}


SymbolOop OopFactory::new_symbol( const char *name, std::int32_t len ) {
    return Universe::symbol_table->lookup( name, len );
}


SymbolOop OopFactory::new_symbol( const char *name ) {
    return new_symbol( name, strlen( name ) );
}


SymbolOop OopFactory::new_symbol( ByteArrayOop b ) {
    return new_symbol( const_cast<char *>( b->chars()), b->length() );
}


AssociationOop OopFactory::new_association( SymbolOop key, Oop value, bool is_constant ) {
    AssociationOop as = AssociationOop( Universe::associationKlassObject()->klass_part()->allocateObject() );
    st_assert( as->is_association(), "type check" );
    as->set_key( key );
    as->set_value( value );
    as->set_is_constant( is_constant );
    return as;
}


VirtualFrameOop OopFactory::new_vframe( ProcessOop process, std::int32_t index ) {
    BlockScavenge     bs;
    VirtualFrameKlass *vk = (VirtualFrameKlass *) Universe::vframeKlassObject()->klass_part();

    VirtualFrameOop result = VirtualFrameOop( vk->allocateObject() );

    result->set_process( process );
    result->set_index( index );
    result->set_time_stamp( process->process()->time_stamp() );

    return result;
}


SmallIntegerOop OopFactory::new_smi( std::int32_t value ) {
    return smiOopFromValue( value );
}
