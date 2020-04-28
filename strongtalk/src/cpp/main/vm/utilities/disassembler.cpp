
//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/util.hpp"
#include "disassembler.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/code/RelocationInformation.hpp"

#include "disasm.h"
#include "udis86.h"
#include "vm/system/sizes.hpp"



// -----------------------------------------------------------------------------

constexpr int MAX_HEXBUF_SIZE{ 256 };
constexpr int MAX_OUTBUF_SIZE{ 256 };

static char tohex( uint8_t c );

static const char * bintohex( const char * data, int bytes );

static void printRelocInfo( RelocationInformationIterator * iter, ConsoleOutputStream * stream );

static void printRelocInfo( const NativeMethod * nm, const char * pc, int lendis, ConsoleOutputStream * stream );

static void printProgramCounterDescriptorInfo( const NativeMethod * nm, const char * pc, ConsoleOutputStream * stream );


// -----------------------------------------------------------------------------

//static int     segsize{ 32 };    //
//static int32_t offset{ 0 };      //
//static int     autosync{ 0 };    //
//static iflag_t prefer{ 0 };      //


// -----------------------------------------------------------------------------

static void st_disasm( const char * begin, const char * end, const NativeMethod * nm, ConsoleOutputStream * stream ) {

    static char output[MAX_OUTBUF_SIZE];
    size_t      outbufsize{ sizeof( output ) };
    int32_t     data_size{ 4 }; //

    ud_t ud_obj;

//    ud_init( &ud_obj );
//    ud_set_input_file( &ud_obj, stdin );
//    ud_set_input_buffer( &ud_obj, begin, static_cast<size_t>( end - begin ) );
//    ud_set_mode( &ud_obj, 64 );
//    ud_set_syntax( &ud_obj, UD_SYN_INTEL );


    for ( const char * pc = begin; pc < end; pc += data_size ) {

        // int32_t disasm(uint8_t *data, int32_t data_size, char *output, int outbufsize, int segsize,
        //               int64_t offset, int autosync, iflag_t *prefer)
//        data_size = disasm( ( uint8_t * ) pc, data_size, output, outbufsize, segsize, offset, autosync, &prefer );

        if ( data_size ) {
            stream->print( "%p %-20s    %-40s", pc, bintohex( pc, data_size ), output );
            if ( nm ) {
                stream->print( "; " );
                printProgramCounterDescriptorInfo( nm, pc, stream );
                printRelocInfo( nm, pc, data_size, stream );
            }
        }

        stream->cr();
    }

}


static char tohex( uint8_t c ) {
    const char * digits = "0123456789ABCDEF";
    if ( c > 0xf )
        return '?';
    return digits[ c ];
}


static const char * bintohex( const char * data, int bytes ) {
    static char buf[MAX_HEXBUF_SIZE];

    char * p = buf;
    while ( bytes-- ) {
        *p++ = tohex( ( *data & 0xF0 ) >> 4 );
        *p++ = tohex( *data & 0x0F );
        data++;
    }

    *p = '\0';
    return buf;
}


static void printRelocInfo( RelocationInformationIterator * iter, ConsoleOutputStream * stream ) {

    PrimitiveDescriptor * pd;
    const char          * target;
    int                 * addr;

    stream->print( "[reloc @ " );
    addr = iter->word_addr();
    switch ( iter->type() ) {
        case RelocationInformation::RelocationType::none:
            stream->print( "none" );
            break;

        case RelocationInformation::RelocationType::oop_type:
            stream->print( "%p, embedded Oop, ", addr );
            Oop( *addr )->print_value();
            break;

        case RelocationInformation::RelocationType::ic_type:
            stream->print( "%p, inline cache", addr );
            break;

        case RelocationInformation::RelocationType::primitive_type:
            stream->print( "%p, primitive call, ", addr );
            target = ( const char * ) ( *addr + ( int ) addr + oopSize );

            pd = Primitives::lookup( ( primitiveFunctionType ) target );
            if ( pd not_eq nullptr ) {
                stream->print( "(%s)", pd->name() );
            } else {
                stream->print( "runtime routine" );
            }
            break;

        case RelocationInformation::RelocationType::runtime_call_type:
            stream->print( "%p, runtime call", addr );
            break;

        case RelocationInformation::RelocationType::external_word_type:
            stream->print( "%p, external word", addr );
            break;

        case RelocationInformation::RelocationType::internal_word_type:
            stream->print( "%p, internal word", addr );
            break;

        case RelocationInformation::RelocationType::uncommon_type:
            stream->print( "%p, uncommon trap ", addr );
            if ( iter->wasUncommonTrapExecuted() )
                stream->print( " (taken)" );
            else
                stream->print( " (not taken)" );
            break;

        case RelocationInformation::RelocationType::dll_type:
            stream->print( "%p, dll", addr );
            break;

        default:
            stream->print( "???" );
            break;
    }
    stream->print( "]" );
}


static void printRelocInfo( const NativeMethod * nm, const char * pc, int lendis, ConsoleOutputStream * stream ) {

    RelocationInformationIterator iter( nm );
    char * addr;

    while ( iter.next() ) {
        addr = ( char * ) iter.word_addr();
        if ( addr > pc and addr < ( pc + lendis ) ) {
            printRelocInfo( &iter, stream );
            break;
        }
    }
}


static void printProgramCounterDescriptorInfo( const NativeMethod * nm, const char * pc, ConsoleOutputStream * stream ) {
    ProgramCounterDescriptor * pcs;

    pcs = nm->containingProgramCounterDescriptor( pc, nullptr );
    if ( not pcs ) {
        return;
    }

    stream->print( "bc = %03ld ", pcs->_byteCode );
    if ( pcs->is_prologue() ) {
        stream->print( "prologue " );
    } else if ( pcs->is_epilogue() ) {
        stream->print( "epilogue " );
    }

}


void Disassembler::decode( const NativeMethod * nm, ConsoleOutputStream * stream ) {
    st_disasm( nm->instructionsStart(), nm->instructionsEnd(), nm, stream );
}


void Disassembler::decode( const char * begin, const char * end, ConsoleOutputStream * stream ) {
    st_disasm( begin, end, nullptr, stream );
}
