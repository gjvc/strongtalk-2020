//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/recompiler/RecompilerFrame.hpp"


// A Recompilee represents something to be recompiled (either an interpreted method or a compiled method).

class Recompilee : public ResourceObject {
protected:
    RecompilerFrame *_rf;


    Recompilee( RecompilerFrame *rf ) {
        _rf = rf;
    }


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

    RecompilerFrame *rframe() const {
        return _rf;
    }


    static Recompilee *new_Recompilee( RecompilerFrame *rf );
};


class InterpretedRecompilee : public Recompilee {

private:
    LookupKey *_key;
    MethodOop _method;

public:
    InterpretedRecompilee( RecompilerFrame *rf, LookupKey *k, MethodOop m ) :
            Recompilee( rf ) {
        _key    = k;
        _method = m;
    }


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
            Recompilee( rf ) {
        _nativeMethod = nm;
    }


    bool is_compiled() const {
        return true;
    }


    LookupKey *key() const;

    MethodOop method() const;


    NativeMethod *code() const {
        return _nativeMethod;
    }
};
