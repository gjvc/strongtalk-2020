//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/DeltaProcess.hpp"


void ProcessOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    MemOopDescriptor::bootstrap_header( stream );
    set_process( nullptr );
    MemOopDescriptor::bootstrap_body( stream, header_size() );
}


SymbolOop ProcessOopDescriptor::status_symbol() {
    if ( not is_live() )
        return vmSymbols::dead();
    return process()->status_symbol();
}


double ProcessOopDescriptor::user_time() {
    if ( not is_live() )
        return 0.0;
    return process()->user_time();
}


double ProcessOopDescriptor::system_time() {
    if ( not is_live() )
        return 0.0;
    return process()->system_time();
}
