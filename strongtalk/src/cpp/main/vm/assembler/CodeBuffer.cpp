//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/code/RelocationInformation.hpp"
#include "vm/assembler/CodeBuffer.hpp"
#include "vm/assembler/MacroAssembler.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/utilities/disassembler.hpp"


MacroAssembler *theMacroAssembler = nullptr;


CodeBuffer::CodeBuffer( int instsSize, int locsSize ) {
    _codeStart         = _codeEnd = new_resource_array<char>( instsSize );
    _codeOverflow      = _codeStart + instsSize;
    _locsStart         = _locsEnd = (RelocationInformation *) new_resource_array<char>( locsSize );
    _locsOverflow      = (RelocationInformation *) ( (const char *) _locsStart + locsSize );
    _last_reloc_offset = code_size();
    _decode_begin      = nullptr;
}


CodeBuffer::CodeBuffer( const char *code_start, int code_size ) {
    _codeStart         = (const char *) code_start;
    _codeEnd           = (char *) code_start;
    _codeOverflow      = _codeStart + code_size;
    _locsStart         = _locsEnd = nullptr;
    _locsOverflow      = nullptr;
    _last_reloc_offset = CodeBuffer::code_size();
    _decode_begin      = nullptr;
}


const char *CodeBuffer::decode_begin() {
    return _decode_begin == nullptr ? (const char *) _codeStart : _decode_begin;
}


void CodeBuffer::set_code_end( const char *end ) {
    if ( (const char *) end < _codeStart or _codeOverflow < (const char *) end ) st_fatal( "code end out of bounds" );
    _codeEnd = (char *) end;
}


void CodeBuffer::relocate( const char *at, RelocationInformation::RelocationType rtype ) {
    st_assert( code_begin() <= at and at <= code_end(), "cannot relocate data outside code boundaries" );
    if ( _locsEnd == nullptr ) {
        // no Space for relocation information provided => code cannot be relocated
        // make sure that relocate is only called with rtypes that can be ignored for
        // this kind of code.
        st_assert( rtype == RelocationInformation::RelocationType::none or rtype == RelocationInformation::RelocationType::runtime_call_type or rtype == RelocationInformation::RelocationType::external_word_type, "code needs relocation information" );

    } else {
        st_assert( sizeof( RelocationInformation ) == sizeof( int16_t ), "change this code" );
        int len    = at - _codeStart;
        int offset = len - _last_reloc_offset;
        _last_reloc_offset = len;
        *_locsEnd++ = RelocationInformation( rtype, offset );
        if ( _locsEnd >= _locsOverflow ) st_fatal( "routine is too long to compile" );
    }
}


void CodeBuffer::decode() {
    Disassembler::decode( (const char *) decode_begin(), (const char *) code_end() );
    _decode_begin = code_end();
}


void CodeBuffer::decode_all() {
    Disassembler::decode( (const char *) code_begin(), (const char *) code_end() );
}


void CodeBuffer::copyTo( NativeMethod *nm ) {

    const char hlt = '\xF4';

    while ( code_size() % oopSize not_eq 0 )
        *_codeEnd++ = hlt; // align code

    while ( reloc_size() % oopSize not_eq 0 )
        *_locsEnd++ = RelocationInformation( RelocationInformation::RelocationType::oop_type, 0 ); // align relocation info

    copy_oops( (Oop *) _codeStart, (Oop *) nm->instructionsStart(), code_size() / oopSize );
    copy_oops( (Oop *) _locsStart, (Oop *) nm->locs(), reloc_size() / oopSize );

    // Fix the pc relative information after the move
    int delta = (const char *) _codeStart - (const char *) nm->instructionsStart();
    nm->fix_relocation_at_move( delta );
}


void CodeBuffer::print() {
    _console->print_cr( "CodeBuffer: Code  [0x%x <- used -> 0x%x[ <- free -> 0x%x[", _codeStart, _codeEnd, _codeOverflow );
    _console->print_cr( "CodeBuffer: Reloc [0x%x <- used -> 0x%x[ <- free -> 0x%x[", _locsStart, _locsEnd, _locsOverflow );
}
