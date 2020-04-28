//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/smiKlass.hpp"
#include "vm/utilities/OutputStream.hpp"


KlassOop smiKlass::create_subclass( MixinOop mixin, Format format ) {
    return nullptr;
}


void setKlassVirtualTableFromSmiKlass( Klass * k ) {
    smiKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop smiKlass::oop_shallow_copy( Oop obj, bool_t tenured ) {
    return obj;
}


void smiKlass::oop_print_value( Oop obj, ConsoleOutputStream * stream ) {
    stream->print( "%d", SMIOop( obj )->value() );
}
