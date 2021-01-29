//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/lookup/LookupType.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/runtime/ResourceObject.hpp"


// RecompilerFrame decorate stack frames with some extra information needed by the recompiler.
// The recompiler views the stack (at the time of recompilation) as a list of rframes.

class DeltaVirtualFrame;

class RecompilerFrame : public PrintableResourceObject {

protected:
    Frame _frame;                                      // my frame
    RecompilerFrame *_caller;           //
    RecompilerFrame *_callee;            // caller / callee RecompilerFrame (or nullptr)
    std::int32_t _num;                // stack frame number (0 = most recent)
    std::int32_t _distance;           // recompilation search "distance" (measured in # of interpreted frames)
    std::int32_t _invocations;        // current invocation estimate (for this frame) (i.e., how often was thus frame called)
    std::int32_t _ncallers;           // number of callers
    std::int32_t _sends;              // sends caused by this frame
    std::int32_t _cumulSends;         // sends including sends from nested blocks
    std::int32_t _loopDepth;          // loop depth of callee

    RecompilerFrame( Frame frame, const RecompilerFrame *callee );

    virtual void init() = 0;    // compute invocations, loopDepth, etc.
    void print( const char *name );

public:

    static RecompilerFrame *new_RFrame( Frame frame, const RecompilerFrame *callee );


    virtual bool is_interpreted() const {
        return false;
    }


    virtual bool is_compiled() const {
        return false;
    }


    bool is_super() const;        // invoked by super send?
    std::int32_t invocations() const {
        return _invocations;
    }


    std::int32_t sends() const {
        return _sends;
    }


    std::int32_t cumulSends() const {
        return _cumulSends;
    }


    std::int32_t loopDepth() const {
        return _loopDepth;
    }


    std::int32_t num() const {
        return _num;
    }


    std::int32_t distance() const {
        return _distance;
    }


    void set_distance( std::int32_t d );


    std::int32_t nCallers() const {
        return _ncallers;
    }


    bool is_blockMethod() const;


    Frame fr() const {
        return _frame;
    }


    virtual LookupKey *key() const = 0;    // lookup key or nullptr (for block invoc.)
    virtual std::int32_t cost() const = 0;    // estimated inlining cost (size)
    virtual MethodOop top_method() const = 0;

    virtual DeltaVirtualFrame *top_vframe() const = 0;

    virtual void cleanupStaleInlineCaches() = 0;


    virtual NativeMethod *nm() const {
        ShouldNotCallThis();
        return nullptr;
    }


    bool hasBlockArgs() const;        // does top method receive block arguments?
    GrowableArray<BlockClosureOop> *blockArgs() const;  // return list of all block args

    RecompilerFrame *caller();


    RecompilerFrame *callee() const {
        return _callee;
    }


    RecompilerFrame *parent() const;        // rframe containing lexical scope (if any)
    void print() = 0;

    static std::int32_t computeSends( MethodOop m );

    static std::int32_t computeSends( NativeMethod *nm );

    static std::int32_t computeCumulSends( MethodOop m );

    static std::int32_t computeCumulSends( NativeMethod *nm );
};


class CompiledRecompilerFrame : public RecompilerFrame {        // frame containing a compiled method

protected:
    NativeMethod      *_nativeMethod;
    DeltaVirtualFrame *_deltaVirtualFrame;        // top VirtualFrame; may be nullptr (for most recent frame)

    CompiledRecompilerFrame( Frame fr, const RecompilerFrame *callee );

    void init();

    friend class RecompilerFrame;

public:

    CompiledRecompilerFrame( Frame fr );    // for NativeMethod triggering its counter (callee == nullptr)
    bool is_compiled() const {
        return true;
    }


    MethodOop top_method() const;


    DeltaVirtualFrame *top_vframe() const {
        return _deltaVirtualFrame;
    }


    NativeMethod *nm() const {
        return _nativeMethod;
    }


    LookupKey *key() const;

    std::int32_t cost() const;

    void cleanupStaleInlineCaches();

    void print();
};

class InterpretedRecompilerFrame : public RecompilerFrame {    // interpreter frame

protected:
    MethodOop _method;              //
    std::int32_t       _byteCodeIndex;       // current byteCodeIndex
    KlassOop  _receiverKlass;       //
    DeltaVirtualFrame *_deltaVirtualFrame;  // may be nullptr (for most recent frame)
    LookupKey         *_lookupKey;          // cached value of key()

    InterpretedRecompilerFrame( Frame fr, const RecompilerFrame *callee );

    void init();

    friend class RecompilerFrame;

public:
    InterpretedRecompilerFrame( Frame fr, MethodOop m, KlassOop rcvrKlass );    // for method triggering its invocation counter
    bool is_interpreted() const {
        return true;
    }


    MethodOop top_method() const {
        return _method;
    }


    DeltaVirtualFrame *top_vframe() const {
        return _deltaVirtualFrame;
    }


    LookupKey *key() const;

    std::int32_t cost() const;

    void cleanupStaleInlineCaches();

    void print();
};
