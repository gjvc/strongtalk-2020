//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/OopDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"

class ProgramCounterDescriptorNode : public ResourceObject {
public:
    std::int32_t _pcOffset;
    ScopeInfo    _scopeInfo;
    std::int32_t _byteCodeIndex;
};


class ProgramCounterDescriptorInfoClass : public ResourceObject {

protected:
    ProgramCounterDescriptorNode *_nodes;
    std::int32_t                 _end;
    std::int32_t                 _size;

public:
    ProgramCounterDescriptorInfoClass( std::int32_t size );


    std::int32_t length() {
        return _end;
    }


    void extend( std::int32_t newSize );

    void add( std::int32_t pcOffset, ScopeInfo scope, std::int32_t byteCodeIndex );

    void mark_scopes();

    void copy_to( std::int32_t *&addr );
};
