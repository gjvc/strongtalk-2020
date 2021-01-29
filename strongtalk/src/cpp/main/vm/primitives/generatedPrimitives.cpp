//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include <array>

#include "vm/primitives/PrimitivesGenerator.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/system/os.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "PrimitivesGenerator.hpp"



// -----------------------------------------------------------------------------

// entry points
const char *GeneratedPrimitives::_allocateContext_var = nullptr;

const char *GeneratedPrimitives::_smiOopPrimitives_add       = nullptr;
const char *GeneratedPrimitives::_smiOopPrimitives_subtract  = nullptr;
const char *GeneratedPrimitives::_smiOopPrimitives_multiply  = nullptr;
const char *GeneratedPrimitives::_smiOopPrimitives_mod       = nullptr;
const char *GeneratedPrimitives::_smiOopPrimitives_div       = nullptr;
const char *GeneratedPrimitives::_smiOopPrimitives_quo       = nullptr;
const char *GeneratedPrimitives::_smiOopPrimitives_remainder = nullptr;

const char *GeneratedPrimitives::_double_add      = nullptr;
const char *GeneratedPrimitives::_double_subtract = nullptr;
const char *GeneratedPrimitives::_double_multiply = nullptr;
const char *GeneratedPrimitives::_double_divide   = nullptr;
const char *GeneratedPrimitives::_double_from_smi = nullptr;

std::array<const char *, 10>GeneratedPrimitives::_primitiveValue;
std::array<const char *, 10>GeneratedPrimitives::_primitiveNew;
std::array<const char *, 10>GeneratedPrimitives::_allocateBlock;
std::array<const char *, 3> GeneratedPrimitives::_allocateContext;
const char *GeneratedPrimitives::_primitiveInlineAllocations = nullptr;

extern "C" void scavenge_and_allocate( std::int32_t size );


// -----------------------------------------------------------------------------

// macros

void PrimitivesGenerator::scavenge( std::int32_t size ) {
    masm->set_last_Delta_frame_after_call();
    masm->pushl( size );
    masm->call( (const char *) &scavenge_and_allocate, RelocationInformation::RelocationType::runtime_call_type );
    masm->addl( esp, 4 );
    masm->reset_last_Delta_frame();
    masm->addl( eax, size * OOP_SIZE );
}


void PrimitivesGenerator::test_for_scavenge( Register dst, std::int32_t size, Label &need_scavenge ) {
    masm->movl( dst, Address( (std::int32_t) &eden_top, RelocationInformation::RelocationType::external_word_type ) );
    masm->addl( dst, size );
    masm->cmpl( dst, Address( (std::int32_t) &eden_end, RelocationInformation::RelocationType::external_word_type ) );
    masm->jcc( Assembler::Condition::greater, need_scavenge );
    masm->movl( Address( (std::int32_t) &eden_top, RelocationInformation::RelocationType::external_word_type ), dst );
}


void PrimitivesGenerator::error_jumps() {

#define VMSYMBOL_SUFFIX  _enum
#define VMSYMBOL_ENUM_NAME( name ) name##VMSYMBOL_SUFFIX

    Address _smi_overflow                  = Address( (std::int32_t) &vm_symbols[ VMSYMBOL_ENUM_NAME( smi_overflow ) ], RelocationInformation::RelocationType::external_word_type );
    Address _division_by_zero              = Address( (std::int32_t) &vm_symbols[ VMSYMBOL_ENUM_NAME( division_by_zero ) ], RelocationInformation::RelocationType::external_word_type );
    Address _receiver_has_wrong_type       = Address( (std::int32_t) &vm_symbols[ VMSYMBOL_ENUM_NAME( receiver_has_wrong_type ) ], RelocationInformation::RelocationType::external_word_type );
    Address _division_not_exact            = Address( (std::int32_t) &vm_symbols[ VMSYMBOL_ENUM_NAME( division_not_exact ) ], RelocationInformation::RelocationType::external_word_type );
    Address _first_argument_has_wrong_type = Address( (std::int32_t) &vm_symbols[ VMSYMBOL_ENUM_NAME( first_argument_has_wrong_type ) ], RelocationInformation::RelocationType::external_word_type );
    Address _allocation_failure            = Address( (std::int32_t) &vm_symbols[ VMSYMBOL_ENUM_NAME( failed_allocation ) ], RelocationInformation::RelocationType::external_word_type );

#undef  VMSYMBOL_SUFFIX
#undef  VMSYMBOL_ENUM_NAME

    masm->bind( error_receiver_has_wrong_type );
    masm->movl( eax, _receiver_has_wrong_type );
    masm->addl( eax, 2 );
    masm->ret( 2 * OOP_SIZE );
    masm->bind( error_first_argument_has_wrong_type );
    masm->movl( eax, _first_argument_has_wrong_type );
    masm->addl( eax, 2 );
    masm->ret( 2 * OOP_SIZE );
    masm->bind( error_overflow );
    masm->movl( eax, _smi_overflow );
    masm->addl( eax, 2 );
    masm->ret( 2 * OOP_SIZE );
    masm->bind( error_division_by_zero );
    masm->movl( eax, _division_by_zero );
    masm->addl( eax, 2 );
    masm->ret( 2 * OOP_SIZE );
    masm->bind( allocation_failure );
    masm->movl( eax, _allocation_failure );
    masm->addl( eax, 2 );
    masm->ret( 2 * OOP_SIZE );
}


// generators are in xxx_prims_gen.cpp files

void GeneratedPrimitives::set_primitiveValue( std::int32_t n, const char *entry_point ) {
    st_assert( 0 <= n and n <= 9, "index out of range" )
    _primitiveValue[ n ] = entry_point;
}


// Parametrized accessors

const char *GeneratedPrimitives::primitiveValue( std::int32_t n ) {
    st_assert( 0 <= n and n <= 9, "index out of range" )
    st_assert( _primitiveValue[ n ], "primitiveValues not initialized yet" );
    return _primitiveValue[ n ];
}


extern "C" Oop primitiveNew0( Oop );
extern "C" Oop primitiveNew1( Oop );
extern "C" Oop primitiveNew2( Oop );
extern "C" Oop primitiveNew3( Oop );
extern "C" Oop primitiveNew4( Oop );
extern "C" Oop primitiveNew5( Oop );
extern "C" Oop primitiveNew6( Oop );
extern "C" Oop primitiveNew7( Oop );
extern "C" Oop primitiveNew8( Oop );
extern "C" Oop primitiveNew9( Oop );


const char *GeneratedPrimitives::primitiveNew( std::int32_t n ) {
    st_assert( _is_initialized, "GeneratedPrimitives not initialized yet" );
    st_assert( 0 <= n and n <= 9, "index out of range" )
    spdlog::info( "GeneratedPrimitives::primitiveNew [{}]", n );
    return _primitiveNew[ n ];
}


extern "C" BlockClosureOop allocateBlock( SMIOop nofArgs );
extern "C" BlockClosureOop allocateBlock0();
extern "C" BlockClosureOop allocateBlock1();
extern "C" BlockClosureOop allocateBlock2();
extern "C" BlockClosureOop allocateBlock3();
extern "C" BlockClosureOop allocateBlock4();
extern "C" BlockClosureOop allocateBlock5();
extern "C" BlockClosureOop allocateBlock6();
extern "C" BlockClosureOop allocateBlock7();
extern "C" BlockClosureOop allocateBlock8();
extern "C" BlockClosureOop allocateBlock9();


const char *GeneratedPrimitives::allocateBlock( std::int32_t n ) {
    st_assert( _is_initialized, "GeneratedPrimitives not initialized yet" );
    if ( n == -1 )
        return (const char *) ::allocateBlock;        // convenience
    st_assert( 0 <= n and n <= 9, "index out of range" )
    spdlog::info( "%primitive-generate:  GeneratedPrimitives::allocateBlock [{}]", n );

    return _allocateBlock[ n ];
}


extern "C" ContextOop allocateContext( SMIOop nofVars );
extern "C" ContextOop allocateContext0();
extern "C" ContextOop allocateContext1();
extern "C" ContextOop allocateContext2();


const char *GeneratedPrimitives::allocateContext( std::int32_t n ) {
    st_assert( _is_initialized, "GeneratedPrimitives not initialized yet" );
    if ( n == -1 )
        return _allocateContext_var;        // convenience
    if ( n == -1 )
        return (const char *) ::allocateContext;        // convenience
    st_assert( 0 <= n and n <= 2, "index out of range" )
    return _allocateContext[ n ];
}

// Initialization

bool       GeneratedPrimitives::_is_initialized = false;
//char GeneratedPrimitives::_code[GeneratedPrimitives::_code_size];
const char *GeneratedPrimitives::_code          = nullptr;


const char *GeneratedPrimitives::patch( const char *name, const char *entry_point ) {
    Primitives::patch( name, entry_point );
    return entry_point;
}


const char *GeneratedPrimitives::patch( const char *name, const char *entry_point, std::int32_t argument ) {
    char formatted_name[100];
    st_assert( strlen( name ) < 100, "primitive name longer than 100 characters - buffer overrun" );
    sprintf( formatted_name, name, argument );
    Primitives::patch( formatted_name, entry_point );
    return entry_point;
}


void GeneratedPrimitives::init() {
    if ( _is_initialized )
        return;

    ResourceMark resourceMark;
    _code = os::exec_memory( _code_size );

    CodeBuffer          *code = new CodeBuffer( _code, _code_size );
    MacroAssembler      *masm = new MacroAssembler( code );
    PrimitivesGenerator gen( masm );

    // add generators here

    gen.error_jumps();
    _smiOopPrimitives_add       = patch( "primitiveAdd:ifFail:", gen.smiOopPrimitives_add() );
    _smiOopPrimitives_subtract  = patch( "primitiveSubtract:ifFail:", gen.smiOopPrimitives_subtract() );
    _smiOopPrimitives_multiply  = patch( "primitiveMultiply:ifFail:", gen.smiOopPrimitives_multiply() );
    _smiOopPrimitives_mod       = patch( "primitiveMod:ifFail:", gen.smiOopPrimitives_mod() );
    _smiOopPrimitives_div       = patch( "primitiveDiv:ifFail:", gen.smiOopPrimitives_div() );
    _smiOopPrimitives_quo       = patch( "primitiveQuo:ifFail:", gen.smiOopPrimitives_quo() );
    _smiOopPrimitives_remainder = patch( "primitiveRemainder:ifFail:", gen.smiOopPrimitives_remainder() );

    PrimitivesGenerator::arith_op op_add = PrimitivesGenerator::op_add;
    _double_add = patch( "primitiveFloatAdd:ifFail:", gen.double_op( op_add ), op_add );

    PrimitivesGenerator::arith_op op_sub = PrimitivesGenerator::op_sub;
    _double_subtract = patch( "primitiveFloatSubtract:ifFail:", gen.double_op( op_sub ), op_sub );

    PrimitivesGenerator::arith_op op_mul = PrimitivesGenerator::op_mul;
    _double_multiply = patch( "primitiveFloatMultiply:ifFail:", gen.double_op( op_mul ), op_mul );

    PrimitivesGenerator::arith_op op_div = PrimitivesGenerator::op_div;
    _double_divide = patch( "primitiveFloatDivide:ifFail:", gen.double_op( op_div ), op_div );

    _double_from_smi = patch( "primitiveAsFloat", gen.double_from_smi() );

    for ( std::int32_t n = 0; n <= 9; n++ ) {
        _primitiveNew[ n ] = patch( "primitiveNew%1d:ifFail:", gen.primitiveNew( n ), n );
    }

    for ( std::int32_t n = 0; n <= 9; n++ ) {
        _allocateBlock[ n ] = patch( "primitiveCompiledBlockAllocate%1d", gen.allocateBlock( n ), n );
    }

    _allocateContext_var = patch( "primitiveCompiledContextAllocate:", gen.allocateContext_var() );

    for ( std::int32_t n = 0; n <= 2; n++ ) {
        _allocateContext[ n ] = patch( "primitiveCompiledContextAllocate%1d", gen.allocateContext( n ), n );
    }

    _primitiveInlineAllocations = patch( "primitiveInlineAllocations:count:", gen.inline_allocation() );

    masm->finalize();
    _is_initialized = true;

}


// patch some primitives defined in the interpreter

void GeneratedPrimitives::patch_primitiveValue() {

    Primitives::patch( "primitiveValue", primitiveValue( 0 ) );
    Primitives::patch( "primitiveValue:", primitiveValue( 1 ) );
    Primitives::patch( "primitiveValue:value:", primitiveValue( 2 ) );
    Primitives::patch( "primitiveValue:value:value:", primitiveValue( 3 ) );
    Primitives::patch( "primitiveValue:value:value:value:", primitiveValue( 4 ) );
    Primitives::patch( "primitiveValue:value:value:value:value:", primitiveValue( 5 ) );
    Primitives::patch( "primitiveValue:value:value:value:value:value:", primitiveValue( 6 ) );
    Primitives::patch( "primitiveValue:value:value:value:value:value:value:", primitiveValue( 7 ) );
    Primitives::patch( "primitiveValue:value:value:value:value:value:value:value:", primitiveValue( 8 ) );
    Primitives::patch( "primitiveValue:value:value:value:value:value:value:value:value:", primitiveValue( 9 ) );

}


typedef Oop (__CALLING_CONVENTION *smiOp)( Oop, Oop );


Oop GeneratedPrimitives::invoke( const char *op, Oop receiver, Oop argument ) {
    st_assert( _is_initialized, "Generated primitives have not been initialized" );
    return ( (smiOp) op )( argument, receiver );
}


Oop GeneratedPrimitives::smiOopPrimitives_add( Oop receiver, Oop argument ) {
    return invoke( _smiOopPrimitives_add, receiver, argument );
}


Oop GeneratedPrimitives::smiOopPrimitives_subtract( Oop receiver, Oop argument ) {
    return invoke( _smiOopPrimitives_subtract, receiver, argument );
}


Oop GeneratedPrimitives::smiOopPrimitives_multiply( Oop receiver, Oop argument ) {
    return invoke( _smiOopPrimitives_multiply, receiver, argument );
}


Oop GeneratedPrimitives::smiOopPrimitives_mod( Oop receiver, Oop argument ) {
    return invoke( _smiOopPrimitives_mod, receiver, argument );
}


Oop GeneratedPrimitives::smiOopPrimitives_div( Oop receiver, Oop argument ) {
    return invoke( _smiOopPrimitives_div, receiver, argument );
}


Oop GeneratedPrimitives::smiOopPrimitives_quo( Oop receiver, Oop argument ) {
    return invoke( _smiOopPrimitives_quo, receiver, argument );
}


Oop GeneratedPrimitives::smiOopPrimitives_remainder( Oop receiver, Oop argument ) {
    return invoke( _smiOopPrimitives_remainder, receiver, argument );
}


void generatedPrimitives_init_before_interpreter() {
    spdlog::info( "%system-init:  generatedPrimitives_init_before_interpreter" );

    GeneratedPrimitives::init();
}


void generatedPrimitives_init_after_interpreter() {
    spdlog::info( "%system-init:  generatedPrimitives_init_after_interpreter" );

    GeneratedPrimitives::patch_primitiveValue();
}
