//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/recompiler/RecompilerFrame.hpp"


// A Recompilee represents something to be recompiled (either an interpreted method or a compiled method).

class Recompilee : public ResourceObject {
protected:
    RecompilerFrame *_recompilerFrame;


    Recompilee( RecompilerFrame *recompilerFrame ) :
        _recompilerFrame{ recompilerFrame } {
    }

    Recompilee() = default;
    virtual ~Recompilee() = default;
    Recompilee( const Recompilee & ) = default;
    Recompilee &operator=( const Recompilee & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



public:
    virtual bool is_interpreted() const {
        return false;
    }


    virtual bool is_compiled() const {
        return false;
    }


    virtual LookupKey *key() const = 0;


    virtual MethodOop method() const = 0;


    virtual NativeMethod *code() const {
        ShouldNotCallThis();
        return nullptr;
    }    // only for compiled recompileed


    RecompilerFrame *recompilerFrame() const {
        return _recompilerFrame;
    }


    static Recompilee *new_Recompilee( RecompilerFrame *recompilerFrame );
};


class InterpretedRecompilee : public Recompilee {

private:
    LookupKey *_key;
    MethodOop _method;

public:
    InterpretedRecompilee( RecompilerFrame *rf, LookupKey *k, MethodOop m ) :
        Recompilee( rf ),
        _key{ k },
        _method{ m } {
    }


    InterpretedRecompilee() = default;
    virtual ~InterpretedRecompilee() = default;
    InterpretedRecompilee( const InterpretedRecompilee & ) = default;
    InterpretedRecompilee &operator=( const InterpretedRecompilee & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    bool is_interpreted() const {
        return true;
    }


    LookupKey *key() const {
        return _key;
    }


    MethodOop method() const {
        return _method;
    }
};


class CompiledRecompilee : public Recompilee {

private:
    NativeMethod *_nativeMethod;

public:
    CompiledRecompilee( RecompilerFrame *rf, NativeMethod *nm ) :
        Recompilee( rf ),
        _nativeMethod{ nm } {
    }


    CompiledRecompilee() = default;
    virtual ~CompiledRecompilee() = default;
    CompiledRecompilee( const CompiledRecompilee & ) = default;
    CompiledRecompilee &operator=( const CompiledRecompilee & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    bool is_compiled() const {
        return true;
    }


    LookupKey *key() const;


    MethodOop method() const;


    NativeMethod *code() const {
        return _nativeMethod;
    }
};
