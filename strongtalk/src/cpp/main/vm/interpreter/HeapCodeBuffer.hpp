/* Copyright (c) 2010, Stephen Rees 
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
	  disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Sun Microsystems nor the names of its contributors may be used to endorse or promote products derived 
	  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT 
NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/


#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"


class HeapCodeBuffer : public ResourceObject {

private:
    GrowableArray<std::uint32_t> *_bytes;
    GrowableArray<Oop>           *_oops;

    void align();

    bool isAligned();

public:
    HeapCodeBuffer() :
        _bytes{ new GrowableArray<std::uint32_t>() },
        _oops{ new GrowableArray<Oop>() } {
    }
    virtual ~HeapCodeBuffer() = default;
    HeapCodeBuffer( const HeapCodeBuffer & ) = default;
    HeapCodeBuffer &operator=( const HeapCodeBuffer & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    void pushByte( std::uint8_t op );

    void pushOop( Oop arg );


    std::int32_t byteLength() {
        return _bytes->length();
    }


    std::int32_t oopLength() {
        return _oops->length();
    }


    ByteArrayOop bytes();

    ObjectArrayOop oops();

};
