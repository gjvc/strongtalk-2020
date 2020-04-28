//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/OopDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"

class ProgramCounterDescriptorNode : public ResourceObject {
    public:
        int _pcOffset;
        ScopeInfo _scopeInfo;
        int _byteCodeIndex;
};

class ProgramCounterDescriptorInfoClass : public ResourceObject {

    protected:
        ProgramCounterDescriptorNode * _nodes;
        int _end;
        int _size;

    public:
        ProgramCounterDescriptorInfoClass( int size );


        int length() {
            return _end;
        }


        void extend( int newSize );

        void add( int pcOffset, ScopeInfo scope, int byteCodeIndex );

        void mark_scopes();

        void copy_to( int *& addr );
};

