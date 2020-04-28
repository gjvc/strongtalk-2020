//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/vmOperations.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/runtime/ErrorHandler.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/runtime/ResourceMark.hpp"


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
        _console->print_cr( "*** BEFORE ***" );
        Processes::print();
    }
    Universe::scavenge( _addr );
    if ( PrintStackAtScavenge ) {
        _console->print_cr( "*** AFTER ***" );
        Processes::print();
        _console->print_cr( "******" );
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
    DeltaProcess * caller = calling_process();
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
