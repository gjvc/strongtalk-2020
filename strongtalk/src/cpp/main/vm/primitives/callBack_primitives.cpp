//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/ObjectIDTable.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/runtime/CallBack.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/primitives/callBack_primitives.hpp"
#include "vm/primitives/primitive_tracing.hpp"


TRACE_FUNC( TraceCallBackPrims, "callBack" )


std::int32_t callBackPrimitives::number_of_calls;


PRIM_DECL_2( callBackPrimitives::initialize, Oop receiver, Oop selector ) {
    PROLOGUE_2( "initialize", receiver, selector );
    if ( not selector->is_symbol() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    CallBack::initialize( receiver, SymbolOop( selector ) );
    return receiver;
}


PRIM_DECL_3( callBackPrimitives::registerPascalCall, Oop index, Oop nofArgs, Oop result ) {
    PROLOGUE_3( "registerPascalCall", index, nofArgs, result );
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not nofArgs->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );
    ProxyOop( result )->set_pointer( CallBack::registerPascalCall( SMIOop( index )->value(), SMIOop( nofArgs )->value() ) );
    return result;
}


PRIM_DECL_2( callBackPrimitives::registerCCall, Oop index, Oop result ) {
    PROLOGUE_2( "registerCCall", index, result );
    if ( not index->is_smi() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    ProxyOop( result )->set_pointer( CallBack::registerCCall( SMIOop( index )->value() ) );
    return result;
}


PRIM_DECL_1( callBackPrimitives::unregister, Oop proxy ) {
    PROLOGUE_1( "unregister", proxy );
    if ( not proxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    CallBack::unregister( ProxyOop( proxy )->get_pointer() );
    ProxyOop( proxy )->set_pointer( nullptr );
    return proxy;
}


typedef std::int32_t     (__CALLING_CONVENTION *mytype)( std::int32_t a, std::int32_t b );


PRIM_DECL_1( callBackPrimitives::invokePascal, Oop proxy ) {
    PROLOGUE_1( "invokePascal", proxy );
    if ( not proxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    mytype f = (mytype) ProxyOop( proxy )->get_pointer();
    return smiOopFromValue( ( *f )( 10, 5 ) );
}


PRIM_DECL_1( callBackPrimitives::invokeC, Oop proxy ) {
    PROLOGUE_1( "invokeC", proxy );
    if ( not proxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    primitiveFunctionType f = (primitiveFunctionType) ProxyOop( proxy )->get_pointer();
    return smiOopFromValue( (std::int32_t) ( *f )( 10, 5 ) );
}
