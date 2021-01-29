//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/assembler/Location.hpp"
#include "vm/runtime/ResourceObject.hpp"


// A NameDescriptor describes the source-level value of a name in some Delta scope
// (e.g. an argument, local, or expression stack entry).
// NameDescriptor objects are usually stored in the Zone as part of the debugging information.

class NameDescriptor : public PrintableResourceObject { // ResourceObject because some are created on-the-fly

public:
    virtual bool isLocation() const {
        return false;
    }


    virtual bool isValue() const {
        return false;
    }


    virtual bool isBlockValue() const {
        return false;
    }


    virtual bool isMemoizedBlock() const {
        return false;
    }


    virtual bool isIllegal() const {
        return false;
    }


    virtual bool hasLocation() const {
        return false;
    }


    virtual Location location() const {
        SubclassResponsibility();
        return Location::UNALLOCATED_LOCATION;
    }


    virtual Oop value( const Frame *f = nullptr ) const {
        SubclassResponsibility();
        return nullptr;
    }


    virtual bool verify() {
        return true;
    }


    virtual void print() = 0;


    virtual bool equal( NameDescriptor *other ) const {
        return false;
    }


    std::int32_t offset;
};

// something stored at a location
struct LocationNameDescriptor : public NameDescriptor {
    Location _location;


    LocationNameDescriptor( Location loc ) {
        _location = loc;
    }


    bool isLocation() const {
        return true;
    }


    Location location() const {
        return _location;
    }


    bool hasLocation() const {
        return true;
    }


    bool equal( NameDescriptor *other ) const;

    void print();
};

// a run-time constant
struct ValueNameDescriptor : public NameDescriptor {
    Oop _v;


    ValueNameDescriptor( Oop v ) {
        _v = v;
    }


    bool isValue() const {
        return true;
    }


    Oop value( const Frame *f = nullptr ) const {
        return _v;
    }


    bool equal( NameDescriptor *other ) const;

    void print();
};


class ScopeDescriptor;

// a block closure "constant", i.e., a block that has been optimized away
struct BlockValueNameDescriptor : public NameDescriptor {
    MethodOop       _blockMethod;
    ScopeDescriptor *_parentScope;


    BlockValueNameDescriptor( MethodOop block_method, ScopeDescriptor *parent_scope ) {
        _blockMethod = block_method;
        _parentScope = parent_scope;
    }


    bool isBlockValue() const {
        return true;
    }


    // Returns a blockClosureOop
    // There are two cases:
    // 1. During deoptmization, where the contextOop referred by the block must be canonicalized to preserve language semantics.
    // 2. Normal operation (use during stack tracing etc.), where contextOop canonicalization is not needed.
    Oop value( const Frame *f = nullptr ) const;


    MethodOop block_method() const {
        return _blockMethod;
    }


    ScopeDescriptor *parent_scope() const {
        return _parentScope;
    }


    bool equal( NameDescriptor *other ) const;

    void print();
};

// a block closure that may or may not be created at runtime, so location l contains either the real block or a dummy block
struct MemoizedBlockNameDescriptor : public NameDescriptor {
    Location        _location;
    MethodOop       _blockMethod;
    ScopeDescriptor *_parentScope;


    MemoizedBlockNameDescriptor( Location loc, MethodOop block_method, ScopeDescriptor *parent_scope ) {
        _location    = loc;
        _blockMethod = block_method;
        _parentScope = parent_scope;
    }


    bool isMemoizedBlock() const {
        return true;
    }


    Location location() const {
        return _location;
    }


    MethodOop block_method() const {
        return _blockMethod;
    }


    ScopeDescriptor *parent_scope() const {
        return _parentScope;
    }


    static Oop uncreatedBlockValue() {
        return smiOop_zero;
    }


    bool hasLocation() const {
        return true;
    }


    Oop value( const Frame *f = nullptr ) const;

    bool equal( NameDescriptor *other ) const;

    void print();
};

struct IllegalNameDescriptor : public NameDescriptor {

    IllegalNameDescriptor() {
    }


    bool isIllegal() const {
        return true;
    }


    bool equal( NameDescriptor *other ) const;

    void print();
};
