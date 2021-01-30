//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


// RecompilationScope represent the inlined scopes of a NativeMethod during recompilation.
// Essentially, they are another view of the ScopeDescriptor information, in a
// format more convenient for the (re)compiler and extended with additional
// info from PICs and uncommon branches.

class InlinedScope;

class NonDummyRecompilationScope;

class InterpretedRecompilationScope;

class RecompilationScope : public PrintableResourceObject {

protected:
    NonDummyRecompilationScope *_sender;
    const std::int32_t         _senderByteCodeIndex;

public:
    std::int32_t _invocationCount;        // estimated # of invocations (-1 == unknown)

    RecompilationScope( NonDummyRecompilationScope *s, std::int32_t byteCodeIndex );


    // Type tests
    virtual bool isInterpretedScope() const {
        return false;
    }


    virtual bool isInlinedScope() const {
        return false;
    }


    virtual bool isNullScope() const {
        return false;
    }


    virtual bool isUninlinableScope() const {
        return false;
    }


    virtual bool isPICScope() const {
        return false;
    }


    virtual bool isUntakenScope() const {
        return false;
    }


    virtual bool isDatabaseScope() const {
        return false;
    }


    virtual bool isCompiled() const {
        return false;
    }


    NonDummyRecompilationScope *sender() const {
        return (NonDummyRecompilationScope *) _sender;
    }


    std::int32_t senderByteCodeIndex() const {
        return _senderByteCodeIndex;
    }


    virtual bool equivalent( InlinedScope *s ) const = 0;

    virtual bool equivalent( LookupKey *l ) const = 0;


    virtual RecompilationScope *subScope( std::int32_t byteCodeIndex, LookupKey *l ) const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual GrowableArray<RecompilationScope *> *subScopes( std::int32_t byteCodeIndex ) const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual bool hasSubScopes( std::int32_t byteCodeIndex ) const {
        ShouldNotCallThis();
        return false;
    }


    virtual bool isUncommonAt( std::int32_t byteCodeIndex ) const {
        ShouldNotCallThis();
        return false;
    }


    // was send at this byteCodeIndex uncommon (never taken) in recompilee?  (false means "don't know")
    virtual bool isNotUncommonAt( std::int32_t byteCodeIndex ) const {
        ShouldNotCallThis();
        return false;
    }


    // return true iff the send at this byteCodeIndex *must not* be made uncommon
    virtual bool isLite() const {
        return false;
    }


    virtual NativeMethod *get_nativeMethod() const {
        return nullptr;
    }


    virtual Expression *receiverExpression( PseudoRegister *p ) const;

    virtual KlassOop receiverKlass() const = 0;

    virtual MethodOop method() const = 0;

    virtual LookupKey *key() const = 0;

    bool wasNeverExecuted() const;                // was method never executed?
    virtual void print();

    virtual void print_short() = 0;

    virtual void printTree( std::int32_t byteCodeIndex, std::int32_t level ) const;


    // Support for inlining database:
    // - returns the size of the inlining database tree.
    virtual std::int32_t inlining_database_size() {
        return 0;
    }


    // - prints the inlining database tree on a stream.
    virtual void print_inlining_database_on( ConsoleOutputStream *stream, GrowableArray<ProgramCounterDescriptor *> *uncommon, std::int32_t byteCodeIndex = -1, std::int32_t level = 0 ) {
    }


    virtual void extend() {
    }


    friend class NonDummyRecompilationScope;
};


// dummy scope for convenience; e.g. can call subScope() w/o checking for this==nullptr
class NullRecompilationScope : public RecompilationScope {
public:
    NullRecompilationScope() :
        RecompilationScope( nullptr, 0 ) {
    }


    NullRecompilationScope( NonDummyRecompilationScope *sender, std::int32_t byteCodeIndex ) :
        RecompilationScope( sender, byteCodeIndex ) {
    }


    bool isNullScope() const {
        return true;
    }


    bool equivalent( InlinedScope *s ) const {
        return false;
    }


    bool equivalent( LookupKey *l ) const {
        return false;
    }


    RecompilationScope *subScope( std::int32_t byteCodeIndex, LookupKey *l ) const {
        return (RecompilationScope *) this;
    }


    GrowableArray<RecompilationScope *> *subScopes( std::int32_t byteCodeIndex ) const;


    bool hasSubScopes( std::int32_t byteCodeIndex ) const {
        return false;
    }


    bool isUncommonAt( std::int32_t byteCodeIndex ) const {
        return false;
    }


    bool isNotUncommonAt( std::int32_t byteCodeIndex ) const {
        return false;
    }


    KlassOop receiverKlass() const {
        ShouldNotCallThis();
        return nullptr;
    }


    MethodOop method() const {
        ShouldNotCallThis();
        return nullptr;
    }


    LookupKey *key() const {
        ShouldNotCallThis();
        return nullptr;
    }


    void print() {
        print_short();
    }


    void printTree( std::int32_t byteCodeIndex, std::int32_t level ) const;

    void print_short();
};


class UninlinableRecompilationScope : public NullRecompilationScope {
    // scope marking "callee" of an uninlinable or megamorphic send
public:
    UninlinableRecompilationScope( NonDummyRecompilationScope *sender, std::int32_t byteCodeIndex );        // for interpreted senders

    bool isUninlinableScope() const {
        return true;
    }


    bool isNullScope() const {
        return false;
    }


    void print_inlining_database_on( ConsoleOutputStream *stream, GrowableArray<ProgramCounterDescriptor *> *uncommon, std::int32_t byteCodeIndex, std::int32_t level );

    void print_short();
};


class RUncommonBranch;

// abstract class -- a non-dummy scope with subscopes
class NonDummyRecompilationScope : public RecompilationScope {

protected:
    const std::int32_t                  _level;         // distance from root
    const std::int32_t                  _ncodes;        // # byte codes in method
    GrowableArray<RecompilationScope *> **_subScopes;   // indexed by byteCodeIndex

public:
    GrowableArray<RUncommonBranch *> uncommon;          // list of uncommon branches

    NonDummyRecompilationScope( NonDummyRecompilationScope *s, std::int32_t byteCodeIndex, MethodOop m, std::int32_t level );

    RecompilationScope *subScope( std::int32_t byteCodeIndex, LookupKey *l ) const;

    // return the subscope matching the lookup
    // return NullScope if no scope
    GrowableArray<RecompilationScope *> *subScopes( std::int32_t byteCodeIndex ) const;

    // return all subscopes at byteCodeIndex
    bool hasSubScopes( std::int32_t byteCodeIndex ) const;


    std::int32_t level() const {
        return _level;
    }


    bool isUncommonAt( std::int32_t byteCodeIndex ) const;

    bool isNotUncommonAt( std::int32_t byteCodeIndex ) const;

    void addScope( std::int32_t byteCodeIndex, RecompilationScope *s );

    void printTree( std::int32_t byteCodeIndex, std::int32_t level ) const;

    virtual void unify( NonDummyRecompilationScope *s );

    static std::int32_t compare( NonDummyRecompilationScope **a, NonDummyRecompilationScope **b ); // for sorting

protected:
    virtual void constructSubScopes( bool trusted );


    virtual std::int32_t scopeID() const {
        ShouldNotCallThis();
        return 0;
    }


    static bool trustPICs( MethodOop m );

    void printSubScopes() const;

    static NonDummyRecompilationScope *constructRScopes( const NativeMethod *nm, bool trusted = true, std::int32_t level = 0 );

    friend class Compiler;

    friend class NativeMethod;

    friend class InliningDatabase;
};

class InterpretedRecompilationScope : public NonDummyRecompilationScope {
    // a scope corresponding to an interpreted method
    LookupKey *_key;
    MethodOop _method;
    bool      _is_trusted;            // is PolymorphicInlineCache info trusted?
    bool      extended;        // subScopes computed?
public:
    InterpretedRecompilationScope( NonDummyRecompilationScope *sender, std::int32_t byteCodeIndex, LookupKey *key, MethodOop m, std::int32_t level, bool trusted );


    bool isInterpretedScope() const {
        return true;
    }


    MethodOop method() const {
        return _method;
    }


    LookupKey *key() const {
        return _key;
    }


    KlassOop receiverKlass() const;

    bool equivalent( InlinedScope *s ) const;

    bool equivalent( LookupKey *l ) const;

    bool isUncommonAt( std::int32_t byteCodeIndex ) const;


    bool isNotUncommonAt( std::int32_t byteCodeIndex ) const {
        return false;
    }


    void extend();


    void print() {
        print_short();
    }


    void print_short();
};

class InlinedRecompilationScope : public NonDummyRecompilationScope {
    // an inlined scope in the recompilee
public:
    const NativeMethod    *nm;        // containing NativeMethod
    const ScopeDescriptor *desc;    // scope

    InlinedRecompilationScope( NonDummyRecompilationScope *s, std::int32_t byteCodeIndex, const NativeMethod *nm, ScopeDescriptor *d, std::int32_t level );


    bool isInlinedScope() const {
        return true;
    }


    bool isCompiled() const {
        return true;
    }


    MethodOop method() const;


    NativeMethod *get_nativeMethod() const {
        return (NativeMethod *) nm;
    }


    LookupKey *key() const;

    KlassOop receiverKlass() const;

    bool isLite() const;

    bool equivalent( LookupKey *l ) const;

    bool equivalent( InlinedScope *s ) const;

    void print();

    void print_short();

    std::int32_t inlining_database_size();

    void print_inlining_database_on( ConsoleOutputStream *stream, GrowableArray<ProgramCounterDescriptor *> *uncommon, std::int32_t byteCodeIndex, std::int32_t level );

protected:
    void constructSubScopes() {
        ShouldNotCallThis();
    }


    std::int32_t scopeID() const {
        return desc->offset();
    }
};


//
// this class should be removed -- it's just a top-level inlined or interpreted scope -- fix this   -Urs
//
class PICRecompilationScope : public NonDummyRecompilationScope {

    // a scope called by the recompilee via a compiled PolymorphicInlineCache
protected:
    const NativeMethod             *caller;    // calling NativeMethod
    const CompiledInlineCache      *_sd;    // calling InlineCache
    const ProgramCounterDescriptor *programCounterDescriptor;    // calling programCounterDescriptor
    const KlassOop                 klass;            // receiver klass
    const NativeMethod             *nm;        // called NativeMethod (or nullptr if interpreted)
    const MethodOop                _method;    // called method
    const bool                     trusted;        // is PolymorphicInlineCache info trusted?
    bool                           _extended;        // subScopes computed?
    const ScopeDescriptor          *_desc;    // scope (or nullptr if interpreted)

public:
    PICRecompilationScope( const NativeMethod *caller, ProgramCounterDescriptor *pc, CompiledInlineCache *s, KlassOop k, ScopeDescriptor *d, NativeMethod *n, MethodOop m, std::int32_t nsends, std::int32_t level, bool trusted );


    bool isPICScope() const {
        return true;
    }


    bool isCompiled() const {
        return nm not_eq nullptr;
    }


    MethodOop method() const {
        return _method;
    }


    NativeMethod *get_nativeMethod() const {
        return (NativeMethod *) nm;
    }


    KlassOop receiverKlass() const {
        return klass;
    }


    CompiledInlineCache *sd() const {
        return (CompiledInlineCache *) _sd;
    }


    LookupKey *key() const;

    bool isLite() const;

    bool equivalent( InlinedScope *s ) const;

    bool equivalent( LookupKey *l ) const;

    void unify( NonDummyRecompilationScope *s );

    void extend();

    void print();

    void print_short();

protected:
    std::int32_t scopeID() const {
        return programCounterDescriptor->_scope;
    }


    static bool trustPICs( const NativeMethod *nm );

    friend NonDummyRecompilationScope *NonDummyRecompilationScope::constructRScopes( const NativeMethod *nm, bool trusted, std::int32_t level );
};

class InliningDatabaseRecompilationScope : public NonDummyRecompilationScope {
    // a scope created from the inlining database
private:
    KlassOop            _receiver_klass;
    MethodOop           _method;
    LookupKey           *_key;
    GrowableArray<bool> *_uncommon;  // list of uncommon branch
public:
    InliningDatabaseRecompilationScope( NonDummyRecompilationScope *sender, std::int32_t byteCodeIndex, KlassOop receiver_klass, MethodOop method, std::int32_t level );


    bool isDatabaseScope() const {
        return true;
    }


    MethodOop method() const {
        return _method;
    }


    LookupKey *key() const {
        return _key;
    }


    KlassOop receiverKlass() const {
        return _receiver_klass;
    }


    bool isUncommonAt( std::int32_t byteCodeIndex ) const;

    bool isNotUncommonAt( std::int32_t byteCodeIndex ) const;


    // Mark a byteCodeIndex as uncommon (used when constructing database scopes)
    void mark_as_uncommon( std::int32_t byteCodeIndex ) {
        _uncommon->at_put( byteCodeIndex, true );
    }


    bool equivalent( InlinedScope *s ) const;

    bool equivalent( LookupKey *l ) const;

    void print();

    void print_short();
};

class UntakenRecompilationScope : public NonDummyRecompilationScope {
    //  send/inline cache that was never executed (either an empty ic or an untaken uncommon branch)
    const bool                     isUncommon;            // true iff untaken uncommon branch
    const ProgramCounterDescriptor *pc;
public:
    UntakenRecompilationScope( NonDummyRecompilationScope *sender, ProgramCounterDescriptor *pc, bool isUnlikely );


    bool isUntakenScope() const {
        return true;
    }


    bool isUnlikely() const {
        return isUncommon;
    }


    bool equivalent( InlinedScope *s ) const {
        return false;
    }


    bool equivalent( LookupKey *l ) const {
        return false;
    }


    Expression *receiverExpression( PseudoRegister *p ) const;


    MethodOop method() const {
        ShouldNotCallThis();
        return nullptr;
    }


    LookupKey *key() const {
        ShouldNotCallThis();
        return nullptr;
    }


    KlassOop receiverKlass() const {
        ShouldNotCallThis();
        return nullptr;
    }


    void extend() {
    }


    void print();

    void print_short();

protected:
    std::int32_t scopeID() const {
        return pc->_scope;
    }
};


class RUncommonBranch : public PrintableResourceObject {
    // represents a taken uncommon branch
public:
    RecompilationScope       *scope;
    ProgramCounterDescriptor *programCounterDescriptor;        // where the trap instruction is

    RUncommonBranch( RecompilationScope *r, ProgramCounterDescriptor *pc ) {
        scope                    = r;
        programCounterDescriptor = pc;
    }


    std::int32_t byteCodeIndex() const {
        return programCounterDescriptor->_byteCodeIndex;
    }


    void print();
};
