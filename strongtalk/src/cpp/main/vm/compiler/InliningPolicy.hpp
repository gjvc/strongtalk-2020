
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/ResourceObject.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"


class InliningPolicy : public ResourceObject {

    // the instance variables only serve as temporary storage during shouldInline()
protected:
    MethodOop _methodOop;        // target method

public:
    std::int32_t calleeCost;        // cost of inlining candidate

    InliningPolicy() :
        _methodOop{ nullptr },
        calleeCost{ 0 } {
    }
    virtual ~InliningPolicy() = default;
    InliningPolicy( const InliningPolicy & ) = default;
    InliningPolicy &operator=( const InliningPolicy & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    const char *basic_shouldInline( MethodOop method );
    // should send be inlined?  returns nullptr (--> yes) or rejection msg
    // doesn't rely on compiler-internal information

    static bool isCriticalSmiSelector( const SymbolOop sel );

    static bool isCriticalArraySelector( const SymbolOop sel );

    static bool isCriticalBoolSelector( const SymbolOop sel );

    // predicted by compiler?
    static bool isPredictedSmiSelector( const SymbolOop sel );

    static bool isPredictedArraySelector( const SymbolOop sel );

    static bool isPredictedBoolSelector( const SymbolOop sel );

    // predicted by interpreter?
    static bool isInterpreterPredictedSmiSelector( const SymbolOop sel );

    static bool isInterpreterPredictedArraySelector( const SymbolOop sel );

    static bool isInterpreterPredictedBoolSelector( const SymbolOop sel );

protected:
    virtual KlassOop receiverKlass() const = 0;        // return receiver klass (nullptr if unknown)
    virtual KlassOop nthArgKlass( std::int32_t nth ) const = 0;    // return nth argument of method (nullptr if unknown)
    bool shouldNotInline() const;

    bool isBuiltinMethod() const;
};


class Expression;

class InlinedScope;

// inlining policy of compiler
class CompilerInliningPolicy : public InliningPolicy {
protected:
    InlinedScope *_sender;     // sending scope
    Expression   *_receiver;   // target receiver

    KlassOop receiverKlass() const;

    KlassOop nthArgKlass( std::int32_t n ) const;

public:
    CompilerInliningPolicy() :
        InliningPolicy(),
        _sender{ nullptr },
        _receiver{ nullptr } {

    }


    virtual ~CompilerInliningPolicy() = default;
    CompilerInliningPolicy( const CompilerInliningPolicy & ) = default;
    CompilerInliningPolicy &operator=( const CompilerInliningPolicy & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    const char *shouldInline( InlinedScope *sender, InlinedScope *callee );
    // should send be inlined?  returns nullptr (--> yes) or rejection msg
};


class RecompilerFrame;

class DeltaVirtualFrame;

// "inlining policy" of recompiler (i.e., guesses whether method will be inlined or not
class RecompilerInliningPolicy : public InliningPolicy {

protected:
    DeltaVirtualFrame *_deltaVirtualFrame;        // top VirtualFrame of this method/NativeMethod; may be nullptr
    KlassOop receiverKlass() const;

    KlassOop nthArgKlass( std::int32_t n ) const;

    const char *shouldInline( const NativeMethod *nm );    // should nm be inlined?

public:
    RecompilerInliningPolicy() :
        InliningPolicy(),
        _deltaVirtualFrame{ nullptr } {

    }

    virtual ~RecompilerInliningPolicy() = default;
    RecompilerInliningPolicy( const RecompilerInliningPolicy & ) = default;
    RecompilerInliningPolicy &operator=( const RecompilerInliningPolicy & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    const char *shouldInline( RecompilerFrame *recompilerFrame );
    // would send be inlined by compiler?  returns nullptr (--> yes) or rejection msg
};
