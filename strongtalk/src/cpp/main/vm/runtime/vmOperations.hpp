
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/code/ScopeDescriptor.hpp"


// -----------------------------------------------------------------------------

std::int32_t vm_main( std::int32_t argc, char *argv[] );
std::int32_t createVMProcess();
std::int32_t vmProcessMain( void *ignored );
void load_image();


// The following classes are used for operations initiated by a delta process but must take place in the vmProcess.

class VM_Operation : public PrintableStackAllocatedObject {

private:
    DeltaProcess *_calling_process;

public:
    VM_Operation() :
        _calling_process{ nullptr } {
    }


    void set_calling_process( DeltaProcess *p ) {
        _calling_process = p;
    }


    DeltaProcess *calling_process() const {
        return _calling_process;
    }


    virtual bool is_scavenge() const {
        return false;
    }


    virtual bool is_garbage_collect() const {
        return false;
    }


    virtual bool is_single_step() const {
        return false;
    }


    void evaluate();

    // Evaluate is called in the vmProcess
    virtual void doit() = 0;


    void print() {
        _console->print( "%s", name() );
    }


    virtual const char *name() {
        return "vanilla";
    }
};


class VM_Scavenge : public VM_Operation {
private:
    Oop *_addr;
public:
    bool is_scavenge() const {
        return true;
    }


    VM_Scavenge( Oop *addr ) :
        VM_Operation(), _addr{ addr } {
    }


    void doit();


    const char *name() {
        return "scavenge";
    }
};


class VM_Genesis : public VM_Operation {
public:
    VM_Genesis();

    void doit();


    const char *name() {
        return "genesis";
    }
};


class VM_GarbageCollect : public VM_Operation {

private:
    VM_GarbageCollect();
    Oop *_addr;

public:


    bool is_garbage_collect() const {
        return true;
    }


    VM_GarbageCollect( Oop *addr ) : _addr{ addr } {
    }


    void doit();


    const char *name() {
        return "garbage collect";
    }
};


class VM_TerminateProcess : public VM_Operation {

private:
    DeltaProcess *_target;

public:
    VM_TerminateProcess( DeltaProcess *target ) : _target{ target } {
    }


    void doit();


    const char *name() {
        return "terminate process";
    }
};


class VM_DeoptimizeStacks : public VM_Operation {
public:
    void doit();


    const char *name() {
        return "deoptimize stacks";
    }
};


class VM_OptimizeMethod : public VM_Operation {

private:
    LookupKey    _key;
    MethodOop    _method;
    NativeMethod *_nativeMethod;

public:
    VM_OptimizeMethod( LookupKey *key, MethodOop method ) :
        _key{ key },
        _method{ method },
        _nativeMethod{ nullptr } {
    }


    NativeMethod *result() const {
        return _nativeMethod;
    }


    void doit();


    const char *name() {
        return "optimize method";
    }
};


// -----------------------------------------------------------------------------


class RecompilationScope;

class VM_OptimizeRScope : public VM_Operation {

private:
    RecompilationScope *_scope;
    NativeMethod       *_nativeMethod;

public:
    VM_OptimizeRScope( RecompilationScope *scope ) :
        _scope{ scope },
        _nativeMethod{ nullptr } {
    }


    NativeMethod *result() const {
        return _nativeMethod;
    }


    void doit();


    const char *name() {
        return "optimize rscope";
    }

};


// -----------------------------------------------------------------------------

class VM_OptimizeBlockMethod : public VM_Operation {

private:
    BlockClosureOop                _closure;
    NonInlinedBlockScopeDescriptor *_scope;
    NativeMethod                   *_nativeMethod;

public:
    VM_OptimizeBlockMethod( BlockClosureOop closure, NonInlinedBlockScopeDescriptor *scope ) :
        _closure{ closure },
        _scope{ scope },
        _nativeMethod{ nullptr } {
    }


    void doit();


    NativeMethod *method() const {
        return _nativeMethod;
    }


    const char *name() {
        return "optimize block method";
    }
};
