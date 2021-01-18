//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include <array>

#include "vm/interpreter/Floats.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/flags.hpp"


std::array<const char *, Floats::max_number_of_functions>Floats::_function_table{};
std::array<const char *, Floats::max_number_of_functions>Floats::_function_names{
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


const char *Floats::function_name_for( Function f ) {
    st_assert( 0 <= static_cast<int>(f) and f < Floats::Function::number_of_functions, "illegal function" );
    return _function_names[ static_cast<int>(f) ];
}


bool_t Floats::has_selector_for( Function f ) {
    if ( f == Floats::Function::undefined )
        return false;
    if ( f == Floats::Function::zero )
        return false;
    if ( f == Floats::Function::one )
        return false;
    return true;
}


SymbolOop Floats::selector_for( Function f ) {
    switch ( f ) {
        case Floats::Function::abs:
            return vmSymbols::abs();
        case Floats::Function::negated:
            return vmSymbols::negated();
        case Floats::Function::squared:
            return vmSymbols::squared();
        case Floats::Function::sqrt:
            return vmSymbols::sqrt();
        case Floats::Function::sin:
            return vmSymbols::sin();
        case Floats::Function::cos:
            return vmSymbols::cos();
        case Floats::Function::tan:
            return vmSymbols::tan();
        case Floats::Function::exp:
            return vmSymbols::exp();
        case Floats::Function::ln:
            return vmSymbols::ln();
        case Floats::Function::add:
            return vmSymbols::plus();
        case Floats::Function::subtract:
            return vmSymbols::minus();
        case Floats::Function::multiply:
            return vmSymbols::multiply();
        case Floats::Function::divide:
            return vmSymbols::divide();
        case Floats::Function::modulo:
            return vmSymbols::mod();
        case Floats::Function::is_zero:
            return vmSymbols::equal();
        case Floats::Function::is_not_zero:
            return vmSymbols::not_equal();
        case Floats::Function::oopify:
            return vmSymbols::as_float();
        case Floats::Function::is_equal:
            return vmSymbols::equal();
        case Floats::Function::is_not_equal:
            return vmSymbols::not_equal();
        case Floats::Function::is_less:
            return vmSymbols::less_than();
        case Floats::Function::is_less_equal:
            return vmSymbols::less_than_or_equal();
        case Floats::Function::is_greater:
            return vmSymbols::greater_than();
        case Floats::Function::is_greater_equal:
            return vmSymbols::greater_than_or_equal();
        case Floats::Function::floatify:
            return vmSymbols::as_float_value();
        default:
            return nullptr;
    }
}


// Initialization/debugging

void Floats::generate_tst( MacroAssembler *masm, Assembler::Condition cc ) {

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
    masm->movl( eax, Address( (int) &trueObj, RelocationInformation::RelocationType::external_word_type ) );
    masm->jcc( cond, L );
    masm->movl( eax, Address( (int) &falseObj, RelocationInformation::RelocationType::external_word_type ) );
    masm->bind( L );
}


void Floats::generate_cmp( MacroAssembler *masm, Assembler::Condition cc ) {
    int                  mask;
    Assembler::Condition cond;
    MacroAssembler::fpu_mask_and_cond_for( cc, mask, cond );
    Label L;
    masm->fcompp();
    masm->fwait();
    masm->fnstsw_ax();
    masm->testl( eax, mask );
    masm->movl( eax, Address( (int) &trueObj, RelocationInformation::RelocationType::external_word_type ) );
    masm->jcc( cond, L );
    masm->movl( eax, Address( (int) &falseObj, RelocationInformation::RelocationType::external_word_type ) );
    masm->bind( L );
}


void Floats::generate( MacroAssembler *masm, Function f ) {

    //
    const char *entry_point = masm->pc();

    //
    switch ( f ) {
        // nullary functions
        case Floats::Function::zero:
            masm->fldz();
            break;

        case Floats::Function::one:
            masm->fld1();
            break;

            // unary functions
        case Floats::Function::abs:
            masm->fabs();
            break;
        case Floats::Function::negated:
            masm->fchs();
            break;
        case Floats::Function::squared:
            masm->fmul( 0 );
            break;
        case Floats::Function::sqrt:
            masm->int3(); /* Unimplemented */        break;
        case Floats::Function::sin:
            masm->int3(); /* Unimplemented */        break;
        case Floats::Function::cos:
            masm->int3(); /* Unimplemented */        break;
        case Floats::Function::tan:
            masm->int3(); /* Unimplemented */        break;
        case Floats::Function::exp:
            masm->int3(); /* Unimplemented */        break;
        case Floats::Function::ln:
            masm->int3(); /* Unimplemented */        break;

            // binary functions
        case Floats::Function::add:
            masm->faddp();
            break;
        case Floats::Function::subtract:
            masm->fsubp();
            break;
        case Floats::Function::multiply:
            masm->fmulp();
            break;
        case Floats::Function::divide:
            masm->fdivp();
            break;
        case Floats::Function::modulo:
            masm->fxch();
            masm->fprem();
            break;

            // unary functions to Oop
        case Floats::Function::is_zero:
            generate_tst( masm, Assembler::Condition::zero );
            break;
        case Floats::Function::is_not_zero:
            generate_tst( masm, Assembler::Condition::notZero );
            break;
        case Floats::Function::oopify:
            masm->hlt();    /* see InterpreterGenerator */    break;

            // binary functions to Oop
            // (Note: This is comparing ST(1) with ST while the bits in the FPU status word (assume comparison of ST with ST(1) -> reverse the conditions).
        case Floats::Function::is_equal:
            generate_cmp( masm, Assembler::Condition::equal );
            break;
        case Floats::Function::is_not_equal:
            generate_cmp( masm, Assembler::Condition::notEqual );
            break;
        case Floats::Function::is_less:
            generate_cmp( masm, Assembler::Condition::greater );
            break;
        case Floats::Function::is_less_equal:
            generate_cmp( masm, Assembler::Condition::greaterEqual );
            break;
        case Floats::Function::is_greater:
            generate_cmp( masm, Assembler::Condition::less );
            break;
        case Floats::Function::is_greater_equal:
            generate_cmp( masm, Assembler::Condition::lessEqual );
            break;

        default: ShouldNotReachHere();
    }
    masm->ret( 0 );
    _function_table[ static_cast<int>(f) ] = entry_point;

    int        length = masm->pc() - entry_point;
    const char *name  = function_name_for( f );
    _console->print_cr( "%%float-generate: Float function index [%d]: name [%s], length [%d] bytes, entry point [0x%x]", f, name, length, entry_point );
    if ( PrintInterpreter ) {
        masm->code()->decode();
        _console->cr();
    }

}


bool_t Floats::_is_initialized = false;


void Floats::init( MacroAssembler *masm ) {
    if ( is_initialized() )
        return;

    _console->print_cr( "%%system-init:  Floats::init" );
    _console->print_cr( "%%system-init: _function_names.size() %ld", _function_names.size() );
    _console->print_cr( "%%system-init: number_of_functions %ld", Floats::Function::number_of_functions );

    st_assert( _function_names.size() == static_cast<int>( Floats::Function::number_of_functions ), "Floats: number of _functions_names not equal number_of_functions" );
    if ( sizeof( _function_names ) / sizeof( const char * ) not_eq static_cast<int>( Floats::Function::number_of_functions ) ) {
        st_fatal( "Floats: number of _functions_names not equal number_of_functions" );
    }

    // pre-initialize whole table
    // (make sure there's an entry for each index so that illegal indices
    // can be caught during execution without additional index range check)
    for ( std::size_t i = max_number_of_functions; i-- > 0; ) {
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
    generate( masm, Floats::Function::zero );
    generate( masm, Floats::Function::one );

    // unary functions
    generate( masm, Floats::Function::abs );
    generate( masm, Floats::Function::negated );
    generate( masm, Floats::Function::squared );
    generate( masm, Floats::Function::sqrt );
    generate( masm, Floats::Function::sin );
    generate( masm, Floats::Function::cos );
    generate( masm, Floats::Function::tan );
    generate( masm, Floats::Function::exp );
    generate( masm, Floats::Function::ln );

    // binary functions
    generate( masm, Floats::Function::add );
    generate( masm, Floats::Function::subtract );
    generate( masm, Floats::Function::multiply );
    generate( masm, Floats::Function::divide );
    generate( masm, Floats::Function::modulo );

    // unary functions to Oop
    generate( masm, Floats::Function::oopify );
    generate( masm, Floats::Function::is_zero );
    generate( masm, Floats::Function::is_not_zero );

    // binary functions to Oop
    generate( masm, Floats::Function::is_equal );
    generate( masm, Floats::Function::is_not_equal );
    generate( masm, Floats::Function::is_less );
    generate( masm, Floats::Function::is_less_equal );
    generate( masm, Floats::Function::is_greater );
    generate( masm, Floats::Function::is_greater_equal );

    _is_initialized = true;
}


void Floats::print() {
    if ( _is_initialized ) {
        _console->print_cr( "Float functions:" );
        for ( std::size_t i = 0; i < static_cast<int>( Floats::Function::number_of_functions ); i++ ) {
            _console->print_cr( "%3d: 0x%x %s", i, _function_table[ i ], function_name_for( Function( i ) ) );
        }
    } else {
        _console->print_cr( "Floats not yet initialized" );
    }
    _console->cr();
}
