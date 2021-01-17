//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/code/LogicalAddress.hpp"


LogicalAddress::LogicalAddress( NameNode * physical_address, int pc_offset ) {
    _physicalAddress = physical_address;
    _pcOffset        = pc_offset;
    _next            = nullptr;
    _offset          = -1; // Illegal value
}


void LogicalAddress::append( NameNode * physical_address, int pc_offset ) {
    if ( next() ) {
        // hopefully these lists are not getting too long...
        next()->append( physical_address, pc_offset );
    } else {
        st_assert( _pcOffset <= pc_offset, "checking progress" );
        _next = new LogicalAddress( physical_address, pc_offset );
    }
}


NameNode * LogicalAddress::physical_address_at( int pc_offset ) {
    LogicalAddress * current = this;
    while ( current->next() and current->next()->pc_offset() > pc_offset ) {
        current = current->next();
    }
    return current->physical_address();
}


int LogicalAddress::length() {
    LogicalAddress * current = this;
    int result = 1;
    while ( current->next() ) {
        current = current->next();
        result++;
    }
    return result;
}


void LogicalAddress::generate( ScopeDescriptorRecorder * rec ) {

    // emit:
    //  [first ]
    //  [next  ], offset
    // where last element has the is_last bit set.
    LogicalAddress * n = next();
    physical_address()->generate( rec, n == nullptr );

    int last_pc_offset = 0;
    while ( n not_eq nullptr ) {
        LogicalAddress * current = n;
        n = n->next();
        current->physical_address()->generate( rec, n == nullptr );
        st_assert( last_pc_offset <= current->pc_offset(), "checking progress" );
        rec->genValue( current->pc_offset() - last_pc_offset );
        last_pc_offset = current->pc_offset();
    }
}
