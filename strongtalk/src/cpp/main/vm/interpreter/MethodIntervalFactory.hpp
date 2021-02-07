
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/interpreter/MethodNode.hpp"


// factory to parameterize construction of nodes
class AbstractMethodIntervalFactory : StackAllocatedObject {

public:
    AbstractMethodIntervalFactory() = default;
    ~AbstractMethodIntervalFactory() = default;


    void operator delete( void *ptr ) {}


public:
    virtual MethodInterval *new_MethodInterval( MethodOop method, MethodInterval *parent ) = 0;

    virtual MethodInterval *new_MethodInterval( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex = -1, bool failureBlock = false ) = 0;

    virtual AndNode *new_AndNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) = 0;

    virtual OrNode *new_OrNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) = 0;

    virtual WhileNode *new_WhileNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t cond_offset, std::int32_t end_offset ) = 0;

    virtual IfNode *new_IfNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool cond, std::int32_t else_offset, std::uint8_t structure ) = 0;

    virtual PrimitiveCallNode *new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc ) = 0;

    virtual PrimitiveCallNode *new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc, std::int32_t end_offset ) = 0;

    virtual DLLCallNode *new_DLLCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, Interpreted_DLLCache *cache ) = 0;
};


// default factory (used by everyone except the compiler)
class MethodIntervalFactory : public AbstractMethodIntervalFactory {

public:
    MethodIntervalFactory() = default;
    ~MethodIntervalFactory() = default;


    void operator delete( void *ptr ) {}


public:
    MethodInterval *new_MethodInterval( MethodOop method, MethodInterval *parent );

    MethodInterval *new_MethodInterval( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex = -1, bool failureBlock = false );

    AndNode *new_AndNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );

    OrNode *new_OrNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset );

    WhileNode *new_WhileNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t cond_offset, std::int32_t end_offset );

    IfNode *new_IfNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool cond, std::int32_t else_offset, std::uint8_t structure );

    PrimitiveCallNode *new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc );

    PrimitiveCallNode *new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc, std::int32_t end_offset );

    DLLCallNode *new_DLLCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, Interpreted_DLLCache *cache );
};
