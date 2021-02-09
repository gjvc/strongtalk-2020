//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/primitives/DoubleOopPrimitives.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/util.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/DoubleByteArrayOopDescriptor.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"


TRACE_FUNC( TraceDoublePrims, "double" )


std::int32_t DoubleOopPrimitives::number_of_calls;


static Oop new_double( double value ) {
    DoubleOop d = as_doubleOop( Universe::allocate( sizeof( DoubleOopDescriptor ) / OOP_SIZE ) );
    d->init_untagged_contents_mark();
    d->set_klass_field( doubleKlassObject );
    d->set_value( value );
    return d;
}


#define ASSERT_RECEIVER st_assert(receiver->isDouble(), "receiver must be double")

#define DOUBLE_RELATIONAL_OP( op )                                      \
  ASSERT_RECEIVER;                                                      \
  if (not argument->isDouble())                                        \
    return markSymbol(vmSymbols::first_argument_has_wrong_type());      \
  return DoubleOop(receiver)->value() op DoubleOop(argument)->value()   \
         ? trueObject : falseObject

#define DOUBLE_ARITH_OP( op )                                           \
  ASSERT_RECEIVER;                                                      \
  if (not argument->isDouble())                                        \
    return markSymbol(vmSymbols::first_argument_has_wrong_type());      \
  return new_double(DoubleOop(receiver)->value() op DoubleOop(argument)->value())


PRIM_DECL_2( DoubleOopPrimitives::lessThan, Oop receiver, Oop argument ) {
    PROLOGUE_2( "lessThan", receiver, argument );
    DOUBLE_RELATIONAL_OP( < );
}


PRIM_DECL_2( DoubleOopPrimitives::greaterThan, Oop receiver, Oop argument ) {
    PROLOGUE_2( "greaterThan", receiver, argument );
    DOUBLE_RELATIONAL_OP( > );
}


PRIM_DECL_2( DoubleOopPrimitives::lessThanOrEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "lessThanOrEqual", receiver, argument );
    DOUBLE_RELATIONAL_OP( <= );
}


PRIM_DECL_2( DoubleOopPrimitives::greaterThanOrEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "greaterThanOrEqual", receiver, argument );
    DOUBLE_RELATIONAL_OP( >= );
}


PRIM_DECL_2( DoubleOopPrimitives::equal, Oop receiver, Oop argument ) {
    PROLOGUE_2( "equal", receiver, argument );
    DOUBLE_RELATIONAL_OP( == );
}


PRIM_DECL_2( DoubleOopPrimitives::notEqual, Oop receiver, Oop argument ) {
    PROLOGUE_2( "notEqual", receiver, argument );
    DOUBLE_RELATIONAL_OP( not_eq );
}

/*
PRIM_DECL_2(DoubleOopPrimitives::add, Oop receiver, Oop argument) {
  PROLOGUE_2("add", receiver, argument);
  DOUBLE_ARITH_OP(+);
}

PRIM_DECL_2(DoubleOopPrimitives::subtract, Oop receiver, Oop argument) {
  PROLOGUE_2("subtract", receiver, argument);
  DOUBLE_ARITH_OP(-);
}


PRIM_DECL_2(DoubleOopPrimitives::multiply, Oop receiver, Oop argument) {
  PROLOGUE_2("multiply", receiver, argument);
  DOUBLE_ARITH_OP(*);
}

PRIM_DECL_2(DoubleOopPrimitives::divide, Oop receiver, Oop argument) {
  PROLOGUE_2("divide", receiver, argument);
  DOUBLE_ARITH_OP(/);
}
*/

PRIM_DECL_2( DoubleOopPrimitives::mod, Oop receiver, Oop argument ) {
    PROLOGUE_2( "mod", receiver, argument );
    ASSERT_RECEIVER;
    if ( not argument->isDouble() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    double result = fmod( DoubleOop( receiver )->value(), DoubleOop( argument )->value() );
    return new_double( result );
}


PRIM_DECL_1( DoubleOopPrimitives::cosine, Oop receiver ) {
    PROLOGUE_1( "cosine", receiver );
    ASSERT_RECEIVER;
    return new_double( cos( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::sine, Oop receiver ) {
    PROLOGUE_1( "sine", receiver );
    ASSERT_RECEIVER;
    return new_double( sin( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::tangent, Oop receiver ) {
    PROLOGUE_1( "tangent", receiver );
    ASSERT_RECEIVER;
    return new_double( tan( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::arcCosine, Oop receiver ) {
    PROLOGUE_1( "arcCosine", receiver );
    ASSERT_RECEIVER;
    return new_double( acos( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::arcSine, Oop receiver ) {
    PROLOGUE_1( "arcSine", receiver );
    ASSERT_RECEIVER;
    return new_double( asin( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::arcTangent, Oop receiver ) {
    PROLOGUE_1( "arcTangent", receiver );
    ASSERT_RECEIVER;
    return new_double( atan( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::hyperbolicCosine, Oop receiver ) {
    PROLOGUE_1( "hyperbolicCosine", receiver );
    ASSERT_RECEIVER;
    return new_double( cosh( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::hyperbolicSine, Oop receiver ) {
    PROLOGUE_1( "hyperbolicSine", receiver );
    ASSERT_RECEIVER;
    return new_double( sinh( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::hyperbolicTangent, Oop receiver ) {
    PROLOGUE_1( "hyperbolicTangent", receiver );
    ASSERT_RECEIVER;
    return new_double( tanh( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::sqrt, Oop receiver ) {
    PROLOGUE_1( "sqrt", receiver );
    ASSERT_RECEIVER;
    return new_double( ::sqrt( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::squared, Oop receiver ) {
    PROLOGUE_1( "squared", receiver );
    ASSERT_RECEIVER;
    return new_double( DoubleOop( receiver )->value() * 2 );
}


PRIM_DECL_1( DoubleOopPrimitives::ln, Oop receiver ) {
    PROLOGUE_1( "ln", receiver );
    ASSERT_RECEIVER;
    return new_double( log( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::exp, Oop receiver ) {
    PROLOGUE_1( "exp", receiver );
    ASSERT_RECEIVER;
    return new_double( ::exp( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::log10, Oop receiver ) {
    PROLOGUE_1( "log10", receiver );
    ASSERT_RECEIVER;
    return new_double( ::log10( DoubleOop( receiver )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::isNan, Oop receiver ) {
    PROLOGUE_1( "isNan", receiver );
    ASSERT_RECEIVER;
    return std::isnan( DoubleOop( receiver )->value() ) ? trueObject : falseObject;
}


PRIM_DECL_1( DoubleOopPrimitives::isFinite, Oop receiver ) {
    PROLOGUE_1( "isFinite", receiver );
    ASSERT_RECEIVER;
    return std::isfinite( DoubleOop( receiver )->value() ) ? trueObject : falseObject;
}


PRIM_DECL_1( DoubleOopPrimitives::floor, Oop receiver ) {
    PROLOGUE_1( "floor", receiver );
    ASSERT_RECEIVER;
    double result = ::floor( DoubleOop( receiver )->value() );
    return new_double( result );
}


PRIM_DECL_1( DoubleOopPrimitives::smi_floor, Oop receiver ) {
    PROLOGUE_1( "smi_floor", receiver );
    ASSERT_RECEIVER;
    double result = ::floor( DoubleOop( receiver )->value() );
    if ( result < 0.0 ) {
        if ( result > SMI_MIN_VALUE )
            return smiOopFromValue( (std::int32_t) result );
    } else {
        if ( result < SMI_MAX_VALUE )
            return smiOopFromValue( (std::int32_t) result );
    }
    return markSymbol( vmSymbols::conversion_failed() );

}


PRIM_DECL_1( DoubleOopPrimitives::ceiling, Oop receiver ) {
    PROLOGUE_1( "ceiling", receiver );
    ASSERT_RECEIVER;
    double result = ceil( DoubleOop( receiver )->value() );
    return new_double( result );
}


PRIM_DECL_1( DoubleOopPrimitives::exponent, Oop receiver ) {
    PROLOGUE_1( "exponent", receiver );
    ASSERT_RECEIVER;
    std::int32_t result;
    (void) frexp( DoubleOop( receiver )->value(), &result );
    return smiOopFromValue( result );
}


PRIM_DECL_1( DoubleOopPrimitives::mantissa, Oop receiver ) {
    PROLOGUE_1( "mantissa", receiver );
    ASSERT_RECEIVER;
    std::int32_t exp;
    return new_double( frexp( DoubleOop( receiver )->value(), &exp ) );
}


PRIM_DECL_1( DoubleOopPrimitives::truncated, Oop receiver ) {
    PROLOGUE_1( "truncated", receiver );
    ASSERT_RECEIVER;
    double value = DoubleOop( receiver )->value();
    return new_double( value > 0.0 ? ::floor( value ) : ::ceil( value ) );
}


PRIM_DECL_2( DoubleOopPrimitives::timesTwoPower, Oop receiver, Oop argument ) {
    PROLOGUE_2( "timesTwoPower", receiver, argument )
    ASSERT_RECEIVER;
    if ( not argument->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return new_double( ldexp( DoubleOop( receiver )->value(), SmallIntegerOop( argument )->value() ) );
}


PRIM_DECL_1( DoubleOopPrimitives::roundedAsSmallInteger, Oop receiver ) {
    PROLOGUE_1( "roundedAsSmallInteger", receiver );
    ASSERT_RECEIVER;
    if ( DoubleOop( receiver )->value() < 0.0 ) {
        double result = ::ceil( DoubleOop( receiver )->value() - 0.5 );
        if ( result > SMI_MIN_VALUE )
            return smiOopFromValue( (std::int32_t) result );
    } else {
        double result = ::floor( DoubleOop( receiver )->value() + 0.5 );
        if ( result < SMI_MAX_VALUE )
            return smiOopFromValue( (std::int32_t) result );
    }
    return markSymbol( vmSymbols::smi_conversion_failed() );
}


PRIM_DECL_1( DoubleOopPrimitives::asSmallInteger, Oop receiver ) {
    PROLOGUE_1( "asSmallInteger", receiver );
    ASSERT_RECEIVER;
    double value = DoubleOop( receiver )->value();
    if ( value not_eq ::floor( value ) )
        return markSymbol( vmSymbols::smi_conversion_failed() );
    if ( value < 0.0 ) {
        if ( value > SMI_MIN_VALUE )
            return smiOopFromValue( (std::int32_t) value );
    } else {
        if ( value < SMI_MAX_VALUE )
            return smiOopFromValue( (std::int32_t) value );
    }
    return markSymbol( vmSymbols::smi_conversion_failed() );
}


PRIM_DECL_2( DoubleOopPrimitives::printFormat, Oop receiver, Oop argument ) {
    PROLOGUE_2( "printFormat", receiver, argument );
    ASSERT_RECEIVER;
    const std::int32_t size = 100;
    char               format[size];

    if ( argument->isByteArray() ) {
        ByteArrayOop( argument )->copy_null_terminated( format, size );
    } else if ( argument->isDoubleByteArray() ) {
        DoubleByteArrayOop( argument )->copy_null_terminated( format, size );
    } else {
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    }
    SPDLOG_INFO( format, DoubleOop( receiver )->value() );
    return receiver;
}


PRIM_DECL_1( DoubleOopPrimitives::printString, Oop receiver ) {
    PROLOGUE_1( "printString", receiver );
    ASSERT_RECEIVER;
    ResourceMark resourceMark;
    char         *result = new_resource_array<char>( 4 * 1024 );
    std::int32_t len     = sprintf( result, "%1.6f", DoubleOop( receiver )->value() );
    while ( len > 1 and result[ len - 1 ] == '0' and result[ len - 2 ] not_eq '.' )
        len--;
    result[ len ]         = '\0';

    BlockScavenge      bs;
    ByteArrayOop       ba = OopFactory::new_byteArray( len );
    for ( std::int32_t i  = 0; i < len; i++ ) {
        ba->byte_at_put( i + 1, result[ i ] );
    }
    return ba;
}


PRIM_DECL_0( DoubleOopPrimitives::max_value ) {
    PROLOGUE_0( "max_value" );
    return new_double( DBL_MAX );
}


PRIM_DECL_0( DoubleOopPrimitives::min_positive_value ) {
    PROLOGUE_0( "min_positive_value" );
    return new_double( DBL_MIN );
}


PRIM_DECL_1( DoubleOopPrimitives::store_string, Oop receiver ) {
    PROLOGUE_1( "printFormat", receiver );
    ASSERT_RECEIVER;
    BlockScavenge bs;

    double       value = DoubleOop( receiver )->value();
    std::uint8_t *addr = (std::uint8_t *) &value;

    ByteArrayOop       result = OopFactory::new_byteArray( 8 );
    for ( std::int32_t index  = 0; index < 8; index++ ) {
        result->byte_at_put( index + 1, addr[ index ] );
    }
    return result;
}


PRIM_DECL_3( DoubleOopPrimitives::mandelbrot, Oop re, Oop im, Oop n ) {
    PROLOGUE_3( "mandelbrot", re, im, n );

    if ( not re->isDouble() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not im->isDouble() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not n->isSmallIntegerOop() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );

    double c_re = DoubleOop( re )->value();
    double c_im = DoubleOop( im )->value();
    double z_re = c_re;
    double z_im = c_im;
    double d_re = z_re * z_re;
    double d_im = z_im * z_im;

    std::int32_t i    = 0;
    std::int32_t imax = SmallIntegerOop( n )->value() - 1;

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
    static_cast<void>(receiver); // unused
    static_cast<void>(argument); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}


extern "C" Oop __CALLING_CONVENTION double_divide( Oop receiver, Oop argument ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(argument); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}


extern "C" Oop __CALLING_CONVENTION double_add( Oop receiver, Oop argument ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(argument); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}


extern "C" Oop __CALLING_CONVENTION double_multiply( Oop receiver, Oop argument ) {
    static_cast<void>(receiver); // unused
    static_cast<void>(argument); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}


extern "C" Oop __CALLING_CONVENTION double_from_smi( Oop receiver ) {
    static_cast<void>(receiver); // unused
    trap();
    return markSymbol( vmSymbols::primitive_trap() );
}
