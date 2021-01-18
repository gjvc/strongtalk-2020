
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/lookup/LookupKey.hpp"


// The following classes are used for operations initiated by a delta process but must take place in the vmProcess.

class VM_Operation : public PrintableStackAllocatedObject {

private:
    DeltaProcess *_calling_process;

public:
    void set_calling_process( DeltaProcess *p ) {
        _calling_process = p;
    }


    DeltaProcess *calling_process() const {
        return _calling_process;
    }


    virtual bool_t is_scavenge() const {
        return false;
    }


    virtual bool_t is_garbage_collect() const {
        return false;
    }


    virtual bool_t is_single_step() const {
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
    bool_t is_scavenge() const {
        return true;
    }


    VM_Scavenge( Oop *addr ) {
        this->_addr = addr;
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
    Oop *_addr;

public:
    bool_t is_garbage_collect() const {
        return true;
    }


    VM_GarbageCollect( Oop *addr ) {
        this->_addr = addr;
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
    VM_TerminateProcess( DeltaProcess *target ) {
        this->_target = target;
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
    LookupKey _key;
    MethodOop _method;
    NativeMethod *_nativeMethod;

public:
    VM_OptimizeMethod( LookupKey *key, MethodOop method ) :
            _key( key ) {
        _method = method;
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
    VM_OptimizeRScope( RecompilationScope *scope ) {
        _scope = scope;
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

class NonInlinedBlockScopeDescriptor;

class VM_OptimizeBlockMethod : public VM_Operation {

private:
    BlockClosureOop _closure;
    NonInlinedBlockScopeDescriptor *_scope;
    NativeMethod                   *_nativeMethod;

public:
    VM_OptimizeBlockMethod( BlockClosureOop closure, NonInlinedBlockScopeDescriptor *scope ) {
        this->_closure = closure;
        this->_scope   = scope;
    }


    void doit();


    NativeMethod *method() const {
        return _nativeMethod;
    }


    const char *name() {
        return "optimize block method";
    }
};


// -----------------------------------------------------------------------------

int vm_main( int argc, char *argv[] );
int createVMProcess();
int vmProcessMain( void *ignored );
void load_image();
