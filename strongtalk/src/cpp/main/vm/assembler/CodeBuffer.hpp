//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/RelocationInformation.hpp"
#include "vm/runtime/ResourceObject.hpp"


class CodeBuffer : public PrintableResourceObject {

private:
    const char *_codeStart;
    char       *_codeEnd;
    const char *_codeOverflow;

    RelocationInformation *_locsStart;
    RelocationInformation *_locsEnd;
    RelocationInformation *_locsOverflow;
    std::int32_t          _last_reloc_offset;

    const char *_decode_begin;

    const char *decode_begin();

public:
    CodeBuffer( const char *code_start, std::int32_t code_size );

    CodeBuffer( std::int32_t instsSize, std::int32_t locsSize );

    virtual ~CodeBuffer() = default;
    CodeBuffer( const CodeBuffer & ) = default;
    CodeBuffer &operator=( const CodeBuffer & ) = default;
    void operator delete( void *ptr ) { (void)ptr; }




    const char *code_begin() const {
        return _codeStart;
    };


    const char *code_end() const {
        return _codeEnd;
    }


    const char *code_limit() const {
        return _codeOverflow;
    }


    std::int32_t code_size() const {
        return _codeEnd - _codeStart;
    }


    std::int32_t reloc_size() const {
        return ( _locsEnd - _locsStart ) * sizeof( RelocationInformation );
    }


    void set_code_end( const char *end );

    void relocate( const char *at, RelocationInformation::RelocationType rtype );

    void decode();

    void decode_all();

    void copyTo( NativeMethod *nm );

    void print();
};
