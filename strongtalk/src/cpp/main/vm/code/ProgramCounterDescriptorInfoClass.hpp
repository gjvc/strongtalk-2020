//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oop/OopDescriptor.hpp"
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
    std::size_t                 _end;
    std::size_t                 _size;

public:
    ProgramCounterDescriptorInfoClass( std::size_t size );

    ProgramCounterDescriptorInfoClass() = default;
    virtual ~ProgramCounterDescriptorInfoClass() = default;
    ProgramCounterDescriptorInfoClass( const ProgramCounterDescriptorInfoClass & ) = default;
    ProgramCounterDescriptorInfoClass &operator=( const ProgramCounterDescriptorInfoClass & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    std::size_t length() {
        return _end;
    }


    void extend( std::size_t newSize );

    void add( std::int32_t pcOffset, ScopeInfo scope, std::size_t byteCodeIndex );

    void mark_scopes();

    void copy_to( std::size_t *&addr );
};
