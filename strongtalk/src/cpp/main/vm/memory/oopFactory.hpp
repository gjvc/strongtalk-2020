//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "allocation.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/oops/DoubleByteArrayOopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/utilities/GrowableArray.hpp"


// The oopFactory is a utility to create new objects.

class oopFactory : AllStatic {

public:
    static ByteArrayOop new_byteArray( std::int32_t size );

    static ByteArrayOop new_byteArray( const char *name );

    static ObjectArrayOop new_objArray( std::int32_t size );

    static ObjectArrayOop new_objArray( GrowableArray<Oop> *array );

    static SMIOop new_smi( std::int32_t value );

    static DoubleOop new_double( double value );

    static DoubleOop clone_double_to_oldspace( DoubleOop value );

    static SymbolOop new_symbol( const char *name, std::int32_t len );

    static SymbolOop new_symbol( const char *name );

    static SymbolOop new_symbol( ByteArrayOop b );

    static AssociationOop new_association( SymbolOop key, Oop value, bool is_constant );

    static VirtualFrameOop new_vframe( ProcessOop process, std::int32_t index );
};
