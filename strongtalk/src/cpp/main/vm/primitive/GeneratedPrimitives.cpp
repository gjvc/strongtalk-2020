
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include <array>

#include "vm/primitive/PrimitivesGenerator.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/primitive/Primitives.hpp"
#include "vm/platform/os.hpp"
#include "vm/utility/OutputStream.hpp"
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
    masm->set_last_delta_frame_after_call();
    masm->pushl( size );
    masm->call( (const char *) &scavenge_and_allocate, RelocationInformation::RelocationType::runtime_call_type );
    masm->addl( esp, 4 );
    masm->reset_last_delta_frame();
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


const char *GeneratedPrimitives::primitiveNew( std::int32_t n ) {
    st_assert( _is_initialized, "GeneratedPrimitives not initialized yet" );
    st_assert( 0 <= n and n <= 9, "index out of range" )
    SPDLOG_INFO( "GeneratedPrimitives::primitiveNew [{}]", n );
    return _primitiveNew[ n ];
}


extern "C" BlockClosureOop allocateBlock( SmallIntegerOop nofArgs );
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

    return _allocateBlock[ n ];
}


extern "C" ContextOop allocateContext( SmallIntegerOop nofVars );
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

    if ( _is_initialized ) {
        return;
    }

    ResourceMark resourceMark;
    _code = os::exec_memory( _code_size );

    CodeBuffer          *code = new CodeBuffer( _code, _code_size );
    MacroAssembler      *masm = new MacroAssembler( code );
    PrimitivesGenerator gen( masm );

    gen.error_jumps();
    _smiOopPrimitives_add       = patch( "primitiveAdd:ifFail:", gen.smiOopPrimitives_add() );
    _smiOopPrimitives_subtract  = patch( "primitiveSubtract:ifFail:", gen.smiOopPrimitives_subtract() );
    _smiOopPrimitives_multiply  = patch( "primitiveMultiply:ifFail:", gen.smiOopPrimitives_multiply() );
    _smiOopPrimitives_mod       = patch( "primitiveMod:ifFail:", gen.smiOopPrimitives_mod() );
    _smiOopPrimitives_div       = patch( "primitiveDiv:ifFail:", gen.smiOopPrimitives_div() );
    _smiOopPrimitives_quo       = patch( "primitiveQuo:ifFail:", gen.smiOopPrimitives_quo() );
    _smiOopPrimitives_remainder = patch( "primitiveRemainder:ifFail:", gen.smiOopPrimitives_remainder() );
    _double_add                 = patch( "primitiveFloatAdd:ifFail:", gen.double_op( PrimitivesGenerator::arith_op::op_add ), PrimitivesGenerator::arith_op::op_add );
    _double_subtract            = patch( "primitiveFloatSubtract:ifFail:", gen.double_op( PrimitivesGenerator::arith_op::op_sub ), PrimitivesGenerator::arith_op::op_sub );
    _double_multiply            = patch( "primitiveFloatMultiply:ifFail:", gen.double_op( PrimitivesGenerator::arith_op::op_mul ), PrimitivesGenerator::arith_op::op_mul );
    _double_divide              = patch( "primitiveFloatDivide:ifFail:", gen.double_op( PrimitivesGenerator::arith_op::op_div ), PrimitivesGenerator::arith_op::op_div );
    _double_from_smi            = patch( "primitiveAsFloat", gen.double_from_smi() );

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
    SPDLOG_INFO( "system-init:  generatedPrimitives_init_before_interpreter" );

    GeneratedPrimitives::init();
}


void generatedPrimitives_init_after_interpreter() {
    SPDLOG_INFO( "system-init:  generatedPrimitives_init_after_interpreter" );

    GeneratedPrimitives::patch_primitiveValue();
}
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/assembler/Address.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/assembler/MacroAssembler.hpp"

#include "vm/primitive/BehaviorPrimitives.hpp"
#include "vm/primitive/PrimitivesGenerator.hpp"


static bool stop = false;


const char *PrimitivesGenerator::primitiveNew( std::int32_t n ) {
    Address      klass_addr = Address( esp, +2 * OOP_SIZE );
    Label        need_scavenge, fill_object;
    std::int32_t size       = n + 2;

    // %note: it looks like the compiler assumes we spill only eax/ebx here -Marc 04/07

    const char *entry_point = masm->pc();

    test_for_scavenge( eax, size * OOP_SIZE, allocation_failure );
    Address _stop = Address( (std::int32_t) &stop, RelocationInformation::RelocationType::external_word_type );
    Label   _break, no_break;
    masm->bind( fill_object );
    masm->movl( ebx, _stop );
    masm->testl( ebx, ebx );
    masm->jcc( Assembler::Condition::notEqual, _break );
    masm->bind( no_break );
    masm->movl( ebx, klass_addr );
    masm->movl( Address( eax, ( -size + 0 ) * OOP_SIZE ), 0x80000003 );    // obj->init_mark()
    masm->movl( Address( eax, ( -size + 1 ) * OOP_SIZE ), ebx );        // obj->init_mark()

    if ( n > 0 ) {
        masm->movl( ebx, nil_addr() );
        for ( std::size_t i = 0; i < n; i++ ) {
            masm->movl( Address( eax, ( -size + 2 + i ) * OOP_SIZE ), ebx );    // obj->obj_at_put(i,nilObject)
        }
    }

    masm->subl( eax, ( size * OOP_SIZE ) - 1 );
    masm->ret( 2 * OOP_SIZE );

    masm->bind( _break );
    masm->int3();
    masm->jmp( no_break );

    masm->bind( need_scavenge );
    scavenge( size );
    masm->jmp( fill_object );

    return entry_point;
}
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/primitive/PrimitivesGenerator.hpp"


const char *PrimitivesGenerator::double_op( arith_op op ) {
    Label need_scavenge, fill_object;

    const char *entry_point = masm->pc();

    // 	Tag test for argument
    masm->movl( ebx, Address( esp, +OOP_SIZE ) );
    masm->movl( edx, doubleKlass_addr() );
    masm->testb( ebx, 0x01 );
    masm->jcc( Assembler::Condition::zero, error_first_argument_has_wrong_type );

    // 	klass test for argument
    masm->cmpl( edx, Address( ebx, +3 ) );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );

    // 	allocate_double
    test_for_scavenge( eax, 4 * OOP_SIZE, need_scavenge );

    // 	mov	DWORD PTR [eax-(4 * OOP_SIZE)], 0A0000003H ; obj->init_mark()
    // 	fld  QWORD PTR [ecx+07]
    // 	mov	DWORD PTR [eax-(3 * OOP_SIZE)], edx        ; obj->set_klass(klass)
    //
    // 	op   QWORD PTR [ebx+07]

    masm->bind( fill_object );
    masm->movl( ecx, Address( esp, +2 * OOP_SIZE ) );

    masm->movl( edx, doubleKlass_addr() );
    masm->movl( Address( eax, -4 * OOP_SIZE ), 0xA0000003 );         // obj->init_mark()
    masm->movl( Address( eax, -3 * OOP_SIZE ), edx );                // obj->set_klass(klass)

    masm->fld_d( Address( ecx, ( 2 * OOP_SIZE ) - 1 ) );

    switch ( op ) {
        case op_add:
            masm->fadd_d( Address( ebx, ( 2 * OOP_SIZE ) - 1 ) );
            break;
        case op_sub:
            masm->fsub_d( Address( ebx, ( 2 * OOP_SIZE ) - 1 ) );
            break;
        case op_mul:
            masm->fmul_d( Address( ebx, ( 2 * OOP_SIZE ) - 1 ) );
            break;
        case op_div:
            masm->fdiv_d( Address( ebx, ( 2 * OOP_SIZE ) - 1 ) );
            break;
    }

    masm->subl( eax, ( 4 * OOP_SIZE ) - 1 );

    // 	eax result   DoubleOop
    // 	ecx receiver DoubleOop
    // 	ebx argument DoubleOop
    masm->fstp_d( Address( eax, ( 2 * OOP_SIZE ) - 1 ) );
    masm->ret( 8 );

    masm->bind( need_scavenge );
    scavenge( 4 );
    masm->movl( ebx, Address( esp, +OOP_SIZE ) );
    masm->movl( edx, doubleKlass_addr() );
    masm->jmp( fill_object );

    return entry_point;
}


const char *PrimitivesGenerator::double_from_smi() {
    Label need_scavenge, fill_object;

    const char *entry_point = masm->pc();

    test_for_scavenge( eax, 4 * OOP_SIZE, need_scavenge );

    masm->bind( fill_object );
    masm->movl( ecx, Address( esp, +OOP_SIZE ) );
    masm->movl( edx, doubleKlass_addr() );
    masm->sarl( ecx, TAG_SIZE );
    masm->movl( Address( eax, -4 * OOP_SIZE ), 0xA0000003 );         // obj->init_mark()
    masm->movl( Address( esp, -OOP_SIZE ), ecx );
    masm->movl( Address( eax, -3 * OOP_SIZE ), edx );                // obj->set_klass(klass)
    masm->fild_s( Address( esp, -OOP_SIZE ) );
    masm->subl( eax, ( 4 * OOP_SIZE ) - 1 );

    //	eax result   DoubleOop
    masm->fstp_d( Address( eax, ( 2 * OOP_SIZE ) - 1 ) );
    masm->ret( 4 );

    masm->bind( need_scavenge );
    scavenge( 4 );
    masm->jmp( fill_object );

    return entry_point;
}
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/assembler/Address.hpp"
#include "vm/assembler/Label.hpp"
#include "vm/assembler/MacroAssembler.hpp"
#include "vm/primitive/PrimitivesGenerator.hpp"


const char *PrimitivesGenerator::smiOopPrimitives_add() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );
    Label   _overflow;

    const char *entry_point = masm->pc();

    masm->movl( eax, receiver );
    masm->addl( eax, argument );
    masm->jcc( Assembler::Condition::overflow, _overflow );
    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->ret( 8 );

    masm->bind( _overflow );
    masm->movl( eax, argument );
    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->jmp( error_overflow );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_subtract() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );
    Label   _overflow;

    const char *entry_point = masm->pc();

    masm->movl( eax, receiver );
    masm->subl( eax, argument );
    masm->jcc( Assembler::Condition::overflow, _overflow );
    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->ret( 8 );

    masm->bind( _overflow );
    masm->movl( eax, argument );
    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->jmp( error_overflow );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_multiply() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );

    const char *entry_point = masm->pc();

    // masm->int3();
    masm->movl( edx, argument );
    masm->movl( eax, receiver );
    masm->testb( edx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->sarl( edx, 2 );
    masm->imull( edx );
    masm->jcc( Assembler::Condition::overflow, error_overflow );
    masm->ret( 8 );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_mod() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );
    Label   _equal, _positive;

    const char *entry_point = masm->pc();

    // PUBLIC _smiOopPrimitives_mod@8
    //
    // ; Intel definition of mod delivers:
    // ;   0 <= |x%y| < |y|
    // ;
    // ; Standard definition requires:
    // ;   y>0:
    // ;     0 <= x mod y < y
    // ;   y<0:
    // ;     y <  x mod y <= 0
    // ;
    // ; Conversion:
    // ;
    // ;   sgn(y)=sgn(x%y):
    // ;     x mod y = x%y
    // ;
    // ;   sgn(y)#sgn(x%y):
    // ;     x mod y = x%y + y
    // ;

//  masm->int3();
    masm->movl( eax, receiver );
    masm->movl( ecx, argument );
    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, error_division_by_zero );

    masm->testb( ecx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );

    masm->sarl( ecx, 2 );
    masm->sarl( eax, 2 );
    masm->cdq();
    masm->idivl( ecx );
    masm->jcc( Assembler::Condition::overflow, error_overflow );

    masm->movl( eax, edx );
    masm->testl( eax, eax );
    masm->jcc( Assembler::Condition::equal, _equal );

    masm->xorl( edx, ecx );
    masm->jcc( Assembler::Condition::negative, _positive );

    masm->bind( _equal );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    masm->bind( _positive );
    masm->addl( eax, ecx );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_div() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );
    Label   _equal, _positive;

    const char *entry_point = masm->pc();

    // ; Intel definition of div delivers:
    // ;   x = (x/y)*y + (x%y)
    // ;
    // ; Standard definition requires:
    // ;   x = (x div y)*y + (x mod y)
    // ;
    // ; Conversion:
    // ;
    // ;   sgn(y)=sgn(x%y):
    // ;     x div y = x/y
    // ;
    // ;   sgn(y)#sgn(x%y):
    // ;     x div y = x/y-1
    // ;
    //

    masm->movl( ecx, argument );
    masm->movl( eax, receiver );
    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, error_division_by_zero );

    masm->testb( ecx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );

    masm->sarl( ecx, 2 );
    masm->sarl( eax, 2 );
    masm->cdq();
    masm->idivl( ecx );

    masm->jcc( Assembler::Condition::overflow, error_overflow );

    masm->testl( edx, edx );
    masm->jcc( Assembler::Condition::equal, _equal );

    masm->xorl( ecx, edx );
    masm->jcc( Assembler::Condition::negative, _positive );

    masm->bind( _equal );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    masm->bind( _positive );
    masm->decl( eax );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_quo() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );

    const char *entry_point = masm->pc();

    masm->movl( ecx, argument );
    masm->movl( eax, receiver );

    masm->testb( eax, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_receiver_has_wrong_type );

    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, error_division_by_zero );

    masm->testb( ecx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );

    masm->sarl( ecx, 2 );
    masm->sarl( eax, 2 );
    masm->cdq();
    masm->idivl( ecx );

    masm->jcc( Assembler::Condition::overflow, error_overflow );
    masm->shll( eax, 2 );
    masm->ret( 8 );

    return entry_point;
}


const char *PrimitivesGenerator::smiOopPrimitives_remainder() {
    Address argument = Address( esp, 4 );
    Address receiver = Address( esp, 8 );

    const char *entry_point = masm->pc();

    masm->movl( ecx, argument );
    masm->movl( eax, receiver );
    masm->testl( ecx, ecx );
    masm->jcc( Assembler::Condition::equal, error_division_by_zero );
    masm->testb( ecx, 0x03 );
    masm->jcc( Assembler::Condition::notEqual, error_first_argument_has_wrong_type );
    masm->sarl( ecx, 2 );
    masm->sarl( eax, 2 );
    masm->cdq();
    masm->idivl( ecx );
    masm->jcc( Assembler::Condition::overflow, error_overflow );
    masm->movl( eax, edx );
    masm->sarl( eax, 2 );
    masm->ret( 8 );

    return entry_point;
}


extern "C" void scavenge_and_allocate( std::int32_t size );


// -----------------------------------------------------------------------------

extern "C" BlockClosureOop allocateBlock( SmallIntegerOop nofArgs );
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

extern "C" ContextOop allocateContext( SmallIntegerOop nofVars );
extern "C" ContextOop allocateContext0();
extern "C" ContextOop allocateContext1();
extern "C" ContextOop allocateContext2();


typedef Oop (__CALLING_CONVENTION *smiOp)( Oop, Oop );
