//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/SavedRegisters.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/utilities/OutputStream.hpp"


// Need to be static so they can be accessed in assembly code
// of SavedRegisters::save_registers() (compiler doesn't accept
// static class variables).

static std::int32_t *stored_frame_pointer = nullptr;
static Oop          saved_eax;
static Oop          saved_ecx;
static Oop          saved_edx;
static Oop          saved_ebx;
static Oop          saved_esi;
static Oop          saved_edi;


Oop SavedRegisters::fetch( std::int32_t register_number, std::int32_t *frame_pointer ) {
    if ( frame_pointer not_eq stored_frame_pointer ) {
        SPDLOG_INFO( "Cannot fetch register from non-bottom frame:" );
        SPDLOG_INFO( " register number = {}, fp = 0x%lx", register_number, static_cast<const void *>(frame_pointer) );
        st_fatal( "vm aborted" );
    }
    if ( register_number == eax.number() )
        return saved_eax;
    if ( register_number == ecx.number() )
        return saved_ecx;
    if ( register_number == edx.number() )
        return saved_edx;
    if ( register_number == ebx.number() )
        return saved_ebx;
    if ( register_number == esi.number() )
        return saved_esi;
    if ( register_number == edi.number() )
        return saved_edi;
    st_fatal( "cannot fetch esp or ebp from saved registers" );
    return nullptr;
}


void SavedRegisters::clear() {
    stored_frame_pointer = nullptr;
}


/*
#define Naked __declspec( naked )
Naked void SavedRegisters::save_registers() {
  __asm {
    // save the registers
    mov	saved_eax, eax
    mov	saved_ecx, ecx
    mov	saved_edx, edx
    mov	saved_ebx, ebx
    mov	saved_esi, esi
    mov	saved_edi, edi
    // save frame pointer w/o destroying any register contents
    mov eax, last_Delta_fp
    mov stored_frame_pointer, eax
    mov eax, saved_eax
    // return
    ret
  }
}
#undef Naked
*/

void SavedRegisters::generate_save_registers( MacroAssembler *masm ) {
    // save the registers
    masm->movl( Address( (std::int32_t) &saved_eax, RelocationInformation::RelocationType::external_word_type ), eax );
    masm->movl( Address( (std::int32_t) &saved_ecx, RelocationInformation::RelocationType::external_word_type ), ecx );
    masm->movl( Address( (std::int32_t) &saved_edx, RelocationInformation::RelocationType::external_word_type ), edx );
    masm->movl( Address( (std::int32_t) &saved_ebx, RelocationInformation::RelocationType::external_word_type ), ebx );
    masm->movl( Address( (std::int32_t) &saved_esi, RelocationInformation::RelocationType::external_word_type ), esi );
    masm->movl( Address( (std::int32_t) &saved_edi, RelocationInformation::RelocationType::external_word_type ), edi );
    // save frame pointer w/o destroying any register contents
    masm->movl( eax, Address( (std::int32_t) &last_Delta_fp, RelocationInformation::RelocationType::external_word_type ) );
    masm->movl( Address( (std::int32_t) &stored_frame_pointer, RelocationInformation::RelocationType::external_word_type ), eax );
    masm->movl( eax, Address( (std::int32_t) &saved_eax, RelocationInformation::RelocationType::external_word_type ) );
    // return
    // %note: we don't return because the code is inlined in stubs -Marc 04/07
//  masm->ret();
}
