
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/os.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/interpreter/MethodIntervalFactory.hpp"


// The MethodIterator iterates over the byte code structures of a methodOop
// Usage:
//    MethodIterator(method, &SomeMethodClosure);


// When creating a block closure, AllocationType specifies what is used in the context field of that block closure.
// When value is sent to the block, the context field is copied into the activation frame of the block.

enum class AllocationType {
    tos_as_scope,           // top of stack is used as context (usually nil or self)
    context_as_scope        // context of current stack frame (i.e. content of temp0) is used a context
};


class MethodClosure;


// A MethodIterator iterates over a MethodInterval and dispatches calls to the provided MethodClosure
class MethodIterator : StackAllocatedObject {

private:
    void dispatch( MethodClosure *blk );

    void unknown_code( std::uint8_t code );

    void should_never_encounter( std::uint8_t code );

    MethodInterval               *_interval;
    static MethodIntervalFactory defaultFactory;      // default factory

public:
    static AbstractMethodIntervalFactory *factory;      // used to build nodes

    MethodIterator( MethodOop m, MethodClosure *blk, AbstractMethodIntervalFactory *f = &defaultFactory );

    MethodIterator( MethodInterval *interval, MethodClosure *blk, AbstractMethodIntervalFactory *f = &defaultFactory );

    MethodIterator() = default;
    virtual ~MethodIterator() = default;
    MethodIterator( const MethodIterator & ) = default;
    MethodIterator &operator=( const MethodIterator & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    MethodInterval *interval() const {
        return _interval;
    }
};
