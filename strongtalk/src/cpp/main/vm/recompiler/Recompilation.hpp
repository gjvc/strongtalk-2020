//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/recompiler/Recompilee.hpp"
#include "vm/recompiler/RecompilerFrame.hpp"


extern std::int32_t nstages;                            // # of recompilation stages
extern smi_t        *compileCounts;                     // # of compilations indexed by stage
extern std::int32_t *recompileLimits;                   // recompilation limits indexed by stage

constexpr std::int32_t MAX_RECOMPILATION_LEVELS               = 4;      // max. # recompilation levels
constexpr std::int32_t MAX_NATIVE_METHOD_RECOMPILATION_LEVELS = 4 - 1;  // desired max. # NativeMethod recompilations


// The recompilation system determines which interpreted methods should be compiled
// by the native-code compiler, or which compiled methods need to be recompiled for
// further optimization. This file contains the interface to the run-time system.
//
// A Recompilation performs a recompilation from interpreted or compiled code to
// (hopefully better) compiled code.
//
// The global theRecompilation is set only during a recompilation.


extern NativeMethod        *recompilee;         // method currently being recompiled
extern class Recompilation *theRecompilation;   // forward declaration


class Recompilation : public VM_Operation {

private:
    Oop               _receiver;            // receiver of trigger method/NativeMethod
    MethodOop         _method;              // trigger method
    NativeMethod      *_nativeMethod;       // trigger NativeMethod (if compiled, nullptr otherwise)
    NativeMethod      *_newNativeMethod;    // new NativeMethod replacing trigger (if any)
    DeltaVirtualFrame *_deltaVirtualFrame;  // VirtualFrame of trigger method/NativeMethod (NOT COMPLETELY INITIALIZED)
    bool              _isUncommonBranch;    // recompiling because of uncommon branch?
    bool              _recompiledTrigger;   // is newNM the new version of _nm?

public:

    Recompilation( Oop receiver, MethodOop method ) {            // used if interpreted method triggers counter
        _method           = method;
        _receiver         = receiver;
        _nativeMethod     = nullptr;
        _isUncommonBranch = false;
        init();
    }


    Recompilation( Oop receiver, NativeMethod *nm, bool unc = false ) {  // used if compiled method triggers counter
        _method           = nm->method();
        _receiver         = receiver;
        _nativeMethod     = nm;
        _isUncommonBranch = unc;
        init();
    }


    ~Recompilation() {
        theRecompilation = nullptr;
    }


    void doit();


    bool isCompiled() const {
        return _nativeMethod not_eq nullptr;
    }


    bool recompiledTrigger() const {
        return _recompiledTrigger;
    }


    const char *result() const {
        return _newNativeMethod ? _newNativeMethod->verifiedEntryPoint() : nullptr;
    }


    Oop receiverOf( DeltaVirtualFrame *vf ) const;     // same as vf->receiver() but also works for trigger
    const char *name() {
        return "recompile";
    }


    // entry points dealing with invocation counter overflow
    static const char *methodOop_invocation_counter_overflow( Oop receiver, MethodOop method ); // called by interpreter
    static const char *nativeMethod_invocation_counter_overflow( Oop receiver, char *pc ); // called by nativeMethods

protected:
    void init();

    void recompile( Recompilee *r );

    void recompile_method( Recompilee *r );

    void recompile_block( Recompilee *r );

    bool handleStaleInlineCache( RecompilerFrame *first );
};
