//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/OopDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"

class ProgramCounterDescriptorNode : public ResourceObject {
public:
    int       _pcOffset;
    ScopeInfo _scopeInfo;
    int       _byteCodeIndex;
};

class ProgramCounterDescriptorInfoClass : public ResourceObject {

protected:
    ProgramCounterDescriptorNode *_nodes;
    int _end;
    std::size_t _size;

public:
    ProgramCounterDescriptorInfoClass( std::size_t size );

    int length() {
        return _end;
    }

    void extend( std::size_t newSize );

    void add( int pcOffset, ScopeInfo scope, int byteCodeIndex );

    void mark_scopes();

    void copy_to( int *&addr );
};
