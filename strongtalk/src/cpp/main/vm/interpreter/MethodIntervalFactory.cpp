
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/interpreter/MethodIntervalFactory.hpp"


MethodInterval *MethodIntervalFactory::new_MethodInterval( MethodOop method, MethodInterval *parent ) {
    return new MethodInterval( method, parent );
}


MethodInterval *MethodIntervalFactory::new_MethodInterval( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex, bool failureBlock ) {
    return new MethodInterval( method, parent, begin_byteCodeIndex, end_byteCodeIndex, failureBlock );
}


AndNode *MethodIntervalFactory::new_AndNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) {
    return new AndNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, dest_offset );
}


OrNode *MethodIntervalFactory::new_OrNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t dest_offset ) {
    return new OrNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, dest_offset );
}


WhileNode *MethodIntervalFactory::new_WhileNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, std::int32_t cond_offset, std::int32_t end_offset ) {
    return new WhileNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, cond_offset, end_offset );
}


IfNode *MethodIntervalFactory::new_IfNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool cond, std::int32_t else_offset, std::uint8_t structure ) {
    return new IfNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, cond, else_offset, structure );
}


PrimitiveCallNode *MethodIntervalFactory::new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc ) {
    return new PrimitiveCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, has_receiver, name, pdesc );
}


PrimitiveCallNode *MethodIntervalFactory::new_PrimitiveCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, bool has_receiver, SymbolOop name, PrimitiveDescriptor *pdesc, std::int32_t end_offset ) {
    return new PrimitiveCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, has_receiver, name, pdesc, end_offset );
}


DLLCallNode *MethodIntervalFactory::new_DLLCallNode( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t next_byteCodeIndex, Interpreted_DLLCache *cache ) {
    return new DLLCallNode( method, parent, begin_byteCodeIndex, next_byteCodeIndex, cache );
}
