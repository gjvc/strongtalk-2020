//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/utilities/OutputStream.hpp"


void KlassOopDescriptor::bootstrap_object( Bootstrap * stream ) {

    klass_part()->bootstrap_klass_part_one( stream );
    MemOopDescriptor::bootstrap_header( stream );

    klass_part()->bootstrap_klass_part_two( stream );
    MemOopDescriptor::bootstrap_body( stream, header_size() );

}


// don't remove -- useful for debugging  -Urs
void KlassOopDescriptor::print_superclasses() {

    ConsoleOutputStream stream;

    for ( KlassOop k = this; k not_eq nilObj; k = k->klass_part()->superKlass() ) {
        k->print_value_on( &stream );
        stream.print_cr( "" );
    }
    stream.cr();
}
