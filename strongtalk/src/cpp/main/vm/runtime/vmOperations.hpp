
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


    virtual ~VM_Operation() = default;
    VM_Operation( const VM_Operation & ) = default;
    VM_Operation &operator=( const VM_Operation & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


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
        SPDLOG_INFO( "{}", name() );
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


    VM_Scavenge() = default;
    virtual ~VM_Scavenge() = default;
    VM_Scavenge( const VM_Scavenge & ) = default;
    VM_Scavenge &operator=( const VM_Scavenge & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

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


    virtual ~VM_GarbageCollect() = default;
    VM_GarbageCollect( const VM_GarbageCollect & ) = default;
    VM_GarbageCollect &operator=( const VM_GarbageCollect & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



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


    VM_TerminateProcess() = default;
    virtual ~VM_TerminateProcess() = default;
    VM_TerminateProcess( const VM_TerminateProcess & ) = default;
    VM_TerminateProcess &operator=( const VM_TerminateProcess & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


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


    VM_OptimizeMethod() = default;
    virtual ~VM_OptimizeMethod() = default;
    VM_OptimizeMethod( const VM_OptimizeMethod & ) = default;
    VM_OptimizeMethod &operator=( const VM_OptimizeMethod & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



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


    VM_OptimizeRScope() = default;
    virtual ~VM_OptimizeRScope() = default;
    VM_OptimizeRScope( const VM_OptimizeRScope & ) = default;
    VM_OptimizeRScope &operator=( const VM_OptimizeRScope & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


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


    VM_OptimizeBlockMethod() = default;
    virtual ~VM_OptimizeBlockMethod() = default;
    VM_OptimizeBlockMethod( const VM_OptimizeBlockMethod & ) = default;
    VM_OptimizeBlockMethod &operator=( const VM_OptimizeBlockMethod & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    void doit();


    NativeMethod *method() const {
        return _nativeMethod;
    }


    const char *name() {
        return "optimize block method";
    }
};
