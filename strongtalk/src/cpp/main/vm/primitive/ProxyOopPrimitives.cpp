//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitive/ProxyOopPrimitives.hpp"
#include "vm/platform/platform.hpp"
#include "vm/platform/os.hpp"
#include "vm/utility/ObjectIDTable.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/oop/MixinOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/oop/ProxyOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/code/NativeMethod.hpp"


TRACE_FUNC( TraceProxyPrims, "proxy" )


std::int32_t ProxyOopPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->is_proxy(), "receiver must be proxy")

#define ASSERT_RECEIVER_ACCESS \
  ASSERT_RECEIVER \
 /* if (proxyOop(receiver)->is_null()) \
    return markSymbol(vmSymbols::null_proxy_access());	*/ \



PRIM_DECL_1( ProxyOopPrimitives::getSmi, Oop receiver ) {
    PROLOGUE_1( "getSmi", receiver );
    ASSERT_RECEIVER;
    std::uint32_t value   = (std::uint32_t) ProxyOop( receiver )->get_pointer();
    std::uint32_t topBits = value >> ( BITS_PER_WORD - TAG_SIZE );
    if ( ( topBits not_eq 0 ) and ( topBits not_eq 3 ) )
        return markSymbol( vmSymbols::smi_conversion_failed() );
    return smiOopFromValue( (std::int32_t) value );
}


PRIM_DECL_2( ProxyOopPrimitives::set, Oop receiver, Oop value ) {
    PROLOGUE_2( "getSmi", receiver, value );
    ASSERT_RECEIVER;
    if ( value->isSmallIntegerOop() ) {
        ProxyOop( receiver )->set_pointer( (void *) SmallIntegerOop( value )->value() );
    } else if ( value->is_proxy() ) {
        ProxyOop( receiver )->set_pointer( ProxyOop( value )->get_pointer() );
    } else
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    return receiver;
}


PRIM_DECL_3( ProxyOopPrimitives::setHighLow, Oop receiver, Oop high, Oop low ) {
    PROLOGUE_3( "setHighLow", receiver, high, low );
    ASSERT_RECEIVER;
    if ( not high->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not low->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    std::uint32_t h     = (std::uint32_t) SmallIntegerOop( high )->value();
    std::uint32_t l     = (std::uint32_t) SmallIntegerOop( low )->value();
    std::uint32_t value = ( h << 16 ) | l;
    ProxyOop( receiver )->set_pointer( (void *) value );
    return receiver;
}


PRIM_DECL_1( ProxyOopPrimitives::getHigh, Oop receiver ) {
    PROLOGUE_1( "getHigh", receiver );
    ASSERT_RECEIVER;
    std::uint32_t value = (std::int32_t) ProxyOop( receiver )->get_pointer();
    value = value >> 16;
    return smiOopFromValue( value );
}


PRIM_DECL_1( ProxyOopPrimitives::getLow, Oop receiver ) {
    PROLOGUE_1( "getLow", receiver );
    ASSERT_RECEIVER;
    std::uint32_t value = (std::int32_t) ProxyOop( receiver )->get_pointer();
    value &= 0x0000ffff;
    return smiOopFromValue( value );
}


PRIM_DECL_1( ProxyOopPrimitives::isNull, Oop receiver ) {
    PROLOGUE_1( "isNull", receiver );
    ASSERT_RECEIVER;
    return ProxyOop( receiver )->is_null() ? trueObject : falseObject;
}


PRIM_DECL_1( ProxyOopPrimitives::isAllOnes, Oop receiver ) {
    PROLOGUE_1( "isAllOnes", receiver );
    ASSERT_RECEIVER;
    return ProxyOop( receiver )->is_allOnes() ? trueObject : falseObject;
}


PRIM_DECL_2( ProxyOopPrimitives::malloc, Oop receiver, Oop size ) {
    PROLOGUE_2( "malloc", receiver, size );
    ASSERT_RECEIVER;
    if ( not size->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    ProxyOop( receiver )->set_pointer( os::malloc( SmallIntegerOop( size )->value() ) );
    return receiver;
}


PRIM_DECL_2( ProxyOopPrimitives::calloc, Oop receiver, Oop size ) {
    PROLOGUE_2( "calloc", receiver, size );
    ASSERT_RECEIVER;
    if ( not size->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    ProxyOop( receiver )->set_pointer( os::calloc( SmallIntegerOop( size )->value(), 1 ) );
    return receiver;
}


PRIM_DECL_1( ProxyOopPrimitives::free, Oop receiver ) {
    PROLOGUE_1( "free", receiver );
    ASSERT_RECEIVER;
    os::free( ProxyOop( receiver )->get_pointer() );
    ProxyOop( receiver )->null_pointer();
    return receiver;
}


PRIM_DECL_2( ProxyOopPrimitives::byteAt, Oop receiver, Oop offset ) {
    PROLOGUE_2( "byteAt", receiver, offset );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    return smiOopFromValue( ProxyOop( receiver )->byte_at( SmallIntegerOop( offset )->value() ) );
}


PRIM_DECL_3( ProxyOopPrimitives::byteAtPut, Oop receiver, Oop offset, Oop value ) {
    PROLOGUE_3( "byteAtPut", receiver, offset, value );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not value->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    ProxyOop( receiver )->byte_at_put( SmallIntegerOop( offset )->value(), SmallIntegerOop( value )->value() );
    return receiver;
}


PRIM_DECL_2( ProxyOopPrimitives::doubleByteAt, Oop receiver, Oop offset ) {
    PROLOGUE_2( "doubleByteAt", receiver, offset );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    return smiOopFromValue( ProxyOop( receiver )->doubleByte_at( SmallIntegerOop( offset )->value() ) );
}


PRIM_DECL_3( ProxyOopPrimitives::doubleByteAtPut, Oop receiver, Oop offset, Oop value ) {
    PROLOGUE_3( "doubleByteAtPut", receiver, offset, value );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not value->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    ProxyOop( receiver )->doubleByte_at_put( SmallIntegerOop( offset )->value(), SmallIntegerOop( value )->value() );
    return receiver;
}


PRIM_DECL_2( ProxyOopPrimitives::smiAt, Oop receiver, Oop offset ) {
    PROLOGUE_2( "smiAt", receiver, offset );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    std::uint32_t value   = (std::uint32_t) ProxyOop( receiver )->long_at( SmallIntegerOop( offset )->value() );
    std::uint32_t topBits = value >> ( BITS_PER_WORD - TAG_SIZE );
    if ( ( topBits not_eq 0 ) and ( topBits not_eq 3 ) )
        return markSymbol( vmSymbols::smi_conversion_failed() );
    return smiOopFromValue( (std::int32_t) value );
}


PRIM_DECL_3( ProxyOopPrimitives::smiAtPut, Oop receiver, Oop offset, Oop value ) {
    PROLOGUE_3( "smiAtPut", receiver, offset, value );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not value->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    ProxyOop( receiver )->long_at_put( SmallIntegerOop( offset )->value(), SmallIntegerOop( value )->value() );
    return receiver;
}


PRIM_DECL_3( ProxyOopPrimitives::subProxyAt, Oop receiver, Oop offset, Oop result ) {
    PROLOGUE_3( "subProxyAt", receiver, offset, result );
    ASSERT_RECEIVER;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    ProxyOop( result )->set_pointer( (void *) ( (const char *) ProxyOop( receiver )->get_pointer() + SmallIntegerOop( offset )->value() ) );
    return result;
}


PRIM_DECL_3( ProxyOopPrimitives::proxyAt, Oop receiver, Oop offset, Oop result ) {
    PROLOGUE_3( "proxyAt", receiver, offset, result );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    ProxyOop( result )->set_pointer( (void *) ProxyOop( receiver )->long_at( SmallIntegerOop( offset )->value() ) );
    return result;
}


PRIM_DECL_3( ProxyOopPrimitives::proxyAtPut, Oop receiver, Oop offset, Oop value ) {
    PROLOGUE_3( "proxyAtPut", receiver, offset, value );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not value->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    ProxyOop( receiver )->long_at_put( SmallIntegerOop( offset )->value(), (std::int32_t) ProxyOop( value )->get_pointer() );
    return receiver;
}


PRIM_DECL_2( ProxyOopPrimitives::singlePrecisionFloatAt, Oop receiver, Oop offset ) {
    PROLOGUE_2( "singlePrecisionFloatAt", receiver, offset );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    return OopFactory::new_double( (double) ProxyOop( receiver )->float_at( SmallIntegerOop( offset )->value() ) );
}


PRIM_DECL_3( ProxyOopPrimitives::singlePrecisionFloatAtPut, Oop receiver, Oop offset, Oop value ) {
    PROLOGUE_3( "singlePrecisionFloatAtPut", receiver, offset, value );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not value->isDouble() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    ProxyOop( receiver )->float_at_put( SmallIntegerOop( offset )->value(), (float) DoubleOop( value )->value() );
    return receiver;
}


PRIM_DECL_2( ProxyOopPrimitives::doublePrecisionFloatAt, Oop receiver, Oop offset ) {
    PROLOGUE_2( "doublePrecisionFloatAt", receiver, offset );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    return OopFactory::new_double( ProxyOop( receiver )->double_at( SmallIntegerOop( offset )->value() ) );
}


PRIM_DECL_3( ProxyOopPrimitives::doublePrecisionFloatAtPut, Oop receiver, Oop offset, Oop value ) {
    PROLOGUE_3( "doublePrecisionFloatAtPut", receiver, offset, value );
    ASSERT_RECEIVER_ACCESS;
    if ( not offset->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not value->isDouble() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );

    ProxyOop( receiver )->double_at_put( SmallIntegerOop( offset )->value(), DoubleOop( value )->value() );
    return receiver;
}


static bool convert_to_arg( Oop arg, std::int32_t *addr ) {
    if ( arg->isSmallIntegerOop() ) {
        *addr = SmallIntegerOop( arg )->value();
        return true;
    }
    if ( arg->is_proxy() ) {
        *addr = (std::int32_t) ProxyOop( arg )->get_pointer();
        return true;
    }
    return false;
}


typedef void *(__CALLING_CONVENTION *call_out_func_0)();


PRIM_DECL_2( ProxyOopPrimitives::callOut0, Oop receiver, Oop result ) {
    PROLOGUE_2( "callOut0", receiver, result );
    ASSERT_RECEIVER_ACCESS;

    if ( not receiver->is_proxy() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    call_out_func_0 f = (call_out_func_0) ProxyOop( receiver )->get_pointer();
    ProxyOop( result )->set_pointer( ( *f )() );
    return result;
}


typedef void *(__CALLING_CONVENTION *call_out_func_1)( std::int32_t a );


PRIM_DECL_3( ProxyOopPrimitives::callOut1, Oop receiver, Oop arg1, Oop result ) {
    PROLOGUE_3( "callOut1", receiver, arg1, result );
    ASSERT_RECEIVER_ACCESS;

    std::int32_t a1;
    if ( not receiver->is_proxy() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );
    if ( not convert_to_arg( arg1, &a1 ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );

    call_out_func_1 f = (call_out_func_1) ProxyOop( receiver )->get_pointer();
    ProxyOop( result )->set_pointer( ( *f )( a1 ) );
    return result;
}


typedef void *(__CALLING_CONVENTION *call_out_func_2)( std::int32_t a, std::int32_t b );


PRIM_DECL_4( ProxyOopPrimitives::callOut2, Oop receiver, Oop arg1, Oop arg2, Oop result ) {
    PROLOGUE_4( "callOut2", receiver, arg1, arg2, result );
    ASSERT_RECEIVER_ACCESS;

    std::int32_t a1, a2;
    if ( not receiver->is_proxy() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );
    if ( not convert_to_arg( arg1, &a1 ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not convert_to_arg( arg2, &a2 ) )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );

    call_out_func_2 f = (call_out_func_2) ProxyOop( receiver )->get_pointer();
    ProxyOop( result )->set_pointer( ( *f )( a1, a2 ) );
    return result;
}


typedef void *(__CALLING_CONVENTION *call_out_func_3)( std::int32_t a, std::int32_t b, std::int32_t c );


PRIM_DECL_5( ProxyOopPrimitives::callOut3, Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop result ) {
    PROLOGUE_5( "callOut3", receiver, arg1, arg2, arg3, result );
    ASSERT_RECEIVER_ACCESS;

    std::int32_t a1, a2, a3;
    if ( not receiver->is_proxy() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );
    if ( not convert_to_arg( arg1, &a1 ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not convert_to_arg( arg2, &a2 ) )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not convert_to_arg( arg3, &a3 ) )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::fourth_argument_has_wrong_type() );

    call_out_func_3 f = (call_out_func_3) ProxyOop( receiver )->get_pointer();
    ProxyOop( result )->set_pointer( ( *f )( a1, a2, a3 ) );
    return result;
}


typedef void *(__CALLING_CONVENTION *call_out_func_4)( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d );


PRIM_DECL_6( ProxyOopPrimitives::callOut4, Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop result ) {
    PROLOGUE_6( "callOut4", receiver, arg1, arg2, arg3, arg4, result );
    ASSERT_RECEIVER_ACCESS;

    std::int32_t a1, a2, a3, a4;
    if ( not receiver->is_proxy() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );
    if ( not convert_to_arg( arg1, &a1 ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not convert_to_arg( arg2, &a2 ) )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not convert_to_arg( arg3, &a3 ) )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );
    if ( not convert_to_arg( arg4, &a4 ) )
        return markSymbol( vmSymbols::fourth_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::fifth_argument_has_wrong_type() );

    call_out_func_4 f = (call_out_func_4) ProxyOop( receiver )->get_pointer();
    ProxyOop( result )->set_pointer( ( *f )( a1, a2, a3, a4 ) );
    return result;
}


typedef void *(__CALLING_CONVENTION *call_out_func_5)( std::int32_t a, std::int32_t b, std::int32_t c, std::int32_t d, std::int32_t e );


PRIM_DECL_7( ProxyOopPrimitives::callOut5, Oop receiver, Oop arg1, Oop arg2, Oop arg3, Oop arg4, Oop arg5, Oop result ) {
    PROLOGUE_7( "callOut5", receiver, arg1, arg2, arg3, arg4, arg5, result );
    ASSERT_RECEIVER_ACCESS;

    std::int32_t a1, a2, a3, a4, a5;
    if ( not receiver->is_proxy() )
        return markSymbol( vmSymbols::receiver_has_wrong_type() );
    if ( ProxyOop( receiver )->is_null() )
        return markSymbol( vmSymbols::illegal_state() );
    if ( not convert_to_arg( arg1, &a1 ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not convert_to_arg( arg2, &a2 ) )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not convert_to_arg( arg3, &a3 ) )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );
    if ( not convert_to_arg( arg4, &a4 ) )
        return markSymbol( vmSymbols::fourth_argument_has_wrong_type() );
    if ( not convert_to_arg( arg5, &a5 ) )
        return markSymbol( vmSymbols::fifth_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::sixth_argument_has_wrong_type() );

    call_out_func_5 f = (call_out_func_5) ProxyOop( receiver )->get_pointer();
    ProxyOop( result )->set_pointer( ( *f )( a1, a2, a3, a4, a5 ) );
    return result;
}
