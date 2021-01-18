//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/IntervalInfo.hpp"


IntervalInfo::IntervalInfo( MethodInterval *interval, InlinedScope *scope ) {
    _interval = interval;
    _scope    = scope;
}


bool_t IntervalInfo::isParentOf( IntervalInfo *other ) const {

    MethodInterval *parent = other->interval()->parent();

    while ( parent and parent not_eq _interval )
        parent = parent->parent();

    return parent not_eq nullptr;
}


bool_t IntervalInfo::dominates( int byteCodeIndex, IntervalInfo *other, int otherByteCodeIndex ) const {

    // "x dominates y" --> "if y is executed, x was executed earlier"
    // Bytecode byteCodeIndex1 in interval i1 dominates bytecode byteCodeIndex2 in interval i2 if
    // - i1 == i2 and byteCodeIndex1 <= byteCodeIndex2  (i1 == i2 implies straight-line code)
    // - i1 is a parent of i2 and byteCodeIndex1 < byteCodeIndex2
    //   (e.g., i1 is a method and i2 a loop body or if condition)

    if ( this == other and byteCodeIndex <= otherByteCodeIndex )
        return true;

    return isParentOf( other ) and byteCodeIndex < otherByteCodeIndex;
}


void IntervalInfo::print() {
    lprintf( "((IntervalInfo*)%#x\n", this );
}
