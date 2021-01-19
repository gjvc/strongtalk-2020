//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"


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
    const int _senderByteCodeIndex;

public:
    int _invocationCount;        // estimated # of invocations (-1 == unknown)

    RecompilationScope( NonDummyRecompilationScope *s, int byteCodeIndex );


    // Type tests
    virtual bool_t isInterpretedScope() const {
        return false;
    }


    virtual bool_t isInlinedScope() const {
        return false;
    }


    virtual bool_t isNullScope() const {
        return false;
    }


    virtual bool_t isUninlinableScope() const {
        return false;
    }


    virtual bool_t isPICScope() const {
        return false;
    }


    virtual bool_t isUntakenScope() const {
        return false;
    }


    virtual bool_t isDatabaseScope() const {
        return false;
    }


    virtual bool_t isCompiled() const {
        return false;
    }


    NonDummyRecompilationScope *sender() const {
        return (NonDummyRecompilationScope *) _sender;
    }


    int senderByteCodeIndex() const {
        return _senderByteCodeIndex;
    }


    virtual bool_t equivalent( InlinedScope *s ) const = 0;

    virtual bool_t equivalent( LookupKey *l ) const = 0;


    virtual RecompilationScope *subScope( int byteCodeIndex, LookupKey *l ) const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual GrowableArray<RecompilationScope *> *subScopes( int byteCodeIndex ) const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual bool_t hasSubScopes( int byteCodeIndex ) const {
        ShouldNotCallThis();
        return false;
    }


    virtual bool_t isUncommonAt( int byteCodeIndex ) const {
        ShouldNotCallThis();
        return false;
    }


    // was send at this byteCodeIndex uncommon (never taken) in recompilee?  (false means "don't know")
    virtual bool_t isNotUncommonAt( int byteCodeIndex ) const {
        ShouldNotCallThis();
        return false;
    }


    // return true iff the send at this byteCodeIndex *must not* be made uncommon
    virtual bool_t isLite() const {
        return false;
    }


    virtual NativeMethod *get_nativeMethod() const {
        return nullptr;
    }


    virtual Expression *receiverExpression( PseudoRegister *p ) const;

    virtual KlassOop receiverKlass() const = 0;

    virtual MethodOop method() const = 0;

    virtual LookupKey *key() const = 0;

    bool_t wasNeverExecuted() const;                // was method never executed?
    virtual void print();

    virtual void print_short() = 0;

    virtual void printTree( int byteCodeIndex, int level ) const;


    // Support for inlining database:
    // - returns the size of the inlining database tree.
    virtual int inlining_database_size() {
        return 0;
    }


    // - prints the inlining database tree on a stream.
    virtual void print_inlining_database_on( ConsoleOutputStream *stream, GrowableArray<ProgramCounterDescriptor *> *uncommon, int byteCodeIndex = -1, int level = 0 ) {
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


    NullRecompilationScope( NonDummyRecompilationScope *sender, int byteCodeIndex ) :
            RecompilationScope( sender, byteCodeIndex ) {
    }


    bool_t isNullScope() const {
        return true;
    }


    bool_t equivalent( InlinedScope *s ) const {
        return false;
    }


    bool_t equivalent( LookupKey *l ) const {
        return false;
    }


    RecompilationScope *subScope( int byteCodeIndex, LookupKey *l ) const {
        return (RecompilationScope *) this;
    }


    GrowableArray<RecompilationScope *> *subScopes( int byteCodeIndex ) const;


    bool_t hasSubScopes( int byteCodeIndex ) const {
        return false;
    }


    bool_t isUncommonAt( int byteCodeIndex ) const {
        return false;
    }


    bool_t isNotUncommonAt( int byteCodeIndex ) const {
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


    void printTree( int byteCodeIndex, int level ) const;

    void print_short();
};


class UninlinableRecompilationScope : public NullRecompilationScope {
    // scope marking "callee" of an uninlinable or megamorphic send
public:
    UninlinableRecompilationScope( NonDummyRecompilationScope *sender, int byteCodeIndex );        // for interpreted senders

    bool_t isUninlinableScope() const {
        return true;
    }


    bool_t isNullScope() const {
        return false;
    }


    void print_inlining_database_on( ConsoleOutputStream *stream, GrowableArray<ProgramCounterDescriptor *> *uncommon, int byteCodeIndex, int level );

    void print_short();
};


class RUncommonBranch;

class NonDummyRecompilationScope : public RecompilationScope {
    // abstract -- a non-dummy scope with subscopes
protected:
    const int                           _level;                // distance from root
    const int                           ncodes;                        // # byte codes in method
    GrowableArray<RecompilationScope *> **_subScopes;        // indexed by byteCodeIndex
public:
    GrowableArray<RUncommonBranch *> uncommon;    // list of uncommon branches

    NonDummyRecompilationScope( NonDummyRecompilationScope *s, int byteCodeIndex, MethodOop m, int level );

    RecompilationScope *subScope( int byteCodeIndex, LookupKey *l ) const;

    // return the subscope matching the lookup
    // return NullScope if no scope
    GrowableArray<RecompilationScope *> *subScopes( int byteCodeIndex ) const;

    // return all subscopes at byteCodeIndex
    bool_t hasSubScopes( int byteCodeIndex ) const;


    int level() const {
        return _level;
    }


    bool_t isUncommonAt( int byteCodeIndex ) const;

    bool_t isNotUncommonAt( int byteCodeIndex ) const;

    void addScope( int byteCodeIndex, RecompilationScope *s );

    void printTree( int byteCodeIndex, int level ) const;

    virtual void unify( NonDummyRecompilationScope *s );

    static int compare( NonDummyRecompilationScope **a, NonDummyRecompilationScope **b ); // for sorting

protected:
    virtual void constructSubScopes( bool_t trusted );


    virtual int scopeID() const {
        ShouldNotCallThis();
        return 0;
    }


    static bool_t trustPICs( MethodOop m );

    void printSubScopes() const;

    static NonDummyRecompilationScope *constructRScopes( const NativeMethod *nm, bool_t trusted = true, int level = 0 );

    friend class Compiler;

    friend class NativeMethod;

    friend class InliningDatabase;
};

class InterpretedRecompilationScope : public NonDummyRecompilationScope {
    // a scope corresponding to an interpreted method
    LookupKey *_key;
    MethodOop _method;
    bool_t    _is_trusted;            // is PolymorphicInlineCache info trusted?
    bool_t    extended;        // subScopes computed?
public:
    InterpretedRecompilationScope( NonDummyRecompilationScope *sender, int byteCodeIndex, LookupKey *key, MethodOop m, int level, bool_t trusted );


    bool_t isInterpretedScope() const {
        return true;
    }


    MethodOop method() const {
        return _method;
    }


    LookupKey *key() const {
        return _key;
    }


    KlassOop receiverKlass() const;

    bool_t equivalent( InlinedScope *s ) const;

    bool_t equivalent( LookupKey *l ) const;

    bool_t isUncommonAt( int byteCodeIndex ) const;


    bool_t isNotUncommonAt( int byteCodeIndex ) const {
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

    InlinedRecompilationScope( NonDummyRecompilationScope *s, int byteCodeIndex, const NativeMethod *nm, ScopeDescriptor *d, int level );


    bool_t isInlinedScope() const {
        return true;
    }


    bool_t isCompiled() const {
        return true;
    }


    MethodOop method() const;


    NativeMethod *get_nativeMethod() const {
        return (NativeMethod *) nm;
    }


    LookupKey *key() const;

    KlassOop receiverKlass() const;

    bool_t isLite() const;

    bool_t equivalent( LookupKey *l ) const;

    bool_t equivalent( InlinedScope *s ) const;

    void print();

    void print_short();

    int inlining_database_size();

    void print_inlining_database_on( ConsoleOutputStream *stream, GrowableArray<ProgramCounterDescriptor *> *uncommon, int byteCodeIndex, int level );

protected:
    void constructSubScopes() {
        ShouldNotCallThis();
    }


    int scopeID() const {
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
    const KlassOop klass;            // receiver klass
    const NativeMethod *nm;        // called NativeMethod (or nullptr if interpreted)
    const MethodOop _method;    // called method
    const bool_t    trusted;        // is PolymorphicInlineCache info trusted?
    bool_t          _extended;        // subScopes computed?
    const ScopeDescriptor *_desc;    // scope (or nullptr if interpreted)

public:
    PICRecompilationScope( const NativeMethod *caller, ProgramCounterDescriptor *pc, CompiledInlineCache *s, KlassOop k, ScopeDescriptor *d, NativeMethod *n, MethodOop m, int nsends, int level, bool_t trusted );


    bool_t isPICScope() const {
        return true;
    }


    bool_t isCompiled() const {
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

    bool_t isLite() const;

    bool_t equivalent( InlinedScope *s ) const;

    bool_t equivalent( LookupKey *l ) const;

    void unify( NonDummyRecompilationScope *s );

    void extend();

    void print();

    void print_short();

protected:
    int scopeID() const {
        return programCounterDescriptor->_scope;
    }


    static bool_t trustPICs( const NativeMethod *nm );

    friend NonDummyRecompilationScope *NonDummyRecompilationScope::constructRScopes( const NativeMethod *nm, bool_t trusted, int level );
};

class InliningDatabaseRecompilationScope : public NonDummyRecompilationScope {
    // a scope created from the inlining database
private:
    KlassOop  _receiver_klass;
    MethodOop _method;
    LookupKey             *_key;
    GrowableArray<bool_t> *_uncommon;  // list of uncommon branch
public:
    InliningDatabaseRecompilationScope( NonDummyRecompilationScope *sender, int byteCodeIndex, KlassOop receiver_klass, MethodOop method, int level );


    bool_t isDatabaseScope() const {
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


    bool_t isUncommonAt( int byteCodeIndex ) const;

    bool_t isNotUncommonAt( int byteCodeIndex ) const;


    // Mark a byteCodeIndex as uncommon (used when constructing database scopes)
    void mark_as_uncommon( int byteCodeIndex ) {
        _uncommon->at_put( byteCodeIndex, true );
    }


    bool_t equivalent( InlinedScope *s ) const;

    bool_t equivalent( LookupKey *l ) const;

    void print();

    void print_short();
};

class UntakenRecompilationScope : public NonDummyRecompilationScope {
    //  send/inline cache that was never executed (either an empty ic or an untaken uncommon branch)
    const bool_t isUncommon;            // true iff untaken uncommon branch
    const ProgramCounterDescriptor *pc;
public:
    UntakenRecompilationScope( NonDummyRecompilationScope *sender, ProgramCounterDescriptor *pc, bool_t isUnlikely );


    bool_t isUntakenScope() const {
        return true;
    }


    bool_t isUnlikely() const {
        return isUncommon;
    }


    bool_t equivalent( InlinedScope *s ) const {
        return false;
    }


    bool_t equivalent( LookupKey *l ) const {
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
    int scopeID() const {
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


    int byteCodeIndex() const {
        return programCounterDescriptor->_byteCodeIndex;
    }


    void print();
};
