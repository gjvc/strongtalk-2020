//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


void KlassOopDescriptor::bootstrap_object( Bootstrap *stream ) {


    SPDLOG_INFO("KlassOopDescriptor::bootstrap_object");
    klass_part()->bootstrap_klass_part_one( stream );
    MemOopDescriptor::bootstrap_header( stream );

    klass_part()->bootstrap_klass_part_two( stream );
    MemOopDescriptor::bootstrap_body( stream, header_size() );

}


// don't remove -- useful for debugging  -Urs
void KlassOopDescriptor::print_superclasses() {

    ConsoleOutputStream stream;

    for ( KlassOop k = this; k not_eq nilObject; k = k->klass_part()->superKlass() ) {
        k->print_value_on( &stream );
        stream.print_cr( "" );
    }
    stream.cr();
}
