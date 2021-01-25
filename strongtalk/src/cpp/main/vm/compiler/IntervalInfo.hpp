//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/runtime/ResourceObject.hpp"


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


    MethodInterval *interval() const {
        return _interval;
    }


    InlinedScope *scope() const {
        return _scope;
    }


    bool_t dominates( std::int32_t byteCodeIndex, IntervalInfo *other, std::int32_t otherByteCodeIndex ) const; // does bytecode (receiver, byteCodeIndex) dominate (other, otherByteCodeIndex)?
    bool_t isParentOf( IntervalInfo *other ) const; // is receiver a parent of other?

    void print();

};
