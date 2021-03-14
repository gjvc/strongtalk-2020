
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/klass/SmallIntegerKlass.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


KlassOop SMIKlass::create_subclass( MixinOop mixin, Format format ) {
    st_unused( mixin ); // unused
    st_unused( format ); // unused

    return nullptr;
}


void setKlassVirtualTableFromSmiKlass( Klass *k ) {
    SMIKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop SMIKlass::oop_shallow_copy( Oop obj, bool tenured ) {
    st_unused( tenured ); // unused

    return obj;
}


void SMIKlass::oop_print_value( Oop obj, ConsoleOutputStream *stream ) {
    stream->print( "%d", SmallIntegerOop( obj )->value() );
}
