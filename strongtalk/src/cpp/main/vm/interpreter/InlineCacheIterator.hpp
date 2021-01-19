
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/code/CompiledInlineCache.hpp"

//
// InlineCacheShape describes the (logical) shape in which an InlineCache at a particular
// call site apears; i.e. how many receivers have been registered in that InlineCache.
//
// Note: InlineCacheShape is the LOGICAL shape and it does not necessarily
// correspond with the physical implementation of a call site; e.g.
// a call site may contain a pic but nevertheless hold only one
// receiver and therefore be monomorphic!
//

enum class InlineCacheShape {

    anamorphic,     // send has never been executed => no type information (size = 0)
    monomorphic,    // only one receiver type available	(size = 1)
    polymorphic,    // more than one receiver type available (size > 1)
    megamorphic     // many receiver types, only last one is available (size = 1)

};


// InlineCacheIterator is the abstract superclass of all InlineCache iterators in the system.
// It SHOULD BE USED whenever iteration over an inline cache is required.

class InlineCacheIterator : public PrintableResourceObject {

public:
    // InlineCache information
    virtual int number_of_targets() const = 0;

    virtual InlineCacheShape shape() const = 0;

    virtual SymbolOop selector() const = 0;


    virtual InterpretedInlineCache *interpreted_ic() const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual CompiledInlineCache *compiled_ic() const {
        ShouldNotCallThis();
        return nullptr;
    }


    virtual bool_t is_interpreted_ic() const {
        return false;
    }    // is sender interpreted?
    virtual bool_t is_compiled_ic() const {
        return false;
    }    // is sender compiled?
    virtual bool_t is_super_send() const = 0;            // is super send?

    // Iterating through entries
    virtual void init_iteration() = 0;

    virtual void advance() = 0;

    virtual bool_t at_end() const = 0;

    // Accessing entries
    virtual KlassOop klass() const = 0;

    virtual bool_t is_interpreted() const = 0;    // is current target interpreted?
    virtual bool_t is_compiled() const = 0;    // is current target compiled?

    virtual MethodOop interpreted_method() const = 0;    // target methodOop (always non-nullptr)
    virtual NativeMethod *compiled_method() const = 0;    // target NativeMethod; nullptr if interpreted

    // methods for direct access to ith element (will set iteration state to i)
    void goto_elem( int i );

    MethodOop interpreted_method( int i );

    NativeMethod *compiled_method( int i );

    KlassOop klass( int i );

};


// InlineCache is the implementation independent representation for ICs. It allows manipulation
// of ICs without dealing with concrete interpreted or compiled ICs and it SHOULD BE
// USED instead whenever possible.
//
// Class hierarchy:
//
// InlineCache ---- uses ---->	InlineCacheIterator
//			  InterpretedInlineCacheIterator --- uses --->	InterpretedInlineCache
//			  CompiledInlineCacheIterator    --- uses --->	CompiledInlineCache & PolymorphicInlineCacheIterator
//
// (Note: One could also imagine InlineCache being the abstract super class of InterpretedInlineCache
// and CompiledInlineCache, which in turn are using InterpretedInlineCacheIterator and CompiledInlineCacheIterator.
// However, right now, CompiledInlineCache shares a common super class with PrimitiveInlineCache, and
// furthermore, the real ICs are composed out of at least 2 classes, one for the
// monormorphic case and one for the polymorphic case. Having InlineCache based on the
// iterator seems to simplify this.)

class InlineCache : public PrintableResourceObject {
private:
    const InlineCacheIterator *_iter;

public:
    InlineCache( InlineCacheIterator *iter ) :
            _iter( iter ) {
    }


    InlineCache( CompiledInlineCache *ic );

    InlineCache( InterpretedInlineCache *ic );


    InlineCacheIterator *iterator() const {
        return (InlineCacheIterator *) _iter;
    }


    // InlineCache information
    int number_of_targets() const {
        return _iter->number_of_targets();
    }


    InlineCacheShape shape() const {
        return _iter->shape();
    }


    SymbolOop selector() const {
        return _iter->selector();
    }


    InterpretedInlineCache *interpreted_ic() const {
        return _iter->interpreted_ic();
    }


    CompiledInlineCache *compiled_ic() const {
        return _iter->compiled_ic();
    }


    bool_t is_interpreted_ic() const {
        return _iter->is_interpreted_ic();
    }


    bool_t is_compiled_ic() const {
        return _iter->is_compiled_ic();
    }


    bool_t is_super_send() const {
        return _iter->is_super_send();
    }


    GrowableArray<KlassOop> *receiver_klasses() const;

    // InlineCache manipulation
    void replace( NativeMethod *nm );        // replace entry matching nm's key with nm

    // Debugging
    void print();
};



// A PolymorphicInlineCacheIterator is used get information out of a PolymorphicInlineCache.
// It is used to display PICs visually, to generate new PICs out of old ones and for GC purposes.
// In the case of a MegamorphicInlineCache, the PolymorphicInlineCacheIterator will be at_end after setup (no entries).

class PolymorphicInlineCacheIterator : public PrintableResourceObject {
public:
    enum State {
        at_smi_nativeMethod, at_nativeMethod, at_methodOop, at_the_end
    };

private:
    PolymorphicInlineCache *_pic;                  // the PolymorphicInlineCache over which is iterated
    char                   *_pos;                  // the current iterator position
    enum State _state;                  // current iterator state
    int        _methodOop_counter;      // remaining no. of methodOop entries

    int *nativeMethod_disp_addr() const;    // valid if state() in {at_smi_nativeMethod, at_nativeMethod}
    void computeNextState();

public:
    PolymorphicInlineCacheIterator( PolymorphicInlineCache *pic );

    // Iterating through PolymorphicInlineCache entries
    void advance();


    State state() const {
        return _state;
    }


    bool_t at_end() const {
        return state() == at_the_end;
    }


    // Accessing PolymorphicInlineCache entries
    KlassOop get_klass() const;

    char *get_call_addr() const;

    bool_t is_interpreted() const;

    bool_t is_compiled() const;

    MethodOop interpreted_method() const;

    NativeMethod *compiled_method() const;

    // Modifying PolymorphicInlineCache entries
    void set_klass( KlassOop klass );

    void set_nativeMethod( NativeMethod *nm );

    void set_methodOop( MethodOop method );

    // Must be public for oops_do in CompiledPIC
    MethodOop *methodOop_addr() const;         // valid if state() is at_PolymorphicInlineCache_methodOop
    KlassOop *klass_addr() const;              // valid if state() in {at_nativeMethod, at_methodOop}

    // Debugging
    void print();
};


class CompiledInlineCacheIterator : public InlineCacheIterator {
private:
    CompiledInlineCache            *_ic;
    PolymorphicInlineCacheIterator *_picit;

    int              _number_of_targets;    // the no. of InlineCache entries
    InlineCacheShape _shape;                // shape of inline cache
    int              _index;                // the next entry no.

public:
    CompiledInlineCacheIterator( CompiledInlineCache *ic );


    // InlineCache information
    bool_t is_compiled_ic() const {
        return true;
    }


    bool_t is_super_send() const;


    int number_of_targets() const {
        return _number_of_targets;
    }


    InlineCacheShape shape() const {
        return _shape;
    }


    SymbolOop selector() const {
        return _ic->selector();
    }


    CompiledInlineCache *compiled_ic() const {
        return _ic;
    }


    // Iterating through entries
    void init_iteration();

    void advance();            // advance iterator to next target
    bool_t at_end() const {
        return _index >= number_of_targets();
    }


    // Accessing entries
    KlassOop klass() const;            // receiver klass of current target

    bool_t is_interpreted() const;          // is current target interpreted?
    bool_t is_compiled() const;

    MethodOop interpreted_method() const;   // current target method (whether compiled or not)
    NativeMethod *compiled_method() const; // current compiled target or nullptr if interpreted

    // Debugging
    void print();
};
