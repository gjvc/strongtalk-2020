//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/primitives/double_primitives.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/util.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/DoubleByteArrayOopDescriptor.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"


TRACE_FUNC( TraceDoublePrims, "double" )


std::size_t doubleOopPrimitives::number_of_calls;


static Oop new_double( double value ) {
    DoubleOop d = as_doubleOop( Universe::allocate( sizeof( DoubleOopDescriptor ) / oopSize ) );
    d->init_untagged_contents_mark();
    d->set_klass_field( doubleKlassObj );
    d->set_value( value );
    return d;
}


#define ASSERT_RECEIVER st_assert(receiver->is_double(), "receiver must be double")

#define DOUBLE_RELATIONAL_OP( op )                                      \
  ASSERT_RECEIVER;                                                      \
  if (not argument->is_double())                                        \
    return markSymbol(vmSymbols::first_argument_has_wrong_type());      \
  return DoubleOop(receiver)->value() op DoubleOop(argument)->value()   \
         ? trueObj : falseObj

#define DOUBLE_ARITH_OP( op )                                           \
  ASSERT_RECEIVER;                                                      \
  if (not argument->is_double())                                        \
    return markSymbol(vmSymbols::first_argument_has_wrong_type());      \
  return new_double(DoubleOop(receiver)->value() op DoubleOop(argument)->value())


PRIM_DECL_2( doubleOopPrimitives::lessThan, Oop receiver, Oop argument ) {
    PROLOGUE_2( "lessThan", receiver, argument );
    DOUBLE_RELATIONAL_OP( < );
}


PRIM_DECL_2( doubleOopPrimitives::greaterThan, Oop receiver, Oop argument ) {
    PROLOGUE_2( "greaterThan", receiver, argument );
    DOUBLE_RELATIONAL_OP( > );
}


PRIM_DECL_2( doubleOopPrimitives::lessThanOrEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "lessThanOrEqual", receiver, argument );
    DOUBLE_RELATIONAL_OP( <= );
}


PRIM_DECL_2( doubleOopPrimitives::greaterThanOrEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "greaterThanOrEqual", receiver, argument );
    DOUBLE_RELATIONAL_OP( >= );
}


PRIM_DECL_2( doubleOopPrimitives::equal, Oop receiver, Oop argument ) {
    PROLOGUE_2( "equal", receiver, argument );
    DOUBLE_RELATIONAL_OP( == );
}


PRIM_DECL_2( doubleOopPrimitives::notEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "notEqual", receiver, argument );
    DOUBLE_RELATIONAL_OP( not_eq );
}

/*
PRIM_DECL_2(doubleOopPrimitives::add, Oop receiver, Oop argument) {
  PROLOGUE_2("add", receiver, argument);
  DOUBLE_ARITH_OP(+);
}

PRIM_DECL_2(doubleOopPrimitives::subtract, Oop receiver, Oop argument) {
  PROLOGUE_2("subtract", receiver, argument);
  DOUBLE_ARITH_OP(-);
}


PRIM_DECL_2(doubleOopPrimitives::multiply, Oop receiver, Oop argument) {
  PROLOGUE_2("multiply", receiver, argument);
  DOUBLE_ARITH_OP(*);
}

PRIM_DECL_2(doubleOopPrimitives::divide, Oop receiver, Oop argument) {
  PROLOGUE_2("divide", receiver, argument);
  DOUBLE_ARITH_OP(/);
}
*/

PRIM_DECL_2( doubleOopPrimitives::mod, Oop receiver, Oop argument ) {
    PROLOGUE_2( "mod", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->is_double() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    double result = fmod( DoubleOop( receiver )->value(), DoubleOop( argument )->value() );
    return new_double( result );
}


PRIM_DECL_1( doubleOopPrimitives::cosine, Oop receiver ) {
    PROLOGUE_1( "cosine", receiver );
    ASSERT_RECEIVER;
    return new_double( cos( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::sine, Oop receiver ) {
    PROLOGUE_1( "sine", receiver );
    ASSERT_RECEIVER;
    return new_double( sin( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::tangent, Oop receiver ) {
    PROLOGUE_1( "tangent", receiver );
    ASSERT_RECEIVER;
    return new_double( tan( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::arcCosine, Oop receiver ) {
    PROLOGUE_1( "arcCosine", receiver );
    ASSERT_RECEIVER;
    return new_double( acos( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::arcSine, Oop receiver ) {
    PROLOGUE_1( "arcSine", receiver );
    ASSERT_RECEIVER;
    return new_double( asin( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::arcTangent, Oop receiver ) {
    PROLOGUE_1( "arcTangent", receiver );
    ASSERT_RECEIVER;
    return new_double( atan( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::hyperbolicCosine, Oop receiver ) {
    PROLOGUE_1( "hyperbolicCosine", receiver );
    ASSERT_RECEIVER;
    return new_double( cosh( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::hyperbolicSine, Oop receiver ) {
    PROLOGUE_1( "hyperbolicSine", receiver );
    ASSERT_RECEIVER;
    return new_double( sinh( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::hyperbolicTangent, Oop receiver ) {
    PROLOGUE_1( "hyperbolicTangent", receiver );
    ASSERT_RECEIVER;
    return new_double( tanh( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::sqrt, Oop receiver ) {
    PROLOGUE_1( "sqrt", receiver );
    ASSERT_RECEIVER;
    return new_double( ::sqrt( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::squared, Oop receiver ) {
    PROLOGUE_1( "squared", receiver );
    ASSERT_RECEIVER;
    return new_double( DoubleOop( receiver )->value() * 2 );
}


PRIM_DECL_1( doubleOopPrimitives::ln, Oop receiver ) {
    PROLOGUE_1( "ln", receiver );
    ASSERT_RECEIVER;
    return new_double( log( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::exp, Oop receiver ) {
    PROLOGUE_1( "exp", receiver );
    ASSERT_RECEIVER;
    return new_double( ::exp( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::log10, Oop receiver ) {
    PROLOGUE_1( "log10", receiver );
    ASSERT_RECEIVER;
    return new_double( ::log10( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::isNan, Oop receiver ) {
    PROLOGUE_1( "isNan", receiver );
    ASSERT_RECEIVER;
    return std::isnan( DoubleOop( receiver )->value() ) ? trueObj : falseObj;
}


PRIM_DECL_1( doubleOopPrimitives::isFinite, Oop receiver ) {
    PROLOGUE_1( "isFinite", receiver );
    ASSERT_RECEIVER;
    return std::isfinite( DoubleOop( receiver )->value() ) ? trueObj : falseObj;
}


PRIM_DECL_1( doubleOopPrimitives::floor, Oop receiver ) {
    PROLOGUE_1( "floor", receiver );
    ASSERT_RECEIVER;
    double result = ::floor( DoubleOop( receiver )->value() );
    return new_double( result );
}


PRIM_DECL_1( doubleOopPrimitives::smi_floor, Oop receiver ) {
    PROLOGUE_1( "smi_floor", receiver );
    ASSERT_RECEIVER;
    double result = ::floor( DoubleOop( receiver )->value() );
    if ( result < 0.0 ) {
        if ( result > smi_min )
            return smiOopFromValue( (int) result );
    } else {
        if ( result < smi_max )
            return smiOopFromValue( (int) result );
    }
    return markSymbol( vmSymbols::conversion_failed() );

}


PRIM_DECL_1( doubleOopPrimitives::ceiling, Oop receiver ) {
    PROLOGUE_1( "ceiling", receiver );
    ASSERT_RECEIVER;
    double result = ceil( DoubleOop( receiver )->value() );
    return new_double( result );
}


PRIM_DECL_1( doubleOopPrimitives::exponent, Oop receiver ) {
    PROLOGUE_1( "exponent", receiver );
    ASSERT_RECEIVER;
    int result;
    (void) frexp( DoubleOop( receiver )->value(), &result );
    return smiOopFromValue( result );
}


PRIM_DECL_1( doubleOopPrimitives::mantissa, Oop receiver ) {
    PROLOGUE_1( "mantissa", receiver );
    ASSERT_RECEIVER;
    int exp;
    return new_double( frexp( DoubleOop( receiver )->value(), &exp ) );
}


PRIM_DECL_1( doubleOopPrimitives::truncated, Oop receiver ) {
    PROLOGUE_1( "truncated", receiver );
    ASSERT_RECEIVER;
    double value = DoubleOop( receiver )->value();
    return new_double( value > 0.0 ? ::floor( value ) : ::ceil( value ) );
}


PRIM_DECL_2( doubleOopPrimitives::timesTwoPower, Oop receiver, Oop argument ) {
    PROLOGUE_2( "timesTwoPower", receiver, argument )
    ASSERT_RECEIVER;
    if ( not argument->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return new_double( ldexp( DoubleOop( receiver )->value(), SMIOop( argument )->value() ) );
}


PRIM_DECL_1( doubleOopPrimitives::roundedAsSmallInteger, Oop receiver ) {
    PROLOGUE_1( "roundedAsSmallInteger", receiver );
    ASSERT_RECEIVER;
    if ( DoubleOop( receiver )->value() < 0.0 ) {
        double result = ::ceil( DoubleOop( receiver )->value() - 0.5 );
        if ( result > smi_min )
            return smiOopFromValue( (int) result );
    } else {
        double result = ::floor( DoubleOop( receiver )->value() + 0.5 );
        if ( result < smi_max )
            return smiOopFromValue( (int) result );
    }
    return markSymbol( vmSymbols::smi_conversion_failed() );
}


PRIM_DECL_1( doubleOopPrimitives::asSmallInteger, Oop receiver ) {
    PROLOGUE_1( "asSmallInteger", receiver );
    ASSERT_RECEIVER;
    double value = DoubleOop( receiver )->value();
    if ( value not_eq ::floor( value ) )
        return markSymbol( vmSymbols::smi_conversion_failed() );
    if ( value < 0.0 ) {
        if ( value > smi_min )
            return smiOopFromValue( (int) value );
    } else {
        if ( value < smi_max )
            return smiOopFromValue( (int) value );
    }
    return markSymbol( vmSymbols::smi_conversion_failed() );
}


PRIM_DECL_2( doubleOopPrimitives::printFormat, Oop receiver, Oop argument ) {
    PROLOGUE_2( "printFormat", receiver, argument );
    ASSERT_RECEIVER;
    const std::size_t size = 100;
    char      format[size];

    if ( argument->is_byteArray() ) {
        ByteArrayOop( argument )->copy_null_terminated( format, size );
    } else if ( argument->is_doubleByteArray() ) {
        DoubleByteArrayOop( argument )->copy_null_terminated( format, size );
    } else {
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    }
    lprintf( format, DoubleOop( receiver )->value() );
    return receiver;
}


PRIM_DECL_1( doubleOopPrimitives::printString, Oop receiver ) {
    PROLOGUE_1( "printString", receiver );
    ASSERT_RECEIVER;
    ResourceMark resourceMark;
    char *result = new_resource_array<char>( 4 * 1024 );
    int len = sprintf( result, "%1.6f", DoubleOop( receiver )->value() );
    while ( len > 1 and result[ len - 1 ] == '0' and result[ len - 2 ] not_eq '.' )
        len--;
    result[ len ]    = '\0';

    BlockScavenge bs;
    ByteArrayOop  ba = oopFactory::new_byteArray( len );
    for ( int     i  = 0; i < len; i++ ) {
        ba->byte_at_put( i + 1, result[ i ] );
    }
    return ba;
}


PRIM_DECL_0( doubleOopPrimitives::max_value ) {
    PROLOGUE_0( "max_value" );
    return new_double( DBL_MAX );
}


PRIM_DECL_0( doubleOopPrimitives::min_positive_value ) {
    PROLOGUE_0( "min_positive_value" );
    return new_double( DBL_MIN );
}


PRIM_DECL_1( doubleOopPrimitives::store_string, Oop receiver ) {
    PROLOGUE_1( "printFormat", receiver );
    ASSERT_RECEIVER;
    BlockScavenge bs;

    double value = DoubleOop( receiver )->value();
    std::uint8_t *addr = (std::uint8_t *) &value;

    ByteArrayOop result = oopFactory::new_byteArray( 8 );
    for ( int    index  = 0; index < 8; index++ ) {
        result->byte_at_put( index + 1, addr[ index ] );
    }
    return result;
}


PRIM_DECL_3( doubleOopPrimitives::mandelbrot, Oop re, Oop im, Oop n ) {
    PROLOGUE_3( "mandelbrot", re, im, n );

    if ( not re->is_double() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not im->is_double() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not n->is_smi() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );

    double c_re = DoubleOop( re )->value();
    double c_im = DoubleOop( im )->value();
    double z_re = c_re;
    double z_im = c_im;
    double d_re = z_re * z_re;
    double d_im = z_im * z_im;

    std::size_t i    = 0;
    int imax = SMIOop( n )->value() - 1;

    while ( i < imax and d_re + d_im <= 4.0 ) {
        z_im = 2.0 * z_re * z_im + c_im;
        z_re = d_re - d_im + c_re;
        d_re = z_re * z_re;
        d_im = z_im * z_im;
        i++;
    }

    return smiOopFromValue( i );
}


static void trap() {
    st_assert( false, "This primitive should be patched" );
}


extern "C" Oop __CALLING_CONVENTION double_subtract( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
extern "C" Oop __CALLING_CONVENTION double_divide( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
extern "C" Oop __CALLING_CONVENTION double_add( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
extern "C" Oop __CALLING_CONVENTION double_multiply( Oop receiver, Oop argument ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
extern "C" Oop __CALLING_CONVENTION double_from_smi( Oop receiver ) {
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
