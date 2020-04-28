//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include <array>

#include "vm/interpreter/Floats.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/flags.hpp"


std::array <const char *, Floats::max_number_of_functions>Floats::_function_table{};
std::array <const char *, Floats::max_number_of_functions>Floats::_function_names{
    "zero", //
    "one", //
    "abs", //
    "negated", //
    "squared", //
    "sqrt", //
    "sin", //
    "cos", //
    "tan", //
    "exp", //
    "ln", //
    "add", //
    "subtract", //
    "multiply", //
    "divide", //
    "modulo", //
    "is_zero", //
    "is_not_zero", //
    "oopify", //
    "is_equal", //
    "is_not_equal", //
    "is_less", //
    "is_less_equal", //
    "is_greater", //
    "is_greater_equal" //
};


const char * Floats::function_name_for( Function f ) {
    st_assert( 0 <= f and f < number_of_functions, "illegal function" );
    return _function_names[ f ];
}


bool_t Floats::has_selector_for( Function f ) {
    if ( f == undefined )
        return false;
    if ( f == zero )
        return false;
    if ( f == one )
        return false;
    return true;
}


SymbolOop Floats::selector_for( Function f ) {
    switch ( f ) {
        case abs:
            return vmSymbols::abs();
            break;
        case negated:
            return vmSymbols::negated();
            break;
        case squared:
            return vmSymbols::squared();
            break;
        case sqrt:
            return vmSymbols::sqrt();
            break;
        case sin:
            return vmSymbols::sin();
            break;
        case cos:
            return vmSymbols::cos();
            break;
        case tan:
            return vmSymbols::tan();
            break;
        case exp:
            return vmSymbols::exp();
            break;
        case ln:
            return vmSymbols::ln();
            break;
        case add:
            return vmSymbols::plus();
            break;
        case subtract:
            return vmSymbols::minus();
            break;
        case multiply:
            return vmSymbols::multiply();
            break;
        case divide:
            return vmSymbols::divide();
            break;
        case modulo:
            return vmSymbols::mod();
            break;
        case is_zero:
            return vmSymbols::equal();
            break;
        case is_not_zero:
            return vmSymbols::not_equal();
            break;
        case oopify:
            return vmSymbols::as_float();
            break;
        case is_equal:
            return vmSymbols::equal();
            break;
        case is_not_equal:
            return vmSymbols::not_equal();
            break;
        case is_less:
            return vmSymbols::less_than();
            break;
        case is_less_equal:
            return vmSymbols::less_than_or_equal();
            break;
        case is_greater:
            return vmSymbols::greater_than();
            break;
        case is_greater_equal:
            return vmSymbols::greater_than_or_equal();
            break;
        case floatify:
            return vmSymbols::as_float_value();
            break;
    }
    return nullptr;
}


// Initialization/debugging

void Floats::generate_tst( MacroAssembler * masm, Assembler::Condition cc ) {

    int                  mask;
    Assembler::Condition cond;
    MacroAssembler::fpu_mask_and_cond_for( cc, mask, cond );
    Label L;

//    masm->int3();    // remove this if it works
    masm->ftst();
    masm->fwait();
    masm->fnstsw_ax();
    masm->fpop();        // explicitly pop argument
    masm->testl( eax, mask );
    masm->movl( eax, Address( ( int ) &trueObj, RelocationInformation::RelocationType::external_word_type ) );
    masm->jcc( cond, L );
    masm->movl( eax, Address( ( int ) &falseObj, RelocationInformation::RelocationType::external_word_type ) );
    masm->bind( L );
}


void Floats::generate_cmp( MacroAssembler * masm, Assembler::Condition cc ) {
    int                  mask;
    Assembler::Condition cond;
    MacroAssembler::fpu_mask_and_cond_for( cc, mask, cond );
    Label L;
    masm->fcompp();
    masm->fwait();
    masm->fnstsw_ax();
    masm->testl( eax, mask );
    masm->movl( eax, Address( ( int ) &trueObj, RelocationInformation::RelocationType::external_word_type ) );
    masm->jcc( cond, L );
    masm->movl( eax, Address( ( int ) &falseObj, RelocationInformation::RelocationType::external_word_type ) );
    masm->bind( L );
}


void Floats::generate( MacroAssembler * masm, Function f ) {

    //
    const char * entry_point = masm->pc();

    //
    switch ( f ) {
        // nullary functions
        case zero:
            masm->fldz();
            break;

        case one:
            masm->fld1();
            break;

        // unary functions
        case abs:
            masm->fabs();
            break;
        case negated:
            masm->fchs();
            break;
        case squared:
            masm->fmul( 0 );
            break;
        case sqrt:
            masm->int3(); /* Unimplemented */        break;
        case sin:
            masm->int3(); /* Unimplemented */        break;
        case cos:
            masm->int3(); /* Unimplemented */        break;
        case tan:
            masm->int3(); /* Unimplemented */        break;
        case exp:
            masm->int3(); /* Unimplemented */        break;
        case ln:
            masm->int3(); /* Unimplemented */        break;

        // binary functions
        case add:
            masm->faddp();
            break;
        case subtract:
            masm->fsubp();
            break;
        case multiply:
            masm->fmulp();
            break;
        case divide:
            masm->fdivp();
            break;
        case modulo:
            masm->fxch();
            masm->fprem();
            break;

        // unary functions to Oop
        case is_zero:
            generate_tst( masm, Assembler::Condition::zero );
            break;
        case is_not_zero:
            generate_tst( masm, Assembler::Condition::notZero );
            break;
        case oopify:
            masm->hlt();    /* see InterpreterGenerator */    break;

        // binary functions to Oop
        // (Note: This is comparing ST(1) with ST while the bits in the FPU status word (assume comparison of ST with ST(1) -> reverse the conditions).
        case is_equal:
            generate_cmp( masm, Assembler::Condition::equal );
            break;
        case is_not_equal:
            generate_cmp( masm, Assembler::Condition::notEqual );
            break;
        case is_less:
            generate_cmp( masm, Assembler::Condition::greater );
            break;
        case is_less_equal:
            generate_cmp( masm, Assembler::Condition::greaterEqual );
            break;
        case is_greater:
            generate_cmp( masm, Assembler::Condition::less );
            break;
        case is_greater_equal:
            generate_cmp( masm, Assembler::Condition::lessEqual );
            break;

        default: ShouldNotReachHere();
    }
    masm->ret( 0 );
    _function_table[ f ] = entry_point;

    int length = masm->pc() - entry_point;
    const char * name = function_name_for( f );
    _console->print_cr( "%%float-generate: Float function index [%d]: name [%s], length [%d] bytes, entry point [0x%x]", f, name, length, entry_point );
    if ( PrintInterpreter ) {
        masm->code()->decode();
        _console->cr();
    }

}


bool_t Floats::_is_initialized = false;


void Floats::init( MacroAssembler * masm ) {
    if ( is_initialized() )
        return;

    _console->print_cr( "yes" );
    _console->print_cr( "%%system-init:  Floats::init" );
    _console->print_cr( "%%system-init: _function_names.size() %ld", _function_names.size());
    _console->print_cr( "%%system-init: number_of_functions %ld", number_of_functions );
//    st_assert( _function_names.size() == number_of_functions, "Floats: number of _functions_names not equal number_of_functions" );
    _console->print_cr( "hello" );
//    if ( sizeof( _function_names ) / sizeof( const char * ) not_eq number_of_functions ) {
//        fatal( "Floats: number of _functions_names not equal number_of_functions" );
//    }

    // pre-initialize whole table
    // (make sure there's an entry for each index so that illegal indices
    // can be caught during execution without additional index range check)
    for ( int i = max_number_of_functions; i-- > 0; ) {
        _function_table[ i ] = masm->pc();
        _console->print_cr( "%%system-init:  Floats::init() _function_table index [%ld] pc [0x%08x]", i, masm->pc() );
    }
    masm->hlt();

    _console->print_cr( "Undefined float functions entry point" );
    if ( PrintInterpreter ) {
        masm->code()->decode();
        _console->cr();
    }

    // nullary functions
    generate( masm, zero );
    generate( masm, one );

    // unary functions
    generate( masm, abs );
    generate( masm, negated );
    generate( masm, squared );
    generate( masm, sqrt );
    generate( masm, sin );
    generate( masm, cos );
    generate( masm, tan );
    generate( masm, exp );
    generate( masm, ln );

    // binary functions
    generate( masm, add );
    generate( masm, subtract );
    generate( masm, multiply );
    generate( masm, divide );
    generate( masm, modulo );

    // unary functions to Oop
    generate( masm, oopify );
    generate( masm, is_zero );
    generate( masm, is_not_zero );

    // binary functions to Oop
    generate( masm, is_equal );
    generate( masm, is_not_equal );
    generate( masm, is_less );
    generate( masm, is_less_equal );
    generate( masm, is_greater );
    generate( masm, is_greater_equal );

    _is_initialized = true;
}


void Floats::print() {
    if ( _is_initialized ) {
        _console->print_cr( "Float functions:" );
        for ( int i = 0; i < number_of_functions; i++ ) {
            _console->print_cr( "%3d: 0x%x %s", i, _function_table[ i ], function_name_for( Function( i ) ) );
        }
    } else {
        _console->print_cr( "Floats not yet initialized" );
    }
    _console->cr();
}
