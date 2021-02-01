//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/vmOperations.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/ErrorHandler.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/runtime/arguments.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/system/os.hpp"
#include "vm/runtime/init.hpp"


void VM_Operation::evaluate() {
    EventMarker  em( "VM operation %s", name() );
    ResourceMark resourceMark;
    doit();
}


VM_Genesis::VM_Genesis() {
}


void VM_Genesis::doit() {
    ErrorHandler::genesis();
}


void VM_Scavenge::doit() {
    // For debugging gc-problems
    if ( false ) {
        ResourceMark resourceMark;
        FlagSetting  fs( PrintLongFrames, true );
        Frame        f = calling_process()->last_frame();
        f.print();
    }

    if ( PrintStackAtScavenge ) {
        spdlog::info( "*** BEFORE ***" );
        Processes::print();
    }
    Universe::scavenge( _addr );
    if ( PrintStackAtScavenge ) {
        spdlog::info( "*** AFTER ***" );
        Processes::print();
        spdlog::info( "******" );
    }
}


void VM_GarbageCollect::doit() {
    *_addr = MarkSweep::collect( *_addr );
}


void VM_DeoptimizeStacks::doit() {
    Processes::deoptimize_all();
    CompiledCodeOnly = false;
}


void VM_TerminateProcess::doit() {
    DeltaProcess *caller = calling_process();
    // must reset calling process in case stack does not get freed, but before terminate in case stack does get freed
    set_calling_process( nullptr );
    VMProcess::terminate( _target );
    delete caller;
}


void VM_OptimizeMethod::doit() {
    if ( _method->is_blockMethod() ) {
//        Compiler c( closure, scope );
//        return c.compile();
        compiler_warning( "can't recompile block yet" );
        _nativeMethod = nullptr;
        return;
    }
    Compiler c( &_key, _method );
    _nativeMethod = c.compile();
}


void VM_OptimizeRScope::doit() {
    Compiler c( _scope );
    _nativeMethod = c.compile();
}


void VM_OptimizeBlockMethod::doit() {
    Compiler c( _closure, _scope );
    _nativeMethod = c.compile();
}


void load_image() {

    ResourceMark resourceMark;

    Bootstrap bootstrap( image_basename );
    bootstrap.load();

    vmSymbols::initialize();

    bootstrappingInProgress = false;
}


std::int32_t vmProcessMain( void *ignored ) {
    static_cast<void>(ignored); // unused

    Processes::start( new VMProcess );
    return 0;
}


std::int32_t createVMProcess() {

    std::int32_t ignored;
    spdlog::info( "createVMProcess() calling os::create_thread( &vmProcessMain, nullptr, &ignored )" );
    os::create_thread( &vmProcessMain, nullptr, &ignored );

    return 0;
}


std::int32_t vm_main( std::int32_t argc, char *argv[] ) {

    parse_arguments( argc, argv );
    init_globals();

    load_image();
    spdlog::info( "%status-image-loaded" );

    if ( UseInliningDatabase )
        InliningDatabase::load_index_file();

    DeltaProcess::createMainProcess();
    spdlog::info( "%status-main-process-created" );

    createVMProcess();
    spdlog::info( "%vm-process-created" );

    DeltaProcess::runMainProcess();

    return 0;
}
