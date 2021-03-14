
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/interpreter/MethodInterval.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"


IntervalInfo::IntervalInfo( MethodInterval *interval, InlinedScope *scope ) :
    _interval{ interval },
    _scope{ scope } {
}


bool IntervalInfo::isParentOf( IntervalInfo *other ) const {

    MethodInterval *parent = other->interval()->parent();

    while ( parent and parent not_eq _interval ) {
        parent = parent->parent();
    }

    return parent not_eq nullptr;
}


bool IntervalInfo::dominates( std::int32_t byteCodeIndex, IntervalInfo *other, std::int32_t otherByteCodeIndex ) const {

    // "x dominates y" --> "if y is executed, x was executed earlier"
    // Bytecode byteCodeIndex1 in interval i1 dominates bytecode byteCodeIndex2 in interval i2 if
    // - i1 == i2 and byteCodeIndex1 <= byteCodeIndex2  (i1 == i2 implies straight-line code)
    // - i1 is a parent of i2 and byteCodeIndex1 < byteCodeIndex2
    //   (e.g., i1 is a method and i2 a loop body or if condition)

    if ( this == other and byteCodeIndex <= otherByteCodeIndex ) {
        return true;
    }

    return isParentOf( other ) and byteCodeIndex < otherByteCodeIndex;
}


void IntervalInfo::print() {
    SPDLOG_INFO( "(IntervalInfo*){0:x}", static_cast<void *>(this) );
}


MethodInterval::MethodInterval( MethodOop method, MethodInterval *parent ) :
    _method{ method },
    _parent{ parent },
    _begin_byteCodeIndex{ 1 },
    _end_byteCodeIndex{ method->end_byteCodeIndex() },
    _in_primitive_failure{ false },
    _info{ nullptr } {
    initialize( method, parent, 1, method->end_byteCodeIndex(), false );
}


MethodInterval::MethodInterval( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex, bool failBlock ) :
    _method{ method },
    _parent{ parent },
    _begin_byteCodeIndex{ 1 },
    _end_byteCodeIndex{ end_byteCodeIndex },
    _in_primitive_failure{ failBlock },
    _info{ nullptr } {
    initialize( method, parent, begin_byteCodeIndex, end_byteCodeIndex, failBlock );
}


void MethodInterval::initialize( MethodOop method, MethodInterval *parent, std::int32_t begin_byteCodeIndex, std::int32_t end_byteCodeIndex, bool failBlock ) {
    _method               = method;
    _parent               = parent;
    _begin_byteCodeIndex  = begin_byteCodeIndex;
    _end_byteCodeIndex    = end_byteCodeIndex;
    _in_primitive_failure = failBlock;
    _info                 = nullptr;
}
