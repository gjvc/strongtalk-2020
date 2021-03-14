
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/platform/os.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/runtime/ResourceObject.hpp"


class IntervalInfo;

// A MethodInterval represents an interval of byte codes
class MethodInterval : public ResourceObject {

protected:
    MethodOop      _method;                 //
    MethodInterval *_parent;                // enclosing interval (or nullptr if top-level)
    std::int32_t   _begin_byteCodeIndex;    //
    std::int32_t   _end_byteCodeIndex;      //
    bool           _in_primitive_failure;   // currently in primitive failure block?
    IntervalInfo   *_info;                  //

    void initialize( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex, bool failBlock );


    void set_end_byteCodeIndex( std::int32_t byteCodeIndex ) {
        _end_byteCodeIndex = byteCodeIndex;
    }


    // Constructors
    MethodInterval( MethodOop method, MethodInterval *parent );

    MethodInterval( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex = -1, bool failureBlock = false );

    friend class MethodIntervalFactory;

public:

    MethodInterval() = default;
    virtual ~MethodInterval() = default;
    MethodInterval( const MethodInterval & ) = default;
    MethodInterval &operator=( const MethodInterval & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    // Test operations
    bool includes( std::int32_t byteCodeIndex ) const {
        return begin_byteCodeIndex() <= byteCodeIndex and byteCodeIndex < end_byteCodeIndex();
    }


    // Accessor operations
    MethodOop method() const {
        return _method;
    }


    std::int32_t begin_byteCodeIndex() const {
        return _begin_byteCodeIndex;
    }


    std::int32_t end_byteCodeIndex() const {
        return _end_byteCodeIndex;
    }


    MethodInterval *parent() const {
        return _parent;
    }


    // primitive failure block recognition (for inlining policy of compiler)
    bool in_primitive_failure_block() const {
        return _in_primitive_failure;
    }


    void set_primitive_failure( bool f ) {
        _in_primitive_failure = f;
    }


    // compiler support (not fully implemented/used yet)
    IntervalInfo *info() const {
        return _info;
    }


    void set_info( IntervalInfo *i ) {
        _info = i;
    }
};



//class IntervalInfo; // #include "vm/compiler/IntervalInfo.hpp"

// A IntervalInfo contains compiler-related data/functionality that is associated with a MethodInterval.

// (It's not in MethodInterval itself to avoid cluttering it up.)
// The main purpose of IntervalInfo is to exploit the method structure for optimization (esp. to get a cheap "dominates" relationship).
// This code is only halfway finished -- not used yet.  -Urs 9/96


class IntervalInfo : public PrintableResourceObject {

private:
    MethodInterval *_interval;      // my interval
    InlinedScope   *_scope;         // my scope

public:
    IntervalInfo( MethodInterval *interval, InlinedScope *scope );
    IntervalInfo() = default;
    virtual ~IntervalInfo() = default;
    IntervalInfo( const IntervalInfo & ) = default;
    IntervalInfo &operator=( const IntervalInfo & ) = default;


    MethodInterval *interval() const {
        return _interval;
    }


    InlinedScope *scope() const {
        return _scope;
    }


    bool dominates( std::int32_t byteCodeIndex, IntervalInfo *other, std::int32_t otherByteCodeIndex ) const; // does bytecode (receiver, byteCodeIndex) dominate (other, otherByteCodeIndex)?
    bool isParentOf( IntervalInfo *other ) const; // is receiver a parent of other?

    void print();

};
