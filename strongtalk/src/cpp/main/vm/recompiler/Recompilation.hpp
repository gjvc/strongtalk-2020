//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/ResourceObject.hpp"

// The recompilation system determines which interpreted methods should be compiled
// by the native-code compiler, or which compiled methods need to be recompiled for
// further optimization. This file contains the interface to the run-time system.
//
// A Recompilation performs a recompilation from interpreted or compiled code to
// (hopefully better) compiled code.
//
// The global theRecompilation is set only during a recompilation.

class Recompilation;

class Recompilee;

class RecompilerFrame;

extern Recompilation * theRecompilation;
extern NativeMethod  * recompilee;        // method currently being recompiled

class Recompilation : public VM_Operation {

    private:
        Oop       _receiver;            // receiver of trigger method/NativeMethod
        MethodOop _method;              // trigger method
        NativeMethod      * _nativeMethod;       // trigger NativeMethod (if compiled, nullptr otherwise)
        NativeMethod      * _newNativeMethod;    // new NativeMethod replacing trigger (if any)
        DeltaVirtualFrame * _deltaVirtualFrame;  // VirtualFrame of trigger method/NativeMethod (NOT COMPLETELY INITIALIZED)
        bool_t _isUncommonBranch;    // recompiling because of uncommon branch?
        bool_t _recompiledTrigger;   // is newNM the new version of _nm?

    public:

        Recompilation( Oop receiver, MethodOop method ) {            // used if interpreted method triggers counter
            _method           = method;
            _receiver         = receiver;
            _nativeMethod     = nullptr;
            _isUncommonBranch = false;
            init();
        }


        Recompilation( Oop receiver, NativeMethod * nm, bool_t unc = false ) {  // used if compiled method triggers counter
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


        bool_t isCompiled() const {
            return _nativeMethod not_eq nullptr;
        }


        bool_t recompiledTrigger() const {
            return _recompiledTrigger;
        }


        const char * result() const {
            return _newNativeMethod ? _newNativeMethod->verifiedEntryPoint() : nullptr;
        }


        Oop receiverOf( DeltaVirtualFrame * vf ) const;     // same as vf->receiver() but also works for trigger
        const char * name() {
            return "recompile";
        }


        // entry points dealing with invocation counter overflow
        static const char * methodOop_invocation_counter_overflow( Oop receiver, MethodOop method ); // called by interpreter
        static const char * nativeMethod_invocation_counter_overflow( Oop receiver, char * pc ); // called by nativeMethods

    protected:
        void init();

        void recompile( Recompilee * r );

        void recompile_method( Recompilee * r );

        void recompile_block( Recompilee * r );

        bool_t handleStaleInlineCache( RecompilerFrame * first );
};


// A Recompilee represents something to be recompiled (either an interpreted method or a compiled method).

class Recompilee : public ResourceObject {
    protected:
        RecompilerFrame * _rf;


        Recompilee( RecompilerFrame * rf ) {
            _rf = rf;
        }


    public:
        virtual bool_t is_interpreted() const {
            return false;
        }


        virtual bool_t is_compiled() const {
            return false;
        }


        virtual LookupKey * key() const = 0;

        virtual MethodOop method() const = 0;


        virtual NativeMethod * code() const {
            ShouldNotCallThis();
            return nullptr;
        }    // only for compiled recompileed

        RecompilerFrame * rframe() const {
            return _rf;
        }


        static Recompilee * new_Recompilee( RecompilerFrame * rf );
};


class InterpretedRecompilee : public Recompilee {

    private:
        LookupKey * _key;
        MethodOop _method;

    public:
        InterpretedRecompilee( RecompilerFrame * rf, LookupKey * k, MethodOop m ) :
            Recompilee( rf ) {
            _key    = k;
            _method = m;
        }


        bool_t is_interpreted() const {
            return true;
        }


        LookupKey * key() const {
            return _key;
        }


        MethodOop method() const {
            return _method;
        }
};

class CompiledRecompilee : public Recompilee {

    private:
        NativeMethod * _nativeMethod;

    public:
        CompiledRecompilee( RecompilerFrame * rf, NativeMethod * nm ) :
            Recompilee( rf ) {
            _nativeMethod = nm;
        }


        bool_t is_compiled() const {
            return true;
        }


        LookupKey * key() const;

        MethodOop method() const;


        NativeMethod * code() const {
            return _nativeMethod;
        }
};


extern int nstages;                     // # of recompilation stages
extern smi_t * compileCounts;           // # of compilations indexed by stage
extern int   * recompileLimits;         // recompilation limits indexed by stage

constexpr int MaxRecompilationLevels = 4;           // max. # recompilation levels
constexpr int MaxVersions            = 4 - 1;       // desired max. # NativeMethod recompilations

