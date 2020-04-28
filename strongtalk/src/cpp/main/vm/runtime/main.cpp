//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/runtime/arguments.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/system/os.hpp"
#include "vm/runtime/init.hpp"


static void load_image() {

    ResourceMark resourceMark;

    Bootstrap bootstrap( image_basename );
    bootstrap.load();

    vmSymbols::initialize();

    bootstrappingInProgress = false;
}


int vmProcessMain( void * ignored ) {
    Processes::start( new VMProcess );
    return 0;
}


int createVMProcess() {

    int ignored;
    os::create_thread( &vmProcessMain, nullptr, &ignored );

    return 0;
}


int vm_main( int argc, char * argv[] ) {

    parse_arguments( argc, argv );
    init_globals();

    load_image();
    _console->print_cr( "%%status-image-loaded" );

    if ( UseInliningDatabase )
        InliningDatabase::load_index_file();

    DeltaProcess::createMainProcess();
    _console->print_cr( "%%status-main-process-created" );

    createVMProcess();
    _console->print_cr( "%%vm-process-created" );

    os::sleep( 10 * 1000 );
    DeltaProcess::runMainProcess();

    return 0;
}

