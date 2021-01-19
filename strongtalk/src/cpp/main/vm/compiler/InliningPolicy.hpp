//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/runtime/ResourceObject.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"


class InliningPolicy : public ResourceObject {

    // the instance variables only serve as temporary storage during shouldInline()
protected:
    MethodOop _methodOop;        // target method

public:
    int calleeCost;        // cost of inlining candidate

    InliningPolicy() {
        _methodOop = nullptr;
    }


    const char *basic_shouldInline( MethodOop method );
    // should send be inlined?  returns nullptr (--> yes) or rejection msg
    // doesn't rely on compiler-internal information

    static bool_t isCriticalSmiSelector( const SymbolOop sel );

    static bool_t isCriticalArraySelector( const SymbolOop sel );

    static bool_t isCriticalBoolSelector( const SymbolOop sel );

    // predicted by compiler?
    static bool_t isPredictedSmiSelector( const SymbolOop sel );

    static bool_t isPredictedArraySelector( const SymbolOop sel );

    static bool_t isPredictedBoolSelector( const SymbolOop sel );

    // predicted by interpreter?
    static bool_t isInterpreterPredictedSmiSelector( const SymbolOop sel );

    static bool_t isInterpreterPredictedArraySelector( const SymbolOop sel );

    static bool_t isInterpreterPredictedBoolSelector( const SymbolOop sel );

protected:
    virtual KlassOop receiverKlass() const = 0;        // return receiver klass (nullptr if unknown)
    virtual KlassOop nthArgKlass( std::size_t nth ) const = 0;    // return nth argument of method (nullptr if unknown)
    bool_t shouldNotInline() const;

    bool_t isBuiltinMethod() const;
};


class Expression;

class InlinedScope;

// inlining policy of compiler
class CompilerInliningPolicy : public InliningPolicy {
protected:
    InlinedScope *_sender;     // sending scope
    Expression   *_receiver;   // target receiver

    KlassOop receiverKlass() const;

    KlassOop nthArgKlass( std::size_t n ) const;

public:
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

    KlassOop nthArgKlass( std::size_t n ) const;

    const char *shouldInline( const NativeMethod *nm );    // should nm be inlined?

public:
    const char *shouldInline( RecompilerFrame *recompilerFrame );
    // would send be inlined by compiler?  returns nullptr (--> yes) or rejection msg
};
