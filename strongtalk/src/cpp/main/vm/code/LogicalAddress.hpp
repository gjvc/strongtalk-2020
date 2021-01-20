//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/code/NameNode.hpp"
#include "vm/runtime/ResourceObject.hpp"

class NameNode;

class ScopeDescriptorRecorder;

// A LogicalAddress describes a source level location.

// Since backend optimizations reshuffles code a source level location may change its physical location.

class LogicalAddress : public ResourceObject {

private:
    NameNode *_physicalAddress;
    int _pcOffset;
    LogicalAddress *_next;
    std::size_t _offset;

public:
    LogicalAddress( NameNode *physical_address, int pc_offset = 0 );


    NameNode *physical_address() const {
        return _physicalAddress;
    }


    int pc_offset() const {
        return _pcOffset;
    }


    LogicalAddress *next() const {
        return _next;
    }


    void append( NameNode *physical_address, int pc_offset );

    NameNode *physical_address_at( int pc_offset );

    void generate( ScopeDescriptorRecorder *rec );

    int length();
};
